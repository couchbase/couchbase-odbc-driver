#include "driver/include/statement.h"
#include "driver/include/cJSON.h"
#include "driver/escaping/include/escape_sequences.h"
#include "driver/escaping/include/lexer.h"
#include "driver/platform/platform.h"
#include "driver/utils/include/utils.h"
#include <Poco/UUIDGenerator.h>

#include "driver/include/metadata.h"
#include <cctype>
#include <cstdio>
#include <sstream>
#include <ctime>

extern "C" {
lcb_STATUS lcb_cmdanalytics_create(lcb_CMDANALYTICS **);
lcb_STATUS lcb_cmdanalytics_destroy(lcb_CMDANALYTICS *);
lcb_STATUS lcb_analytics(lcb_INSTANCE *, void *, const lcb_CMDANALYTICS *);
}

Statement::Statement(Connection & connection)
    : ChildType(connection)
{
    allocateImplicitDescriptors();
}

Statement::~Statement() {
    deallocateImplicitDescriptors();
}

const TypeInfo & Statement::getTypeInfo(const std::string & type_name, const std::string & type_name_without_parameters) const {
    return getParent().getTypeInfo(type_name, type_name_without_parameters);
}

long long Statement::getMillisecondsFromODBCtimestamp(const std::string& timestamp_exp) {
    struct tm timeinfo;
    int year, month, day, hour, minute, second;

    std::stringstream ss(timestamp_exp);
    char dash, space, colon;

    ss >> year >> dash >> month >> dash >> day >> space >> hour >> colon >> minute >> colon >> second;

    if (ss.fail() || dash != '-' || colon != ':' || ss.peek() != EOF) {
        std::cerr << "Error parsing the timestamp." << std::endl;
        return -1;
    }

    timeinfo.tm_year = year - 1900; // Years since 1900
    timeinfo.tm_mon = month - 1;    // Months since January (0-based)
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_isdst = -1; // Let the system determine if DST is in effect

    // Convert the tm structure to a time_t value
    time_t rawtime = mktime(&timeinfo); // or mktime(&timeinfo) on some platforms

    // Calculate milliseconds since epoch (Unix epoch time: January 1, 1970)
    long long milliseconds = static_cast<long long>(rawtime) * 1000LL;

    return milliseconds;
}

void Statement::prepareQuery(const std::string & q) {
    closeCursor();

    is_prepared = false;
    query = q;
    processEscapeSequences();
    extractParametersinfo();
    is_prepared = true;
}

bool Statement::isPrepared() const {
    return is_prepared;
}

bool Statement::isExecuted() const {
    return is_executed;
}

void Statement::executeQuery(std::unique_ptr<ResultMutator> && mutator) {
    if (!is_prepared)
        throw std::runtime_error("statement not prepared");

    if (is_executed && is_forward_executed) {
        is_forward_executed = false;
        return;
    }

    auto * param_set_processed_ptr = getEffectiveDescriptor(SQL_ATTR_IMP_PARAM_DESC).getAttrAs<SQLULEN *>(SQL_DESC_ROWS_PROCESSED_PTR, 0);
    if (param_set_processed_ptr)
        *param_set_processed_ptr = 0;

    next_param_set_idx = 0;
    requestNextPackOfResultSets(std::move(mutator));
    is_executed = true;
}

