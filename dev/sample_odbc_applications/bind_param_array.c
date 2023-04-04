#include <sql.h>
#include <sqlext.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define ROWSET_SIZE 3      // How many rows at a time
#define PARAM_ARRAY_SIZE 2 // Number of params in total
#define DATA_ARRAY_SIZE 3  // Buffers for rowsets

int main() {
    FILE *fp;
    fp = fopen("bind_param_array.output", "w"); // open file in write mode

    SQLHENV henv = SQL_NULL_HENV;    // Environment
    SQLHDBC hdbc = SQL_NULL_HDBC;    // Connection handle
    SQLHSTMT hstmt = SQL_NULL_HSTMT; // Statement handle

    SQLRETURN retcode;

    SQLCHAR stmt[]
        = "SELECT hv.hotel_id, hv.hotel_name FROM `travel-sample`.`inventory`.`hotel_view` hv WHERE hv.hotel_free_parking=? LIMIT 5;";

    SQLCHAR params_hotel_free_parking[] = {true, false};

    // Define structure for data
    typedef struct tagHotelStruct {
        SQLBIGINT hotelID;
        SQLLEN hotelIDInd;
        SQLCHAR hotelName[1024];
        SQLLEN hotelNameLenOrInd;
    } HotelStruct;
    HotelStruct HotelArray[DATA_ARRAY_SIZE];

    SQLUSMALLINT ParamStatusArray[PARAM_ARRAY_SIZE];

    // Operation Array used to indicate whether a parameter from the parameter
    // array should be ignored or used. Here the first 2 parameters are ignored
    // and will not appear in the results.
    SQLUSMALLINT ParamOperationsArray[PARAM_ARRAY_SIZE] = {
        SQL_PARAM_IGNORE,
        SQL_PARAM_IGNORE,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
        SQL_PARAM_PROCEED,
    };

    SQLLEN ParamsProcessed = 0;
    int i;

    SQLUSMALLINT RowStatusArray[DATA_ARRAY_SIZE], Action, RowNum;
    SQLLEN NumUpdates = 0, NumInserts = 0, NumDeletes = 0;
    SQLLEN BindOffset = 0;
    SQLLEN RowsFetched = 0;
    SQLLEN Concurrency = SQL_CONCUR_LOCK;
    SQLLEN rowCount;

    //
    // Column-wise binding
    //
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)", henv, SQL_HANDLE_ENV, fp);

    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLCHAR *)(void *)SQL_OV_ODBC3, -1);
    CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)", henv, SQL_HANDLE_ENV, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQLAllocHandle)", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
    CHECK_ERROR(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)", hdbc, SQL_HANDLE_DBC, fp);

    // retcode = SQLConnect(hdbc, (SQLCHAR *)"DATASOURCE", SQL_NTS, (SQLCHAR *)NULL, 0, NULL, 0);
    retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    CHECK_ERROR(retcode, " SQLDriverConnect(\"DSN=Couchbase DSN (ANSI);\")", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)", hstmt, SQL_HANDLE_STMT, fp);

    // Setup for SQLFetchScroll and SQLMoreResults
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0);
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(HotelStruct), 0);
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)3, 0);
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, RowStatusArray, 0);
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_OFFSET_PTR, &BindOffset, 0);
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &RowsFetched, 0);
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER)SQL_CONCUR_LOCK, 0);

    // Setup for parameter array processing
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)PARAM_ARRAY_SIZE, 0);
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAM_STATUS_PTR, ParamStatusArray, PARAM_ARRAY_SIZE);
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &ParamsProcessed, 0);
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAM_OPERATION_PTR, ParamOperationsArray, PARAM_ARRAY_SIZE);

    // Bind array values of parameter 1 data in
    retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, params_hotel_free_parking, 0, NULL);

    // Bind columns for data out
    retcode = SQLBindCol(hstmt, 1, SQL_C_SBIGINT, &HotelArray[0].hotelID, 0, &HotelArray[0].hotelIDInd);
    retcode = SQLBindCol(hstmt, 2, SQL_C_CHAR, (SQLPOINTER)HotelArray[0].hotelName, 1024, (SQLLEN *)&HotelArray[0].hotelNameLenOrInd);

    retcode = SQLExecDirect(hstmt, stmt, SQL_NTS);
    CHECK_ERROR(retcode, "SQLExecDirect()", hstmt, SQL_HANDLE_STMT, fp);

    do {
        int rowSet = 1;
        while (SQLFetchScroll(hstmt, SQL_FETCH_NEXT, 0) == SQL_SUCCESS) {
            if (rowSet == 1)
                printf("\nParams Processed : %i\n", (int)ParamsProcessed);
                fprintf(fp, "\nParams Processed : %i\n", (int)ParamsProcessed);
            printf("\nRowSet: %d\n", rowSet);
            fprintf(fp, "\nRowSet: %d\n", rowSet);

            /* You can also see the Status of Each row in the row set
            printf("\nRowset Status Array : \n");
            fprintf(fp, "\nRowset Status Array : \n");
            for (i = 0; i < DATA_ARRAY_SIZE; i++) {
                switch (RowStatusArray[i]) {
                    case SQL_ROW_SUCCESS_WITH_INFO:
                    case SQL_ROW_SUCCESS:
                        printf("\n  %i - ROW SUCCESS\n", i);
                        fprintf(fp, "\n  %i - ROW SUCCESS\n", i);
                        break;
                    case SQL_ROW_NOROW:
                        printf("\n  %i - NO ROW\n", i);
                        fprintf(fp, "\n  %i - NO ROW\n", i);
                        break;
                    default:
                        printf("\n  %i - ?\n", (int)RowStatusArray[i]);
                        fprintf(fp, "\n  %i - ?\n", (int)RowStatusArray[i]);
                }
            }
            */

            for (i = 0; i < RowsFetched; i++) {
                printf("\trow %i: \n", i);
                fprintf(fp, "\trow %i: \n", i);
                printf("\t\tHotelID: %ld \n", HotelArray[i].hotelID);
                fprintf(fp, "\t\tHotelID: %ld \n", HotelArray[i].hotelID);
                printf("\t\tHotelName: %s \n", HotelArray[i].hotelName);
                fprintf(fp, "\t\tHotelName: %s \n", HotelArray[i].hotelName);
            }
            rowSet++;
        }
    } while (SQLMoreResults(hstmt) == SQL_SUCCESS);

exit:

    printf("\nComplete.\n");
    fprintf(fp, "\nComplete.\n");

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