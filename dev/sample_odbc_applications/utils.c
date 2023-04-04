#include "utils.h"
#include <odbcinst.h>
#include <stdio.h>
#include <string.h>


void extract_error(char * fn, SQLHANDLE handle, SQLSMALLINT type, FILE *fp) {
    SQLINTEGER i = 0;
    SQLINTEGER NativeError;
    SQLCHAR SQLState[7];
    SQLCHAR MessageText[256];
    SQLSMALLINT TextLength;
    SQLRETURN ret;

    fprintf(stderr, "\nThe driver reported the following error %s\n", fn);
    fprintf(fp, "\nThe driver reported the following error %s\n", fn);
    do {
        ret = SQLGetDiagRec(type, handle, ++i, SQLState, &NativeError, MessageText, sizeof(MessageText), &TextLength);
        if (SQL_SUCCEEDED(ret)) {
            printf("%s:%ld:%ld:%s\n", SQLState, (long)i, (long)NativeError, MessageText);
            fprintf(fp, "%s:%ld:%ld:%s\n", SQLState, (long)i, (long)NativeError, MessageText);
        }
    } while (ret == SQL_SUCCESS);
}

bool check_and_print_null(const char * nameBuff, SQLLEN * ind, FILE *fp) {
    if (ind && *ind == SQL_NULL_DATA) {
        printf("%s: NULL\n", nameBuff);
        fprintf(fp, "%s: NULL\n", nameBuff);
        return true;
    }

    return false;
}

// Check for dsn availability in odbc.ini file
bool is_dsn_availability(SQLHENV env, const char * dsn) {
    SQLCHAR dsn_buff[1024];
    SQLLEN dsn_len;

    bool isDsnPresent = false;
    while (SQLDataSources(env, SQL_FETCH_NEXT, dsn_buff, 1024, &dsn_len, NULL, 0, NULL) == SQL_SUCCESS) {
        if (strcmp(dsn, dsn_buff) == 0) {
            isDsnPresent = true;
        }
    }

    return isDsnPresent;
}

// Check for availability of driver in odbcinst.ini file
bool is_driver_availability(SQLHENV env, const char * driver_name) {
    SQLCHAR buff[1024];
    SQLLEN len;

    bool is_driver_available = false;

    if (SQLGetInstalledDrivers(buff, 1024, &len)) {
        SQLCHAR driver[1024];
        int driver_idx = 0;
        int i = strlen(buff) + 1;
        for (; i < 1024; i++) {
            driver[driver_idx] = buff[i];
            driver_idx++;
            if (buff[i] == '\0') {
                if (strcmp(driver_name, driver) == 0) {
                    is_driver_available = true;
                }
                driver_idx = 0;
                if (i != 1023 && buff[i + 1] == '\0')
                    break;
                continue;
            }
        }
    }

    return is_driver_available;
}