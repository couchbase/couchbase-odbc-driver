#include "utils.h"

#define DTSIZE 35

int main () {
    FILE *fp;
    fp = fopen("sql_get_info.output", "w"); // open file in write mode

    SQLHENV  henv  = SQL_NULL_HENV;   	// Environment
    SQLHDBC  hdbc  = SQL_NULL_HDBC;   	// Connection handle
    SQLHSTMT hstmt = SQL_NULL_HSTMT;  	// Statement handle
    SQLRETURN retcode;			// Return status
    SQLSMALLINT columns;
    SQLCHAR         buffer[255];
    SQLSMALLINT     outlen;

    // Create environment handle
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    check_error(retcode, "SQLAllocHandle(ENV)", henv, SQL_HANDLE_ENV, fp);

    // Set ODBC 3 behaviour
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                                        (SQLCHAR *)(void*)SQL_OV_ODBC3, -1);
    check_error(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    check_error(retcode, "SQLAllocHandle(DBC)",
                hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
    check_error(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
                hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLDriverConnect(hdbc, NULL, "DSN=Couchbase DSN (ANSI);", SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
        check_error(retcode, " SQLDriverConnect(\"DSN=Couchbase DSN (ANSI);\")", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    check_error(retcode, "SQLAllocHandle(STMT)",
                hstmt, SQL_HANDLE_STMT, fp);

    retcode = SQLGetInfo(hdbc, SQL_DBMS_NAME, buffer, 255, &outlen);
    check_error(retcode, "SQLGetInfo",
                hstmt, SQL_HANDLE_STMT, fp);
    printf("\nSQL_DBMS_NAME: %s\n", buffer);
    fprintf(fp, "\nSQL_DBMS_NAME: %s\n", buffer);

    retcode = SQLGetInfo(hdbc, SQL_SCHEMA_USAGE, buffer, 255, &outlen);
    check_error(retcode, "SQLGetInfo",
                hstmt, SQL_HANDLE_STMT, fp);

    printf("\nSQL_SCHEMA_USAGE: %s\n", buffer);
    fprintf(fp, "\nSQL_SCHEMA_USAGE: %s\n", buffer);

exit:

    printf ("\nComplete.\n");
    fprintf (fp, "\nComplete.\n");

    // Free handles
    // Statement
    if (hstmt != SQL_NULL_HSTMT)
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    // Connection
    if (hdbc != SQL_NULL_HDBC) {
        SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    }

    // Environment
    if (henv != SQL_NULL_HENV)
        SQLFreeHandle(SQL_HANDLE_ENV, henv);

    fclose(fp);
    return 0;
}
