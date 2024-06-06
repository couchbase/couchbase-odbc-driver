#pragma once

#include "driver/include/driver.h"
#include "driver/include/environment.h"
#include "driver/config/config.h"

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/URI.h>

#include <libcouchbase/couchbase.h>

#include <memory>
#include <mutex>

class DescriptorRecord;
class Descriptor;
class Statement;

class Connection : public Child<Environment, Connection> {
private:
    using ChildType = Child<Environment, Connection>;
    const std::string session_id;
    std::string url;
    std::string proto;
    std::string password;
    int port = 0;
    bool verify_connection_early = false;
    std::string sslmode;
    std::string private_key_file;
    std::string certificate_file;
    std::string ca_location;
    std::string path;
    std::string default_format;
    bool huge_int_as_string = false;
    // Reset all configuration fields to their default/unintialized values.
    void resetConfiguration();
    // Set configuration fields to:
    //     a) values from connection string, or
    //     b) values from DSN, if unintialized, or
    //     c) values deduced from values of other fields, if unintialized.
    void setConfiguration(const key_value_map_t & cs_fields, const key_value_map_t & dsn_fields);
    std::unordered_map<SQLHANDLE, std::shared_ptr<Descriptor>> descriptors;
    std::unordered_map<SQLHANDLE, std::shared_ptr<Statement>> statements;
    const int DEFAULT_PRODUCTION_SSL_PORT = 11207;
    const int DEFAULT_PRODUCTION_PORT = 11210;
public:
    std::string dsn;
    std::uint32_t login_timeout;
    bool is_set_login_timeout;
    std::uint32_t query_timeout;
    std::string username;
    std::string server;
    std::uint32_t connection_timeout = 0;
    std::uint32_t timeout = 0;
    std::string catalog;
    std::int32_t string_max_length = 0;
    std::string browse_result;
    int browse_connect_step = 0;
    std::string useragent;
    std::unique_ptr<Poco::Net::HTTPClientSession> session;
    int retry_count = 3;
    int redirect_limit = 10;
    bool database_entity_support = false;
    bool connect_to_capella = false;
    bool two_part_scope_name = false;
    std::string scope_part_one;
    std::string scope_part_two;
    lcb_INSTANCE* lcb_instance;
    explicit Connection(Environment & environment);
    // Lookup TypeInfo for given name of type.
    const TypeInfo & getTypeInfo(const std::string & type_name, const std::string & type_name_without_parameters) const;
    void connect(const std::string & connection_string);
    void cb_check(lcb_STATUS err, const char * msg);
    // Reset the descriptor and initialize it with default attributes.
    void init_as_ad(Descriptor & desc, bool user = false); // as Application Descriptor
    void init_as_id(Descriptor & desc); // as Implementation Descriptor

    // Reset the descriptor and initialize it with default attributes.
    void init_as_desc(Descriptor & desc, SQLINTEGER role, bool user = false); // ARD, APD, IRD, IPD

    // Reset the descriptor record and initialize it with default attributes.
    void init_as_adrec(DescriptorRecord & rec); // as a record of Application Descriptor
    void init_as_idrec(DescriptorRecord & rec); // as a record of Implementation Descriptor

    // Reset the descriptor record and initialize it with default attributes.
    void init_as_desc_rec(DescriptorRecord & rec, SQLINTEGER desc_role); // ARD, APD, IRD, IPD

    // Leave unimplemented for general case.
    template <typename T> T & allocateChild();
    template <typename T> void deallocateChild(SQLHANDLE) noexcept;

    std::string handleNativeSql(const std::string& q);

    //Build Connection String
    void build_conn_str_on_prem_ssl(char *);
    void build_conn_str_capella(char *);
    void build_conn_str_on_prem_without_ssl(char *);
    void check_if_two_part_scope_name();
    int extractPort(std::string&);
    void setDefaultPortIfZero(int&, int);
};

template <> Descriptor& Connection::allocateChild<Descriptor>();
template <> void Connection::deallocateChild<Descriptor>(SQLHANDLE handle) noexcept;

template <> Statement& Connection::allocateChild<Statement>();
template <> void Connection::deallocateChild<Statement>(SQLHANDLE handle) noexcept;
