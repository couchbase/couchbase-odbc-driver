#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

int main(int argc, char * argv[]) {
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

    const char * queryTxt = "SELECT hv.hotel_geo_lat, hv.hotel_id, hv.hotel_price, hv.hotel_free_parking FROM "
                            "`travel-sample`.`inventory`.`hotel_view` AS hv LIMIT 2;";

    SQLExecDirect(stmt, (SQLCHAR *)queryTxt, strlen(queryTxt));

    /* How many columns are there */
    SQLNumResultCols(stmt, &columns);
    printf("totalCols: %u\n", columns);

    SQLDOUBLE doubleColVal;
    SQLCHAR stringColVal[1024];
    SQLBIGINT bigIntColVal;
    SQLSCHAR boolColVal;
    SQLLEN doubleInd, stringInd, bigintInd, boolInd;

    if (argc == 2) {
        printf("argv[1]: %s\n", argv[1]);
        if (strcmp((const char*) argv[1], (const char *)("bind")) == 0) {
            printf("___BIND___\n");
            for (int i = 1; i <= columns; i++) {
                char nameBuff[1024];
                SQLSMALLINT nameBuffLenUsed;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);

                SQLLEN columnType = 0;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);

                //Binding
                switch (columnType) {
                    case SQL_DOUBLE:
                        SQLBindCol(stmt, (SQLUSMALLINT)i, SQL_C_DOUBLE, (SQLPOINTER)(&doubleColVal), (SQLLEN)0, &doubleInd);
                        break;
                    case SQL_BIGINT:
                        SQLBindCol(stmt, (SQLUSMALLINT)i, SQL_C_SBIGINT, (SQLPOINTER)(&bigIntColVal), (SQLLEN)0, &bigintInd);
                        break;
                    case SQL_VARCHAR:
                        SQLBindCol(stmt, (SQLUSMALLINT)i, SQL_C_CHAR, (SQLPOINTER)stringColVal, (SQLLEN)1024, &stringInd);
                        break;
                    case SQL_BIT:
                        SQLBindCol(stmt, (SQLUSMALLINT)i, SQL_C_BIT, (SQLPOINTER)(&boolColVal), (SQLLEN)0, &boolInd);
                        break;
                }
            }

            int row = 1;
            while (SQLFetch(stmt) == SQL_SUCCESS) {
                printf("____ row:%d____\n", row);
                for (int i = 1; i <= columns; i++) {
                    char nameBuff[1024];
                    SQLSMALLINT nameBuffLenUsed;
                    SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);
                    SQLLEN columnType = 0;
                    SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);

                    switch (columnType) {
                        case SQL_DOUBLE:
                            if (!check_and_print_null(nameBuff, &doubleInd)) {
                                printf("%s: %lf\n", nameBuff, doubleColVal);
                            }
                            break;
                        case SQL_BIGINT:
                            if (!check_and_print_null(nameBuff, &bigintInd)) {
                                printf("%s: %ld\n", nameBuff, bigIntColVal);
                            }
                            break;
                        case SQL_VARCHAR:
                            if (!check_and_print_null(nameBuff, &stringInd)) {
                                printf("%s: %s\n", nameBuff, stringColVal);
                            }
                            break;
                        case SQL_BIT:
                            if (!check_and_print_null(nameBuff, &boolInd)) {
                                char * boolVal = boolColVal == 0 ? "false" : "true";
                                printf("%s: %s\n", nameBuff, boolVal);
                            }
                            break;
                    }
                }

                printf("\n\n");
                row++;
            }
        }
    } else {
        printf("___GET_DATA___\n");
        int row = 1;
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            printf("____ row:%d____\n", row);
            for (int i = 1; i <= columns; i++) {
                char nameBuff[1024];
                SQLSMALLINT nameBuffLenUsed;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);
                SQLLEN columnType = 0;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);

                switch (columnType) {
                case SQL_DOUBLE:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_DOUBLE, (SQLPOINTER)(&doubleColVal), (SQLLEN)0, &doubleInd);
                    if (!check_and_print_null(nameBuff, &doubleInd)) {
                        printf("%s: %lf\n", nameBuff, doubleColVal);
                    }
                    break;
                case SQL_BIGINT:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_SBIGINT, (SQLPOINTER)(&bigIntColVal), (SQLLEN)0, &bigintInd);
                    if (!check_and_print_null(nameBuff, &bigintInd)) {
                        printf("%s: %ld\n", nameBuff, bigIntColVal);
                    }
                    break;
                case SQL_VARCHAR:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_CHAR, (SQLPOINTER)stringColVal, (SQLLEN)1024, &stringInd);
                    if (!check_and_print_null(nameBuff, &stringInd)) {
                        printf("%s: %s\n", nameBuff, stringColVal);
                    }
                    break;
                case SQL_BIT:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_BIT, (SQLPOINTER)(&boolColVal), (SQLLEN)0, &boolInd);
                    if (!check_and_print_null(nameBuff, &boolInd)) {
                        char * boolVal = boolColVal == 0 ? "false" : "true";
                        printf("%s: %s\n", nameBuff, boolVal);
                    }
                    break;
                }
            }
            printf("\n\n");
            row++;
        }
    }

    return 0;
}
