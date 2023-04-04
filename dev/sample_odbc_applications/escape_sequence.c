#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

int main(int argc, char * argv[]) {
    FILE *fp;
    fp = fopen("escape_sequence.output", "w"); // open file in write mode
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;       /* ODBC API return status */
    SQLSMALLINT columns; /* number of columns in result-set */

    SQLLEN doubleInd;
    SQLDOUBLE doubleColVal;
    SQLRETURN retcode;

    /* Allocate an environment handle */
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    /* We want ODBC 3 support */
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
    /* Allocate a connection handle */
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    SQLDriverConnect(dbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);

    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char * queryTxt = "select { fn convert(10.23, SQL_DOUBLE) };";

    SQLExecDirect(stmt, (SQLCHAR *)queryTxt, strlen(queryTxt));

    /* How many columns are there */
    SQLNumResultCols(stmt, &columns);
    printf("totalCols: %u\n", columns);
    fprintf(fp, "totalCols: %u\n", columns);


    printf("\n\n ------- OUTPUT --------- \n\n");
    fprintf(fp, "\n\n ------- OUTPUT --------- \n\n");

    while ((retcode = SQLFetch(stmt)) == SQL_SUCCESS) {
        SQLGetData(stmt, (SQLUSMALLINT)1, SQL_C_DOUBLE, (SQLPOINTER)(&doubleColVal), (SQLLEN)0, &doubleInd);
        if (!check_and_print_null("nameBuff", &doubleInd, fp))
            printf("%lf\n", doubleColVal);
            fprintf(fp, "%lf\n", doubleColVal);
    }

exit:
    fclose(fp);
    return 0;
}
