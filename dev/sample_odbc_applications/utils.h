#include <sql.h>
#include <sqlext.h>
#include <stdbool.h>
#include <stdio.h>

#define CHECK_ERROR(e, s, h, t, fp)                           \
    ({                                                        \
        if (e != SQL_SUCCESS && e != SQL_SUCCESS_WITH_INFO) { \
            extract_error(s, h, t, fp);                       \
            goto exit;                                        \
        }                                                     \
    })

bool check_and_print_null(const char * nameBuff, SQLLEN * ind, FILE *fp);
void extract_error(char * fn, SQLHANDLE handle, SQLSMALLINT type, FILE *fp);
bool is_dsn_availability(SQLHENV env, const char * dsn);
bool is_driver_availability(SQLHENV env, const char * driver_name);

