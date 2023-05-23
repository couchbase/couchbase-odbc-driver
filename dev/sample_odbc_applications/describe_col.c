#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

#define MAXFIELDS 20
#define MAXQUERY  100

#define TEXTSIZE  100 	    // How big the text fields are
#define MAXBATCHSIZE 20+2   // Maximum read/write in one go

#define TRUE 1

#define MAX_COLS 5
#define MAX_COL_NAME_LEN  256

const char * Statement = "select ?;";
SQLSMALLINT     NumParams, NumColumns;
SQLSMALLINT     i, DataType, DecimalDigits, Nullable, paramNo;
SQLULEN         bytesRemaining;

//
// Prompts user for input
//
void gStr (char * str, char * prompt, int max) {

    // Ask user for something to prepare
    printf("%s : ", prompt);

    // Get the name, with size limit.
    fgets (str, max, stdin);

    // remove \n
    str[strlen(str)-1] = '\0';
    return;
}

//
// Uses gStr() to prompt user for parameter value
//
void GetParamValue(SQLPOINTER BufferPtr, SQLINTEGER BufferLen,
                   SQLLEN *LenOrInd, SQLSMALLINT ParamNum, SQLSMALLINT DataType) {

    char ParamPrompt[]="Parameter %i";
    char Prompt[sizeof(ParamPrompt)+10];

    SQLINTEGER tmp;
    
    sprintf (Prompt, ParamPrompt, ParamNum);
    
    gStr (BufferPtr, Prompt, 100);

    printf("\n\n\nhello world\n\n\n");
    if (DataType != SQL_LONGVARCHAR) {
        *LenOrInd=strlen (BufferPtr);
    }
    return;
}

//
// Allocated buffer to either Column or Parameter data
//
void AllocBuffer(SQLUINTEGER buffSize, SQLPOINTER *Ptr, SQLLEN *BufferLen) {

    *Ptr=malloc (buffSize);
    memset (*Ptr, ' ', buffSize);
    if (BufferLen != NULL) {
        *BufferLen=buffSize;
    }
    return;
}

//
// Returns the parameter number associated with the paramId pointer returned
// by SQLParamData()
//
SQLSMALLINT findParam (SQLSMALLINT NumParams, PTR pParamId,
                       SQLPOINTER PtrArray[]) {

    int i;
    for (i=0; i<NumParams; i++) {
        if (PtrArray[i]==pParamId)
            return i;
    }
    return -1;
}


char* rtrim(char* string, char junk)
{
    char* original = string + strlen(string);
    while(*--original == junk);
        *(original + 1) = '\0';
    return string;
}