void Statement::requestNextPackOfResultSets(std::unique_ptr<ResultMutator> && mutator) {
    result_reader.reset();

    const auto param_set_array_size = getEffectiveDescriptor(SQL_ATTR_APP_PARAM_DESC).getAttrAs<SQLULEN>(SQL_DESC_ARRAY_SIZE, 1);
    if (next_param_set_idx >= param_set_array_size)
        return;

    getDiagHeader().setAttr(SQL_DIAG_ROW_COUNT, 0);

    auto & connection = getParent();

    if (connection.session && response && in)
        if (in->fail() || !in->eof())
            connection.session->reset();
    const auto param_bindings = getParamsBindingInfo(next_param_set_idx);

    cbas_params_args.clear();
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        std::string value;

        if (param_bindings.size() <= i) { // not enough parameters are bounded
            value = "\\N";
        } else {
            const auto & binding_info = param_bindings[i];

            if (!isInputParam(binding_info.io_type) || isStreamParam(binding_info.io_type))
                throw std::runtime_error("Unable to extract data from bound param buffer: param IO type is not supported");

            if (binding_info.value == nullptr)
                value = "\\N";
            else
                readReadyDataTo(binding_info, value);
        }
        switch (param_bindings[i].sql_type) {
            case SQL_CHAR:
            case SQL_WCHAR:
            case SQL_VARCHAR:
            case SQL_WVARCHAR:
            case SQL_LONGVARCHAR:
            case SQL_WLONGVARCHAR: {
                std::ostringstream oss;
                oss << "\"";
                oss << LOSSLESS_ADM_DELIMETER << value << "\"";
                cbas_params_args.emplace_back(oss.str());
                break;
            }
            case SQL_INTEGER: {
                cbas_params_args.emplace_back(value);
                break;
            }
            case SQL_DOUBLE: {
                double doubleVal = *reinterpret_cast<double *>(param_bindings[i].value);
                std::ostringstream oss;
                oss << "\"";
                auto types_id_it = types_id_g.find("Float64");
                if (types_id_it != types_id_g.end()) {
                    uint64_t doubleTypeTag = types_id_it->second;
                    oss << customHex((doubleTypeTag >> 4) & 0x0f);
                    oss << customHex(doubleTypeTag & 0x0f);
                }
                oss << LOSSLESS_ADM_DELIMETER << ieee_double_to_ull(doubleVal) << "\"";
                cbas_params_args.emplace_back(oss.str());
                break;
            }
            case SQL_BIT: {
                SQLCHAR boolVal = *reinterpret_cast<SQLCHAR *>(param_bindings[i].value);
                std::string boolArgVal = boolVal ? "true" : "false";
                cbas_params_args.emplace_back(boolArgVal);
            }
            case SQL_TYPE_TIMESTAMP:{
                std::ostringstream oss;
                oss << "\"";
                long long milliseconds = getMillisecondsFromODBCtimestamp(value);
                oss << LOSSLESS_ADM_DELIMETER_DATETIME << milliseconds << "\"";
                cbas_params_args.emplace_back(oss.str());
                break;
            }
            default:
                break;
        }
        

        const auto param_name = getParamFinalName(i);
    }

    std::cout << query;
    // TODO: set this only after this single query is fully fetched (when output parameter support is added)
    auto * param_set_processed_ptr = getEffectiveDescriptor(SQL_ATTR_IMP_PARAM_DESC).getAttrAs<SQLULEN *>(SQL_DESC_ROWS_PROCESSED_PTR, 0);
    if (param_set_processed_ptr)
        *param_set_processed_ptr = next_param_set_idx;
    std::ostringstream payload;
    payload << "{\"signature\":true,\"client-type\":\"jdbc\",\"plan-format\":\"string\",\"format\":\"lossless-adm\",\"max-warnings\":"
                "10,\"sql-compat\":true,"
                "\"statement\":\""
            << query;
    if (cbas_params_args.size()) {
        payload << "\","
                << "\"args\":"
                << "[";
        for (int i = 0; i < cbas_params_args.size(); i++) {
            payload << cbas_params_args[i];
            if (i != cbas_params_args.size() - 1) {
                payload << ",";
            }
        }
        payload << "]";
    } else {
        payload << "\"";
    }
    payload << "}";

    std::string payloadStr = payload.str();

    lcb_CMDANALYTICS * cmd;
    lcb_cmdanalytics_create(&cmd);
    lcb_cmdanalytics_callback(cmd, queryCallback);
    lcb_cmdanalytics_payload(cmd, payloadStr.c_str(), payloadStr.size());

    std::ostringstream oss;
    oss << (is_set_stmt_query_timeout ? stmt_query_timeout : connection.query_timeout) * 1000000;
    //connection.cb_check(lcb_cntl_string(connection.lcb_instance, "analytics_timeout", oss.str().c_str()), "set analytics timeout");

    try{
        connection.cb_check(lcb_analytics(connection.lcb_instance, &cbCookie, cmd), "Schedule Analytics Query");
    } catch (std::exception & ex) {
        lcb_cmdanalytics_destroy(cmd);
        throw std::runtime_error(ex.what());
    }

    lcb_cmdanalytics_destroy(cmd);
    lcb_wait(connection.lcb_instance, LCB_WAIT_DEFAULT);

    result_reader = make_result_reader("CBAS", "absurd_time_zone", *in, std::move(mutator), cbCookie, expected_column_order);

    if (cbCookie.errorInResponse == true) {
        cbCookie.errorInResponse = false;
        std::string str = cbCookie.queryResultStrm.str();

        cbCookie.cleanUp();

        throw std::runtime_error(str);
    }
    

    ++next_param_set_idx;
}

