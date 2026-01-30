//This is a simple program that connects to couchbase analytics/ enterprise analytics WITH client cert auth
#include <libcouchbase/couchbase.h>
#include <iostream>
#include <string>

void analytics_callback(lcb_INSTANCE *instance, int type, const lcb_RESPANALYTICS *resp) {
    lcb_STATUS rc = lcb_respanalytics_status(resp);
    if (rc != LCB_SUCCESS) {
        std::cerr << "Query Failed: " << lcb_strerror_short(rc) << std::endl;
        return;
    }
    if (lcb_respanalytics_is_final(resp)) {
        std::cout << ":white_check_mark: Query Completed." << std::endl;
    } else {
        const char *row;
        size_t nrow;
        lcb_respanalytics_row(resp, &row, &nrow);
        std::cout << "Result: " << std::string(row, nrow) << std::endl;
    }
}

int main() {
    lcb_INSTANCE *instance;
    lcb_CREATEOPTS *options = NULL;

    // note the port '11307' is the KV Port configured using alternate address for 9091 mgmt port
    // 11207 can be used for 8091 mgmt port
    // 11998 for the 9000 mgmt port
    // std::string conn_str =
    //         "couchbases://localhost:11307?truststorepath=/Users/janhavi.tripurwar/Desktop/test/server_ca.pem&certpath=/Users/janhavi.tripurwar/Desktop/test/user.pem&keypath=/Users/janhavi.tripurwar/Desktop/test/user.key";

    // This connection string is for the following setup
    // EA/operational running on aws -> Aws NLB (internet facing)
    // NLB has exposed ports (18091, 18095), no KV port (11207) exposed
    // mgmtSSL and cbasSSl (18091, 18095) ports addred in alternate address also
    // config_node_timeout=10000000 is required beacuse LCB_ERROR_TIMEOUT was coming
    std::string conn_str = "couchbases://ea-nlb-9311f41fc97dec77.elb.ap-southeast-2.amazonaws.com:18091=http?truststorepath=/Users/janhavi.tripurwar/Desktop/test/server_ca.pem&certpath=/Users/janhavi.tripurwar/Desktop/test/user.pem&keypath=/Users/janhavi.tripurwar/Desktop/test/user.key&network=external&config_node_timeout=10000000";

    // PRINTING THE CONNECTION STRING
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "LOG: Connection String -> " << conn_str << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    lcb_createopts_create(&options, LCB_TYPE_CLUSTER);
    lcb_createopts_connstr(options, conn_str.c_str(), strlen(conn_str.c_str()));

    lcb_STATUS err = lcb_create(&instance, options);
    lcb_createopts_destroy(options);

    if (err != LCB_SUCCESS) {
        std::cerr << "Failed to connect: " << lcb_strerror_short(err) << std::endl;
        return 1;
    }

    err = lcb_connect(instance);
    lcb_wait(instance, LCB_WAIT_DEFAULT);

    err = lcb_get_bootstrap_status(instance);
    if (err != LCB_SUCCESS) {
        std::cerr << "Failed to connect: " << lcb_strerror_short(err) << std::endl;
        lcb_destroy(instance);
        return 1;
    }
    std::cout << ":white_check_mark: Connected to Cluster!" << std::endl;

    lcb_CMDANALYTICS *cmd;
    // std::string query = "select 1;";
    std::string query = "select * from `travel-sample`.inventory.landmark limit 1;";
    lcb_cmdanalytics_create(&cmd);
    lcb_cmdanalytics_statement(cmd, query.c_str(), query.length());
    lcb_cmdanalytics_callback(cmd, analytics_callback);

    std::cout << "Executing: " << query << "..." << std::endl;
    err = lcb_analytics(instance, NULL, cmd);
    lcb_cmdanalytics_destroy(cmd);

    if (err == LCB_SUCCESS) {
        lcb_wait(instance, LCB_WAIT_DEFAULT);
    } else {
        std::cerr << "Failed to schedule query: " << lcb_strerror_short(err) << std::endl;
    }

    lcb_destroy(instance);
    return 0;
}


/*
Step 1: command to compile:
g++ -std=c++17 2.client_cert_auth.cpp -o prog \
  -I/opt/homebrew/opt/libcouchbase/include \
  -L/opt/homebrew/opt/libcouchbase/lib \
  -lcouchbase

Step 2:  command to compile:
./prog


Expected Output example
------------------------------------------------
LOG: Connection String -> couchbases://localhost:11307?truststorepath=/Users/janhavi.tripurwar/Desktop/certs/server_ca.pem&certpath=/Users/janhavi.tripurwar/Desktop/certs/user.pem&keypath=/Users/janhavi.tripurwar/Desktop/certs/user.key
------------------------------------------------
:white_check_mark: Connected to Cluster!
Executing: select * from `travel-sample`.inventory.landmark limit 1;...
Result: ...
:white_check_mark: Query Completed.


In case of error, set the logging:
export LCB_LOGLEVEL=5 -> from the terminal


Check what ports are mapped to using http://localhost:8091/pools/default/nodeServices
or
http://localhost:9000/pools/default/nodeServices
rerun the program
*/