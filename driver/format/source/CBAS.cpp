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

                cJSON * type = types->child;
                int cIdx = 0;
                while (type) {
                    auto & column_info = indexMapper.size() > cIdx ? columns_info[indexMapper[cIdx]] : columns_info[cIdx];
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
                    }

                    type = type->next;
                    cIdx++;
                }
            }
        }
    }

    cookie.queryMeta.clear();
    finished = columns_info.empty();
    expected_column_order = nullptr;
}

bool CBASResultSet::readNextRow(Row & row) {
    std::string doc;
    if (std::getline(cookie.queryResultStrm, doc)) {
        cJSON * json;
        json = cJSON_Parse(doc.c_str());
        if (json && json->type == cJSON_Object) {
            int cIdx = 0;
            while (cIdx < columns_info.size()) {
                auto & column_info = indexMapper.size() > cIdx ? columns_info[indexMapper[cIdx]] : columns_info[cIdx];
                DataSourceType<DataSourceTypeId::String> strVal;
                value_manip::to_null(strVal.value);
                const char* json_key = column_info.name.c_str();
                cJSON * col = cJSON_GetObjectItem(json, json_key);
                bool is_null = false;
                if (col->type == cJSON_String) { // Handle String, Double, Date, Time, DateTime
                    strVal.value = col->valuestring;
                    if (column_info.type == "String") {
                        if (strVal.value[0] == LOSSLESS_ADM_DELIMETER) {
                            strVal.value = strVal.value.substr(1);
                        }
                    } else if (column_info.type == "Float64") { // Double
                        std::ostringstream oss;
                        auto types_id_it = types_id_g.find(column_info.type);
                        if (types_id_it != types_id_g.end()) {
                            uint64_t doubleTypeTag = types_id_it->second;
                            oss << customHex((doubleTypeTag >> 4) & 0x0f);
                            oss << customHex(doubleTypeTag & 0x0f);
                        }
                        oss << LOSSLESS_ADM_DELIMETER;
                        if (strVal.value.substr(0, 3) == oss.str()) {
                            strVal.value = strVal.value.substr(3);
                        }
                    }
                    else if(column_info.type == "Date"){
                        if(strVal.value.substr(0,3) == LOSSLESS_ADM_DELIMETER_DATE){
                            std::string daysInString = strVal.value.substr(3,strVal.value.length());
                            std::int32_t daysInt = std::stoi(daysInString);

                            std::string dateInCorrectFormat = convertDaysSinceEpochToDateString(daysInt);
                            strVal.value = dateInCorrectFormat;
                        }
                    }
                    else if (column_info.type == "Time"){
                        if(strVal.value.substr(0,3) == LOSSLESS_ADM_DELIMETER_TIME){
                            std::string timeInString = strVal.value.substr(3);

                            std::int64_t timeLong = std::stoi(timeInString);
                            std::string timeInCorrectFormat = convertMillisecondsSinceBeginningOfDayToTimeString(timeLong);
                            strVal.value = timeInCorrectFormat;
                        }
                    }
                    else if (column_info.type == "DateTime"){
                        if(strVal.value.substr(0,3) == LOSSLESS_ADM_DELIMETER_DATETIME){
                            std::string datetimeInString = strVal.value.substr(3);
                            long long milliseconds = std::stoll(datetimeInString);

                            std::string datetimeInCorrectFormat = convertMillisecondsSinceEpochToDateTimeString(milliseconds);
                            strVal.value = datetimeInCorrectFormat;
                        }
                    }
                } else if (col->type == cJSON_Number) {
                    std::ostringstream oss;
                    oss << col->valueint;
                    strVal.value = oss.str();
                } else if (col->type == cJSON_NULL) {
                    is_null = true;
                } else if (col->type == cJSON_True || col->type == cJSON_False) {
                    strVal.value = cJSON_Print(col); // true/false
                } else {
                    //todo: throw error ?
                }

                if (is_null) {
                    row.fields[(indexMapper.size() > cIdx) ? indexMapper[cIdx] : cIdx].data = DataSourceType<DataSourceTypeId::Nothing> {};
                } else {
                    if (column_info.display_size_so_far < strVal.value.size())
                        column_info.display_size_so_far = strVal.value.size();
                    row.fields[(indexMapper.size() > cIdx) ? indexMapper[cIdx] : cIdx].data = std::move(strVal);
                }
                cIdx++;
            }
        }
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