void Statement::processEscapeSequences() {
    if (getAttrAs<SQLULEN>(SQL_ATTR_NOSCAN, SQL_NOSCAN_OFF) != SQL_NOSCAN_ON)
        query = replaceEscapeSequences(query);
}

void Statement::extractParametersinfo() {
    auto & apd_desc = getEffectiveDescriptor(SQL_ATTR_APP_PARAM_DESC);
    auto & ipd_desc = getEffectiveDescriptor(SQL_ATTR_IMP_PARAM_DESC);

    const auto apd_record_count = apd_desc.getRecordCount();
    auto ipd_record_count = ipd_desc.getRecordCount();

    // Reset IPD records but preserve those that may have been modified by SQLBindParameter and are still relevant.
    ipd_record_count = std::min(ipd_record_count, apd_record_count);
    ipd_desc.setAttr(SQL_DESC_COUNT, ipd_record_count);

    parameters.clear();

    // TODO: implement this all in an upgraded Lexer.

    Poco::UUIDGenerator uuid_gen;
    auto generate_placeholder = [&] () {
        std::string placeholder;
        do {
            const auto uuid = uuid_gen.createOne();
            placeholder = '@' + uuid.toString();
        } while (query.find(placeholder) != std::string::npos);
        return placeholder;
    };

    // Replace all unquoted ? characters with a placeholder and populate 'parameters' array.
    char quoted_by = '\0';
    for (std::size_t i = 0; i < query.size(); ++i) {
        const char curr = query[i];
        const char next = (i < query.size() ? query[i + 1] : '\0');

        switch (curr) {
            case '\\': {
                ++i; // Skip the next char unconditionally.
                break;
            }

            case '"':
            case '\'': {
                if (quoted_by == curr) {
                    if (next == curr) {
                        ++i; // Skip the next char unconditionally: '' or "" SQL escaping.
                        break;
                    }
                    else {
                        quoted_by = '\0';
                    }
                }
                else {
                    quoted_by = curr;
                }
                break;
            }

            case '?': {
                if (quoted_by == '\0') {
                    ParamInfo param_info;
                    parameters.emplace_back(param_info);
                }
                break;
            }

            case '@': {
                if (quoted_by == '\0') {
                    ParamInfo param_info;
                    param_info.name = '@';
                    for (std::size_t j = i + 1; j < query.size(); ++j) {
                        const char jcurr = query[j];
                        if (jcurr == '_' || std::isalpha(jcurr) || (std::isdigit(jcurr) && j > i + 1)) {
                            param_info.name += jcurr;
                        } else {
                            break;
                        }
                    }

                    if (param_info.name.size() == 1)
                        throw SqlException("Syntax error or access violation", "42000");

                    param_info.tmp_placeholder = generate_placeholder();
                    query.replace(i, param_info.name.size(), param_info.tmp_placeholder);
                    i += param_info.tmp_placeholder.size() - 1; // - 1 to compensate for's next ++i
                    parameters.emplace_back(param_info);
                }
                break;
            }
        }
    }

    // Access the biggest record to [possibly] create all missing ones.
    if (ipd_record_count < parameters.size())
        ipd_desc.getRecord(parameters.size(), SQL_ATTR_IMP_PARAM_DESC);
}

void Statement::executeQuery(const std::string & q, std::unique_ptr<ResultMutator> && mutator) {
    prepareQuery(q);
    executeQuery(std::move(mutator));
}

void Statement::forwardExecuteQuery(std::unique_ptr<ResultMutator> && mutator) {
    if (!is_prepared)
        throw std::runtime_error("statement not prepared");

    if (is_executed)
        return;

    executeQuery(std::move(mutator));
    is_forward_executed = true;
}

bool Statement::hasResultSet() const {
    return (result_reader && result_reader->hasResultSet());
}

ResultSet & Statement::getResultSet() {
    return result_reader->getResultSet();
}

bool Statement::advanceToNextResultSet() {
    if (!is_executed)
        return false;

    getDiagHeader().setAttr(SQL_DIAG_ROW_COUNT, 0);

    std::unique_ptr<ResultMutator> mutator;

    if (result_reader) {
        if (result_reader->advanceToNextResultSet())
            return true;

        mutator = result_reader->releaseMutator();
    }

    requestNextPackOfResultSets(std::move(mutator));
    return hasResultSet();
}

