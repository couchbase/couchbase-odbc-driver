#include <odbcinst.h>
#include <sql.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

SQLHENV env;
SQLHDBC dbc;
SQLHSTMT stmt;
SQLRETURN ret; /* ODBC API return status */
SQLCHAR outstr[1024];
SQLSMALLINT outstrlen;

void test_timeout_via_conn_string(const char * dsn) {
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    SQLCHAR conn_str[1024];
    sprintf(conn_str, "DSN=%s;LoginTimeout=0;", dsn);

    ret = SQLDriverConnect(dbc, NULL, conn_str, SQL_NTS, outstr, sizeof(outstr), &outstrlen, SQL_DRIVER_COMPLETE);
    if (SQL_SUCCEEDED(ret)) {
        printf("Connected\n");
        printf("Returned connection string was:\n\t%s\n\n", outstr);
        if (ret == SQL_SUCCESS_WITH_INFO) {
            printf("Driver reported the following diagnostics\n");
            extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
        }
        SQLDisconnect(dbc); /* disconnect from driver */
    } else {
        fprintf(stderr, "Failed to connect\n");
        extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
    }

    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
}

void test_timeout_via_set_conn_attr(const char * dsn) {
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    ret = SQLSetConnectAttr(dbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)0, 0);
    CHECK_ERROR(ret, "SQLSetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT)", dbc, SQL_HANDLE_DBC);

    SQLCHAR conn_str[1024];
    sprintf(conn_str, "DSN=%s;", dsn);

    ret = SQLDriverConnect(dbc, NULL, conn_str, SQL_NTS, outstr, sizeof(outstr), &outstrlen, SQL_DRIVER_COMPLETE);
    if (SQL_SUCCEEDED(ret)) {
        printf("Connected\n");
        printf("Returned connection string was:\n\t%s\n\n", outstr);
        if (ret == SQL_SUCCESS_WITH_INFO) {
            printf("Driver reported the following diagnostics\n");
            extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
        }
        SQLDisconnect(dbc); /* disconnect from driver */
    } else {
        fprintf(stderr, "Failed to connect\n");
        extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
    }

    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
}

int main() {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);

    const SQLCHAR * dsn = "Couchbase DSN (ANSI)";
    test_timeout_via_conn_string(dsn);
    test_timeout_via_set_conn_attr(dsn);

    SQLFreeHandle(SQL_HANDLE_ENV, env);
    return 0;
}
