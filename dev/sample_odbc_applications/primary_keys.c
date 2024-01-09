#include "utils.h"

#define TAB_LEN SQL_MAX_TABLE_NAME_LEN + 1
#define COL_LEN SQL_MAX_COLUMN_NAME_LEN + 1

int main () {
    FILE *fp;
    fp = fopen("primary_keys.output", "w"); // open file in write mode

    SQLHENV  henv  = SQL_NULL_HENV;   // Environment
    SQLHDBC  hdbc  = SQL_NULL_HDBC;   // Connection handle
    SQLHSTMT hstmt = SQL_NULL_HSTMT;  // Statement handle

    SQLRETURN retcode;
    SQLSMALLINT columns;
    SQLCHAR stringColVal[1024];
    SQLBIGINT bigIntColVal;
    SQLSMALLINT smallIntColVal;
    SQLINTEGER intColVal;
    SQLSCHAR boolColVal;
    SQLLEN stringInd, bigintInd, boolInd, smallIntInd, intInd;
    SQLCHAR outstr[1024];
    SQLSMALLINT outstrlen;

   // Column key sequence (Note: Column 5 in SQLPrimaryKey call is small int) */

    SQLSMALLINT   iKeySeq;
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    check_error(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)", henv, SQL_HANDLE_ENV, fp);

    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                                            (SQLCHAR *)(void*)SQL_OV_ODBC3, -1);
    check_error(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    check_error(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
    check_error(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
                hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLDriverConnect(hdbc, NULL, "DSN=Couchbase DSN (ANSI);", SQL_NTS,
                         outstr, sizeof(outstr), &outstrlen,
                         SQL_DRIVER_COMPLETE);
    check_error(retcode, " SQLDriverConnect", hdbc, SQL_HANDLE_DBC, fp);

    retcode = SQLAllocHandle( SQL_HANDLE_STMT, hdbc, &hstmt);
    check_error(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
                hstmt, SQL_HANDLE_STMT, fp);

    /*Get Primary Key : Capella Columnar
    CREATE COLLECTION new_collection PRIMARY KEY (id:int, name:string,email:string)
    SELECT * FROM new_collection
    [
        {
            "new_collection": {
            "id": 2,
            "name": "peeyush",
            "email": "peeyush@gmail.com"
            }
        },
        {
            "new_collection": {
            "id": 1,
            "name": "janhavi",
            "email": "janhavi@gmail.com"
            }
        },
        {
            "new_collection": {
            "id": 3,
            "name": "utsav",
            "email": "utsav@gmail.com"
            }
        }
        ]

    1. create `tav_new` with primary key = id in `Default`.`Default`
        CREATE ANALYTICS VIEW tav_new(
            id int,
            name string,
            email string) default NULL PRIMARY KEY (id) NOT ENFORCED AS
        SELECT id,name,email
        FROM new_collection;

    2. create `tav_new` with primary key = name in `Default`.`new`
        CREATE ANALYTICS VIEW Default.new.tav_new(
            id int,
            name string,
            email string) default NULL PRIMARY KEY (name) NOT ENFORCED AS
        SELECT id, name,email
        FROM Default.Default.new_collection;

    2. create `tav_new` with primary key = name in `Default`.`new`
        CREATE database new_database
        CREATE SCOPE new_database.new
        CREATE ANALYTICS VIEW `new_database`.`new`.`tav_new`(
            id int,
            name string,
            email string) default NULL PRIMARY KEY (email) NOT ENFORCED AS
        SELECT id,name,email
        FROM Default.Default.new_collection;
    */
    // retcode = SQLPrimaryKeys(hstmt,
    //                          "Default",7 ,                   // Catalog name
    //                          "Default", 7,                   // Schema name
    //                          "tav_new", SQL_NTS);            // Table name
    //Expected output:  COLUMN_NAME	-> id

    retcode = SQLPrimaryKeys(hstmt,
                             "Default",7 ,                    // Catalog name
                             "new", 3,                        // Schema name
                             "tav_new", SQL_NTS);             // Table name
    //Expected output:  COLUMN_NAME	-> name

    // retcode = SQLPrimaryKeys(hstmt,
    //                          "new_database", 12,              // Catalog name
    //                          "new", 3,                        // Schema name
    //                          "tav_new", SQL_NTS);             // Table name
    //Expected output:  COLUMN_NAME	-> email

    /*Get Primary Key : Couchbase Analytics
    two part scope, example: travel-sample.inventory
    Expected output:  COLUMN_NAME	-> route_id
    */
    // retcode = SQLPrimaryKeys(hstmt,
    //                          "travel-sample", 13,             // Catalog name
    //                          "inventory", 9,                  // Schema name
    //                          "route_view", SQL_NTS);          // Table name

    /*Get Primary Key : Couchbase Analytics
    one part scope, example: one
        CREATE ANALYTICS SCOPE one;
        Create col collection -> route collection from travel-sample bucket
        create col_view with route_id as Primary key->
        CREATE ANALYTICS VIEW col_view (
        route_id BIGINT NOT UNKNOWN,
        airline_iata_code STRING NOT UNKNOWN,
        source_airport_iata_code STRING NOT UNKNOWN,
        dest_airport_iata_code STRING NOT UNKNOWN,
        route_stops BIGINT,
        route_equipment STRING,
        route_distance DOUBLE) default NULL
        primary key (route_id) NOT ENFORCED
        AS SELECT id AS route_id,
        airline AS airline_iata_code,
        sourceairport AS source_airport_iata_code,
        destinationairport AS dest_airport_iata_code,
        stops AS route_stops,
        equipment AS route_equipment,
        distance AS route_distance
        FROM `travel-sample`.inventory.route
    */
    // retcode = SQLPrimaryKeys(hstmt,
    //                          "one", 3,               // Catalog name
    //                          "", 0,                  // Schema name
    //                          "col_view", SQL_NTS);   // Table name

    check_error(retcode, "SQLPrimaryKeys", hstmt, SQL_HANDLE_STMT, fp);

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