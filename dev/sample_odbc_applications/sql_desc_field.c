#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

int main () {
    FILE *fp;
    fp = fopen("sql_desc_fields.output", "w"); // open file in write mode

    SQLHENV  henv  = SQL_NULL_HENV;   	// Environment
    SQLHDBC  hdbc  = SQL_NULL_HDBC;   	// Connection handle
    SQLHSTMT hstmt = SQL_NULL_HSTMT;  	// Statement handle
    SQLRETURN retcode;			// Return status
    SQLSMALLINT columns;
    SQLCHAR         buffer[255];
    SQLSMALLINT     outlen;
    SQLHDESC  hArd;
    SQLSMALLINT     setDescAllocType=2;
    SQLSMALLINT     descAllocType=0;
    SQLSMALLINT     descCount=0;

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

    retcode = SQLDriverConnect(hdbc, NULL, "DSN=Couchbase DSN (ANSI);", SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
        CHECK_ERROR(retcode, " SQLDriverConnect(\"DSN=Couchbase DSN (ANSI);\")", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    CHECK_ERROR(retcode, "SQLAllocHandle(STMT)",
                hstmt, SQL_HANDLE_STMT, fp);


    retcode = SQLGetStmtAttr(hstmt, SQL_ATTR_APP_ROW_DESC, &hArd, 0, NULL);
    CHECK_ERROR(retcode, "SQLGetStmtAttr(SQL_HANDLE_STMT)",
                hstmt, SQL_HANDLE_STMT, fp);

    printf("Descriptor Initialized\n");
    fprintf(fp, "Descriptor Initialized\n");
    
    retcode = SQLSetDescField(hArd, 0, SQL_DESC_ALLOC_TYPE,
                                             setDescAllocType, 0);
    CHECK_ERROR(retcode, "SQLSetDescField",
                hstmt, SQL_HANDLE_STMT, fp);

    retcode = SQLGetDescField(hArd, 0, SQL_DESC_ALLOC_TYPE,
                                             &descAllocType, 0, 0);
    CHECK_ERROR(retcode, "SQLGetDescField",
                hstmt, SQL_HANDLE_STMT, fp);

    printf("\n-----------\n OUTPUT Received from SQLGetDescField --- \n");
    fprintf(fp, "\n-----------\n OUTPUT Received from SQLGetDescField --- \n");
    
    printf (" SQL_DESC_ALLOC_TYPE : %i\n", (int) descAllocType);
    fprintf (fp, " SQL_DESC_ALLOC_TYPE : %i\n", (int) descAllocType);


    retcode = SQLGetDescField(hArd, 0, SQL_DESC_COUNT,
                                             &descCount, 0, 0);
    CHECK_ERROR(retcode, "SQLGetDescField",
                hstmt, SQL_HANDLE_STMT, fp);
    printf (" SQL_DESC_COUNT : %i\n", (int) descCount);
    fprintf (fp, " SQL_DESC_COUNT : %i\n", (int) descCount);

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