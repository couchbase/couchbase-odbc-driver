#include <string.h>

#include "driver/include/cJSON.h"
#include "driver/config/ini_defines.h"
#include "driver/include/connection.h"
#include "driver/include/descriptor.h"
#include "driver/include/statement.h"
#include "driver/utils/include/utils.h"
#include "driver/utils/include/WinReg_Value_Fetcher.h"
#include "driver/utils/include/database_entity_support.h"

#include <random>

extern "C" {
lcb_STATUS lcb_createopts_destroy(lcb_CREATEOPTS *);
lcb_STATUS lcb_createopts_connstr(lcb_CREATEOPTS *,const char *,size_t);
lcb_STATUS lcb_createopts_credentials(lcb_CREATEOPTS *, const char *,size_t , const char *, size_t );
}

std::string GenerateSessionId() {
    std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<std::uint64_t> distribution(0);
    return std::string("couchbase_odbc_" + std::to_string(distribution(generator)));
}

Connection::Connection(Environment & environment)
    : ChildType(environment),
      session_id(GenerateSessionId())
{
    resetConfiguration();
}

const TypeInfo & Connection::getTypeInfo(const std::string & type_name, const std::string & type_name_without_parameters) const {
    auto tmp_type_name = type_name;
    auto tmp_type_name_without_parameters = type_name_without_parameters;

    const auto tmp_type_without_parameters_id = convertUnparametrizedTypeNameToTypeId(tmp_type_name_without_parameters);

    if (huge_int_as_string && tmp_type_without_parameters_id == DataSourceTypeId::UInt64) {
        tmp_type_name = "String";
        tmp_type_name_without_parameters = "String";
    }

    return getParent().getTypeInfo(tmp_type_name, tmp_type_name_without_parameters);
}

void Connection::cb_check(lcb_STATUS err, const char * msg) {
    if (err != LCB_SUCCESS) {
        fprintf(stdout, "[\x1b[31mERROR\x1b[0m] %s: %s\n", msg, lcb_strerror_short(err));
        throw std::runtime_error(lcb_strerror_long(err));
    }
}

