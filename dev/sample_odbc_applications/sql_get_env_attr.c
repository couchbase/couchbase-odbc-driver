#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

main () {
    FILE *fp;
    fp = fopen("sql_get_env_attr.output", "w"); // open file in write mode

    // Environment handle
    SQLHENV  	henv  = SQL_NULL_HENV;

    // For SQL_ATTR_CONNECTION_POOLING, SQL_ATTR_CP_MATCH
    SQLUINTEGER	uIntVal;

    // For SQL_ATTR_ODBC_VERSION, SQL_ATTR_OUTPUT_NTS
    SQLINTEGER  sIntVal;

    SQLRETURN retcode;				   // Return status


    // Allocate an environment handle
    retcode=SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
                henv, SQL_HANDLE_ENV, fp);

    // Notify ODBC that this is an ODBC 3.0 app
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                                        (SQLPOINTER) SQL_OV_ODBC3, 0);
    CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_HANDLE_ENV)",
                henv, SQL_HANDLE_ENV, fp);

    printf ("\nEnvironment Attributes : \n");
    fprintf (fp, "\nEnvironment Attributes : \n");

    // Get Env attributes
    retcode = SQLGetEnvAttr (henv, SQL_ATTR_CONNECTION_POOLING,
                             (SQLPOINTER) &uIntVal,
                             (SQLINTEGER) sizeof(uIntVal), NULL);

    CHECK_ERROR(retcode, "SQLGetEnvAttr(SQL_ATTR_CONNECTION_POOLING)",
                henv, SQL_HANDLE_ENV, fp);

    printf ("\nSQL_ATTR_CONNECTION_POOLING : ");
    fprintf (fp, "\nSQL_ATTR_CONNECTION_POOLING : ");
    if (uIntVal==SQL_CP_OFF) {
        printf ("SQL_CP_OFF");
        fprintf (fp, "SQL_CP_OFF");
    }
    if (uIntVal==SQL_CP_ONE_PER_DRIVER) {
        printf ("SQL_CP_ONE_PER_DRIVER");
        fprintf (fp, "SQL_CP_ONE_PER_DRIVER");
    }
    if (uIntVal==SQL_CP_ONE_PER_HENV) {
        printf ("SQL_CP_ONE_PER_HENV");
        fprintf (fp, "SQL_CP_ONE_PER_HENV");
    }

    // Caution: SQL_CP_DRIVER_AWARE may be undefined
    // if (uIntVal==SQL_CP_DRIVER_AWARE)
    // printf ("SQL_CP_DRIVER_AWARE");

    retcode = SQLGetEnvAttr (henv, SQL_ATTR_CP_MATCH,
                             (SQLPOINTER) &uIntVal,
                             (SQLINTEGER) sizeof(uIntVal), NULL);
    CHECK_ERROR(retcode, "SQLGetEnvAttr(SQL_ATTR_CP_MATCH)",
                henv, SQL_HANDLE_ENV, fp);

    printf ("\nSQL_ATTR_CP_MATCH           : ");
    fprintf (fp, "\nSQL_ATTR_CP_MATCH           : ");
    if (uIntVal==SQL_CP_STRICT_MATCH) {
        printf ("SQL_CP_STRICT_MATCH");
        fprintf (fp, "SQL_CP_STRICT_MATCH");
    }
    if (uIntVal==SQL_CP_RELAXED_MATCH) {
        printf ("SQL_CP_RELAXED_MATCH");
        fprintf (fp, "SQL_CP_RELAXED_MATCH");
    }

    retcode = SQLGetEnvAttr (henv, SQL_ATTR_ODBC_VERSION,
                             (SQLPOINTER) &sIntVal,
                             (SQLINTEGER) sizeof(sIntVal), NULL);
    CHECK_ERROR(retcode, "SQLGetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    printf ("\nSQL_ATTR_ODBC_VERSION       : ");
    fprintf (fp, "\nSQL_ATTR_ODBC_VERSION       : ");
    if (sIntVal==SQL_OV_ODBC3_80) {
        printf ("SQL_OV_ODBC3_80");
        fprintf (fp, "SQL_OV_ODBC3_80");
    }
    if (sIntVal==SQL_OV_ODBC3) {
        printf ("SQL_OV_ODBC3");
        fprintf (fp, "SQL_OV_ODBC3");
    }
    if (sIntVal==SQL_OV_ODBC2) {
        printf ("SQL_OV_ODBC2");
        fprintf (fp, "SQL_OV_ODBC2");
    }

    retcode = SQLGetEnvAttr (henv, SQL_ATTR_OUTPUT_NTS,
                             (SQLPOINTER) &sIntVal,
                             (SQLINTEGER) sizeof(sIntVal), NULL);
    CHECK_ERROR(retcode, "SQLGetEnvAttr(SQL_ATTR_OUTPUT_NTS)",
                henv, SQL_HANDLE_ENV, fp);
    printf ("\nSQL_ATTR_OUTPUT_NTS         : ");
    fprintf (fp, "\nSQL_ATTR_OUTPUT_NTS         : ");
    if (sIntVal==SQL_TRUE) {
        printf ("SQL_TRUE");
        fprintf (fp, "SQL_TRUE");
    }
    if (sIntVal==SQL_FALSE) {
        printf ("SQL_FALSE");
        fprintf (fp, "SQL_FALSE");
    }

exit:

    printf ("\nComplete.\n");
    fprintf (fp, "\nComplete.\n");


    fclose(fp);

    // Free Environment
    if (henv != SQL_NULL_HENV)
        SQLFreeHandle(SQL_HANDLE_ENV, henv);

    return 0;
}
