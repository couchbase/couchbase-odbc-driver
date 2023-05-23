#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

#define TAB_LEN SQL_MAX_TABLE_NAME_LEN + 1
#define COL_LEN SQL_MAX_COLUMN_NAME_LEN + 1

bool checkAndPrintNull(SQLLEN * ind, FILE *fp);
bool checkAndPrintNull(SQLLEN * ind, FILE *fp) {
    if (ind && *ind == SQL_NULL_DATA) {
            printf("NULL\t");
            if (fp != NULL)
                fprintf(fp, "NULL\t");
            return true;
        }
    return false;
}

int main () {
    FILE *fp;
    fp = fopen("foreign_keys.output", "w"); // open file in write mode

    SQLHENV  henv  = SQL_NULL_HENV;   // Environment
    SQLHDBC  hdbc  = SQL_NULL_HDBC;   // Connection handle
    SQLHSTMT hstmt = SQL_NULL_HSTMT;  // Statement handle

    SQLRETURN retcode;
    //
    // Four tables used in this example TestTBL5,TestTBL6,TestTBL7 and TestTBL8
    // Table 5 has a primary key and a foreign key linked to the primary key of
    // table 8. Table 6 has primary key and foreign key linked to the primary
    // key of table 5. Table 7 has primary key and foreign key linked to the
    // primary key of table 5. Table 8 has primary key
    //
    char   strTable[] = "good_customers5";
    char   strTable1[] = "good_customers6";



    UCHAR strPkTable[TAB_LEN];       // Primary key table name
    UCHAR strFkTable[TAB_LEN];       // Foreign key table name
    UCHAR strFkTabCat[TAB_LEN];      // Foreign Key table catalog
                                     // Column 5 in SQLForeignKey call is char
    UCHAR strPkCol[COL_LEN];         // Primary key column
    UCHAR strFkCol[COL_LEN];         // Foreign key column

    SQLLEN lenPkTable, lenPkCol, lenFkTabCat, lenFkTable, lenFkCol, lenKeySeq;

    SQLSMALLINT columns;

    SQLCHAR stringColVal[1024];
    SQLBIGINT bigIntColVal;
    SQLSMALLINT smallIntColVal;
    SQLINTEGER intColVal;

    SQLSCHAR boolColVal;
    SQLLEN stringInd, bigintInd, boolInd, smallIntInd, intInd;

   // Column key sequence (Note: Column 5 in SQLPrimaryKey call is small int) */

    SQLSMALLINT   iKeySeq;
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)", henv, SQL_HANDLE_ENV, fp);

    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                                            (SQLCHAR *)(void*)SQL_OV_ODBC3, -1);
    CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
    CHECK_ERROR(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
                hdbc, SQL_HANDLE_DBC, fp);

    // retcode = SQLConnect(hdbc, (SQLCHAR*) "DATASOURCE",
    //                      SQL_NTS, (SQLCHAR*) NULL, 0, NULL, 0);
    // CHECK_ERROR(retcode, "SQLConnect(DATASOURCE)", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLConnect(hdbc, (SQLCHAR*) "Couchbase DSN (ANSI)", SQL_NTS, (SQLCHAR*) "admin", 5, (SQLCHAR*) "000000", 6);
    CHECK_ERROR(retcode, " SQLConnect", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLAllocHandle( SQL_HANDLE_STMT, hdbc, &hstmt);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
                hstmt, SQL_HANDLE_STMT, fp);


    printf("\nPKTableName is present -- CASE:3");
    fprintf(fp, "\nPKTableName is present -- CASE:3");

    retcode = SQLForeignKeys(hstmt,
                             "travel-sample", 13,             // Catalog name
                             "inventory", 9,             // Schema name
                             strTable, SQL_NTS,
                             "travel-sample", 13,
                             "inventory", 9,
                             strTable1, SQL_NTS);   // Table name

    printf ("\nGet primary key of the %s table\n", strTable);
    fprintf (fp, "\nGet primary key of the %s table\n", strTable);
    CHECK_ERROR(retcode, "SQLPrimaryKeys", hstmt, SQL_HANDLE_STMT, fp);

    SQLNumResultCols(hstmt, &columns);

    printf ("Number of Result Columns %i\n", columns);
    fprintf (fp, "Number of Result Columns %i\n", columns);





    for(int i = 1; i <= columns; i++) {
        char nameBuff[1024];
        SQLSMALLINT nameBuffLenUsed;
        SQLColAttribute(hstmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);
        printf("%s\t", nameBuff);
        fprintf(fp, "%s\t", nameBuff);
    }
    while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS) {
        printf("\n\n");
        fprintf(fp, "\n\n");
        for (int i = 1; i<= columns; i++) {
        SQLLEN columnType = 0;
        SQLColAttribute(hstmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);

        switch (columnType) {
            case SQL_SMALLINT:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_C_SSHORT, (SQLPOINTER)(&smallIntColVal), (SQLLEN)0, &smallIntInd);
                    if (!checkAndPrintNull(&smallIntInd, fp)) {
                        printf("%ld\t", smallIntInd);
                        printf("%d\t", smallIntColVal);
                        fprintf(fp, "%d\t", smallIntColVal);
                    }
                    break;

            case SQL_INTEGER:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_INTEGER, (SQLPOINTER)(&intColVal), (SQLLEN)0, &intInd);
                    if (!checkAndPrintNull(&intInd, fp)) {
                        printf("%ld\t", intInd);
                        printf("%d\t", intColVal);
                        fprintf(fp, "%d\t", intColVal);
                    }
                    break;

            case SQL_BIGINT:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_C_SBIGINT, (SQLPOINTER)(&bigIntColVal), (SQLLEN)0, &bigintInd);
                if (!checkAndPrintNull(&bigintInd, fp)) {
                    printf("%ld\t", bigintInd);
                    printf("%ld\t", bigIntColVal);
                    fprintf(fp, "%ld\t", bigIntColVal);
                }
                break;
            case SQL_VARCHAR:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_C_CHAR, (SQLPOINTER)stringColVal, (SQLLEN)1024, &stringInd);
                if (!checkAndPrintNull(&stringInd, fp)) {
                    printf("%s\t", stringColVal);
                    fprintf(fp, "%s\t", stringColVal);
                }
                break;
            case SQL_BIT:
                SQLGetData(hstmt, (SQLUSMALLINT)i, SQL_C_BIT, (SQLPOINTER)(&boolColVal), (SQLLEN)0, &boolInd);
                if (!checkAndPrintNull(&boolInd, fp)) {
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
    fprintf (fp,"\nComplete.\n");
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