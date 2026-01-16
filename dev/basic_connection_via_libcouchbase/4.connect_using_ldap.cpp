//This is a simple progran that connects to couchbase analytics/ enterprise analytics using LDAP
//doc: https://docs.google.com/document/d/1DkxViLgW-e-Y83oISY5kuy0wPyR36I2G2iLi-b3FjQE/edit?usp=sharing

#include <libcouchbase/couchbase.h>
#include <iostream>
#include <string>

void analytics_callback(lcb_INSTANCE* instance, int type, const lcb_RESPANALYTICS* resp) {
    lcb_STATUS rc = lcb_respanalytics_status(resp);
    if (rc != LCB_SUCCESS) {
        std::cerr << "Query Failed: " << lcb_strerror_short(rc) << std::endl;
        return;
    }
    if (lcb_respanalytics_is_final(resp)) {
        std::cout << ":white_check_mark: Query Completed." << std::endl;
    }
    else {
        const char* row;
        size_t nrow;
        lcb_respanalytics_row(resp, &row, &nrow);
        std::cout << "Result: " << std::string(row, nrow) << std::endl;
    }
}


int main() {
    lcb_INSTANCE* instance;
    lcb_CREATEOPTS* options = NULL;

    std::string host = "localhost";

    //std::string port = "11210";
    /*std::string conn_str = "couchbase://" + host + ":" + port + "sasl_mech_force=PLAIN";*/
    std::string port = "11207";
    std::string conn_str = "couchbases://" + host + ":" + port + "?truststorepath=C:\\Users\\Janhavi\\Documents\\server.txt&sasl_mech_force=PLAIN";

    std::string username = "testuser";
    std::string password = "password";


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


    lcb_CMDANALYTICS* cmd;
    std::string query = "SELECT * from wow.sc.col;";
    lcb_cmdanalytics_create(&cmd);
    lcb_cmdanalytics_statement(cmd, query.c_str(), query.length());
    lcb_cmdanalytics_callback(cmd, analytics_callback);


    std::cout << "Executing: " << query << "..." << std::endl;
    err = lcb_analytics(instance, NULL, cmd);
    lcb_cmdanalytics_destroy(cmd);


    if (err == LCB_SUCCESS) {
        lcb_wait(instance, LCB_WAIT_DEFAULT);
    }
    else {
        std::cerr << "Failed to schedule query: " << lcb_strerror_short(err) << std::endl;
    }


    lcb_destroy(instance);
    return 0;
}