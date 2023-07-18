#pragma once

#include "driver/platform/platform.h"

#define MAX_DSN_KEY_LEN   256
#define MAX_DSN_VALUE_LEN 10240

#define INI_DRIVER          "Driver"
#define INI_FILEDSN         "FileDSN"
#define INI_SAVEFILE        "SaveFile"
#define INI_DSN             "DSN"
#define INI_DESC            "Description"     /* Data source description */
#define INI_URL             "Url"             /* Full url of server running the Couchbase service */
#define INI_UID             "UID"             /* Default User Name */
#define INI_USERNAME        "Username"
#define INI_PWD             "PWD"             /* Default Password */
#define INI_PASSWORD        "Password"
#define INI_PROTO           "Proto"           /* HTTP vs HTTPS */
#define INI_SERVER          "Server"          /* Name of Server running the Couchbase service */
#define INI_HOST            "Host"
#define INI_PORT            "Port"            /* Port on which the Couchbase is listening */
#define INI_TIMEOUT         "Timeout"         /* Connection timeout */
#define INI_VERIFY_CONNECTION_EARLY "VerifyConnectionEarly"
#define INI_SSLMODE         "SSLMode"         /* Use 'require' for https connections */
#define INI_PRIVATEKEYFILE  "PrivateKeyFile"
#define INI_CERTIFICATEFILE "CertificateFile"
#define INI_CALOCATION      "CALocation"
#define INI_PATH            "Path"            /* Path portion of the URL */
#define INI_BUCKET        "Bucket"        /* Bucket Name */
#define INI_SOURCE_ID "SourceID"              /* "cb" or "ch" */
#define INI_LOGIN_TIMEOUT "LoginTimeout"
#define INI_QUERY_TIMEOUT "QueryTimeout"
#define INI_HUGE_INT_AS_STRING "HugeIntAsString"
#define INI_STRINGMAXLENGTH "StringMaxLength"
#define INI_DRIVERLOG "DriverLog"
#define INI_DRIVERLOGFILE   "DriverLogFile"

#if defined(UNICODE)
#   define INI_DSN_DEFAULT          DSN_DEFAULT_UNICODE
#else
#   define INI_DSN_DEFAULT          DSN_DEFAULT_ANSI
#endif

#define INI_DESC_DEFAULT            ""
#define INI_URL_DEFAULT             ""
#define INI_USERNAME_DEFAULT        ""
#define INI_PASSWORD_DEFAULT        ""
#define INI_SERVER_DEFAULT          ""
#define INI_PORT_DEFAULT            ""
#define INI_TIMEOUT_DEFAULT         "30"
#define INI_VERIFY_CONNECTION_EARLY_DEFAULT "off"
#define INI_SSLMODE_DEFAULT         ""
#define INI_BUCKET_DEFAULT        ""
#define INI_SOURCE_ID_DEFAULT "cb"
#define INI_LOGIN_TIMEOUT_DEFAULT "5"  // = LCB_DEFAULT_CONFIGURATION_TIMEOUT
#define INI_QUERY_TIMEOUT_DEFAULT "75" // = LCB_DEFAULT_ANALYTICS_TIMEOUT
#define INI_HUGE_INT_AS_STRING_DEFAULT "off"
#define INI_STRINGMAXLENGTH_DEFAULT "1048575"

#ifdef NDEBUG
#    define INI_DRIVERLOG_DEFAULT "off"
#else
#    define INI_DRIVERLOG_DEFAULT "on"
#endif

#ifdef _win_
#    define INI_DRIVERLOGFILE_DEFAULT "\\temp\\couchbase-odbc-driver.log"
#else
#    define INI_DRIVERLOGFILE_DEFAULT "/tmp/couchbase-odbc-driver.log"
#endif

#ifdef _win_
#    define ODBC_INI "ODBC.INI"
#    define ODBCINST_INI "ODBCINST.INI"
#else
#    define ODBC_INI ".odbc.ini"
#    define ODBCINST_INI "odbcinst.ini"
#endif
