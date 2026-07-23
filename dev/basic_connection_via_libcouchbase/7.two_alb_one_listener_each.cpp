// Connects to Enterprise Analytics through two ALBs with one listener each, and
// optionally stays alive to exercise cluster-map refresh.
//
// Topology (see BH/newSetup/ALB-SETUP.md):
//   ea-bootstrap ALB  HTTPS :18443 -> nodes:18091  (mgmt)
//   ea-analytics ALB  HTTPS :18443 -> nodes:18095  (cbasSSL)
//   alternate address on both nodes: hostname=ea-analytics..., ports cbasSSL=18443
//
// Usage
// -----
//   ./prog                 one iteration: connect, dump topology, run a query, exit
//   ./prog 30 10           30 iterations 10s apart: the refresh test
//   CB_USER / CB_PASS      credentials, else prompted
//   CB_CONNSTR             replace the connection string entirely
//
// Connection string examples for this topology
// --------------------------------------------
//   couchbases://<bootstrap-lb>:18443=https?truststorepath=<pem>&network=external
//       &pin_config_nodes=true
//   https-internal://<bootstrap-lb>:18443?truststorepath=<pem>&network=external
//       &pin_config_nodes=true        <- equivalent, scheme instead of =https
//   couchbases://<bootstrap-lb>:18443=https?truststorepath=<pem>&network=external
//                                     <- no pin: reproduces the failure below
//
// =https (or the https-internal:// scheme) is required, not optional: a port given
// without a type inherits the scheme's implicit type, so plain
// couchbases://host:18443 files 18443 as a KV port, leaves the HTTP bootstrap list
// empty, and fails to connect at all.
//
// What breaks without pin_config_nodes
// ------------------------------------
// config_updated() clears the node list and rebuilds it from config-derived MGMT
// endpoints, discarding the bootstrap LB from the connection string. Because the
// alternate address only sets cbasSSL, the mgmt port is inherited, so mgmt becomes
// ea-analytics:18091 -- a host:port no listener answers. Reachable-looking, but dead.
// pin_config_nodes=true (ODBCC-69) keeps config retrieval on the bootstrap LB.
//
// Only a long run can show this. A connect-then-exit run always succeeds: bootstrap
// uses the connection-string host, and the process dies before the rebuilt node list
// is ever needed.
//
// How to trigger a reconnect
// --------------------------
// LCB_CNTL_CONFIG_POLL_INTERVAL has "no effect ... using HTTP or File-based
// configurations", so it cannot force a poll. Config pushes arrive over the
// already-open stream, so a settings change proves nothing. The stream must DROP:
//
//   find the node holding it -- restarting the other one proves nothing:
//     sudo ss -tn state established "( sport = :18091 )"
//   then, while this program runs:
//     sudo systemctl restart enterprise-analytics
//
//   Pinned:   reconnects to the bootstrap LB and picks up a new config revision.
//   Unpinned: retries ea-analytics:18091 forever and the revision never moves.
//   Do NOT hard-failover: a 2-node cluster loses Chronicle quorum.
//
// NOT a hang: with LCB_LOGLEVEL=5 you will see, then silence for seconds,
//   (lcbio_mgr) <ea-analytics...:18443> Idle connection expired
// which is lcbio_mgr reaping its own pooled analytics socket mid-idle. Benign, and
// it recurs every cycle. The per-second dots from pump() make progress visible.
//
// Build (bundled lib -- homebrew's lacks CCBC-1682):
//   LCB=/Users/janhavi.tripurwar/Documents/BH/couchbase-odbc-driver/contrib/libcouchbase-odbc
//   g++ -std=c++17 7.two_alb_one_listener_each.cpp -o prog \
//     -I"$LCB/include" -I"$LCB/build/generated" \
//     -L"$LCB/build/lib" -Wl,-rpath,"$LCB/build/lib" -lcouchbase

#include <libcouchbase/couchbase.h>
#include <libcouchbase/vbucket.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

static bool g_row_seen = false;
static lcb_STATUS g_query_status = LCB_SUCCESS;

static std::string now_hms()
{
    std::time_t t = std::time(nullptr);
    char buf[16];
    std::strftime(buf, sizeof buf, "%H:%M:%S", std::localtime(&t));
    return buf;
}

