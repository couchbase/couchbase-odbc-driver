#include "driver/format/include/CBAS.h"
#include "driver/include/cJSON.h"
#include "driver/utils/include/ieee_754_converter.h"
#include "driver/utils/include/resize_without_initialization.h"
#include "driver/utils/include/utils.h"
#include "driver/utils/include/lossless-adm_to_human-readable_format.h"

CBASResultSet::CBASResultSet(
    const std::string & timezone, AmortizedIStreamReader & stream, std::unique_ptr<ResultMutator> && mutator, CallbackCookie & cbCookie, std::vector<std::string>* expected_column_order)
    : cookie(cbCookie), ResultSet(stream, std::move(mutator)) {
    cJSON * json;
    json = cJSON_Parse(cookie.queryMeta.c_str());
    if (json && json->type == cJSON_Object) {
        cJSON * signature = cJSON_GetObjectItem(json, "signature");
        if (signature && signature->type == cJSON_Object) {
            cJSON * names = cJSON_GetObjectItem(signature, "name");
            if (names && names->type == cJSON_Array) {
                std::int32_t columns_count = cJSON_GetArraySize(names);
                columns_info.resize(columns_count);
                //create the index mapper if expected_column_order is not null
                cJSON * name = names->child;
                int cIdx = 0;
                if (expected_column_order != nullptr && expected_column_order->size() == columns_count) {
                    indexMapper.resize(columns_count);
                    for (std::string& str : *expected_column_order) {
                        std::transform(str.begin(), str.end(), str.begin(),
                                    [](unsigned char c) { return std::tolower(c); });
                        }
                    while (name) {
                        if (name->type == cJSON_String) {
                            std::string name_value = name->valuestring;
                            std::string lowercase_name = name_value;
                            std::transform(lowercase_name.begin(), lowercase_name.end(), lowercase_name.begin(),
                                        [](unsigned char c) { return std::tolower(c); });
                            auto present = std::find(expected_column_order->begin(), expected_column_order->end(), lowercase_name);

                            if (present != expected_column_order->end()) {
                                size_t index = std::distance(expected_column_order->begin(), present);
                                indexMapper[cIdx] = static_cast<int>(index);
                                columns_info[indexMapper[cIdx]].name = name_value;
                            }
                        }
                        name = name->next;
                        cIdx++;
                    }
                } else {
                        while (name) {
                        if (name->type == cJSON_String) {
                            columns_info[cIdx].name = name->valuestring;
                        }
                        name = name->next;
                        cIdx++;
                    }
                }

            }
            cJSON * types = cJSON_GetObjectItem(signature, "type");
            if (types && types->type == cJSON_Array) {
                std::int32_t columns_count = cJSON_GetArraySize(types);
                columns_info.resize(columns_count);
                col_tags.assign(columns_count, (uint8_t)ColTypeTag::Other_);

                cJSON * type = types->child;
                int cIdx = 0;
                while (type) {
                    const int odbc_slot = indexMapper.empty() ? cIdx : indexMapper[cIdx];
                    auto & column_info = columns_info[odbc_slot];
                    if (type->type == cJSON_String) {
                        auto typeIterator = cb_to_ch_types_g.find(type->valuestring);
                        if (typeIterator != cb_to_ch_types_g.end()) {
                            column_info.type = typeIterator->second;
                        } else {
                            column_info.type = "String";
                        }

                        TypeParser parser {column_info.type};
                        TypeAst ast;

                        if (parser.parse(&ast)) {
                            column_info.assignTypeInfo(ast, timezone);

                            if (convertUnparametrizedTypeNameToTypeId(column_info.type_without_parameters)
                                == DataSourceTypeId::Unknown) {
                                // Interpret all unknown types as String.
                                column_info.type_without_parameters = "String";
                            }
                        } else {
                            // Interpret all unparsable types as String.
                            column_info.type_without_parameters = "String";
                        }
                        column_info.updateTypeInfo();

                        // Compute fast dispatch tag once, used by readNextRow() every row.
                        const auto & t = column_info.type;
                        ColTypeTag tag;
                        if      (t == "String")   tag = ColTypeTag::String_;
                        else if (t == "Float64")  tag = ColTypeTag::Float64_;
                        else if (t == "Date")     tag = ColTypeTag::Date_;
                        else if (t == "Time")     tag = ColTypeTag::Time_;
                        else if (t == "DateTime") tag = ColTypeTag::DateTime_;
                        else if (t == "Int8"   || t == "Int16"  || t == "Int32"  || t == "Int64"  ||
                                 t == "UInt8"  || t == "UInt16" || t == "UInt32" || t == "UInt64")
                                                  tag = ColTypeTag::Integer_;
                        else                      tag = ColTypeTag::Other_;
                        col_tags[odbc_slot] = (uint8_t)tag;
                    }

                    type = type->next;
                    cIdx++;
                }
            }
        }
    }

    cJSON_Delete(json);
    cookie.queryMeta.clear();
    finished = columns_info.empty();
    expected_column_order = nullptr;
}

