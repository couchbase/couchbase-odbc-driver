// This is a simple program that connects to couchbase analytics/ enterprise analytics WITHOUT SSL
// and runs a query every 10 seconds.
#include <libcouchbase/couchbase.h>
#include <iostream>
#include <string>
#include <thread> // Added for sleep
#include <chrono> // Added for sleep

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

    std::string host;
    std::string port;

    //take inputs
    std::cout << "Enter the host name: ";
    std::cin >> host;
    std::cout << "Enter the port: ";
    std::cin >> port;

    std::string conn_str = "couchbase://" + host + ":" + port;

    std::cout << "Enter the username: ";
    std::string username;
    std::cin >> username;

    std::cout << "Enter the password: ";
    std::string password;
    std::cin >> password;

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
        std::cerr << "Failed to create instance: " << lcb_strerror_short(err) << std::endl;
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

    // --- LOOP START ---
    int count = 1;
    while (true) {
        std::cout << "\n--- Execution #" << count++ << " ---" << std::endl;

        lcb_CMDANALYTICS *cmd;
        std::string query = "select 1;";

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

        std::cout << "Sleeping for 10 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    // --- LOOP END ---

    lcb_destroy(instance);
    return 0;
}