void Connection::connect(const std::string & connection_string) {
    if (session && session->connected())
        throw SqlException("Connection name in use", "08002");

    auto cs_fields = readConnectionString(connection_string);

    const auto driver_cs_it = cs_fields.find(INI_DRIVER);
    const auto filedsn_cs_it = cs_fields.find(INI_FILEDSN);
    const auto savefile_cs_it = cs_fields.find(INI_SAVEFILE);
    const auto dsn_cs_it = cs_fields.find(INI_DSN);

    if (filedsn_cs_it != cs_fields.end())
        throw SqlException("Optional feature not implemented", "HYC00");

    if (savefile_cs_it != cs_fields.end())
        throw SqlException("Optional feature not implemented", "HYC00");

    key_value_map_t dsn_fields;

    // DRIVER and DSN won't exist in the field map at the same time, readConnectionString() will take care of that.
    if (driver_cs_it == cs_fields.end()) {
        std::string dsn_cs_val;

        if (dsn_cs_it != cs_fields.end())
            dsn_cs_val = dsn_cs_it->second;

        dsn_fields = readDSNInfo(dsn_cs_val);

        // Remove common but unused keys, if any.
        dsn_fields.erase(INI_DRIVER);
        dsn_fields.erase(INI_DESC);

        // Report and remove totally unexpected keys, if any.

        if (dsn_fields.find(INI_DSN) != dsn_fields.end()) {
            LOG("Unexpected key " << INI_DSN << " in DSN, ignoring");
            dsn_fields.erase(INI_DSN);
        }

        if (dsn_fields.find(INI_FILEDSN) != dsn_fields.end()) {
            LOG("Unexpected key " << INI_FILEDSN << " in DSN, ignoring");
            dsn_fields.erase(INI_FILEDSN);
        }

        if (dsn_fields.find(INI_SAVEFILE) != dsn_fields.end()) {
            LOG("Unexpected key " << INI_SAVEFILE << " in DSN, ignoring");
            dsn_fields.erase(INI_SAVEFILE);
        }
    }
    else {
        // Remove common but unused key.
        cs_fields.erase(driver_cs_it);
    }

    resetConfiguration();
    setConfiguration(cs_fields, dsn_fields);

    LOG("Creating session with " << proto << "://" << server << ":" << ", DB: " << catalog);
    const char * cb_username = nullptr;
    const char * cb_password = nullptr;
    std::string conn_str;
    connectInSSLMode = isYes(sslmode);

    if (Poco::UTF8::icompare(auth_mode, "certificate") == 0) {
        // For Client Cert, username/password are NOT used
        cb_username = "";
        cb_password = "";
    } else {
        cb_username = username.c_str();
        cb_password = password.c_str();
    }

    #ifdef _WIN32
        if(collect_logs){
            log_file = getLcbLogPath();
            if(!log_file.empty()){
                redirectCoutToFile(log_file, outFile,  coutBuffer);
            }
        }
    #endif

    logConnectionParams();

    if(connect_to_capella){
        //Always in SSL Mode, uses public certificate.
        build_conn_str_capella(conn_str);
    }
    else if(!connect_to_capella) {
        if(connectInSSLMode){
            std::cout<<"\nLOG: Inside connectInSSLMode";
            build_conn_str_on_prem_ssl(conn_str);
        }
        else {
            std::cout<<"\nLOG: Inside !connectInSSLMode";
            build_conn_str_on_prem_without_ssl(conn_str);
        }
    }
    else {
        std::cout<<"\nLOG: Neither Capella nor Couchbase Server is selected";
    }

    lcb_CREATEOPTS * lcb_create_options = NULL;
    lcb_createopts_create(&lcb_create_options, LCB_TYPE_CLUSTER);
    lcb_createopts_connstr(lcb_create_options, conn_str.c_str(), conn_str.length());
    lcb_createopts_credentials(lcb_create_options, cb_username, strlen(cb_username), cb_password, strlen(cb_password));
    try {
        cb_check(lcb_create(&lcb_instance, lcb_create_options), "create couchbase handle");
    } catch (std::exception & ex) {
        lcb_createopts_destroy(lcb_create_options);
        throw std::runtime_error(ex.what());
    }
    lcb_createopts_destroy(lcb_create_options);
    cb_check(lcb_connect(lcb_instance), "schedule connection");
    lcb_wait(lcb_instance, LCB_WAIT_DEFAULT);
    try {
        cb_check(lcb_get_bootstrap_status(lcb_instance), "bootstrap from cluster");
    } catch (std::exception & ex) {
        throw SqlException("Connection not open", "08003");
    }

    Statement statement(*this);
    database_entity_support = checkDatabaseEntitySupport(statement);
    check_if_two_part_scope_name();
}

void Connection::logConnectionParams() {
    auto now = std::chrono::system_clock::now();
    // Convert to time_t for human-readable format
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::cout << "\nLOG: Logs collected at: " << std::put_time(std::gmtime(&now_time), "%Y-%m-%d %H:%M:%S");
    std::cout << "\nLOG: log_file is :-> " << log_file;
    std::cout << "\nLOG: username is :-> " << username;
    std::cout << "\nLOG: password length is :-> " << password.length();
    std::cout << "\nLOG: sslmode is :-> " << sslmode;
    std::cout << "\nLOG: Connection String (URL) is :-> " << url;
    std::cout << "\nLOG: Host is :-> " << server;
    std::cout << "\nLOG: Scope/Database is :-> " << catalog;
    std::cout << "\nLOG: connectInSSLMode is :-> " << connectInSSLMode;
    std::cout << "\nLOG: connect_to_capella is :-> " << connect_to_capella;
    std::cout << "\nLOG: collect_logs path is :-> " << collect_logs;
    std::cout << "\nLOG: auth_mode is :-> " << auth_mode;
    std::cout << "\nLOG: client_cert path is :-> " << client_cert;
    std::cout << "\nLOG: client_key path is :-> " << client_key;
    std::cout << "\nLOG: advanced_params is :-> " << advanced_params;
    std::cout << "\nLOG: certificate_file path is :-> " << certificate_file;
}