static void analytics_callback(lcb_INSTANCE *, int, const lcb_RESPANALYTICS *resp)
{
    lcb_STATUS rc = lcb_respanalytics_status(resp);
    if (rc != LCB_SUCCESS) {
        g_query_status = rc;
        std::cerr << "    QUERY FAILED: " << lcb_strerror_short(rc);
        // On failure the row payload carries the server's error body, when there is one.
        const char *body = nullptr;
        size_t nbody = 0;
        if (lcb_respanalytics_row(resp, &body, &nbody) == LCB_SUCCESS && body && nbody) {
            std::cerr << "  body=" << std::string(body, nbody);
        }
        std::cerr << std::endl;
        return;
    }
    if (lcb_respanalytics_is_final(resp)) {
        return;
    }
    const char *row = nullptr;
    size_t nrow = 0;
    lcb_respanalytics_row(resp, &row, &nrow);
    std::cout << "    row: " << std::string(row, nrow) << std::endl;
    g_row_seen = true;
}

// Print the topology the SDK currently believes in. SSL mode, because the connection
// string is couchbases:// -- reading PLAIN would report NOT FOUND for everything and
// look like a CCBC-1682 failure.
static void dump_config(lcb_INSTANCE *instance)
{
    lcbvb_CONFIG *vbc = nullptr;
    lcb_cntl(instance, LCB_CNTL_GET, LCB_CNTL_VBCONFIG, &vbc);
    if (vbc == nullptr) {
        std::cout << "    config: <UNAVAILABLE>" << std::endl;
        return;
    }

    unsigned nservers = lcbvb_get_nservers(vbc);
    std::cout << "    config rev=" << lcbvb_get_revision(vbc) << "  servers=" << nservers << std::endl;

    for (unsigned i = 0; i < nservers; i++) {
        const char *hostname = lcbvb_get_hostname(vbc, i);
        unsigned mgmt = lcbvb_get_port(vbc, i, LCBVB_SVCTYPE_MGMT, LCBVB_SVCMODE_SSL);
        unsigned cbas = lcbvb_get_port(vbc, i, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_SSL);
        unsigned kv = lcbvb_get_port(vbc, i, LCBVB_SVCTYPE_DATA, LCBVB_SVCMODE_SSL);
        const char *resturl = lcbvb_get_resturl(vbc, i, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_SSL);

        std::cout << "      node " << i << " " << (hostname ? hostname : "?")
                  << "  mgmtSSL=" << (mgmt ? std::to_string(mgmt) : "NOT FOUND")
                  << "  cbasSSL=" << (cbas ? std::to_string(cbas) : "NOT FOUND")
                  << "  kvSSL=" << (kv ? std::to_string(kv) : "NOT FOUND")
                  << "  analytics=" << (resturl ? resturl : "NULL") << std::endl;
    }

    int ix = lcbvb_get_randhost(vbc, LCBVB_SVCTYPE_ANALYTICS, LCBVB_SVCMODE_SSL);
    if (ix < 0) {
        std::cout << "      !! no analytics node resolvable (randhost=" << ix << ")" << std::endl;
    }
}

