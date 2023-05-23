#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

#define BRWS_LEN 1000


// get string from user.
char getStr (char *label, char *retStr, int len, char confirm) {

    char reply[3];
    char *nl=0;

    strcpy (reply, "Y");

    if (strlen(label) > 0) {
        printf ("%s : ", label);
    }

    fgets(retStr, len, stdin);
    if ( (nl = strchr (retStr, '\n')) != NULL) {*nl='\0';}

    if (confirm == 'Y') {
        printf ("Confirm Y/N? : ");
        fgets(reply, 3, stdin);
    }

    return reply[0];
}


int main () {
    FILE *fp;

    fp = fopen("browse_connect.output", "w"); // open file in write mode

    SQLHENV   henv  = SQL_NULL_HENV;   // Environment
    SQLHDBC   hdbc  = SQL_NULL_HDBC;   // Connection handle
    SQLHSTMT  hstmt = SQL_NULL_HSTMT;  // Statement handle
    SQLRETURN retcode;

    SQLCHAR strConnIn[BRWS_LEN], strConnOut[BRWS_LEN];
    SQLSMALLINT lenConnOut;
    SQLSMALLINT columns;

    // Allocate the environment handle.
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
                henv, SQL_HANDLE_ENV, fp);

    // Set the version environment attribute.
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                            (SQLPOINTER*)SQL_OV_ODBC3, 0);
    CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    // Allocate the connection handle.
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
                hdbc, SQL_HANDLE_DBC, fp);

    // Call SQLBrowseConnect until it returns a value other than SQL_NEED_DATA.
    // The initial call to SQLBrowseConnect is a DSN. If SQL_NEED_DATA is
    // returned, calls getStr (not shown) to build a dialog from the values in
    // strConnOut.  The user-supplied values are passed in the next call
    // to SQLBrowseConnect.

    strcpy((char*)strConnIn, "DSN=Couchbase DSN (ANSI)"); // First call
    do {
        retcode = SQLBrowseConnect(hdbc, strConnIn, SQL_NTS, strConnOut,
                                   BRWS_LEN, &lenConnOut);
        if (retcode == SQL_NEED_DATA) {
            printf ("\nIn  Str : %s\n", strConnIn);
            printf ("\nOut Str : %s\n", strConnOut);
            getStr ("\nEnter Next Value", strConnIn, BRWS_LEN, 'N');
        }
    } while (retcode == SQL_NEED_DATA);

    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
                hdbc, SQL_HANDLE_DBC, fp);
    printf ("\nFinal Out Str : %s\n", strConnOut);
    printf ("\nSuccessfully connected via SQLBrowseConnect.\n");
    SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    const char * queryTxt = "SELECT 1,2";

    SQLExecDirect(hstmt, (SQLCHAR *)queryTxt, strlen(queryTxt));

    /* How many columns are there */
    SQLNumResultCols(hstmt, &columns);
    printf("totalCols: %u\n", columns);
    fprintf(fp, "totalCols: %u\n", columns);

exit:

    printf ("\nComplete.\n");

    fclose(fp);

    // Free handles
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