void Statement::closeCursor() {
    auto & connection = getParent();
    if (connection.session && response && in) {
        if (in->fail() || !in->eof())
            connection.session->reset();
    }

    result_reader.reset();
    in = nullptr;
    response.reset();

    is_executed = false;
    is_forward_executed = false;
}

void Statement::resetColBindings() {
    getEffectiveDescriptor(SQL_ATTR_APP_ROW_DESC).setAttr(SQL_DESC_COUNT, 0);
}

void Statement::resetParamBindings() {
    getEffectiveDescriptor(SQL_ATTR_APP_PARAM_DESC).setAttr(SQL_DESC_COUNT, 0);
}

std::string Statement::getParamFinalName(std::size_t param_idx) {
    auto & ipd_desc = getEffectiveDescriptor(SQL_ATTR_IMP_PARAM_DESC);
    if (param_idx < ipd_desc.getRecordCount()) {
        auto & ipd_record = ipd_desc.getRecord(param_idx + 1, SQL_ATTR_IMP_PARAM_DESC);
        if (ipd_record.getAttrAs<SQLSMALLINT>(SQL_DESC_UNNAMED, SQL_UNNAMED) != SQL_UNNAMED)
            return tryStripParamPrefix(ipd_record.getAttrAs<std::string>(SQL_DESC_NAME));
    }

    if (param_idx < parameters.size() && !parameters[param_idx].name.empty())
        return tryStripParamPrefix(parameters[param_idx].name);

    return "odbc_positional_" + std::to_string(param_idx + 1);
}

std::vector<ParamBindingInfo> Statement::getParamsBindingInfo(std::size_t param_set_idx) {
    std::vector<ParamBindingInfo> param_bindings;

    auto & apd_desc = getEffectiveDescriptor(SQL_ATTR_APP_PARAM_DESC);
    auto & ipd_desc = getEffectiveDescriptor(SQL_ATTR_IMP_PARAM_DESC);

    const auto apd_record_count = apd_desc.getRecordCount();
    const auto ipd_record_count = ipd_desc.getRecordCount();

    const auto fully_bound_param_count = std::min(apd_record_count, ipd_record_count);

    // We allow (apd_record_count < ipd_record_count) here, since we will set
    // all unbound parameters to 'Null' and their types to 'Nullable(Nothing)'.

    if (fully_bound_param_count > 0)
        param_bindings.reserve(fully_bound_param_count);

    auto * array_status_ptr = ipd_desc.getAttrAs<SQLUSMALLINT *>(SQL_DESC_ARRAY_STATUS_PTR, 0);

    const auto bind_type = apd_desc.getAttrAs<SQLULEN>(SQL_DESC_BIND_TYPE, SQL_PARAM_BIND_TYPE_DEFAULT);
    const auto * bind_offset_ptr = apd_desc.getAttrAs<SQLULEN *>(SQL_DESC_BIND_OFFSET_PTR, 0);
    const auto bind_offset = (bind_offset_ptr ? *bind_offset_ptr : 0);

    for (std::size_t param_num = 1; param_num <= fully_bound_param_count; ++param_num) {
        auto & apd_record = apd_desc.getRecord(param_num, SQL_ATTR_APP_PARAM_DESC);
        auto & ipd_record = ipd_desc.getRecord(param_num, SQL_ATTR_IMP_PARAM_DESC);

        const auto * data_ptr = apd_record.getAttrAs<SQLPOINTER>(SQL_DESC_DATA_PTR, 0);
        const auto * sz_ptr = apd_record.getAttrAs<SQLLEN *>(SQL_DESC_OCTET_LENGTH_PTR, 0);
        const auto * ind_ptr = apd_record.getAttrAs<SQLLEN *>(SQL_DESC_INDICATOR_PTR, 0);

        ParamBindingInfo binding_info;
        binding_info.io_type = ipd_record.getAttrAs<SQLSMALLINT>(SQL_DESC_PARAMETER_TYPE, SQL_PARAM_INPUT);
        binding_info.c_type = apd_record.getAttrAs<SQLSMALLINT>(SQL_DESC_CONCISE_TYPE, SQL_C_DEFAULT);
        binding_info.sql_type = ipd_record.getAttrAs<SQLSMALLINT>(SQL_DESC_CONCISE_TYPE, SQL_UNKNOWN_TYPE);
        binding_info.value_max_size = apd_record.getAttrAs<SQLLEN>(SQL_DESC_OCTET_LENGTH, 0);

        const auto next_value_ptr_increment = (bind_type == SQL_PARAM_BIND_BY_COLUMN ? binding_info.value_max_size : bind_type);
        const auto next_sz_ind_ptr_increment = (bind_type == SQL_PARAM_BIND_BY_COLUMN ? sizeof(SQLLEN) : bind_type);

        binding_info.value = (SQLPOINTER)(data_ptr ? ((char *)(data_ptr) + param_set_idx * next_value_ptr_increment + bind_offset) : 0);
        binding_info.value_size = (SQLLEN *)(sz_ptr ? ((char *)(sz_ptr) + param_set_idx * next_sz_ind_ptr_increment + bind_offset) : 0);
        binding_info.indicator = (SQLLEN *)(ind_ptr ? ((char *)(ind_ptr) + param_set_idx * next_sz_ind_ptr_increment + bind_offset) : 0);

        binding_info.is_nullable = (
            ipd_record.getAttrAs<SQLSMALLINT>(SQL_DESC_NULLABLE,
                (isMappedToStringDataSourceType(binding_info.sql_type, binding_info.c_type) ? SQL_NO_NULLS : SQL_NULLABLE)
            ) == SQL_NULLABLE
        );

        binding_info.scale = ipd_record.getAttrAs<SQLSMALLINT>(SQL_DESC_SCALE, 0);
        binding_info.precision = ipd_record.getAttrAs<SQLSMALLINT>(SQL_DESC_PRECISION,
            (binding_info.sql_type == SQL_DECIMAL || binding_info.sql_type == SQL_NUMERIC ? 38 : 0)
        );

        param_bindings.emplace_back(binding_info);
    }

    if (array_status_ptr)
        array_status_ptr[param_set_idx] = SQL_PARAM_SUCCESS; // TODO: elaborate?

    return param_bindings;
}

