#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

bool checkAndPrintNull(SQLLEN * ind, FILE *fp);
bool checkAndPrintNull(SQLLEN * ind, FILE *fp) {
    if (ind && *ind == SQL_NULL_DATA) {
            printf("NULL\t");
            fprintf(fp, "NULL\t");
            return true;
        }
    return false;
}


#define DTSIZE 35
int main () {

    SQLHENV  henv  = SQL_NULL_HENV;   	// Environment
    SQLHDBC  hdbc  = SQL_NULL_HDBC;   	// Connection handle
    SQLHSTMT hstmt = SQL_NULL_HSTMT;  	// Statement handle
    SQLRETURN retcode;			// Return status
    SQLSMALLINT columns;

    SQLCHAR stringColVal[1024];
    SQLBIGINT bigIntColVal;
    SQLSCHAR boolColVal;
    SQLLEN stringInd, bigintInd, boolInd;

    FILE *fp;

    fp = fopen("sql_close_cursor.output", "w"); // open file in write mode

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

    retcode = SQLGetTypeInfo(hstmt, SQL_ALL_TYPES);
    CHECK_ERROR(retcode, "SQLGetTypeInfo",
                hstmt, SQL_HANDLE_STMT, fp);


    /* How many columns are there */
    retcode = SQLNumResultCols(hstmt, &columns);
    CHECK_ERROR(retcode, "SQLNumResultCols",
                hstmt, SQL_HANDLE_STMT, fp);
    printf("\nBefore SQLCloseCursor\n");
    printf("totalCols: %u\n", columns);
    fprintf(fp, "totalCols: %u\n", columns);

    retcode = SQLGetTypeInfo(hstmt, SQL_ALL_TYPES);
    CHECK_ERROR(retcode, "SQLGetTypeInfo",
                hstmt, SQL_HANDLE_STMT, fp);

    retcode = SQLCloseCursor(hstmt);
    CHECK_ERROR(retcode, "SQLCloseCursor",
                hstmt, SQL_HANDLE_STMT, fp);

    retcode = SQLNumResultCols(hstmt, &columns);
    CHECK_ERROR(retcode, "SQLNumResultCols",
                hstmt, SQL_HANDLE_STMT, fp);
    printf("\After SQLCloseCursor\n");
    printf("totalCols: %u\n", columns);
    fprintf(fp, "totalCols: %u\n", columns);

    
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
