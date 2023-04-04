#include <stdio.h>
#include <string.h>
#include "utils.h"

int main(int argc, char * argv[]) {
    FILE *fp;
    fp = fopen("bind_params.output", "w"); // open file in write mode
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

    SQLDriverConnect(dbc, NULL, (SQLCHAR *)"DSN=Couchbase DSN (ANSI);", SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char * queryTxt = "SELECT hv.* FROM `travel-sample`.`inventory`.`hotel_view` AS hv WHERE "
                            "hv.country = ? AND hv.hotel_id = ? AND hv.hotel_geo_lat = ? AND hv.hotel_free_parking = ? LIMIT 2;";

    // Params binding
    SQLCHAR param_country[1024];
    SQLLEN len_param_country = 0;
    SQLBindParameter(
        stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 1024, 0, (SQLPOINTER)param_country, 1024, (SQLLEN *)&len_param_country);

    SQLINTEGER param_hotel_id;
    SQLLEN param_hotel_id_ind;
    SQLBindParameter(
        stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&param_hotel_id, 0, (SQLLEN *)&param_hotel_id_ind);

    SQLDOUBLE param_lat;
    SQLLEN param_lat_ind;
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, (SQLPOINTER)&param_lat, 0, (SQLLEN *)&param_lat_ind);

    SQLCHAR param_free_parking;
    SQLLEN param_free_parking_ind;
    SQLBindParameter(
        stmt, 4, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, (SQLPOINTER)&param_free_parking, 0, (SQLLEN *)&param_free_parking_ind);

    // Fill Params
    strcpy(param_country, (SQLCHAR *)"United Kingdom");
    len_param_country = strlen(param_country);
    param_hotel_id = 10025;
    param_lat = 51.35785;
    param_free_parking = true;

    SQLExecDirect(stmt, (SQLCHAR *)queryTxt, strlen(queryTxt));

    SQLNumResultCols(stmt, &columns);
    printf("total cols: %u\n", columns);
    fprintf(fp, "total cols: %u\n", columns);

    SQLDOUBLE doubleColVal;
    SQLCHAR stringColVal[1024];
    SQLBIGINT bigIntColVal;
    SQLCHAR boolColVal;
    SQLLEN doubleInd, stringInd, bigintInd, boolInd;

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
                    if (!check_and_print_null(nameBuff, &stringInd, fp)) {
                        printf("%s: %lf\n", nameBuff, doubleColVal);
                        fprintf(fp, "%s: %lf\n", nameBuff, doubleColVal);
                    }
                    break;
                case SQL_BIGINT:
                    SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_SBIGINT, (SQLPOINTER)(&bigIntColVal), (SQLLEN)0, &bigintInd);
                    if (!check_and_print_null(nameBuff, &stringInd, fp)) {
                        printf("%s: %ld\n", nameBuff, bigIntColVal);
                        fprintf(fp, "%s: %ld\n", nameBuff, bigIntColVal);
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
                    if (!check_and_print_null(nameBuff, &stringInd, fp)) {
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
    fclose(fp);

    return 0;
}