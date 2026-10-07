//This is a simple progran that connects to couchbase analytics/ operational insights WITHOUT SSL
#include <libcouchbase/couchbase.h>
#include <libcouchbase/vbucket.h>
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
    // std::cout << "Enter the host name: ";
    // std::cin>>host;
    // std::cout << "Enter the port: ";
    // std::cin>>port;

    // std::string conn_str = "couchbase://" + host + ":" + port;
    std::string conn_str = "couchbase://Couchbase-EA-NLB-ccdc0c52e93096c8.elb.ap-southeast-2.amazonaws.com:8091=http?network=external";
    std::cout << "Enter the username: ";
    std::string username;
    std::cin>>username;

    std::cout << "Enter the password: ";
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

    // DIAGNOSTIC CODE: Inspect SDK's internal configuration
    std::cout << "\n========== DIAGNOSTIC INFORMATION ==========" << std::endl;
    std::cout << "Network setting: external (from connection string)" << std::endl;

    // Get vbucket config
    lcbvb_CONFIG *vbc = nullptr;
    lcb_cntl(instance, LCB_CNTL_GET, LCB_CNTL_VBCONFIG, &vbc);

    if (vbc) {
        unsigned nservers = lcbvb_get_nservers(vbc);
        std::cout << "Number of servers in config: " << nservers << std::endl;
        std::cout << "Config revision: " << lcbvb_get_revision(vbc) << std::endl;

        // Check all service ports for each node to see if alternate addresses are working
        std::cout << "\n--- Service Ports (PLAIN mode) for Node 0 ---" << std::endl;
        for (unsigned i = 0; i < nservers; i++) {
            const char *hostname = lcbvb_get_hostname(vbc, i);
            unsigned mgmt_port = lcbvb_get_port(vbc, i, LCBVB_SVCTYPE_MGMT, LCBVB_SVCMODE_PLAIN);
            unsigned cbas_port = lcbvb_get_port(vbc, i, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
            unsigned kv_port = lcbvb_get_port(vbc, i, LCBVB_SVCTYPE_DATA, LCBVB_SVCMODE_PLAIN);

            std::cout << "Node " << i << " (" << (hostname ? hostname : "unknown") << "):" << std::endl;
            std::cout << "  Management port: " << (mgmt_port > 0 ? std::to_string(mgmt_port) : "NOT FOUND") << std::endl;
            std::cout << "  Analytics port (cbas): " << (cbas_port > 0 ? std::to_string(cbas_port) : "NOT FOUND") << std::endl;
            std::cout << "  KV port (data): " << (kv_port > 0 ? std::to_string(kv_port) : "NOT FOUND") << std::endl;

            const char *cbas_hostport = lcbvb_get_hostport(vbc, i, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
            const char *cbas_resturl = lcbvb_get_resturl(vbc, i, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
            std::cout << "  Analytics hostport: " << (cbas_hostport ? cbas_hostport : "NULL") << std::endl;
            std::cout << "  Analytics REST URL: " << (cbas_resturl ? cbas_resturl : "NULL") << std::endl;
        }

        // Try to find a random Analytics node
        std::cout << "\n--- Finding Analytics Node ---" << std::endl;
        int rand_ix = lcbvb_get_randhost(vbc, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
        if (rand_ix >= 0) {
            std::cout << "Found Analytics node at index: " << rand_ix << std::endl;
            const char *resturl = lcbvb_get_resturl(vbc, rand_ix, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
            std::cout << "REST URL: " << (resturl ? resturl : "NULL") << std::endl;
        } else {
            std::cout << "ERROR: Could not find any Analytics node! (lcbvb_get_randhost returned " << rand_ix << ")" << std::endl;
        }
    } else {
        std::cout << "ERROR: Could not retrieve vbucket config!" << std::endl;
    }
    std::cout << "==========================================\n" << std::endl;

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
Expected Output example
------------------------------------------------
LOG: Connection String -> couchbase://localhost:11210
------------------------------------------------
:white_check_mark: Connected to Cluster!
Executing: select 1;...
Result: {"$1":1}
:white_check_mark: Query Completed.
*/