//
// Program to illustrate SQLDescribeParam() and SQLDescribeCol()
//
int main () {
    FILE *fp;
    fp = fopen("describe_col.output", "w"); // open file in write mode

    SQLHENV  henv  = SQL_NULL_HENV;   	// Environment
    SQLHDBC  hdbc  = SQL_NULL_HDBC;   	// Connection handle
    SQLHSTMT hstmt = SQL_NULL_HSTMT;  	// Statement handle
    SQLRETURN retcode;			// Return status

    SQLLEN      status;
    SQLSMALLINT statuslen;
    SQLPOINTER  ParamPtrArray[MAXFIELDS];
    SQLLEN      ParamBufferLenArray[MAXFIELDS];
    SQLLEN      ParamLenOrIndArray[MAXFIELDS];

    SQLPOINTER  ColPtrArray[MAXFIELDS];
    SQLPOINTER  ColNameArray[MAXFIELDS];
    SQLLEN      ColBufferLenArray[MAXFIELDS];
    SQLLEN      ColLenOrIndArray[MAXFIELDS];
    SQLSMALLINT ColDataTypeArray[MAXFIELDS];

    SQLCHAR *      ColumnName[MAX_COLS];
    SQLSMALLINT    ColumnNameLen[MAX_COLS];
    SQLSMALLINT    ColumnDataType[MAX_COLS];
    SQLULEN        ColumnDataSize[MAX_COLS];
    SQLSMALLINT    ColumnDataDigits[MAX_COLS];
    SQLSMALLINT    ColumnDataNullable[MAX_COLS];
    SQLCHAR *      ColumnData[MAX_COLS];
    SQLLEN         ColumnDataLen[MAX_COLS];
    // SQLSMALLINT    i,j;

    char *Str;
    PTR  pParamID;
    int i, j=0;

    SQLCHAR textBatch [MAXBATCHSIZE];

    SQLCHAR     ColName[255];
    SQLSMALLINT ColNameLen;
    SQLULEN     ColumnSize;
    SQLLEN      siText;

    // initialise pointer arrays so can be freed
    for (i=0;i<MAXFIELDS;i++) {
        ParamPtrArray[i]=NULL;
    }

    for (i=0;i<MAXFIELDS;i++) {
        ColPtrArray[i]=NULL;
    }

    // Prompt the user for an SQL statement and prepare it.
    // gStr (Statement, "SQL Statement", MAXQUERY);
    printf  ("Statement is : %s\n", Statement);

    // Allocate the ODBC environment and save handle.
    retcode = SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
                henv, SQL_HANDLE_ENV, fp);

    // Notify ODBC that this is an ODBC 3.0 app.
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                            (SQLPOINTER) SQL_OV_ODBC3, 0);
    CHECK_ERROR(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                henv, SQL_HANDLE_ENV, fp);

    // Allocate ODBC connection handle and connect.
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
                hdbc, SQL_HANDLE_DBC, fp);

    // Connect to the driver
    // retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR *)("DSN=Couchbase DSN (ANSI);"), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    // CHECK_ERROR(retcode, " SQLDriverConnect(\"DSN=Couchbase DSN (ANSI);\")", hdbc, SQL_HANDLE_DBC, fp);



    retcode = SQLConnect(hdbc, (SQLCHAR*) "Couchbase DSN (ANSI)", SQL_NTS, (SQLCHAR*) "admin", 5, (SQLCHAR*) "000000", 6);
    CHECK_ERROR(retcode, " SQLConnect", hdbc, SQL_HANDLE_DBC, fp);


    // Allocate statement handle.
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    CHECK_ERROR(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
                hstmt, SQL_HANDLE_STMT, fp);

    //
    // Set cursor type to DYNAMIC.
    // To get the full content of 'text' column, we use SQLFetch followed by
    // SQLGetData. For this to work however, (unless the text field is the
    // last column in the statement) we have to set the cursor to either
    // DYMANIC or STATIC because the cursor has to effectively 'go back' to
    // the field to get the data again after SQLFetch has retrieved the
    // first chunk and moved the cursor forward in the first read.
    // Note: If the cursor is left as FORWARD and the 'text' column is the
    // last column in the statement, it will work because the cursor still
    // moving forward and not advance to the next record.
    //
    retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE,
                             (SQLPOINTER) SQL_CURSOR_DYNAMIC, 0);

    printf ("Preparing : %s\n", Statement);

    retcode = SQLPrepare(hstmt, Statement, SQL_NTS);
    CHECK_ERROR(retcode, "SQLPrepare(Statement)",
                hstmt, SQL_HANDLE_STMT, fp);

    printf("SQLPrepare(hstmt, Statement, SQL_NTS) OK\n");

    // dumpDescriptors ("HSTMT", hstmt, 'Y', 'Y', 1);
    // Check to see if there are any parameters. If so, process them.
    SQLNumParams(hstmt, &NumParams);
    if (NumParams) {

        printf ("Num Params : %i\n", NumParams);
        for (i = 0; i < NumParams; i++) {
            // Describe the parameter.
            retcode = SQLDescribeParam(hstmt,
                                       i+1,
                                       &DataType,
                                       &bytesRemaining,
                                       &DecimalDigits,
                                       &Nullable);

            CHECK_ERROR(retcode, "SQLPrepare(Statement)",
                        hstmt, SQL_HANDLE_STMT, fp);

            printf("\nSQLDescribeParam() OK\n");
            printf("Data Type : %i, bytesRemaining : %i, DecimalDigits : %i, Nullable %i\n",
                                        (int)DataType, (int)bytesRemaining,
                                        (int)DecimalDigits, (int)Nullable);

            if (DataType==SQL_LONGVARCHAR) {
                AllocBuffer(MAXBATCHSIZE, &ParamPtrArray[i],
                                          &ParamBufferLenArray[i]);
            } else {
                AllocBuffer(bytesRemaining, &ParamPtrArray[i],
                                            &ParamBufferLenArray[i]);
            }

            printf ("Param Buffer Ptr : %p\n",
                                        (SQLPOINTER *) ParamPtrArray[i]);
            printf ("Param Buffer Len : %i\n",
                                        (int)ParamBufferLenArray[i]);

            // Text field
            // 100 bytes overall total
            // indicates data for the parameter will be sent with SQLPutData
            if (DataType==SQL_LONGVARCHAR)  {
                bytesRemaining=(SDWORD) TEXTSIZE;
                ParamLenOrIndArray[i]=SQL_LEN_DATA_AT_EXEC(bytesRemaining);

                printf ("Binding MEMO field - ");
                printf ("Bytes Remaining : %i, ", (int)bytesRemaining);
                printf ("ParamLenOrIndArray : %i\n", (int)ParamLenOrIndArray[i]);

                retcode = SQLBindParameter(hstmt,           // Statment Handle
                                i+1,                        // Parameter Number
                                SQL_PARAM_INPUT,            // Type is INPUT
                                SQL_C_CHAR,                 // C Data Type
                                SQL_LONGVARCHAR,            // SQL Data Type
                                bytesRemaining,             // Parameter size big
                                0,                          // Decimal Digits
                                ParamPtrArray[i],           // Param value Pointer
                                0,                          // Buffer Length
                                &ParamLenOrIndArray[i]);// Len or Indicator

                // data for this column is grabbed when asked for
            } else {
                // Bind the memory to the parameter. Assume that we only have input parameters.
                retcode = SQLBindParameter(hstmt,
                                i+1,
                                SQL_PARAM_INPUT,
                                SQL_C_CHAR,
                                SQL_CHAR,
                                bytesRemaining,
                                DecimalDigits,
                                ParamPtrArray[i],
                                ParamBufferLenArray[i],
                                &ParamLenOrIndArray[i]);

                // Can get the data for this column immediately
                GetParamValue(ParamPtrArray[i], ParamBufferLenArray[i],
                                    &ParamLenOrIndArray[i], i+1, DataType);
            }

            CHECK_ERROR(retcode, "SQLBindParameter(SQL_PARAM_INPUT)",
                        hstmt, SQL_HANDLE_STMT, fp);
            printf("SQLBindParameter() OK\n");
        }
    } else {
        printf ("No Params\n");
    }

    // Check to see if there are any Columns. If so, get their details
    SQLNumResultCols(hstmt, &NumColumns);

    printf ("Number of Result Columns %i\n", NumColumns);

    // Loop round number of columns using SQLDescribeCol to get info about
    // the column, followed by SQLBindCol to bind the column to a data area
    for (i=0;i<NumColumns;i++) {
        ColumnName[i] = (SQLCHAR *) malloc (MAX_COL_NAME_LEN);
        retcode = SQLDescribeCol (
                    hstmt,                    // Select Statement (Prepared)
                    i+1,                      // Columnn Number
                    ColumnName[i],            // Column Name (returned)
                    MAX_COL_NAME_LEN,         // size of Column Name buffer
                    &ColumnNameLen[i],        // Actual size of column name
                    &ColumnDataType[i],       // SQL Data type of column
                    &ColumnDataSize[i],       // Data size of column in table
                    &ColumnDataDigits[i],     // Number of decimal digits
                    &ColumnDataNullable[i]);  // Whether column nullable

        CHECK_ERROR(retcode, "SQLDescribeCol()", hstmt, SQL_HANDLE_STMT, fp);

        // Display column data
        printf("\nColumn : %i\n", i+1);
        printf("Column Name : %s\n  Column Name Len : %i\n  SQL Data Type : %i\n  Data Size : %i\n  DecimalDigits : %i\n  Nullable %i\n",
                 ColumnName[i], (int)ColumnNameLen[i], (int)ColumnDataType[i],
                 (int)ColumnDataSize[i], (int)ColumnDataDigits[i],
                 (int)ColumnDataNullable[i]);

        // Bind column, changing SQL data type to C data type
        // (assumes INT and VARCHAR for now)
        ColumnData[i] = (SQLCHAR *) malloc ((int)ColumnDataSize[i]+1);
        switch (ColumnDataType[i]) {
            case SQL_INTEGER:
                ColumnDataType[i]=SQL_C_LONG;
                break;
            case SQL_VARCHAR:
                ColumnDataType[i]=SQL_C_CHAR;
                break;
            case SQL_DOUBLE:
                ColumnDataType[i]=SQL_C_DOUBLE;
                break;
            case SQL_BIGINT:
                ColumnDataType[i]=SQL_C_SBIGINT;
                break;
            case SQL_BIT:
                ColumnDataType[i]=SQL_C_BIT;
                break;

                

        }

        retcode = SQLBindCol (hstmt,                  // Statement handle
                              i+1 ,                    // Column number
                              ColumnDataType[i],      // C Data Type
                              ColumnData[i],          // Data buffer
                              ColumnDataSize[i],      // Size of Data Buffer
                              &ColumnDataLen[i]); // Size of data returned

        CHECK_ERROR(retcode, "SQLBindCol()", hstmt, SQL_HANDLE_STMT, fp);
    }

    // Fetch records
    printf ("\nRecords ...\n\n");
    retcode = SQLExecute (hstmt);
    CHECK_ERROR(retcode, "SQLExecute()", hstmt, SQL_HANDLE_STMT, fp);

    printf ("\n  Data Records\n  ------------\n");
    for (i=0; ; i++) {
        retcode = SQLFetch(hstmt);

        //No more data?
        if (retcode == SQL_NO_DATA) {
            break;
        }

        CHECK_ERROR(retcode, "SQLFetch()", hstmt, SQL_HANDLE_STMT, fp);

        //Display it
        printf ("\nRecord %i \n", i+1);
        for (j=0;j<NumColumns;j++) {
            printf("Column %s : ", ColumnName[j]);
            switch (ColumnDataType[j]) {
                case SQL_C_SBIGINT:
                    printf(" %i\n", (int) *ColumnData[j]);
                    break;
                case SQL_C_DOUBLE:
                    printf(" %d\n", (int) *ColumnData[j]);
                    break;
                case SQL_C_BIT:
                    if (!check_and_print_null("", &ColumnDataLen[i], fp)) {
                        char * boolVal = *ColumnData[j] == 0 ? "false" : "true";
                        printf("%s\n", boolVal);
                    }
                    break;
                default:
                    printf(" %s\n", rtrim(ColumnData[j], ' '));
                    break;
            }
        }
    }

exit:

    // Free the memory allocated for each parameter and the memory allocated
    // for the arrays of pointers, buffer lengths, and length/indicator values.

    for (i = 0; i < NumParams; i++) {
        if (ParamPtrArray[i]!=NULL) free(ParamPtrArray[i]);
    }

    for (i = 0; i < NumColumns; i++) {
        if (ColPtrArray[i]!=NULL) free(ColPtrArray[i]);
    }

    printf ("\nComplete.\n");
    fclose(fp);

    // Free handles
    // Statement
    if (hstmt != SQL_NULL_HSTMT)
        SQLFreeStmt(SQL_HANDLE_STMT, hstmt);

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