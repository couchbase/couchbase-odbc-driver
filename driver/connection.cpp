#include <string.h>

#include "driver/cJSON.h"
#include "driver/config/ini_defines.h"
#include "driver/connection.h"
#include "driver/descriptor.h"
#include "driver/statement.h"
#include "driver/utils/utils.h"

#include <random>

extern "C" {
lcb_STATUS lcb_createopts_destroy(lcb_CREATEOPTS *);
lcb_STATUS lcb_createopts_connstr(lcb_CREATEOPTS *,const char *,size_t);
lcb_STATUS lcb_createopts_credentials(lcb_CREATEOPTS *, const char *,size_t , const char *, size_t );
}


#if !defined(WORKAROUND_DISABLE_SSL)
#    include <Poco/Net/AcceptCertificateHandler.h>
#    include <Poco/Net/RejectCertificateHandler.h>

#    include <Poco/Net/HTTPSClientSession.h>
#    include <Poco/Net/InvalidCertificateHandler.h>
#    include <Poco/Net/PrivateKeyPassphraseHandler.h>
#    include <Poco/Net/SSLManager.h>
#endif

std::once_flag ssl_init_once;

#if !defined(WORKAROUND_DISABLE_SSL)
void SSLInit(bool ssl_strict, const std::string & private_key_file, const std::string & certificate_file, const std::string & caLocation) {
// http://stackoverflow.com/questions/18315472/https-request-in-c-using-poco
    Poco::Net::initializeSSL();
    Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> ptrHandler;
    if (ssl_strict)
        ptrHandler = new Poco::Net::RejectCertificateHandler(false);
    else
        ptrHandler = new Poco::Net::AcceptCertificateHandler(false);
    Poco::Net::Context::Ptr ptrContext = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE,
        private_key_file
#    if !defined(SECURITY_WIN32)
        // Do not work with poco/NetSSL_Win:
        ,
        certificate_file,
        caLocation,
        ssl_strict ? Poco::Net::Context::VERIFY_STRICT : Poco::Net::Context::VERIFY_RELAXED,
        9,
        true,
        "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"
#    endif
    );
    Poco::Net::SSLManager::instance().initializeClient(0, ptrHandler, ptrContext);
}
#endif

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

    LOG("Creating session with " << proto << "://" << server << ":" << port << ", DB: " << bucket);
    const char * cb_username = username.c_str();
    const char * cb_password = password.c_str();
    char conn_str[1024];
    const std::string EMPTY_STRING = "";
    if (port != 0 && !proto.empty()) {
        if (sprintf(conn_str, "couchbase://%s:%d=%s/%s", server.c_str(), port, proto.c_str(), bucket.c_str()) >= 1024) {
            std::cout << "Insufficient conn_str buffer space\n";
        }
    } else if (port == 0 && proto == EMPTY_STRING) {
        if (sprintf(conn_str, "couchbase://%s/%s", server.c_str(), bucket.c_str()) >= 1024) {
            std::cout << "Insufficient conn_str buffer space\n";
        }
    } else {
        LOG("Invalid Config, Either supply both port and proto or none\n");
    }
    lcb_CREATEOPTS * lcb_create_options = NULL;
    lcb_createopts_create(&lcb_create_options, LCB_TYPE_BUCKET);
    lcb_createopts_connstr(lcb_create_options, conn_str, strlen(conn_str));
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
}

void Connection::resetConfiguration() {
    dsn.clear();
    url.clear();
    proto.clear();
    username.clear();
    password.clear();
    server.clear();
    port = 0;
    connection_timeout = 0;
    timeout = 0;
    query_timeout = 0;
    sslmode.clear();
    private_key_file.clear();
    certificate_file.clear();
    ca_location.clear();
    path.clear();
    default_format.clear();
    bucket.clear();
    browse_result.clear();
    browse_connect_step = 0;
    string_max_length = 0;
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
        else if (Poco::UTF8::icompare(key, INI_PORT) == 0) {
            recognized_key = true;
            unsigned int typed_value = 0;
            valid_value = (value.empty() || (
                Poco::NumberParser::tryParseUnsigned(value, typed_value) &&
                typed_value > 0 &&
                typed_value <= std::numeric_limits<decltype(port)>::max()
            ));
            if (valid_value) {
                port = typed_value;
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
            valid_value = (
                value.empty() ||
                Poco::UTF8::icompare(value, "allow") == 0 ||
                Poco::UTF8::icompare(value, "prefer") == 0 ||
                Poco::UTF8::icompare(value, "require") == 0
            );
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
        else if (Poco::UTF8::icompare(key, INI_BUCKET) == 0) {
            recognized_key = true;
            valid_value = true;
            if (valid_value) {
                bucket = value;
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

        if (port == 0) {
            // TODO(dakovalkov): This doesn't work when you explicitly set 80 for http and 443 for https due to Poco's getPort() behavior.
            const auto tmp_port = uri.getPort();
            if (
                (Poco::UTF8::icompare(proto, "https") == 0 && tmp_port != 443) ||
                (Poco::UTF8::icompare(proto, "http") == 0 && tmp_port != 80)
            )
                port = tmp_port;
        }

        if (path.empty())
            path = uri.getPath();

        for (const auto& parameter : uri.getQueryParameters()) {
            if (Poco::UTF8::icompare(parameter.first, "default_format") == 0) {
                default_format = parameter.second;
            }
            else if (Poco::UTF8::icompare(parameter.first, "bucket") == 0) {
                bucket = parameter.second;
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

    if (bucket.empty())
        bucket = "default";

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