// Keep the event loop turning while idling, so streaming config pushes are read --
// outside lcb_wait() the library is suspended. One dot per second, so an idle gap is
// visibly distinct from a hang.
static void pump(lcb_INSTANCE *instance, int seconds)
{
    std::cout << "    idle " << seconds << "s [" << std::flush;
    bool warned = false;
    for (int i = 0; i < seconds * 10; i++) {
        lcb_STATUS rc = lcb_tick_nowait(instance);
        if (!warned && rc == LCB_ERR_SDK_FEATURE_UNAVAILABLE) {
            // No tick mode -> config pushes are not read during idle, so a refresh
            // would only be noticed at the next lcb_wait().
            std::cout << "tick-unsupported!" << std::flush;
            warned = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (i % 10 == 9) {
            std::cout << "." << std::flush;
        }
    }
    std::cout << "]" << std::endl;
}

int main(int argc, char **argv)
{
    int iterations = (argc > 1) ? std::atoi(argv[1]) : 1;
    int interval_s = (argc > 2) ? std::atoi(argv[2]) : 10;
    if (iterations < 1) {
        iterations = 1;
    }

    std::string conn_str =
        "couchbases://ea-bootstrap-1151734473.us-east-2.elb.amazonaws.com:18443=https"
        "?truststorepath=/Users/janhavi.tripurwar/Documents/BH/newSetup/alb-cert/alb-cert.pem"
        "&network=external"
        "&pin_config_nodes=true"
        // Handshake through the ALB is bimodal (~0.45s or ~2.0s TLS, a SYN retransmit
        // on one of its IPs). The 2s default config_node_timeout is marginal, so
        // bootstrap fails intermittently without these.
        "&config_node_timeout=10"
        "&config_total_timeout=20";
    if (const char *cs = std::getenv("CB_CONNSTR")) {
        conn_str = cs;
    }

    // Env first so a long run can be launched non-interactively.
    std::string username, password;
    if (const char *u = std::getenv("CB_USER")) {
        username = u;
    } else {
        std::cout << "Enter the username: ";
        std::cin >> username;
    }
    if (const char *p = std::getenv("CB_PASS")) {
        password = p;
    } else {
        std::cout << "Enter the password: ";
        std::cin >> password;
    }

    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Connection String -> " << conn_str << std::endl;
    std::cout << "Iterations: " << iterations << "   Interval: " << interval_s << "s" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    lcb_CREATEOPTS *options = nullptr;
    lcb_createopts_create(&options, LCB_TYPE_CLUSTER);
    lcb_createopts_connstr(options, conn_str.c_str(), conn_str.length());
    lcb_createopts_credentials(options, username.c_str(), username.length(), password.c_str(), password.length());

    lcb_INSTANCE *instance = nullptr;
    lcb_STATUS err = lcb_create(&instance, options);
    lcb_createopts_destroy(options);
    if (err != LCB_SUCCESS) {
        std::cerr << "lcb_create failed: " << lcb_strerror_short(err) << std::endl;
        return 1;
    }

    lcb_connect(instance);
    lcb_wait(instance, LCB_WAIT_DEFAULT);

    err = lcb_get_bootstrap_status(instance);
    if (err != LCB_SUCCESS) {
        std::cerr << "Failed to connect: " << lcb_strerror_short(err) << std::endl;
        lcb_destroy(instance);
        return 1;
    }
    std::cout << "Connected to cluster." << std::endl;

    const std::string query = "select 1;";
    int ok_count = 0, fail_count = 0;
    unsigned last_rev = 0;
    bool first = true;

    for (int it = 1; it <= iterations; it++) {
        std::cout << "\n[" << now_hms() << "] iteration " << it << "/" << iterations << std::endl;

        // A bumped revision means a refresh landed.
        lcbvb_CONFIG *vbc = nullptr;
        lcb_cntl(instance, LCB_CNTL_GET, LCB_CNTL_VBCONFIG, &vbc);
        if (vbc != nullptr) {
            unsigned rev = lcbvb_get_revision(vbc);
            if (!first && rev != last_rev) {
                std::cout << "    *** CONFIG REVISION CHANGED " << last_rev << " -> " << rev
                          << " (refresh landed) ***" << std::endl;
            }
            last_rev = rev;
            first = false;
        }

        dump_config(instance);

        g_row_seen = false;
        g_query_status = LCB_SUCCESS;

        lcb_CMDANALYTICS *cmd = nullptr;
        lcb_cmdanalytics_create(&cmd);
        lcb_cmdanalytics_statement(cmd, query.c_str(), query.length());
        lcb_cmdanalytics_callback(cmd, analytics_callback);

        err = lcb_analytics(instance, nullptr, cmd);
        lcb_cmdanalytics_destroy(cmd);

        if (err != LCB_SUCCESS) {
            std::cerr << "    schedule failed: " << lcb_strerror_short(err) << std::endl;
            fail_count++;
        } else {
            lcb_wait(instance, LCB_WAIT_DEFAULT);
            if (g_query_status == LCB_SUCCESS && g_row_seen) {
                std::cout << "    OK" << std::endl;
                ok_count++;
            } else {
                fail_count++;
            }
        }

        if (iterations > 1) {
            std::cout << "    running totals: ok=" << ok_count << " fail=" << fail_count << std::endl;
            if (it < iterations) {
                pump(instance, interval_s);
            }
        }
    }

    std::cout << "\n================ SUMMARY ================" << std::endl;
    std::cout << "iterations : " << iterations << std::endl;
    std::cout << "ok         : " << ok_count << std::endl;
    std::cout << "failed     : " << fail_count << std::endl;
    std::cout << "final rev  : " << last_rev << std::endl;
    std::cout << "=========================================" << std::endl;

    lcb_destroy(instance);
    return fail_count == 0 ? 0 : 1;
}
