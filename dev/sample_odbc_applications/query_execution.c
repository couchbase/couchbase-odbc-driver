#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

int main(int argc, char * argv[]) {
    FILE *fp;
    fp = fopen("query_execution.output", "w"); // open file in write mode
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
    fprintf(fp, "totalCols: %u\n", columns);

    SQLDOUBLE doubleColVal;
    SQLCHAR stringColVal[1024];
    SQLBIGINT bigIntColVal;
    SQLSCHAR boolColVal;
    SQL_DATE_STRUCT dateColVal ;
    SQL_TIME_STRUCT timeColVal;
    SQL_TIMESTAMP_STRUCT datetimeColVal;
    SQLLEN doubleInd, stringInd, bigintInd, boolInd,dateInd,timeInd,datetimeInd;

    if (argc == 2) {
        printf("argv[1]: %s\n", argv[1]);
        fprintf(fp, "argv[1]: %s\n", argv[1]);
        if (strcmp((const char*) argv[1], (const char *)("bind")) == 0) {
            printf("___BIND___\n");
            fprintf(fp, "___BIND___\n");
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
                fprintf(fp, "____ row:%d____\n", row);
                for (int i = 1; i <= columns; i++) {
                    char nameBuff[1024];
                    SQLSMALLINT nameBuffLenUsed;
                    SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);
                    SQLLEN columnType = 0;
                    SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);

                    switch (columnType) {
                        case SQL_DOUBLE:
                            if (!check_and_print_null(nameBuff, &doubleInd, fp)) {
                                printf("%s: %lf\n", nameBuff, doubleColVal);
                                fprintf(fp, "%s: %lf\n", nameBuff, doubleColVal);
                            }
                            break;
                        case SQL_BIGINT:
                            if (!check_and_print_null(nameBuff, &bigintInd, fp)) {
                                //TODO : This dows not work on windows
                                // printf("%s: %ld\n", nameBuff, bigIntColVal);
                                // fprintf(fp, "%s: %ld\n", nameBuff, bigIntColVal);
                            }
                            break;
                        case SQL_VARCHAR:
                            if (!check_and_print_null(nameBuff, &stringInd, fp)) {
                                printf("%s: %s\n", nameBuff, stringColVal);
                                fprintf(fp, "%s: %s\n", nameBuff, stringColVal);
                            }
                            break;
                        case SQL_BIT:
                            if (!check_and_print_null(nameBuff, &boolInd, fp)) {
                                char * boolVal = boolColVal == 0 ? "false" : "true";
                                printf("%s: %s\n", nameBuff, boolVal);
                                fprintf(fp, "%s: %s\n", nameBuff, boolVal);
                            }
                            break;
                    }
                }

                printf("\n\n");
                fprintf(fp, "\n\n");
                row++;
            }
        }
    } else {
        printf("___GET_DATA___\n");
        fprintf(fp, "___GET_DATA___\n");
        int row = 1;
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            printf("____ row:%d____\n", row);
            fprintf(fp, "____ row:%d____\n", row);
            for (int i = 1; i <= columns; i++) {
                char nameBuff[1024];
                SQLSMALLINT nameBuffLenUsed;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);
                SQLLEN columnType = 0;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);

                switch (columnType) {
                case SQL_DOUBLE:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_DOUBLE, (SQLPOINTER)(&doubleColVal), (SQLLEN)0, &doubleInd);
                    if (!check_and_print_null(nameBuff, &doubleInd, fp)) {
                        printf("%s: %lf\n", nameBuff, doubleColVal);
                        fprintf(fp, "%s: %lf\n", nameBuff, doubleColVal);
                    }
                    break;
                case SQL_BIGINT:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_SBIGINT, (SQLPOINTER)(&bigIntColVal), (SQLLEN)0, &bigintInd);
                    if (!check_and_print_null(nameBuff, &bigintInd, fp)) {
                        //TODO : This dows not work on windows
                        // printf("%s: %ld\n", nameBuff, bigIntColVal);
                        // fprintf(fp, "%s: %ld\n", nameBuff, bigIntColVal);
                    }
                    break;
                case SQL_VARCHAR:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_CHAR, (SQLPOINTER)stringColVal, (SQLLEN)1024, &stringInd);
                    if (!check_and_print_null(nameBuff, &stringInd, fp)) {
                        printf("%s: %s\n", nameBuff, stringColVal);
                        fprintf(fp, "%s: %s\n", nameBuff, stringColVal);
                    }
                    break;
                case SQL_BIT:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_BIT, (SQLPOINTER)(&boolColVal), (SQLLEN)0, &boolInd);
                    if (!check_and_print_null(nameBuff, &boolInd, fp)) {
                        char * boolVal = boolColVal == 0 ? "false" : "true";
                        printf("%s: %s\n", nameBuff, boolVal);
                        fprintf(fp, "%s: %s\n", nameBuff, boolVal);
                    }
                    break;
                case SQL_TYPE_DATE:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_TYPE_DATE, (SQLPOINTER)(&dateColVal), (SQLLEN)(sizeof(dateColVal)), &dateInd);
                    if (!check_and_print_null(nameBuff, &dateInd, fp)) {
                        int year = dateColVal.year;
                        int month = dateColVal.month;
                        int day = dateColVal.day;
                        printf("Date: %04d-%02d-%02d\n", year, month, day);
                    }
                    break;
                case SQL_TYPE_TIME:
                    SQLGetData(stmt,(SQLUSMALLINT)i,SQL_C_TYPE_TIME,(SQLPOINTER)(&timeColVal),(SQLLEN)(sizeof(timeColVal)),&timeInd);
                    if (!check_and_print_null(nameBuff, &timeInd, fp)) {
                        int hour = timeColVal.hour;
                        int minute = timeColVal.minute;
                        int second = timeColVal.second;
                        printf("Time: %02d:%02d:%02d\n", hour, minute, second);
                    }
                    break;
                case SQL_TYPE_TIMESTAMP:
                    SQLGetData(stmt,(SQLUSMALLINT)i,SQL_C_TYPE_TIMESTAMP,(SQLPOINTER)(&datetimeColVal),(SQLLEN)(sizeof(datetimeColVal)),&datetimeInd);
                    if (!check_and_print_null(nameBuff, &datetimeInd, fp)) {
                        int year = datetimeColVal.year;
                        int month = datetimeColVal.month;
                        int day = datetimeColVal.day;
                        int hour = datetimeColVal.hour;
                        int minute = datetimeColVal.minute;
                        int second = datetimeColVal.second;
                        int fraction = datetimeColVal.fraction;
                        printf("DateTime: %04d-%02d-%02d %02d:%02d:%02d:%03d", year, month, day,hour, minute, second,fraction);
                    }
                    break;
                }
            }
            printf("\n\n");
            fprintf(fp, "\n\n");
            row++;
        }
    }

    fclose(fp);
    return 0;
}
