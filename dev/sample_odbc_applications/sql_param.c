// TODO
#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>

#include "utils.h"

#define MAXFIELDS 20
#define MAXQUERY  100

#define TEXTSIZE  100 	    // How big the text fields are
#define MAX_BATCH_SIZE 20+2   // Maximum read/write in one go

#define TRUE 1



const char* statement = "select ?;";
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
    gStr (BufferPtr, Prompt, BufferLen);
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


int main () {
    FILE *fp;
    fp = fopen("sql_desc_rec.output", "w"); // open file in write mode

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

    char *Str;
    PTR  pParamID;
    int i, j=0;

    SQLCHAR textBatch [MAX_BATCH_SIZE];

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
    printf  ("Statement is : %s\n", statement);

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

    // Connect to DSN
    retcode = SQLDriverConnect(hdbc, NULL, "DSN=Couchbase DSN (ANSI);", SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    CHECK_ERROR(retcode, " SQLDriverConnect(\"DSN=Couchbase DSN (ANSI);\")", hdbc, SQL_HANDLE_DBC, fp);

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

    printf ("Preparing : %s\n", statement);

    retcode = SQLPrepare(hstmt, statement, SQL_NTS);
    CHECK_ERROR(retcode, "SQLPrepare(statement)",
                hstmt, SQL_HANDLE_STMT, fp);

    printf("SQLPrepare(hstmt, statement, SQL_NTS) OK\n");

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

            CHECK_ERROR(retcode, "SQLPrepare(statement)",
                        hstmt, SQL_HANDLE_STMT, fp);

            printf("\nSQLDescribeParam() OK\n");
            printf("Data Type : %i, bytesRemaining : %i, DecimalDigits : %i, Nullable %i\n",
                                        (int)DataType, (int)bytesRemaining,
                                        (int)DecimalDigits, (int)Nullable);

            if (DataType==SQL_LONGVARCHAR) {
                AllocBuffer(MAX_BATCH_SIZE, &ParamPtrArray[i],
                                          &ParamBufferLenArray[i]);
            } else {
                AllocBuffer(MAX_BATCH_SIZE, &ParamPtrArray[i],
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
                                DataType,
                                MAX_BATCH_SIZE,
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
    if (NumColumns) {

        printf ("Num Columns : %i\n", NumColumns);
        for (i = 0; i < NumColumns; i++) {
            // Describe the parameter.
            retcode = SQLDescribeCol(hstmt,
                                     i+1,
                                     ColName, 255,
                                     &ColNameLen,
                                     &DataType,
                                     &ColumnSize,
                                     &DecimalDigits,
                                     &Nullable);

            CHECK_ERROR(retcode, "SQLDescribeCol()",
                        hstmt, SQL_HANDLE_STMT, fp);

            printf("\nSQLDescribeCol () OK\n");
            printf("Data Type : %i, ColName : %s, DecimalDigits : %i, Nullable %i\n",
                   (int)DataType, ColName, (int)DecimalDigits, (int)Nullable);

            AllocBuffer (ColNameLen+1, &ColNameArray[i], NULL);
            strcpy (ColNameArray[i], ColName);
            ColDataTypeArray[i]=DataType;

            if (DataType==SQL_LONGVARCHAR) {
                AllocBuffer(MAX_BATCH_SIZE, &ColPtrArray[i],
                                                &ColBufferLenArray[i]);
            } else {
                AllocBuffer(ColumnSize, &ColPtrArray[i],
                                                &ColBufferLenArray[i]);
            }

            printf ("Col Buffer Ptr : %p\n",
                                            (SQLPOINTER *) ColPtrArray[i]);
            printf ("Col Buffer Len : %i\n",
                                            (int)ColBufferLenArray[i]);

            if (DataType==SQL_LONGVARCHAR)  {
                //
                // For LONGVARCHAR Columns, we get the first 'Buffer Length'
                // or chunk of data in the first SQLFetch(). For the rest, we
                // can use SQLGetData which will return the whole column (albeit
                // in 'Buffer Length' chunks). This means by using SQLFetch
                // followed by SQLGetData we actually get the first chuck
                // twice. So, we can either NOT BIND, and SQLFetch wont give
                // us the first chunk or BIND and ignore the first chunk when
                // we use SQLGetData.
                //
                retcode = SQLBindCol (
                                hstmt,                     // Statment Handle
                                i+1,                       // Column Number
                                SQL_C_CHAR,                // C Type
                                ColPtrArray[i],            // Column value Pointer
                                ColBufferLenArray[i],      // Buffer Length
                                &ColLenOrIndArray[i]); // Len or Indicator

            } else {
                // Bind the memory to the parameter.
                retcode = SQLBindCol (
                                hstmt,
                                i+1,
                                SQL_C_CHAR,
                                ColPtrArray[i],
                                ColBufferLenArray[i],
                                &ColLenOrIndArray[i]);
            }

            CHECK_ERROR(retcode, "SQLBindCol()",
                                                    hstmt, SQL_HANDLE_STMT, fp);
            printf("SQLBindCol() OK\n");
        }
    } else {
        printf ("No Columns\n");
    }

    // Execute the statement.
    // If SQLExecute encounters a data-at-execution parameter,
    // it returns SQL_NEED_DATA. The application sends the data using
    // SQLParamData and SQLPutData.

    retcode = SQLExecute (hstmt);
    if ( (retcode!=SQL_NEED_DATA) &&
         (retcode!=SQL_SUCCESS) &&
         (retcode!=SQL_SUCCESS_WITH_INFO) )   {
        CHECK_ERROR(retcode, "SQLExecute()", hstmt, SQL_HANDLE_STMT, fp);
    }

    //
    // Select/Delete or Insert?
    //
    // If no columns assume non-select statement
    if (NumColumns == 0) {

        // Get parameter needing more data
        retcode = SQLParamData(hstmt, &pParamID);
        while (retcode==SQL_NEED_DATA) {

            // Find which parameter
            paramNo = findParam (NumParams, pParamID, ParamPtrArray);
            if (paramNo==-1) {
                printf ("\nParam for more data not found\n");
                goto exit;
            }

            while ((int)bytesRemaining > 0) {
                printf ("Bytes still to write %i\n",
                                            (int) bytesRemaining);
                memset (ParamPtrArray[paramNo], ' ',
                                            ParamBufferLenArray[paramNo]);
                GetParamValue(ParamPtrArray[paramNo],
                              ParamBufferLenArray[paramNo],
                              NULL, paramNo+1, SQL_LONGVARCHAR);
                printf ("Param %i, next %i bytes\n", paramNo+1,
                                        (int)strlen(ParamPtrArray[paramNo]));
                retcode = SQLPutData(hstmt,
                                    (UCHAR *)ParamPtrArray[paramNo],
                                    (SQLLEN) strlen(ParamPtrArray[paramNo]));
                CHECK_ERROR(retcode, "SQLPutData()",
                            hstmt, SQL_HANDLE_STMT, fp);
                bytesRemaining-=strlen(ParamPtrArray[paramNo]);
            }

            bytesRemaining=(SDWORD) TEXTSIZE;

            // Make final SQLParamData call.
            printf ("Final Call - SQLParamData\n");
            retcode = SQLParamData(hstmt, &pParamID);
            if ( (retcode!=SQL_NEED_DATA) &&
                 (retcode!=SQL_SUCCESS) &&
                 (retcode!=SQL_SUCCESS_WITH_INFO) )   {
                CHECK_ERROR(retcode, "SQLParamData()",
                                                hstmt, SQL_HANDLE_STMT, fp);
            }
            printf ("Code = %i (%i)\n", (int)retcode, SQL_NEED_DATA);
        }
    } else {
        // initialise buffers
        for (i=0;i<NumColumns;i++) {
            memset( ColPtrArray[i], ' ', ColBufferLenArray[i]);
            strcpy (ColPtrArray[i], "");
        }

        while ( (retcode = SQLFetch(hstmt)) != SQL_NO_DATA ) {
            CHECK_ERROR(retcode, "SQLFetch()", hstmt, SQL_HANDLE_STMT, fp);
            // Loop round each column, output based on TYPE
            printf ("\n\nRecord %i", ++j);
            for (i=0;i<NumColumns;i++) {
                printf ("\nColumn : %i",   i+1);
                printf ("\nName   : %s",   (char *) ColNameArray[i]);
                printf ("\nType   : %i",   (int) ColDataTypeArray[i]);

                // As we are BINDING LONGVARCHAR columns in this example, we
                // ignore the chunk of data returned by the SQLFetch. We get
                // the whole column regardless.
                if (ColDataTypeArray[i] == SQL_LONGVARCHAR) {
                    printf ("\nValue  : ");
                    while ((retcode = SQLGetData(hstmt,
                                                 i+1,
                                                 SQL_CHAR,
                                                 ColPtrArray[i],
                                                 ColBufferLenArray[i],
                                                 &ColLenOrIndArray[i]))
                                                 != SQL_NO_DATA) {
                        CHECK_ERROR(retcode, "SQLGetData()",
                                    hstmt, SQL_HANDLE_STMT, fp);

                        printf ("\n                  %s",
                                                    (char *) ColPtrArray[i]);
                    }
                } else {
                    printf ("\nValue  : %s",
                                                    (char *) ColPtrArray[i]);
                }
            }
            // re-initialise buffers
            for (i=0;i<NumColumns;i++) {
                memset( ColPtrArray[i], ' ', ColBufferLenArray[i]);
                strcpy (ColPtrArray[i], "");
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
