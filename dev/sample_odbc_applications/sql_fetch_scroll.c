#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

#define ROWS 5
#define MODEL_LEN 21

int main(int argc, char * argv[]) {
    FILE *fp;
    fp = fopen("sql_fetch_scroll.output", "w"); // open file in write mode
    
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;       /* ODBC API return status */
    SQLSMALLINT columns; /* number of columns in result-set */


    SQLCHAR        model[ROWS][MODEL_LEN]; /* Record set */
    SQLINTEGER     orind[ROWS];            /* Len or status ind */
    SQLUSMALLINT   rowStatus[ROWS];        /* Status of each row */
    SQLUINTEGER    numRowsFetched;         /* Number of rows fetched */
    int i;                                 /* Loop counter */
    RETCODE        rc=SQL_SUCCESS;         /* Status return code */


    /* Allocate an environment handle */
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    /* We want ODBC 3 support */
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
    /* Allocate a connection handle */
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    SQLDriverConnect(dbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);

    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    SQLSetStmtAttr( stmt, SQL_ATTR_ROW_BIND_TYPE,
    SQL_BIND_BY_COLUMN, 0 );
    /*
    ** Declare that the record set has five rows.
    */
    SQLSetStmtAttr( stmt, SQL_ATTR_ROW_ARRAY_SIZE,
        (SQLPOINTER)ROWS, 0 );
    /*
    ** Bind an array of status pointers to report on the status of
    ** each row fetched.
    */
    SQLSetStmtAttr( stmt, SQL_ATTR_ROW_STATUS_PTR,
        (SQLPOINTER)rowStatus, 0 );
    /*
    ** Bind an integer that reports the number of rows fetched.
    */
    SQLSetStmtAttr( stmt, SQL_ATTR_ROWS_FETCHED_PTR,
        (SQLPOINTER)&numRowsFetched, 0 );
    
    SQLBindCol( stmt,    /* Statement handle */
        1,                /* Column number */
        SQL_C_CHAR,       /* Bind to a C string */
        model,            /* The data to be fetched */
        MODEL_LEN,        /* Maximum length of the data */
        orind );   

    const char * queryTxt = "SELECT 'hello';";

    SQLExecDirect(stmt, (SQLCHAR *)queryTxt, strlen(queryTxt));

    while ( true )
    {
        rc = SQLFetchScroll( stmt, SQL_FETCH_NEXT, 0 );
        /*
        ** Break out of the loop at end of data.
        */
        if (rc == SQL_NO_DATA_FOUND)
        {
            printf("End of record set\n" );
            fprintf(fp, "End of record set\n" );
            break;
        }
        /*
        ** Break out of the loop if an error is found.
        */
        if ( !SQL_SUCCEEDED( rc ) )
        {
            printf( "Error on SQLFetchScroll(), status is %d\n", rc );
            fprintf(fp, "Error on SQLFetchScroll(), status is %d\n", rc );
            break;
        }
        /*
        ** Display the result set.
        */
        for (i = 0; i < numRowsFetched; i++)
        {
            printf("Model: %s\n", model[i]);
            fprintf(fp, "Model: %s\n", model[i]);
        }
    } /* end while */

    fclose(fp);
}