void Connection::resetConfiguration() {
    dsn.clear();
    url.clear();
    proto.clear();
    username.clear();
    password.clear();
    server.clear();
    connection_timeout = 0;
    timeout = 0;
    query_timeout = 0;
    sslmode.clear();
    private_key_file.clear();
    certificate_file.clear();
    ca_location.clear();
    path.clear();
    default_format.clear();
    catalog.clear();
    browse_result.clear();
    browse_connect_step = 0;
    string_max_length = 0;
    advanced_params.clear();
    auth_mode.clear();
    client_cert.clear();
    client_key.clear();
}

void Connection::setConfiguration(const key_value_map_t & cs_fields, const key_value_map_t & dsn_fields) {
    // Returns tuple of bools: ("recognized key", "valid value").
    auto set_config_value = [&] (const std::string & key, const std::string & value) {
        bool recognized_key = false;
        bool valid_value = false;

        if (Poco::UTF8::icompare(key, INI_DSN) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                dsn = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_URL) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                url = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_CONNECT_TO_CAPELLA) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                connect_to_capella = (value == "yes");
            }
        }
        else if (Poco::UTF8::icompare(key, INI_COLLECT_LOG) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                collect_logs = (value == "yes");
            }
        }
        else if (
            Poco::UTF8::icompare(key, INI_UID) == 0 ||
            Poco::UTF8::icompare(key, INI_USERNAME) == 0
        ) {
            recognized_key = true;
            valid_value = (value.find(':') == std::string::npos);
            if (valid_value) {
                username = value;
            }
        }
        else if (
            Poco::UTF8::icompare(key, INI_PWD) == 0 ||
            Poco::UTF8::icompare(key, INI_PASSWORD) == 0
        ) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                password = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_PROTO) == 0) {
            recognized_key = true;
            valid_value = (
                value.empty() ||
                Poco::UTF8::icompare(value, "http") == 0 ||
                Poco::UTF8::icompare(value, "https") == 0
            );
            if (valid_value) {
                proto = value;
            }
        }
        else if (
            Poco::UTF8::icompare(key, INI_SERVER) == 0 ||
            Poco::UTF8::icompare(key, INI_HOST) == 0
        ) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                server = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_TIMEOUT) == 0) {
            recognized_key = true;
            unsigned int typed_value = 0;
            valid_value = (value.empty() || (
                Poco::NumberParser::tryParseUnsigned(value, typed_value) &&
                typed_value <= std::numeric_limits<decltype(timeout)>::max()
            ));
            if (valid_value) {
                timeout = typed_value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_VERIFY_CONNECTION_EARLY) == 0) {
            recognized_key = true;
            valid_value = (value.empty() || isYesOrNo(value));
            if (valid_value) {
                verify_connection_early = isYes(value);
            }
        }
        else if (Poco::UTF8::icompare(key, INI_SSLMODE) == 0) {
            recognized_key = true;
            valid_value = (value.empty() || isYesOrNo(value));
            if (valid_value) {
                sslmode = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_PRIVATEKEYFILE) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                private_key_file = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_CERTIFICATEFILE) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                certificate_file = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_CALOCATION) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                ca_location = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_PATH) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                path = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_CATALOG) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                catalog = value;
            }
        } else if (Poco::UTF8::icompare(key, INI_LOGIN_TIMEOUT) == 0) {
            recognized_key = true;
            unsigned int typed_value = 75;
            valid_value = (value.empty()
                || (Poco::NumberParser::tryParseUnsigned(value, typed_value)
                    && typed_value <= std::numeric_limits<decltype(login_timeout)>::max()));
            if (valid_value) {
                login_timeout = typed_value;
            }
        } else if (Poco::UTF8::icompare(key, INI_QUERY_TIMEOUT) == 0) {
            recognized_key = true;
            unsigned int typed_value = 0;
            valid_value = (value.empty()
                || (Poco::NumberParser::tryParseUnsigned(value, typed_value)
                    && typed_value <= std::numeric_limits<decltype(query_timeout)>::max()));
            if (valid_value) {
                query_timeout = typed_value;
            }
        } else if (Poco::UTF8::icompare(key, INI_HUGE_INT_AS_STRING) == 0) {
            recognized_key = true;
            valid_value = (value.empty() || isYesOrNo(value));
            if (valid_value) {
                huge_int_as_string = isYes(value);
            }
        } else if (Poco::UTF8::icompare(key, INI_STRINGMAXLENGTH) == 0) {
            recognized_key = true;
            unsigned int typed_value = 0;
            valid_value = (value.empty() || (
                Poco::NumberParser::tryParseUnsigned(value, typed_value) &&
                typed_value > 0 &&
                typed_value <= std::numeric_limits<decltype(string_max_length)>::max()
            ));
            if (valid_value) {
                string_max_length = typed_value;
            }
        } else if (Poco::UTF8::icompare(key, INI_DRIVERLOGFILE) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                getDriver().setAttr(CH_SQL_ATTR_DRIVERLOGFILE, value);
            }
        } else if (Poco::UTF8::icompare(key, INI_DRIVERLOG) == 0) {
            recognized_key = true;
            valid_value = (value.empty() || isYesOrNo(value));
            if (valid_value) {
                getDriver().setAttr(CH_SQL_ATTR_DRIVERLOG, (isYes(value) ? SQL_OPT_TRACE_ON : SQL_OPT_TRACE_OFF));
            }
        }
        else if (Poco::UTF8::icompare(key, INI_ADVANCED_PARAMS) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                advanced_params = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_AUTH_MODE) == 0) {
            recognized_key = true;
            valid_value = (Poco::UTF8::icompare(value, INI_AUTH_MODE_BASIC) == 0 ||
                           Poco::UTF8::icompare(value, INI_AUTH_MODE_CERT) == 0);
            if (valid_value) {
                auth_mode = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_CLIENT_CERT) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                client_cert = value;
            }
        }
        else if (Poco::UTF8::icompare(key, INI_CLIENT_KEY) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                client_key = value;
            }
        }

        return std::make_tuple(recognized_key, valid_value);
    };

    // Set recognised attributes from the DSN. Throw on invalid value. (This will overwrite the defaults.)
    for (auto & field : dsn_fields) {
        const auto & key = field.first;
        const auto & value = field.second;

        if (cs_fields.find(key) != cs_fields.end()) {
            LOG("DSN: attribute '" << key << " = " << value << "' unused, overriden by the connection string");
        }
        else {
            const auto res = set_config_value(key, value);
            const auto & recognized_key = std::get<0>(res);
            const auto & valid_value = std::get<1>(res);

            if (recognized_key) {
                if (!valid_value)
                    throw std::runtime_error("DSN: bad value '" + value + "' for attribute '" + key + "'");
            }
            else {
                LOG("DSN: unknown attribute '" << key << "', ignoring");
            }
        }
    }

    // Set recognised attributes from the connection string. Throw on invalid value. (This will overwrite the defaults, and those set from the DSN.)
    for (auto & field : cs_fields) {
        const auto & key = field.first;
        const auto & value = field.second;
        if (dsn_fields.find(key) != dsn_fields.end()) {
            LOG("Connection string: attribute '" << key << " = " << value << "' overrides DSN attribute with the same name");
        }
        const auto res = set_config_value(key, value);
        const auto & recognized_key = std::get<0>(res);
        const auto & valid_value = std::get<1>(res);
        if (recognized_key) {
            if (!valid_value)
                throw std::runtime_error("Connection string: bad value '" + value + "' for attribute '" + key + "'");
        }
        else {
            LOG("Connection string: unknown attribute '" << key << "', ignoring");
        }
    }

    // Deduce and set all the remaining attributes that are still carrying the default/unintialized values. (This will overwrite only some of the defaults.)
    if (dsn.empty())
        dsn = INI_DSN_DEFAULT;

    if (!url.empty()) {
        Poco::URI uri(url);

        if (proto.empty())
            proto = uri.getScheme();

        const auto & user_info = uri.getUserInfo();
        const auto index = user_info.find(':');
        if (index != std::string::npos) {
            if (password.empty())
                password = user_info.substr(index + 1);

            if (username.empty())
                username = user_info.substr(0, index);
        }

        if (server.empty())
            server = uri.getHost();

        if (path.empty())
            path = uri.getPath();

        for (const auto& parameter : uri.getQueryParameters()) {
            if (Poco::UTF8::icompare(parameter.first, "default_format") == 0) {
                default_format = parameter.second;
            }
            else if (Poco::UTF8::icompare(parameter.first, "catalog") == 0) {
                catalog = parameter.second;
            }
        }
    }


    if (username.empty())
        username = "default";

    if (server.empty())
        server = "localhost";


    if (connection_timeout == 0)
        connection_timeout = timeout;

    // ZeroValue != UnknownValue
    if (dsn_fields.find(INI_LOGIN_TIMEOUT) == dsn_fields.end() && cs_fields.find(INI_LOGIN_TIMEOUT) == cs_fields.end()
        && !is_set_login_timeout) {
        login_timeout = 5; // = LCB_DEFAULT_CONFIGURATION_TIMEOUT
    }
    if (dsn_fields.find(INI_QUERY_TIMEOUT) == dsn_fields.end() && cs_fields.find(INI_QUERY_TIMEOUT) == cs_fields.end()) {
        query_timeout = 75; // = LCB_DEFAULT_ANALYTICS_TIMEOUT
    }

    if (path.empty())
        path = "query";

    if (path[0] != '/')
        path = "/" + path;

    if (default_format.empty())
        default_format = "ODBCDriver2";

    if (catalog.empty())
        catalog = "default";

    if (string_max_length == 0)
        string_max_length = TypeInfo::string_max_size;
}

