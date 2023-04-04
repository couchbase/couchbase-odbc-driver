#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

#define FUNCTIONS 100

main () {

    FILE *fp;

    fp = fopen("sql_get_functions.output", "w"); // open file in write mode

    SQLHENV  henv   = SQL_NULL_HENV;     // Environment
    SQLHDBC  hdbc   = SQL_NULL_HDBC;     // Connection handle
    SQLRETURN retcode;

    SQLRETURN retcodeTables, retcodeColumns, retcodeStatistics;
    SQLUSMALLINT fODBC3Exists[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];
    SQLUSMALLINT fALLExists[FUNCTIONS];

    // Allocate environment handle
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
                henv, SQL_HANDLE_ENV, fp);

    // Set ODBC Version
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                                        (SQLPOINTER*)SQL_OV_ODBC3, 0);
    CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    // Allocate connection handle
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
                hdbc, SQL_HANDLE_DBC, fp);

    // Connect to datasource
    retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    CHECK_ERROR(retcode, "SQLDriverConnect(DATASOURCE)",
                hdbc, SQL_HANDLE_DBC, fp);

    // SQL_API_ODBC3_ALL_FUNCTIONS is used by an ODBC 3.x application
    // to determine the level of support of ODBC 3.x and earlier functions.

    // SQL_API_ALL_FUNCTIONS is used by an ODBC 2.x application
    // to determine the level of support of ODBC 2.x and earlier functions.

    retcode = SQLGetFunctions(hdbc,
                              SQL_API_ODBC3_ALL_FUNCTIONS, fODBC3Exists);
    CHECK_ERROR(retcode, "SQLGetFunctions(SQL_API_ODBC3_ALL_FUNCTIONS)",
                hdbc, SQL_HANDLE_DBC, fp);

    // SQLGetFunctions has completed successfully. Check if SQLTables,
    // SQLColumns, and SQLStatistics are supported by the driver.
    if (retcode == SQL_SUCCESS &&
        SQL_FUNC_EXISTS(fODBC3Exists, SQL_API_SQLALLOCHANDLE) == SQL_TRUE &&
        SQL_FUNC_EXISTS(fODBC3Exists, SQL_API_SQLGETDATA) == SQL_TRUE &&
        SQL_FUNC_EXISTS(fODBC3Exists, SQL_API_SQLALLOCHANDLE) == SQL_TRUE)
    {
        printf ("ODBC3: SQL_API_SQLALLOCHANDLE, SQL_API_SQLGETDATA and SQL_API_SQLROWCOUNT are supported\n");
        fprintf (fp, "ODBC3: SQL_API_SQLALLOCHANDLE, SQL_API_SQLGETDATA and SQL_API_SQLROWCOUNT are supported\n");
    }

    retcode = SQLGetFunctions(hdbc,
                              SQL_API_ALL_FUNCTIONS, fALLExists);//currently ClickHouse Codebase doesn't support any of these
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
                henv, SQL_HANDLE_ENV, fp);

    // SQLGetFunctions is completed successfully. Check if SQLTables,
    // SQLColumns, and SQLStatistics are supported by the driver.
    if (retcode == SQL_SUCCESS &&
        fALLExists[SQL_API_SQLTABLES] == SQL_TRUE &&
        fALLExists[SQL_API_SQLCOLUMNS] == SQL_TRUE &&
        fALLExists[SQL_API_SQLSTATISTICS] == SQL_TRUE)
    {
        printf ("ODBC2: SQLTables, SQLColumns, and SQLStatistics are supported\n");
        fprintf (fp, "ODBC2: SQLTables, SQLColumns, and SQLStatistics are supported\n");
    }

exit:

    printf ("\nComplete.\n");
    fprintf (fp, "\nComplete.\n");

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