Descriptor& Statement::getEffectiveDescriptor(SQLINTEGER type) {
    switch (type) {
        case SQL_ATTR_APP_ROW_DESC:   return choose(implicit_ard, explicit_ard);
        case SQL_ATTR_APP_PARAM_DESC: return choose(implicit_apd, explicit_apd);
        case SQL_ATTR_IMP_ROW_DESC:   return choose(implicit_ird, explicit_ird);
        case SQL_ATTR_IMP_PARAM_DESC: return choose(implicit_ipd, explicit_ipd);
    }
    throw std::runtime_error("unknown descriptor type");
}

void Statement::setExplicitDescriptor(SQLINTEGER type, std::shared_ptr<Descriptor> desc) {
    switch (type) {
        case SQL_ATTR_APP_ROW_DESC:   explicit_ard = desc; return;
        case SQL_ATTR_APP_PARAM_DESC: explicit_apd = desc; return;
        case SQL_ATTR_IMP_ROW_DESC:   explicit_ird = desc; return;
        case SQL_ATTR_IMP_PARAM_DESC: explicit_ipd = desc; return;
    }
    throw std::runtime_error("unknown descriptor type");
}

void Statement::setImplicitDescriptor(SQLINTEGER type) {
    return setExplicitDescriptor(type, std::shared_ptr<Descriptor>{});
}

Descriptor & Statement::choose(
    std::shared_ptr<Descriptor> & implicit_desc,
    std::weak_ptr<Descriptor> & explicit_desc
) {
    auto desc = explicit_desc.lock();
    return (desc ? *desc : *implicit_desc);
}

void Statement::allocateImplicitDescriptors() {
    deallocateImplicitDescriptors();

    implicit_ard = allocateDescriptor();
    implicit_apd = allocateDescriptor();
    implicit_ird = allocateDescriptor();
    implicit_ipd = allocateDescriptor();

    getParent().init_as_desc(*implicit_ard, SQL_ATTR_APP_ROW_DESC);
    getParent().init_as_desc(*implicit_apd, SQL_ATTR_APP_PARAM_DESC);
    getParent().init_as_desc(*implicit_ird, SQL_ATTR_IMP_ROW_DESC);
    getParent().init_as_desc(*implicit_ipd, SQL_ATTR_IMP_PARAM_DESC);
}