void Connection::init_as_ad(Descriptor & desc, bool user) {
    desc.resetAttrs();
    desc.setAttr(SQL_DESC_ALLOC_TYPE, (user ? SQL_DESC_ALLOC_USER : SQL_DESC_ALLOC_AUTO));
    desc.setAttr(SQL_DESC_ARRAY_SIZE, 1);
    desc.setAttr(SQL_DESC_ARRAY_STATUS_PTR, 0);
    desc.setAttr(SQL_DESC_BIND_OFFSET_PTR, 0);
    desc.setAttr(SQL_DESC_BIND_TYPE, SQL_BIND_TYPE_DEFAULT);
}

void Connection::init_as_id(Descriptor & desc) {
    desc.resetAttrs();
    desc.setAttr(SQL_DESC_ALLOC_TYPE, SQL_DESC_ALLOC_AUTO);
    desc.setAttr(SQL_DESC_ARRAY_STATUS_PTR, 0);
    desc.setAttr(SQL_DESC_ROWS_PROCESSED_PTR, 0);
}

void Connection::init_as_desc(Descriptor & desc, SQLINTEGER role, bool user) {
    switch (role) {
        case SQL_ATTR_APP_ROW_DESC: {
            init_as_ad(desc, user);
            break;
        }
        case SQL_ATTR_APP_PARAM_DESC: {
            init_as_ad(desc, user);
            break;
        }
        case SQL_ATTR_IMP_ROW_DESC: {
            init_as_id(desc);
            break;
        }
        case SQL_ATTR_IMP_PARAM_DESC: {
            init_as_id(desc);
            break;
        }
    }
}

