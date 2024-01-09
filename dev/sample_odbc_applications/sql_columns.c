#include "utils.h"

#define DTSIZE 35
int main () {

    SQLHENV  henv  = SQL_NULL_HENV;   	// Environment
    SQLHDBC  hdbc  = SQL_NULL_HDBC;   	// Connection handle
    SQLHSTMT hstmt = SQL_NULL_HSTMT;  	// Statement handle
    SQLRETURN retcode;			// Return status
    SQLSMALLINT columns;

    SQLCHAR stringColVal[1024];
    SQLBIGINT bigIntColVal;
    SQLSMALLINT smallIntColVal;
    SQLINTEGER intColVal;

    SQLSCHAR boolColVal;
    SQLLEN stringInd, bigintInd, boolInd, smallIntInd, intInd;

    FILE *fp;

    fp = fopen("sql_columns.output", "w"); // open file in write mode

    // Create environment handle
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    check_error(retcode, "SQLAllocHandle(ENV)", henv, SQL_HANDLE_ENV, fp);

    // Set ODBC 3 behaviour
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                                        (SQLCHAR *)(void*)SQL_OV_ODBC3, -1);
    check_error(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    check_error(retcode, "SQLAllocHandle(DBC)",
                hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
    check_error(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
                hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    check_error(retcode, " SQLDriverConnect(\"DSN=Couchbase DSN (ANSI);\")", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    check_error(retcode, "SQLAllocHandle(STMT)",
                hstmt, SQL_HANDLE_STMT, fp);

    /*
    Sample SQLColumns call
    Note: Verfify following scenarios with Capella Columnar always:
    1) A database with two different scopes, i.e., "a.b1" and "a.b2".
    2) Two different databases with ssame scope name, i.e., "a1.b" and "a2.b".
    (Ref primary_keys.c fpr more)
    */
    retcode = SQLColumns(hstmt, "travel-sample", 14, "inventory", 9, "singlecol", 8, "name", 4);
    check_error(retcode, "SQLColumns",
                hstmt, SQL_HANDLE_STMT, fp);

    /* How many columns are there */
    SQLNumResultCols(hstmt, &columns);
    printf("\ntotalCols: %u\n", columns);
    fprintf(fp, "\ntotalCols: %u\n", columns);

    char colVal_typeName[1024];
    long dataTypeVal;
    char dataValStr[1024];
    char precisionStr[1024];

    for(int i = 1; i <= columns; i++) {
        char nameBuff[1024];
        SQLSMALLINT nameBuffLenUsed;
        retcode = SQLColAttribute(hstmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);
        check_error(retcode, "SQLColAttribute",
                hstmt, SQL_HANDLE_STMT, fp);
        printf("%s\t", nameBuff);
        fprintf(fp, "%s\t", nameBuff);
    }
    while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS) {
        printf("\n\n");
        fprintf(fp, "\n\n");
        for (int i = 1; i<= columns; i++) {
        SQLLEN columnType = 0;
        SQLLEN columnLen = 0;
        SQLColAttribute(hstmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);
        SQLColAttribute(hstmt, (SQLUSMALLINT)i, SQL_DESC_LENGTH, NULL, 0, NULL, &columnLen);

        switch (columnType) {
            case SQL_SMALLINT:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_C_SSHORT, (SQLPOINTER)(&smallIntColVal), (SQLLEN)0, &smallIntInd);
                    if (!check_and_print_null_two_params(&smallIntInd, fp)) {
                        printf("%ld\t", smallIntInd);
                        printf("%d\t", smallIntColVal);
                        fprintf(fp, "%d\t", smallIntColVal);
                    }
                    break;

            case SQL_INTEGER:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_INTEGER, (SQLPOINTER)(&intColVal), (SQLLEN)0, &intInd);
                    if (!check_and_print_null_two_params(&intInd, fp)) {
                        printf("%ld\t", intInd);
                        printf("%d\t", intColVal);
                        fprintf(fp, "%d\t", intColVal);
                    }
                    break;

            case SQL_BIGINT:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_C_SBIGINT, (SQLPOINTER)(&bigIntColVal), (SQLLEN)0, &bigintInd);
                if (!check_and_print_null_two_params(&bigintInd, fp)) {
                    printf("%ld\t", bigintInd);
                    printf("%ld\t", bigIntColVal);
                    fprintf(fp, "%ld\t", bigIntColVal);
                }
                break;
            case SQL_VARCHAR:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_C_CHAR, (SQLPOINTER)stringColVal, (SQLLEN)1024, &stringInd);
                if (!check_and_print_null_two_params(&stringInd, fp)) {
                    printf("%s\t", stringColVal);
                    fprintf(fp, "%s\t", stringColVal);

                    printf("columnLen : %p\n\n", columnLen);
                    printf("columnType : %p\n\n", columnType);
                }
                break;
            case SQL_BIT:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_C_BIT, (SQLPOINTER)(&boolColVal), (SQLLEN)0, &boolInd);
                if (!check_and_print_null_two_params(&boolInd, fp)) {
                    char * boolVal = boolColVal == 0 ? "false" : "true";
                    printf("%s\t", boolVal);
                    fprintf(fp, "%s\t", boolVal);
                }
                break;
            }
        }
        printf("\n\n");
        fprintf(fp, "\n\n");
    }
    printf("\n------------");
    fprintf(fp, "\n------------");
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