void Statement::deallocateImplicitDescriptors() {
    deallocateDescriptor(implicit_ard);
    deallocateDescriptor(implicit_apd);
    deallocateDescriptor(implicit_ird);
    deallocateDescriptor(implicit_ipd);
}

std::shared_ptr<Descriptor> Statement::allocateDescriptor() {
    auto & desc = getParent().allocateChild<Descriptor>();
    return desc.shared_from_this();
}

void Statement::deallocateDescriptor(std::shared_ptr<Descriptor> & desc) {
    if (desc) {
        desc->deallocateSelf();
        desc.reset();
    }
}


void Statement::queryCallback(lcb_INSTANCE * instance, int type, const lcb_RESPANALYTICS * resp) {
    CallbackCookie * cookie;
    const char * row;
    const char *error_message;
    const char *_statement;
    const lcb_ANALYTICS_ERROR_CONTEXT *ctx;

    size_t error_message_len;
    size_t _statement_len;
    uint32_t first_error_code;
    size_t nrow;

    lcb_STATUS rc = lcb_respanalytics_status(resp);
    lcb_respanalytics_cookie(resp, (void **)&cookie);
    lcb_respanalytics_row(resp, &row, &nrow);

    if (rc == LCB_SUCCESS) {
        if (lcb_respanalytics_is_final(resp)) {
            cookie->queryMeta.assign(row, nrow);
        } else {
            cookie->queryResultStrm.write(row, nrow);
            cookie->queryResultStrm << "\n";
        }
    }
    if (rc != LCB_SUCCESS) {
        lcb_respanalytics_error_context(resp, &ctx);
        lcb_errctx_analytics_first_error_code(ctx , &first_error_code);
        lcb_errctx_analytics_first_error_message(ctx, &error_message, &error_message_len);
        lcb_errctx_analytics_statement(ctx, &_statement, &_statement_len);
        cookie->queryResultStrm << "Status Code: " << rc << std::endl << "Return Code: "  << first_error_code << std::endl << "Error Message: " << error_message << std::endl << "query_from_user: " << _statement << std::endl;
        cookie->errorInResponse = true;
    }
}