void Connection::init_as_adrec(DescriptorRecord & rec) {
    rec.resetAttrs();
    rec.setAttr(SQL_DESC_TYPE, SQL_C_DEFAULT); // Also sets SQL_DESC_CONCISE_TYPE (to SQL_C_DEFAULT) and SQL_DESC_DATETIME_INTERVAL_CODE (to 0).
    rec.setAttr(SQL_DESC_OCTET_LENGTH_PTR, 0);
    rec.setAttr(SQL_DESC_INDICATOR_PTR, 0);
    rec.setAttr(SQL_DESC_DATA_PTR, 0);
}

void Connection::init_as_idrec(DescriptorRecord & rec) {
    rec.resetAttrs();
}

void Connection::init_as_desc_rec(DescriptorRecord & rec, SQLINTEGER desc_role) {
    switch (desc_role) {
        case SQL_ATTR_APP_ROW_DESC: {
            init_as_adrec(rec);
            break;
        }
        case SQL_ATTR_APP_PARAM_DESC: {
            init_as_adrec(rec);
            break;
        }
        case SQL_ATTR_IMP_ROW_DESC: {
            init_as_idrec(rec);
            break;
        }
        case SQL_ATTR_IMP_PARAM_DESC: {
            init_as_idrec(rec);
            rec.setAttr(SQL_DESC_PARAMETER_TYPE, SQL_PARAM_INPUT);
            break;
        }
    }
}