bool CBASResultSet::readNextRow(Row & row) {
    std::string doc;
    if (std::getline(cookie.queryResultStrm, doc)) {
        cJSON * json = cJSON_Parse(doc.c_str());
        if (json && json->type == cJSON_Object) {
            const int ncols = (int)columns_info.size();
            for (int cIdx = 0; cIdx < ncols; ++cIdx) {
                const int odbc_slot = indexMapper.empty() ? cIdx : indexMapper[cIdx];
                auto & column_info = columns_info[odbc_slot];
                cJSON * col = cJSON_GetObjectItem(json, column_info.name.c_str());
                if (!col) continue;

                DataSourceType<DataSourceTypeId::String> strVal;
                value_manip::to_null(strVal.value);
                bool is_null = false;
                switch (static_cast<ColTypeTag>(col_tags[odbc_slot])) {

                    case ColTypeTag::String_:
                        if (col->type == cJSON_String) {
                            // lossless-adm strings are prefixed with ':'
                            const char * vs = col->valuestring;
                            strVal.value.assign(vs[0] == LOSSLESS_ADM_DELIMETER ? vs + 1 : vs);
                        } else if (col->type == cJSON_NULL) {
                            is_null = true;
                        }
                        break;

                    case ColTypeTag::Float64_:
                        if (col->type == cJSON_String) {
                            // lossless-adm Float64 prefix is always "0C:"
                            const char * vs = col->valuestring;
                            if (vs[0] == '0' && vs[1] == 'C' && vs[2] == ':')
                                strVal.value.assign(vs + 3);
                            else
                                strVal.value.assign(vs);
                        } else if (col->type == cJSON_NULL) {
                            is_null = true;
                        }
                        break;

                    case ColTypeTag::Date_:
                        if (col->type == cJSON_String) {
                            // lossless-adm Date prefix "11:"
                            const char * vs = col->valuestring;
                            if (vs[0] == '1' && vs[1] == '1' && vs[2] == ':')
                                strVal.value = convertDaysSinceEpochToDateString(std::stoi(vs + 3));
                            else
                                strVal.value.assign(vs);
                        } else if (col->type == cJSON_NULL) {
                            is_null = true;
                        }
                        break;

                    case ColTypeTag::Time_:
                        if (col->type == cJSON_String) {
                            // lossless-adm Time prefix "12:"
                            const char * vs = col->valuestring;
                            if (vs[0] == '1' && vs[1] == '2' && vs[2] == ':')
                                strVal.value = convertMillisecondsSinceBeginningOfDayToTimeString(std::stoll(vs + 3));
                            else
                                strVal.value.assign(vs);
                        } else if (col->type == cJSON_NULL) {
                            is_null = true;
                        }
                        break;

                    case ColTypeTag::DateTime_:
                        if (col->type == cJSON_String) {
                            // lossless-adm DateTime prefix "10:"
                            const char * vs = col->valuestring;
                            if (vs[0] == '1' && vs[1] == '0' && vs[2] == ':')
                                strVal.value = convertMillisecondsSinceEpochToDateTimeString(std::stoll(vs + 3));
                            else
                                strVal.value.assign(vs);
                        } else if (col->type == cJSON_NULL) {
                            is_null = true;
                        }
                        break;

                case ColTypeTag::Integer_:
                    if (col->type == cJSON_Number) {
                        strVal.value = std::to_string(col->valueint);
                    } else if (col->type == cJSON_String) {
                        // CBAS sends bigints (> 2^53) as lossless ADM strings (e.g., ":9876543210").
                        // bigint maps to Integer_, we must also handle the cJSON_String encoding for large values.
                        const char * vs = col->valuestring;
                        strVal.value.assign(vs[0] == LOSSLESS_ADM_DELIMETER ? vs + 1 : vs);
                    } else if (col->type == cJSON_True) {
                        strVal.value = "1";
                    } else if (col->type == cJSON_False) {
                        strVal.value = "0";
                    } else if (col->type == cJSON_NULL) {
                        is_null = true;
                    }
                    break;

                    default: // ColTypeTag::Other_ : boolean (UInt8) or unrecognised type
                        if (col->type == cJSON_True || col->type == cJSON_False)
                            strVal.value = (col->type == cJSON_True) ? "true" : "false";
                        else if (col->type == cJSON_Number)
                            strVal.value = std::to_string(col->valueint);
                        else if (col->type == cJSON_NULL)
                            is_null = true;
                        break;
                }

                if (is_null) {
                    row.fields[odbc_slot].data = DataSourceType<DataSourceTypeId::Nothing>{};
                } else {
                    if (column_info.display_size_so_far < strVal.value.size())
                        column_info.display_size_so_far = strVal.value.size();
                    row.fields[odbc_slot].data = std::move(strVal);
                }
            }
        }
        cJSON_Delete(json);
        return true;
    }

    cookie.queryResultStrm.clear();
    cookie.queryResultStrm.str(std::string());
    return false;
}

CBASResultReader::CBASResultReader(
    const std::string & timezone_, std::istream & raw_stream, std::unique_ptr<ResultMutator> && mutator, CallbackCookie & cbCookie, std::vector<std::string>* expected_column_order)
    : ResultReader(timezone_, raw_stream, std::move(mutator)) {
    result_set = std::make_unique<CBASResultSet>(timezone, stream, releaseMutator(), cbCookie, expected_column_order);
}

bool CBASResultReader::advanceToNextResultSet() {
    if (result_set) {
        result_mutator = result_set->releaseMutator();
        result_set.reset();
    }

    return hasResultSet();
}