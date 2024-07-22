#pragma once

#include "driver/platform/platform.h"
#include "driver/utils/include/utils.h"

#include <string>
#include <map>

using key_value_map_t = std::map<std::string, std::string, UTF8CaseInsensitiveCompare>;

/**
 * Structure to hold all the connection attributes for a specific
 * connection (used for both registry and file, DSN and DRIVER)
 */
struct ConnInfo {
    std::string drivername;
    std::string dsn;
    std::string desc;
    std::string url;
    std::string username;
    std::string password;
    std::string server;
    std::string timeout;
    std::string login_timeout;
    std::string query_timeout;
    std::string verify_connection_early;
    std::string sslmode;
    std::string private_key_file;
    std::string certificate_file;
    std::string caLocation;
    std::string catalog;
    std::string sid;
    std::string huge_int_as_string;
    std::string stringmaxlength;
    std::string driver_log;
    std::string driver_log_file;
    std::string connect_to_capella;
    std::string collect_logs;
};

void readDSNinfo(ConnInfo * ci, bool overwrite);
void writeDSNinfo(const ConnInfo * ci);
key_value_map_t readDSNInfo(const std::string & dsn);
key_value_map_t readConnectionString(const std::string & connection_string);