void Statement::handleGetTypeInfo(std::unique_ptr<ResultMutator> && mutator) {
  result_reader.reset();
  cbCookie.queryMeta = type_info_query_meta;

  cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"boolean\",\
                                \"DATA_TYPE\":" << SQL_SMALLINT << ",\
                                \"COLUMN_SIZE\":5,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":null,\
                                \"MAXIMUM_SCALE\":null,\
                                \"SQL_DATA_TYPE\":" << SQL_SMALLINT << ",\
                                \"SQL_DATETIME_SUB\":null,\
                                \"NUM_PREC_RADIX\":10,\
                                \"INTERVAL_PRECISION\":1\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"int8\",\
                                \"DATA_TYPE\":" << SQL_TINYINT << ",\
                                \"COLUMN_SIZE\":3,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":0,\
                                \"MAXIMUM_SCALE\":0,\
                                \"SQL_DATA_TYPE\":" << SQL_TINYINT << ",\
                                \"SQL_DATETIME_SUB\":null,\
                                \"NUM_PREC_RADIX\":10,\
                                \"INTERVAL_PRECISION\":3\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"int16\",\
                                \"DATA_TYPE\":" << SQL_SMALLINT << ",\
                                \"COLUMN_SIZE\":5,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":0,\
                                \"MAXIMUM_SCALE\":0,\
                                \"SQL_DATA_TYPE\":" << SQL_SMALLINT << ",\
                                \"SQL_DATETIME_SUB\":null,\
                                \"NUM_PREC_RADIX\":10,\
                                \"INTERVAL_PRECISION\":5\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"int32\",\
                                \"DATA_TYPE\":" << SQL_INTEGER << ",\
                                \"COLUMN_SIZE\":10,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":0,\
                                \"MAXIMUM_SCALE\":0,\
                                \"SQL_DATA_TYPE\":" << SQL_INTEGER << ",\
                                \"SQL_DATETIME_SUB\":null,\
                                \"NUM_PREC_RADIX\":10,\
                                \"INTERVAL_PRECISION\":10\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"int64\",\
                                \"DATA_TYPE\":" << SQL_BIGINT << ",\
                                \"COLUMN_SIZE\":19,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":0,\
                                \"MAXIMUM_SCALE\":0,\
                                \"SQL_DATA_TYPE\":" << SQL_BIGINT << ",\
                                \"SQL_DATETIME_SUB\":null,\
                                \"NUM_PREC_RADIX\":10,\
                                \"INTERVAL_PRECISION\":19\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"float\",\
                                \"DATA_TYPE\":" << SQL_FLOAT << ",\
                                \"COLUMN_SIZE\":15,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":0,\
                                \"MAXIMUM_SCALE\":0,\
                                \"SQL_DATA_TYPE\":" << SQL_FLOAT << ",\
                                \"SQL_DATETIME_SUB\":null,\
                                \"NUM_PREC_RADIX\":2,\
                                \"INTERVAL_PRECISION\":7\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"double\",\
                                \"DATA_TYPE\":" << SQL_DOUBLE << ",\
                                \"COLUMN_SIZE\":15,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":0,\
                                \"MAXIMUM_SCALE\":0,\
                                \"SQL_DATA_TYPE\":" << SQL_DOUBLE << ",\
                                \"SQL_DATETIME_SUB\":null,\
                                \"NUM_PREC_RADIX\":2,\
                                \"INTERVAL_PRECISION\":15\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"string\",\
                                \"DATA_TYPE\":" << SQL_VARCHAR << ",\
                                \"COLUMN_SIZE\":2147483647,\
                                \"LITERAL_PREFIX\":\"'\",\
                                \"LITERAL_SUFFIX\":\"'\",\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":null,\
                                \"MAXIMUM_SCALE\":null,\
                                \"SQL_DATA_TYPE\":" << SQL_VARCHAR << ",\
                                \"SQL_DATETIME_SUB\":null,\
                                \"NUM_PREC_RADIX\":10,\
                                \"INTERVAL_PRECISION\":32767\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"date\",\
                                \"DATA_TYPE\":" << SQL_TYPE_DATE << ",\
                                \"COLUMN_SIZE\":10,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":0,\
                                \"MAXIMUM_SCALE\":0,\
                                \"SQL_DATA_TYPE\":" << SQL_DATETIME << ",\
                                \"SQL_DATETIME_SUB\":" << SQL_CODE_DATE << ",\
                                \"NUM_PREC_RADIX\":null,\
                                \"INTERVAL_PRECISION\":null\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"time\",\
                                \"DATA_TYPE\":" << SQL_TYPE_TIME << ",\
                                \"COLUMN_SIZE\":8,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":0,\
                                \"MAXIMUM_SCALE\":0,\
                                \"SQL_DATA_TYPE\":" << SQL_DATETIME << ",\
                                \"SQL_DATETIME_SUB\":" << SQL_CODE_TIME << ",\
                                \"NUM_PREC_RADIX\":null,\
                                \"INTERVAL_PRECISION\":null\
                              }" << "\n";
    cbCookie.queryResultStrm << "{\"TYPE_NAME\":\"datetime\",\
                                \"DATA_TYPE\":" << SQL_TYPE_TIMESTAMP << ",\
                                \"COLUMN_SIZE\":23,\
                                \"LITERAL_PREFIX\":null,\
                                \"LITERAL_SUFFIX\":null,\
                                \"CREATE_PARAMS\":null,\
                                \"NULLABLE\":" << SQL_NULLABLE << ",\
                                \"CASE_SENSITIVE\":" << SQL_FALSE << ",\
                                \"SEARCHABLE\":" << SQL_SEARCHABLE << ",\
                                \"UNSIGNED_ATTRIBUTE\":" << SQL_FALSE << ",\
                                \"FIXED_PREC_SCALE\":" << SQL_FALSE << ",\
                                \"AUTO_UNIQUE_VALUE\":null,\
                                \"LOCAL_TYPE_NAME\":null,\
                                \"MINIMUM_SCALE\":0,\
                                \"MAXIMUM_SCALE\":0,\
                                \"SQL_DATA_TYPE\":" << SQL_DATETIME << ",\
                                \"SQL_DATETIME_SUB\":" << SQL_CODE_TIMESTAMP << ",\
                                \"NUM_PREC_RADIX\":null,\
                                \"INTERVAL_PRECISION\":null\
                              }" << "\n";
  result_reader = make_result_reader(
                      "CBAS",
                      "crap",
                      *in,
                      std::move(mutator),
                      cbCookie,
                      nullptr);
}
std::string Statement::nativeSql(const std::string & q) {
    prepareQuery(q);
    is_prepared = false;
    return query;
}