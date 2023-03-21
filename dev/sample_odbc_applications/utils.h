#include <sql.h>
#include <sqlext.h>
#include <stdbool.h>

#define CHECK_ERROR(e, s, h, t)                               \
    ({                                                        \
        if (e != SQL_SUCCESS && e != SQL_SUCCESS_WITH_INFO) { \
            extract_error(s, h, t);                           \
        }                                                     \
    })

bool check_and_print_null(const char * nameBuff, SQLLEN * ind);
void extract_error(char * fn, SQLHANDLE handle, SQLSMALLINT type);
bool is_dsn_availability(SQLHENV env, const char * dsn);
bool is_driver_availability(SQLHENV env, const char * driver_name);