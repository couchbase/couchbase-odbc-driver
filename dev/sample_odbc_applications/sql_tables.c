#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

#define PERSONID_LEN  2
#define LASTNAME_LEN  255
#define FIRSTNAME_LEN 255
#define ADDRESS_LEN 255
#define CITY_LEN  255

bool check_and_print_null(const char * nameBuff, SQLLEN * ind) {
    if (ind && *ind == SQL_NULL_DATA) {
        printf("%s: NULL\n", nameBuff);
        return true;
    }

    return false;
}

int main(int argc, char * argv[]) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;       /* ODBC API return status */
    SQLSMALLINT columns; /* number of columns in result-set */
    RETCODE retcode;


    SQLLEN cPersonId;
    // SQLCHAR strFirstName[FIRSTNAME_LEN];
    SQLCHAR strLastName[LASTNAME_LEN];
    SQLCHAR strAddress[ADDRESS_LEN];
    SQLCHAR strCity[CITY_LEN];
    SQLLEN lenFirstName=0, lenLastName=0, lenAddress=0, lenCity=0;
    SQLINTEGER strFirstName;

    /* Allocate an environment handle */
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    /* We want ODBC 3 support */
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
    /* Allocate a connection handle */
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    SQLDriverConnect(dbc, NULL, (SQLCHAR *)("DSN=local;"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);

    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    printf("first sqltables call\n");
    retcode = SQLTables( stmt, "%",1, NULL, 0, NULL,0, NULL, 0 );
    /* How many columns are there */
    SQLNumResultCols(stmt, &columns);
    printf("totalCols: %u\n", columns); //expected 5

    printf("first sqltables call\n");
    retcode = SQLTables( stmt, "%",1, NULL, 0, NULL,0, "TABLE,VIEW", 10 );
    /* How many columns are there */
    SQLNumResultCols(stmt, &columns);
    printf("totalCols: %u\n", columns); //expected 5

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