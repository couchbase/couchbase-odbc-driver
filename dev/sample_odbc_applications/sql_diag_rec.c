//CHECK_ERROR function already contains the SQLGetDiagRec internally called do this script just making a Syntax error and the response is getting printed via CHECK_ERROR

#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"
#define DTSIZE 35
int main () {
    FILE *fp;
    fp = fopen("sql_diag_rec.output", "w"); // open file in write mode

    SQLHENV  henv  = SQL_NULL_HENV;   	// Environment
    SQLHDBC  hdbc  = SQL_NULL_HDBC;   	// Connection handle
    SQLHSTMT hstmt = SQL_NULL_HSTMT;  	// Statement handle
    SQLRETURN retcode;			// Return status
    SQLSMALLINT columns;
    SQLCHAR         buffer[255];
    SQLSMALLINT     outlen;
    SQLINTEGER diagPtr;
    SQLSMALLINT strLenPtr;

    // Create environment handle
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    CHECK_ERROR(retcode, "SQLAllocHandle(ENV)", henv, SQL_HANDLE_ENV, fp);

    // Set ODBC 3 behaviour
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                                        (SQLCHAR *)(void*)SQL_OV_ODBC3, -1);
    CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    CHECK_ERROR(retcode, "SQLAllocHandle(DBC)",
                hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
    CHECK_ERROR(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
                hdbc, SQL_HANDLE_DBC, fp);


    retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    CHECK_ERROR(retcode, " SQLDriverConnect(\"DSN=Couchbase DSN (ANSI);\")", hdbc, SQL_HANDLE_DBC, fp);


    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    CHECK_ERROR(retcode, "SQLAllocHandle(STMT)",
                hstmt, SQL_HANDLE_STMT, fp);

    const char * queryTxt
            = "wher;";

    retcode = SQLExecDirect(hstmt, queryTxt, strlen(queryTxt));
    CHECK_ERROR(retcode, "SQLExecDirect",
                    hstmt, SQL_HANDLE_STMT, fp);

exit:
    printf ("\nComplete.\n");
    fprintf (fp, "\nComplete.\n");
    fclose(fp);

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

        return 0;
}

