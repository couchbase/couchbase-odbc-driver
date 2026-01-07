//This is a simple progran that connects to couchbase analytics/ enterprise analytics WITHOUT SSL
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

    // host is localhost if the server is running locally
    std::string host;
    // port is the kv port
    // 11210 is the default kv port for 8091 mgmt port
    // 12000 is the default kv port for 9000 mgmt port
    // give this value based on that
    std::string port;

    //take inputs
    std::cout << "Enter the host name: ";
    std::cin>>host;
    std::cout << "Enter the port: ";
    std::cin>>port;

    std::string conn_str = "couchbase://" + host + ":" + port;

    std::cout << "Enter the username: ";
    std::string username;
    std::cin>>username;

    std::cout << "Enter the passworde: ";
    std::string password;
    std::cin>>password;

    // PRINTING THE CONNECTION STRING
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "LOG: Connection String -> " << conn_str << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    lcb_createopts_create(&options, LCB_TYPE_CLUSTER);
    lcb_createopts_connstr(options, conn_str.c_str(), conn_str.length());
    lcb_createopts_credentials(options, username.c_str(), username.length(), password.c_str(), password.length());

    lcb_STATUS err = lcb_create(&instance, options);
    lcb_createopts_destroy(options);

    if (err != LCB_SUCCESS) {
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
    std::string query = "select 1;";
    // std::string query = "select * from `travel-sample`.inventory.landmark limit 1;";
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
g++ -std=c++17 0.connect_to_analytics_without_ssl.cpp -o prog \
  -I/opt/homebrew/opt/libcouchbase/include \
  -L/opt/homebrew/opt/libcouchbase/lib \
  -lcouchbase

Step 2:  command to run:
./prog

Step 3: give inputs -> host, port, username and password

Expected Output example
------------------------------------------------
LOG: Connection String -> couchbase://localhost:11210
------------------------------------------------
:white_check_mark: Connected to Cluster!
Executing: select 1;...
Result: {"$1":1}
:white_check_mark: Query Completed.


In case of error, set the logging:
export LCB_LOGLEVEL=5 -> from the terminal

rerun the program
*/