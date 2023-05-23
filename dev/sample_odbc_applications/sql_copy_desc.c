#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

#define ROWS 10

#define BOOKMARK_LEN  10
#define PERSONID_LEN  2
#define LASTNAME_LEN  255
#define FIRSTNAME_LEN 255
#define ADDRESS_LEN 255
#define CITY_LEN  255

SQLRETURN retcode;

// Person row (same as TestTBL1) minus PersonID identity field
typedef struct tagCustStruct {
    SQLCHAR         FirstName[255];
    SQLLEN          lenFirstName;
    SQLCHAR         LastName[255];
    SQLLEN          lenLastName;
    SQLCHAR         Address[255];
    SQLLEN          lenAddress;
    SQLCHAR         City[255];
    SQLLEN          lenCity;
} CustStruct;

int main () {

    CustStruct      CustArray[ROWS];            // rowset buffer
    SQLUSMALLINT    sts_ptr[ROWS];              // status pointer

    SQLHDESC        hArd0, hIrd0, hApd1, hIpd1;

    SQLHENV  henv   = SQL_NULL_HENV;     // Environment
    SQLHDBC  hdbc   = SQL_NULL_HDBC;     // Connection handle
    SQLHSTMT hstmt0 = SQL_NULL_HSTMT;    // Statement handle
    SQLHSTMT hstmt1 = SQL_NULL_HSTMT;    // Statement handle
    SQLRETURN retcode;

    SQLLEN RowsFetched = 0, params_processed = 0;
    int i;

    FILE *fp;
    fp = fopen("sql_copy_desc.output", "w"); // open file in write mode

    // Allocate Environment Handle
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
                henv, SQL_HANDLE_ENV, fp);

    // Set ODBC version
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                            (SQLCHAR *)(void*)SQL_OV_ODBC3, -1);
    CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    // Allocate Connection Handle
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
                hdbc, SQL_HANDLE_DBC, fp);

    // Set Login Timeout
    retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
    CHECK_ERROR(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
                hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLDriverConnect(hdbc, NULL, "DSN=Couchbase DSN (ANSI);", SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    CHECK_ERROR(retcode, " SQLDriverConnect(\"DSN=Couchbase DSN (ANSI);\")", hdbc, SQL_HANDLE_DBC, fp);


    // Allocate Statement 0 Handle (For Select)
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt0);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_STMT0)",
                hstmt0, SQL_HANDLE_STMT, fp);

    // Allocate Statement 1 Handle (For Insert)
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt1);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_STMT1)",
                hstmt1, SQL_HANDLE_STMT, fp);

    // Get the ARD and IRD row descriptors for hstmt0
    retcode = SQLGetStmtAttr(hstmt0, SQL_ATTR_APP_ROW_DESC, &hArd0, 0, NULL);
    CHECK_ERROR(retcode, "SQLGetStmtAttr(SQL_ATTR_APP_ROW_DESC)",
                hstmt0, SQL_HANDLE_STMT, fp);

    retcode = SQLGetStmtAttr(hstmt0, SQL_ATTR_IMP_ROW_DESC, &hIrd0, 0, NULL);
    CHECK_ERROR(retcode, "SQLGetStmtAttr(SQL_ATTR_IMP_ROW_DESC)",
                hstmt0, SQL_HANDLE_STMT, fp);

    // Get the APD and IPD param descriptors for hstmt1
    retcode = SQLGetStmtAttr(hstmt1, SQL_ATTR_APP_PARAM_DESC, &hApd1, 0, NULL);
    CHECK_ERROR(retcode, "SQLGetStmtAttr(SQL_ATTR_APP_PARAM_DESC)",
                hstmt1, SQL_HANDLE_STMT, fp);

    retcode = SQLGetStmtAttr(hstmt1, SQL_ATTR_IMP_PARAM_DESC, &hIpd1, 0, NULL);
    CHECK_ERROR(retcode, "SQLGetStmtAttr(SQL_ATTR_IMP_PARAM_DESC)",
                hstmt1, SQL_HANDLE_STMT, fp);

    // Use row-wise binding on hstmt0 to fetch rows
    retcode = SQLSetStmtAttr(hstmt0, SQL_ATTR_ROW_BIND_TYPE,
                            (SQLPOINTER) sizeof(CustStruct), 0);
    CHECK_ERROR(retcode, "SQLSetStmtAttr(SQL_ATTR_ROW_BIND_TYPE)",
                hstmt0, SQL_HANDLE_STMT, fp);

    // Set rowset size for hstmt0
    retcode = SQLSetStmtAttr(hstmt0, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) ROWS, 0);
    CHECK_ERROR(retcode, "SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE)",
                hstmt0, SQL_HANDLE_STMT, fp);

    // Rows fetched
    retcode = SQLSetStmtAttr(hstmt0, SQL_ATTR_ROWS_FETCHED_PTR, &RowsFetched,0);
    CHECK_ERROR(retcode, "SQLSetStmtAttr(SQL_ATTR_ROWS_FETCHED_PTR)",
                hstmt0, SQL_HANDLE_STMT, fp);

    // Execute a SELECT statement
    retcode = SQLExecDirect(hstmt0,
                            "SELECT * from my_standalone_dataset1;", SQL_NTS);
    CHECK_ERROR(retcode, "SQLExecDirect(SQL_HANDLE_STMT0)",
                hstmt0, SQL_HANDLE_STMT, fp);

    // Bind columns for reading records
    // Bind COL 1 - FirstName
    retcode = SQLBindCol(hstmt0, 1, SQL_C_CHAR,
                         &CustArray[0].FirstName,
                         sizeof(CustArray[0].FirstName),
                         &CustArray[0].lenFirstName);
    CHECK_ERROR(retcode, "SQLBindCol(1)",
                hstmt0, SQL_HANDLE_STMT, fp);


    CHECK_ERROR(retcode, "SQLBindCol(4)",
                hstmt0, SQL_HANDLE_STMT, fp);

    // Perform parameter bindings on hstmt1.
    // Copy SELECT ARD to INSERT APD (App RECORD descriptor
    // becomes App PARAM descriptor)
    retcode = SQLCopyDesc(hArd0, hApd1);
    if ( (retcode != SQL_SUCCESS) &&
         (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLCopyDesc(hArd0, hApd1, 0)", hArd0,
                      SQL_HANDLE_DESC, fp);
        extract_error("SQLCopyDesc(hArd0, hApd1, 1)", hApd1,
                      SQL_HANDLE_DESC, fp);
        goto exit;
    }

    // Copy SELECT IRD to INSERT IPD
    // (Imp RECORD descriptor becomes Imp PARAM descriptor)
    retcode = SQLCopyDesc(hIrd0, hIpd1);
    if ( (retcode != SQL_SUCCESS) &&
         (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLCopyDesc(hIrd0, hIpd1, 0)", hIrd0,
                      SQL_HANDLE_DESC, fp);
        extract_error("SQLCopyDesc(hIrd0, hIpd1, 1)", hIpd1,
                     SQL_HANDLE_DESC, fp);
        goto exit;
    }

    // Set the ARRAY_STATUS_PTR field of IRD (ROW STATUS Array for SELECT)
    retcode = SQLSetStmtAttr(hstmt0, SQL_ATTR_ROW_STATUS_PTR, sts_ptr,
                             SQL_IS_POINTER);
    CHECK_ERROR(retcode, "SQLSetStmtAttr(SQL_ATTR_ROW_STATUS_PTR)",
                hstmt0, SQL_HANDLE_STMT, fp);

    // Set the ARRAY_STATUS_PTR field of APD to be the same as that in IRD
    // (ROW STATUS Array for INSERT)
    retcode = SQLSetStmtAttr(hstmt1, SQL_ATTR_PARAM_OPERATION_PTR, sts_ptr,
                             SQL_IS_POINTER);
    CHECK_ERROR(retcode, "SQLSetStmtAttr(PARAM_OPERATION_PTR)",
                hstmt1, SQL_HANDLE_STMT, fp);

    // Set the hIpd1 record input parameters
    // Set Desc Field 1 as INPUT in INSERT IPD
    retcode = SQLSetDescField(hIpd1, 1, SQL_DESC_PARAMETER_TYPE,
                              (SQLPOINTER)SQL_PARAM_INPUT, SQL_IS_INTEGER);
    CHECK_ERROR(retcode, "SQLSetDescField(DESC_PARAMETER_TYPE 1)",
                hstmt1, SQL_HANDLE_STMT, fp);

    // Set Desc Field 2 as INPUT in INSERT IPD
    retcode = SQLSetDescField(hIpd1, 2, SQL_DESC_PARAMETER_TYPE,
                              (SQLPOINTER)SQL_PARAM_INPUT, SQL_IS_INTEGER);
    CHECK_ERROR(retcode, "SQLSetDescField(DESC_PARAMETER_TYPE 2)",
                hstmt1, SQL_HANDLE_STMT, fp);

    // Set Desc Field 3 as INPUT in INSERT IPD
    retcode = SQLSetDescField(hIpd1, 3, SQL_DESC_PARAMETER_TYPE,
                              (SQLPOINTER)SQL_PARAM_INPUT, SQL_IS_INTEGER);
    CHECK_ERROR(retcode, "SQLSetDescField(DESC_PARAMETER_TYPE 3)",
                hstmt1, SQL_HANDLE_STMT, fp);

    // Set Desc Field 4 as INPUT in INSERT IPD
    retcode = SQLSetDescField(hIpd1, 4, SQL_DESC_PARAMETER_TYPE,
                              (SQLPOINTER)SQL_PARAM_INPUT, SQL_IS_INTEGER);
    CHECK_ERROR(retcode, "SQLSetDescField(DESC_PARAMETER_TYPE 4)",
                hstmt1, SQL_HANDLE_STMT, fp);

    // Prepare an INSERT statement on hstmt1.
    // TestTBL1Copy is a copy of TestTBL1
    retcode = SQLPrepare(hstmt1, "UPSERT INTO my_standalone_dataset1 [{'my_pk':'hello2'}];", SQL_NTS);
    CHECK_ERROR(retcode, "SQLPrepare(hstmt1)",
                hstmt1, SQL_HANDLE_STMT, fp);

    // In a loop, fetch a rowset, and copy the fetched rowset to TestTBL1Copy
    // Fetch initial rowset from SELECT
    retcode = SQLFetchScroll(hstmt0, SQL_FETCH_NEXT, 0);

    // Loop while data and success
    while (SQL_SUCCEEDED(retcode)) {

        printf ("Rows Fetched : %i\n", (int) RowsFetched);
        fprintf (fp, "Rows Fetched : %i\n", (int) RowsFetched);

        for (i=0;i<ROWS;i++) {
            printf ("%i", sts_ptr[i]);
            fprintf (fp, "%i", sts_ptr[i]);
            if (i!=ROWS-1)
                printf (", ");
                fprintf (fp, ", ");
            else
                printf ("\n");
                fprintf (fp, "\n");
        }

        for (i=0;i<RowsFetched;i++) {
            printf ("Record %i, Status %i, First Field - %.10s\n",
                    i+1, sts_ptr[i], CustArray[i].FirstName);
            fprintf (fp, "Record %i, Status %i, First Field - %.10s\n",
                    i+1, sts_ptr[i], CustArray[i].FirstName);
        }

        // The row status array (returned by the FetchScroll of hstmt0)
        // is used as input status in the APD of hstmt1 and hence determines
        // which elements of the rowset buffer are inserted.

        // Execute INSERT
        retcode = SQLExecute(hstmt1);
        CHECK_ERROR(retcode, "SQLExecute(hstmt1)",
                    hstmt1, SQL_HANDLE_STMT, fp);
        printf ("SQLExecute(hstmt1) OK\n");
        fprintf (fp, "SQLExecute(hstmt1) OK\n");

        // Fetch Scroll next rowset from SELECT
        retcode = SQLFetchScroll(hstmt0, SQL_FETCH_NEXT, 0);
        if (retcode == SQL_NO_DATA) {
            printf ("SQL_NO_DATA\n");
            fprintf (fp, "SQL_NO_DATA\n");
        } else {
            CHECK_ERROR(retcode, "SQLFetchScroll(SQL_HANDLE_STMT 0)",
                        hstmt0, SQL_HANDLE_STMT, fp);
            printf ("SQLFetchScroll(hstmt0) OK\n");
            fprintf (fp, "SQLFetchScroll(hstmt0) OK\n");
        }
    }

exit:

    printf ("\nComplete.\n");
    fprintf (fp, "\nComplete.\n");

    fclose(fp);

    // Free handles
    // Statement 0
    if (hstmt0 != SQL_NULL_HSTMT)
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt0);

    // Statement 1
    if (hstmt1 != SQL_NULL_HSTMT)
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt1);

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