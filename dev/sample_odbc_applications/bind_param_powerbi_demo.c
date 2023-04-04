#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char * argv[]) {
    FILE *fp;
    fp = fopen("bind_param_powerbi_demo.output", "w"); // open file in write mode
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;       /* ODBC API return status */
    SQLSMALLINT columns; /* number of columns in result-set */

    /* Allocate an environment handle */
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    /* We want ODBC 3 support */
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
    /* Allocate a connection handle */
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    SQLDriverConnect(dbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char * queryTxt = "SELECT count(?) FROM `travel-sample`.`inventory`.`airport_view` apv LIMIT 2;";

    SQLINTEGER paramValue;
    SQLLEN paramValueInd;
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&paramValue, 0, (SQLLEN *)&paramValueInd);


    paramValue = 1;
    SQLExecDirect(stmt, (SQLCHAR *)queryTxt, strlen(queryTxt));

    SQLNumResultCols(stmt, &columns);
    printf("totalCols: %u\n", columns);
    fprintf(fp, "totalCols: %u\n", columns);

    SQLBIGINT bigIntColVal;
    int row = 1;
    while (SQLFetch(stmt) == SQL_SUCCESS) {
        printf("____ row:%d____\n", row);
        fprintf(fp, "____ row:%d____\n", row);
        for (int i = 1; i <= columns; i++) {
            char nameBuff[1024];
            SQLSMALLINT nameBuffLenUsed;
            SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);
            SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_SLONG, (SQLPOINTER)(&bigIntColVal), (SQLLEN)0, NULL);
            printf("%s: %ld\n", nameBuff, bigIntColVal);
            fprintf(fp, "%s: %ld\n", nameBuff, bigIntColVal);
        }
        printf("\n\n");
        fprintf(fp, "\n\n");
        row++;
    }
    fclose(fp);

    return 0;
}
