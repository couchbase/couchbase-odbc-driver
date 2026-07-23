// This is a simple program that connects to couchbase analytics/ enterprise analytics WITHOUT SSL
// This program has better analytics_callback logging, useful when libcouchbase not able to find analytics node.
#include <libcouchbase/couchbase.h>
#include <libcouchbase/vbucket.h>
#include <iostream>
#include <string>

void analytics_callback(lcb_INSTANCE *instance, int type, const lcb_RESPANALYTICS *resp) {
    const char *row;
    size_t nrow;
    lcb_respanalytics_row(resp, &row, &nrow);

    if (lcb_respanalytics_is_final(resp)) {
        lcb_STATUS rc = lcb_respanalytics_status(resp);
        std::cerr << "---> Query response finished" << std::endl;
        if (rc != LCB_SUCCESS) {
            std::cerr << "---> Query failed with library code " << lcb_strerror_short(rc) << std::endl;

            // Print error code (including LCB_ERR_UNSUPPORTED_OPERATION)
            std::cerr << "---> Error code: " << rc << " (" << lcb_strerror_long(rc) << ")" << std::endl;

            const lcb_ANALYTICS_ERROR_CONTEXT *ctx;
            lcb_respanalytics_error_context(resp, &ctx);
            if (ctx) {
                uint32_t code;
                lcb_errctx_analytics_http_response_code(ctx, &code);
                std::cerr << "---> HTTP response code: " << code << std::endl;

                const char *client_id;
                size_t nclient_id;
                lcb_errctx_analytics_client_context_id(ctx, &client_id, &nclient_id);
                if (client_id && nclient_id > 0) {
                    std::cerr << "---> Client context ID: " << std::string(client_id, nclient_id) << std::endl;
                }

                const char *errmsg;
                size_t nerrmsg;
                uint32_t err;
                lcb_errctx_analytics_first_error_message(ctx, &errmsg, &nerrmsg);
                lcb_errctx_analytics_first_error_code(ctx, &err);
                if (errmsg && nerrmsg > 0) {
                    std::cerr << "---> First query error: " << err << " (" << std::string(errmsg, nerrmsg) << ")" << std::endl;
                }
            }
        }
        if (row && nrow > 0) {
            std::cout << std::string(row, nrow) << std::endl;
        }
    } else {
        if (row && nrow > 0) {
            std::cout << std::string(row, nrow) << "," << std::endl;
        }
    }
}

int main() {
    lcb_INSTANCE *instance;
    lcb_CREATEOPTS *options = NULL;
    std::string host;

    std::string port;

    // std::string conn_str = "couchbase://op-nlb-de5844af20de841b.elb.ap-southeast-2.amazonaws.com:8091?network=external";
    // std::string conn_str = "couchbase://op-nlb-de5844af20de841b.elb.ap-southeast-2.amazonaws.com:8091/travel-sample?network=external";
    // std::string conn_str = "couchbase://ea-nlb-9311f41fc97dec77.elb.ap-southeast-2.amazonaws.com:8091?network=external";
    std::string conn_str = "couchbase://ea-nlb-9311f41fc97dec77.elb.ap-southeast-2.amazonaws.com:8091?network=external";


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
    // lcb_createopts_create(&options, LCB_TYPE_BUCKET);
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

    // DEBUG: Inspect cluster map to see what hostnames are configured
    std::cout << "\n==========================================" << std::endl;
    std::cout << "DEBUG: Inspecting Cluster Map" << std::endl;
    std::cout << "==========================================" << std::endl;

    lcbvb_CONFIG *vbc = nullptr;
    lcb_cntl(instance, LCB_CNTL_GET, LCB_CNTL_VBCONFIG, &vbc);

    if (vbc) {
        unsigned nservers = lcbvb_get_nservers(vbc);
        std::cout << "Number of servers in config: " << nservers << std::endl;
        std::cout << "Config revision: " << lcbvb_get_revision(vbc) << std::endl;

        // Check all nodes to see their hostnames and service endpoints
        for (unsigned i = 0; i < nservers; i++) {
            const char *hostname = lcbvb_get_hostname(vbc, i);
            unsigned mgmt_port = lcbvb_get_port(vbc, i, LCBVB_SVCTYPE_MGMT, LCBVB_SVCMODE_PLAIN);
            unsigned cbas_port = lcbvb_get_port(vbc, i, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
            unsigned kv_port = lcbvb_get_port(vbc, i, LCBVB_SVCTYPE_DATA, LCBVB_SVCMODE_PLAIN);

            const char *cbas_hostport = lcbvb_get_hostport(vbc, i, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
            const char *cbas_resturl = lcbvb_get_resturl(vbc, i, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
        }

        // Try to find a random Analytics node
        std::cout << "\n--- Finding Analytics Node ---" << std::endl;
        int rand_ix = lcbvb_get_randhost(vbc, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
        if (rand_ix >= 0) {
            std::cout << "Found Analytics node at index: " << rand_ix << std::endl;
            const char *resturl = lcbvb_get_resturl(vbc, rand_ix, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_PLAIN);
            std::cout << "REST URL: " << (resturl ? resturl : "NULL") << std::endl;
        } else {
            std::cout << "ERROR: Could not find any Analytics node!" << std::endl;
        }
    } else {
        std::cout << "ERROR: Could not retrieve vbucket config!" << std::endl;
    }
    std::cout << "==========================================\n" << std::endl;

    lcb_CMDANALYTICS *cmd;
    std::string query = "select * from `travel-sample`.inventory.airline limit 1;";
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
