#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

#define STRLEN 32

int main () {
    FILE *fp;

    fp = fopen("sql_get_connect_attribute.output", "w"); // open file in write mode

    SQLHENV  henv  = SQL_NULL_HENV;     // Environment handle
    SQLHDBC  hdbc  = SQL_NULL_HDBC;   	// Connection handle
    SQLRETURN retcode;			// Return status

    SQLUINTEGER	uIntVal;		// Unsigned int attribute values
    SQLCHAR	cStr[STRLEN];		// char attribute values



    // Allocate an environment handle
    retcode=SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
                henv, SQL_HANDLE_ENV, fp);

    // Notify ODBC that this is an ODBC 3.0 app.
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                                    (SQLPOINTER) SQL_OV_ODBC3, 0);
    CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_HANDLE_ENV)",
                henv, SQL_HANDLE_ENV, fp);

    // Allocate ODBC connection handle and connect.
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
                hdbc, SQL_HANDLE_DBC, fp);

    // Connect to DATASOURCE
    retcode=SQLDriverConnect(hdbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS,
                             NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    CHECK_ERROR(retcode, "SQLDriverConnect(DATASOURCE)",
                hdbc, SQL_HANDLE_DBC, fp);

    printf ("\nConnection Attributes : \n");
    fprintf (fp, "\nConnection Attributes : \n");

    // Get (some) connection attributes
    // SQLUINTEGER attributes

    retcode = SQLGetConnectAttr (hdbc,
                                SQL_ATTR_AUTOCOMMIT,
                                (SQLPOINTER) &uIntVal,
                                (SQLINTEGER) sizeof(uIntVal),
                                NULL);
    CHECK_ERROR(retcode, "SQLGetConnectAttr(SQL_ATTR_AUTOCOMMIT)",
                hdbc, SQL_HANDLE_DBC, fp);
    printf ("\nSQL_ATTR_AUTOCOMMIT         : ");
    fprintf (fp, "\nSQL_ATTR_AUTOCOMMIT         : ");
    if (uIntVal==SQL_AUTOCOMMIT_OFF) {
        printf ("SQL_AUTOCOMMIT_OFF");
        fprintf (fp, "SQL_AUTOCOMMIT_OFF");
    }
    if (uIntVal==SQL_AUTOCOMMIT_ON) {
        printf ("SQL_AUTOCOMMIT_ON");
        fprintf (fp, "SQL_AUTOCOMMIT_ON");
    }

    retcode = SQLGetConnectAttr (hdbc,
                                 SQL_ATTR_CONNECTION_DEAD,
                                 (SQLPOINTER) &uIntVal,
                                 (SQLINTEGER) sizeof(uIntVal),
                                 NULL);
    CHECK_ERROR(retcode, "SQLGetConnectAttr(SQL_ATTR_CONNECTION_DEAD)",
                hdbc, SQL_HANDLE_DBC, fp);
    printf ("\nSQL_ATTR_CONNECTION_DEAD    : ");
    fprintf (fp, "\nSQL_ATTR_CONNECTION_DEAD    : ");
    if (uIntVal==SQL_CD_TRUE) { 
        printf ("SQL_CD_TRUE");
        fprintf (fp, "SQL_CD_TRUE");
    }
    if (uIntVal==SQL_CD_FALSE) {
        printf ("SQL_CD_FALSE");
        fprintf (fp, "SQL_CD_FALSE");
    }

    retcode = SQLGetConnectAttr (hdbc,
                                 SQL_ATTR_CONNECTION_TIMEOUT,
                                 (SQLPOINTER) &uIntVal,
                                 (SQLINTEGER) sizeof(uIntVal),
                                 NULL);
    CHECK_ERROR(retcode, "SQLGetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT)",
                hdbc, SQL_HANDLE_DBC, fp);
    printf ("\nSQL_ATTR_CONNECTION_TIMEOUT : %i", uIntVal);
    fprintf (fp, "\nSQL_ATTR_CONNECTION_TIMEOUT : %i", uIntVal);

    retcode = SQLGetConnectAttr (hdbc,
                                 SQL_ATTR_LOGIN_TIMEOUT,
                                 (SQLPOINTER) &uIntVal,
                                 (SQLINTEGER) sizeof(uIntVal),
                                 NULL);
    CHECK_ERROR(retcode, "SQLGetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT)",
                hdbc, SQL_HANDLE_DBC, fp);
    printf ("\nSQL_ATTR_LOGIN_TIMEOUT      : %i", uIntVal);
    fprintf (fp, "\nSQL_ATTR_LOGIN_TIMEOUT      : %i", uIntVal);

    retcode = SQLGetConnectAttr (hdbc,
                                 SQL_ATTR_METADATA_ID,
                                 (SQLPOINTER) &uIntVal,
                                 (SQLINTEGER) sizeof(uIntVal),
                                 NULL);
    CHECK_ERROR(retcode, "SQLGetConnectAttr(SQL_ATTR_METADATA_ID)",
                hdbc, SQL_HANDLE_DBC, fp);
    printf ("\nSQL_ATTR_METADATA_ID        : ");
    fprintf (fp, "\nSQL_ATTR_METADATA_ID        : ");
    if (uIntVal==SQL_TRUE) {
        printf ("SQL_TRUE");
        fprintf (fp, "SQL_TRUE");
    }
    if (uIntVal==SQL_FALSE) {
        printf ("SQL_FALSE");
        fprintf (fp, "SQL_FALSE");
    }


    // SQLCHAR attributes
    retcode = SQLGetConnectAttr (hdbc,
                                 SQL_ATTR_CURRENT_CATALOG,
                                 (SQLPOINTER) cStr,
                                 (SQLINTEGER) sizeof(cStr),
                                 NULL);
    CHECK_ERROR(retcode, "SQLGetConnectAttr(SQL_ATTR_CURRENT_CATALOG)",
                hdbc, SQL_HANDLE_DBC, fp);
    printf ("\nSQL_ATTR_CURRENT_CATALOG    : %s", cStr);
    fprintf (fp, "\nSQL_ATTR_CURRENT_CATALOG    : %s", cStr);

exit:

    printf ("\nComplete.\n");
    fprintf (fp, "\nComplete.\n");

    fclose(fp);

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