template <>
Descriptor& Connection::allocateChild<Descriptor>() {
    auto child_sptr = std::make_shared<Descriptor>(*this);
    auto& child = *child_sptr;
    auto handle = child.getHandle();
    descriptors.emplace(handle, std::move(child_sptr));
    return child;
}

template <>
void Connection::deallocateChild<Descriptor>(SQLHANDLE handle) noexcept {
    descriptors.erase(handle);
}

template <>
Statement& Connection::allocateChild<Statement>() {
    auto child_sptr = std::make_shared<Statement>(*this);
    auto& child = *child_sptr;
    auto handle = child.getHandle();
    statements.emplace(handle, std::move(child_sptr));
    return child;
}

template <>
void Connection::deallocateChild<Statement>(SQLHANDLE handle) noexcept {
    statements.erase(handle);
}

std::string Connection::handleNativeSql(const std::string & q) {
    auto & statement = allocateChild<Statement>();
    std::string query = statement.nativeSql(q);
    statement.deallocateSelf();
    return query;
}

void Connection::build_conn_str_on_prem_ssl(std::string& conn_str) {
    std::stringstream ss;
    ss << "couchbases://" << server << "?truststorepath=" << certificate_file;

    if (Poco::UTF8::icompare(auth_mode, "certificate") == 0) {
        ss << "&certpath=" << client_cert;
        ss << "&keypath=" << client_key;
    }
    bool hasQuery = true; // we already have ?truststorepath
    appendAdvancedParams(ss, advanced_params, hasQuery);

    if (collect_logs && !log_file.empty()) {
        ss << "&console_log_level=" << log_level
           << "&console_log_file=" << log_file;
    }

    conn_str = ss.str();
    std::cout << "\nLOG: Connection String: " << conn_str;
}

void Connection::build_conn_str_capella(std::string& conn_str) {
    std::stringstream ss;
    ss << url;

    if (collect_logs && !log_file.empty()) {
        ss << "?console_log_level=" << log_level
           << "&console_log_file=" << log_file;
    }

    conn_str = ss.str();
    std::cout << "\nLOG: Inside build_conn_str_capella WIN32 is :-> " << conn_str;
}

void Connection::build_conn_str_on_prem_without_ssl(std::string& conn_str) {
    std::stringstream ss;
    // couchbase://Host
    ss << "couchbase://" << server;

    bool hasQuery = false;
    appendAdvancedParams(ss, advanced_params, hasQuery);

    if (collect_logs && !log_file.empty()) {
        ss << "?console_log_level=" << log_level
           << "&console_log_file=" << log_file;
    }

    conn_str = ss.str();
    std::cout << "\nLOG: Non-SSL Connection String: " << conn_str;
}

void Connection::appendAdvancedParams(std::stringstream& ss, const std::string& params, bool& hasQuery) {
    if (params.empty()) return;

    std::stringstream pss(params);
    std::string line;

    while (std::getline(pss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string val = line.substr(eqPos + 1);

            Poco::trimInPlace(key);
            Poco::trimInPlace(val);

            if (!key.empty()) {
                ss << (hasQuery ? "&" : "?") << key << "=" << val;
                hasQuery = true;
            }
        }
    }
}

void Connection::check_if_two_part_scope_name(){
    size_t slashPosition = catalog.find("/");
    if (slashPosition != std::string::npos) {
        two_part_scope_name = true;

        // Extract substrings before and after the forward slash
        scope_part_one = catalog.substr(0, slashPosition);
        scope_part_two = catalog.substr(slashPosition + 1);
    }
}

void Connection::redirectCoutToFile(const std::string& filename, std::ofstream& outFile, std::streambuf*& coutBuffer) {
    // Open a file in write mode
    outFile.open(filename);

    // Save the current buffer of std::cout
    coutBuffer = std::cout.rdbuf();

    // Redirect std::cout to outFile
    std::cout.rdbuf(outFile.rdbuf());
}
