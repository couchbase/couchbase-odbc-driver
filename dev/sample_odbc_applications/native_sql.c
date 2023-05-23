#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

#define BUFF_LEN 255



main () {
    FILE *fp;
    fp = fopen("native_sql.output", "w"); // open file in write mode

    SQLHENV   henv  = SQL_NULL_HENV;   // Environment
    SQLHDBC   hdbc  = SQL_NULL_HDBC;   // Connection handle
    SQLHSTMT  hstmt = SQL_NULL_HSTMT;  // Statement handle
    SQLRETURN retcode;

    const char * sqlStatementIN = "SELECT { fn CONVERT (emp, SQL_SMALLINT) };";
 	SQLCHAR    sqlStatementOUT[BUFF_LEN];
	SQLINTEGER lenStatementOUT;
    char reply=' ';

	/* Allocate an environment handle */
	retcode=SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
        CHECK_ERROR(retcode, "SQLAllocHandle (SQL_HANDLE_ENV)",
                    henv, SQL_HANDLE_ENV, fp);

	/* We want ODBC 3 support */
	retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                                            (void *) SQL_OV_ODBC3, 0);
        CHECK_ERROR(retcode, "SQLSetEnvAttr (SQL_ATTR_ODBC_VERSION)",
                    henv, SQL_HANDLE_ENV, fp);

	/* Allocate a connection handle */
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
        CHECK_ERROR(retcode, "SQLAllocHandle (SQL_HANDLE_DBC)",
                    hdbc, SQL_HANDLE_DBC, fp);

	// Set connection timeout
        retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
        CHECK_ERROR(retcode, "SQLSetConnectAttr (SQL_LOGIN_TIMEOUT)",
                    hdbc, SQL_HANDLE_DBC, fp);

	// Connect
	// retcode=SQLDriverConnect(hdbc, NULL, "DSN=DATASOURCE;",
    //                             SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    // CHECK_ERROR(retcode, "SQLDriverConnect (DATASOURCE)",
    //                 hdbc, SQL_HANDLE_DBC);

    retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    CHECK_ERROR(retcode, " SQLDriverConnect(\"DSN=Couchbase DSN (ANSI);\")", hdbc, SQL_HANDLE_DBC, fp);

	memset (sqlStatementOUT,' ', BUFF_LEN);

	retcode = SQLNativeSql (hdbc,
							sqlStatementIN, strlen(sqlStatementIN),
							sqlStatementOUT, BUFF_LEN,
							&lenStatementOUT);
	CHECK_ERROR(retcode, "SQLNativeSql (hdbc)", hdbc, SQL_HANDLE_DBC, fp);

	printf ("\nStatement IN    : %s (Len %i)\n",
                        sqlStatementIN, (int) strlen(sqlStatementIN));
    
    fprintf (fp, "\nStatement IN    : %s (Len %i)\n",
                        sqlStatementIN, (int) strlen(sqlStatementIN));

	printf   ("Statement OUT   : %s (Len %i)\n",
                        sqlStatementOUT, (int) lenStatementOUT);
    
    fprintf   (fp, "Statement OUT   : %s (Len %i)\n",
                        sqlStatementOUT, (int) lenStatementOUT);

	printf ("\nThe End.\n");
    fprintf (fp, "\nThe End.\n");

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