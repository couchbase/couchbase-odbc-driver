// file to store all the constant queries string

const char query_primary_keys[] = "SELECT TABLE_CAT, "
        "       TABLE_SCHEM, "
        "       TABLE_NAME, "
        "       primaryKey[0] AS COLUMN_NAME, "
        "       KEY_SEQ, "
        "       NULL PK_NAME "
        "FROM Metadata.`Dataset` ds "
        "  JOIN Metadata.`Datatype` dt ON ds.DatatypeDataverseName = dt.DataverseName "
        "  AND ds.DatatypeName = dt.DatatypeName "
        "UNNEST ds.ViewDetails.PrimaryKey as primaryKey at p "
        "LET dvname = decode_dataverse_name(ds.DataverseName), "
        "    TABLE_CAT = dvname[0], "
        "    TABLE_SCHEM = CASE ARRAY_LENGTH(dvname) WHEN 1 THEN NULL ELSE dvname[1] END, "
        "    TABLE_NAME = ds.DatasetName, "
        "    KEY_SEQ = p "
        "WHERE ";

const char query_procedure_columns[] = "SELECT "
        "    PROCEDURE_CAT, "
        "    PROCEDURE_SCHEM, "
        "    PROCEDURE_NAME, "
        "    COLUMN_NAME, "
        "    COLUMN_TYPE, "
        "    DATA_TYPE, "
        "    TYPE_NAME, "
        "    NULL COLUMN_SIZE, "
        "    NULL BUFFER_LENGTH, "
        "    NULL DECIMAL_DIGITS, "
        "    NULL NUM_PREC_RADIX, "
        "    NULLABLE, "
        "    REMARKS, "
        "    NULL COLUMN_DEF, "
        "    SQL_DATA_TYPE, "
        "    NULL SQL_DATETIME_SUB, "
        "    NULL CHAR_OCTET_LENGTH, "
        "    ORDINAL_POSITION, "
        "    IS_NULLABLE "
        "FROM Metadata.`Function` AS fn "
        "UNNEST fn.Params as COLUMN_NAME at pos "
        "LET fnname = decode_dataverse_name(fn.DataverseName), "
        "    PROCEDURE_CAT = fnname[0], "
        "    PROCEDURE_SCHEM = CASE ARRAY_LENGTH(fnname) WHEN 1 THEN NULL ELSE fnname[1] END, "
        "    PROCEDURE_NAME = fn.Name, "
        "    COLUMN_TYPE = ";


const char query_procedures[] = "SELECT PROCEDURE_CAT, "
        "       PROCEDURE_SCHEM, "
        "       PROCEDURE_NAME, "
        "       NULL NUM_INPUT_PARAMS, "
        "       NULL NUM_OUTPUT_PARAMS, "
        "       NULL NUM_RESULT_SETS, "
        "       REMARKS, "
        "       PROCEDURE_TYPE "
        "FROM Metadata.`Function` AS fn "
        "LET fnname = decode_dataverse_name(fn.DataverseName), "
        "    PROCEDURE_CAT = fnname[0], "
        "    PROCEDURE_SCHEM = CASE ARRAY_LENGTH(fnname) WHEN 1 THEN NULL ELSE fnname[1] END, "
        "    PROCEDURE_NAME = fn.Name, "
        "    REMARKS = 'Default Remarks', "
        "    PROCEDURE_TYPE = 2 "
        "WHERE" ;


const char query_foreign_keys_fk_with_pk[] = "SELECT subquery.* "
            "FROM Metadata.`Dataset` ds1 "
            "  JOIN Metadata.`Datatype` dt1 ON ds1.DatatypeDataverseName = dt1.DataverseName "
            "  AND ds1.DatatypeName = dt1.DatatypeName "
            "UNNEST ds1.ViewDetails.ForeignKeys AS ForeignKeys "
            "UNNEST ForeignKeys.ForeignKey AS ForeignKey AT p "
            "UNNEST ( "
            "  FROM Metadata.`Dataset` ds2 "
            "    JOIN Metadata.`Datatype` dt2 ON ds2.DatatypeDataverseName = dt2.DataverseName "
            "    AND ds2.DatatypeName = dt2.DatatypeName "
            "  UNNEST ds2.ViewDetails.PrimaryKey AS primaryKey "
            "  LET dvname1 = decode_dataverse_name(ds1.DataverseName), "
            "      dvname2 = decode_dataverse_name(ds2.DataverseName), "
            "      FKTABLE_CAT = dvname1[0], "
            "      FKTABLE_SCHEM = CASE ARRAY_LENGTH(dvname1) WHEN 1 THEN NULL ELSE dvname1[1] END, "
            "      FKTABLE_NAME = ds1.DatasetName, "
            "      fkTable_fk = ForeignKey[0], "
            "      fk_dvname = decode_dataverse_name(ForeignKeys.RefDataverseName), "
            "      fkTable_pk_CAT = fk_dvname[0], "
            "      fkTable_pk_SCHEM = CASE ARRAY_LENGTH(fk_dvname) WHEN 1 THEN NULL ELSE fk_dvname[1] END, "
            "      fkTable_pk_TABLE = ForeignKeys.RefDatasetName, "
            "      PKTABLE_CAT = dvname2[0], "
            "      PKTABLE_SCHEM = CASE ARRAY_LENGTH(dvname2) WHEN 1 THEN NULL ELSE dvname2[1] END, "
            "      PKTABLE_NAME = ds2.DatasetName, "
            "      pkTable_pk = primaryKey[0], "
            "    UPDATE_RULE =  ";


const char query_foreign_keys_pk[] = "SELECT subquery.* "
            "FROM Metadata.`Dataset` ds1 "
            "  JOIN Metadata.`Datatype` dt1 ON ds1.DatatypeDataverseName = dt1.DataverseName "
            "  AND ds1.DatatypeName = dt1.DatatypeName "
            "UNNEST ds1.ViewDetails.PrimaryKey AS primaryKey "
            "UNNEST ( "
            "  FROM Metadata.`Dataset` ds2 "
            "    JOIN Metadata.`Datatype` dt2 ON ds2.DatatypeDataverseName = dt2.DataverseName "
            "    AND ds2.DatatypeName = dt2.DatatypeName "
            "  UNNEST ds2.ViewDetails.ForeignKeys AS ForeignKeys "
            "  UNNEST ForeignKeys.ForeignKey AS ForeignKey AT p "
            "  LET dvname2 = decode_dataverse_name(ds2.DataverseName), "
            "      FKTABLE_CAT = dvname2[0], "
            "      FKTABLE_SCHEM = CASE ARRAY_LENGTH(dvname2) WHEN 1 THEN NULL ELSE dvname2[1] END, "
            "      FKTABLE_NAME = ds2.DatasetName, "
            "      fkTable_fk = ForeignKey[0], "
            "      fk_dvname = decode_dataverse_name(ForeignKeys.RefDataverseName), "
            "      fkTable_pk_CAT = fk_dvname[0], "
            "      fkTable_pk_SCHEM = CASE ARRAY_LENGTH(fk_dvname) WHEN 1 THEN NULL ELSE fk_dvname[1] END, "
            "      fkTable_pk_TABLE = ForeignKeys.RefDatasetName, "
            "      dvname1 = decode_dataverse_name(ds1.DataverseName), "
            "      PKTABLE_CAT = dvname1[0], "
            "      PKTABLE_SCHEM = CASE ARRAY_LENGTH(dvname1) WHEN 1 THEN NULL ELSE dvname1[1] END, "
            "      PKTABLE_NAME = ds1.DatasetName, "
            "      pkTable_pk = primaryKey[0], "
            "    UPDATE_RULE = ";


const char query_foreign_keys_fk[] ="SELECT subquery.*  "
            "FROM Metadata.`Dataset` ds1 "
            "  JOIN Metadata.`Datatype` dt1 ON ds1.DatatypeDataverseName = dt1.DataverseName "
            "  AND ds1.DatatypeName = dt1.DatatypeName "
            "UNNEST ds1.ViewDetails.ForeignKeys AS ForeignKeys "
            "UNNEST ForeignKeys.ForeignKey AS ForeignKey AT p "
            "UNNEST ( "
            "  FROM Metadata.`Dataset` ds2 "
            "    JOIN Metadata.`Datatype` dt2 ON ds2.DatatypeDataverseName = dt2.DataverseName "
            "    AND ds2.DatatypeName = dt2.DatatypeName "
            "  UNNEST ds2.ViewDetails.PrimaryKey AS primaryKey "
            "  LET dvname1 = decode_dataverse_name(ds1.DataverseName), "
            "      dvname2 = decode_dataverse_name(ds2.DataverseName), "
            "      FKTABLE_CAT = dvname1[0], "
            "      FKTABLE_SCHEM = CASE ARRAY_LENGTH(dvname1) WHEN 1 THEN NULL ELSE dvname1[1] END, "
            "      FKTABLE_NAME = ds1.DatasetName, "
            "      fkTable_fk = ForeignKey[0], "
            "      fk_dvname = decode_dataverse_name(ForeignKeys.RefDataverseName), "
            "      fkTable_pk_CAT = fk_dvname[0], "
            "      fkTable_pk_SCHEM = CASE ARRAY_LENGTH(fk_dvname) WHEN 1 THEN NULL ELSE fk_dvname[1] END, "
            "      fkTable_pk_TABLE = ForeignKeys.RefDatasetName, "
            "      PKTABLE_CAT = dvname2[0], "
            "      PKTABLE_SCHEM = CASE ARRAY_LENGTH(dvname2) WHEN 1 THEN NULL ELSE dvname2[1] END, "
            "      PKTABLE_NAME = ds2.DatasetName, "
            "      pkTable_pk = primaryKey[0], "
            "    UPDATE_RULE = ";


const char query_foreign_keys_fk_with_pk_where[] = " AND PKTABLE_CAT = fkTable_pk_CAT "
            "    AND PKTABLE_SCHEM = fkTable_pk_SCHEM "
            "    AND PKTABLE_NAME = fkTable_pk_TABLE "
            "    AND fkTable_fk = pkTable_pk SELECT PKTABLE_CAT, "
            "                                     PKTABLE_SCHEM, "
            "                                     PKTABLE_NAME, "
            "                                     pkTable_pk AS PKCOLUMN_NAME, "
            "                                     FKTABLE_CAT, "
            "                                     FKTABLE_SCHEM, "
            "                                     FKTABLE_NAME, "
            "                                     fkTable_fk AS FKCOLUMN_NAME, "
            "                                     p AS KEY_SEQ, "
            "                                     UPDATE_RULE, "
            "                                     DELETE_RULE, "
            "                                     NULL FK_NAME, "
            "                                     NULL PK_NAME, "
            "                                     DEFERRABILITY) AS subquery ";


const char query_foreign_keys_pk_where[] = "AND PKTABLE_CAT = fkTable_pk_CAT"
            "    AND PKTABLE_SCHEM = fkTable_pk_SCHEM"
            "    AND PKTABLE_NAME = fkTable_pk_TABLE"
            "    AND fkTable_fk = pkTable_pk SELECT PKTABLE_CAT,"
            "                                     PKTABLE_SCHEM,"
            "                                     PKTABLE_NAME,"
            "                                     pkTable_pk AS PKCOLUMN_NAME,"
            "                                     FKTABLE_CAT,"
            "                                     FKTABLE_SCHEM,"
            "                                     FKTABLE_NAME,"
            "                                     fkTable_fk AS FKCOLUMN_NAME,"
            "                                     p AS KEY_SEQ,"
            "                                     UPDATE_RULE,"
            "                                     DELETE_RULE,"
            "                                     NULL FK_NAME,"
            "                                     NULL PK_NAME,"
            "                                     DEFERRABILITY) AS subquery";

const char query_foreign_keys_fk_where[] = "AND PKTABLE_CAT = fkTable_pk_CAT "
            "    AND PKTABLE_SCHEM = fkTable_pk_SCHEM "
            "    AND PKTABLE_NAME = fkTable_pk_TABLE "
            "    AND fkTable_fk = pkTable_pk SELECT PKTABLE_CAT, "
            "                                     PKTABLE_SCHEM, "
            "                                     PKTABLE_NAME, "
            "                                     pkTable_pk AS PKCOLUMN_NAME, "
            "                                     FKTABLE_CAT, "
            "                                     FKTABLE_SCHEM, "
            "                                     FKTABLE_NAME,"
            "                                     fkTable_fk AS FKCOLUMN_NAME, "
            "                                     p AS KEY_SEQ, "
            "                                     UPDATE_RULE, "
            "                                     DELETE_RULE, "
            "                                     NULL FK_NAME, "
            "                                     NULL PK_NAME, "
            "                                     DEFERRABILITY) AS subquery ";


const char query_sql_columns[] = "SELECT TABLE_CAT"                                   //1
                ",TABLE_SCHEM"                                                        //2
                ",TABLE_NAME"                                                         //3
                ",COLUMN_NAME"                                                        //4
                ",case when TYPE_NAME = 'int64' then -5 "
                "     when TYPE_NAME = 'double' then 8 "
                "     when TYPE_NAME = 'string' then 12 "
                "     when TYPE_NAME = 'date' then 91 "
                "     when TYPE_NAME = 'time' then 92 "
                "     when TYPE_NAME = 'datetime' then 93 "
                "     else -1 end DATA_TYPE"                                          //5
                ",TYPE_NAME"                                                          //6
                ",case when TYPE_NAME = 'string' then 32000 "
                "      when TYPE_NAME = 'double' then 15 "
                "      when TYPE_NAME = 'int64' then 19 "
                "      when TYPE_NAME = 'date' then 10 "
                "      when TYPE_NAME = 'time' then 8 "
                "      when TYPE_NAME = 'datetime' then 23 "
                "      else -1 end COLUMN_SIZE"                                       //7
                ",case when TYPE_NAME = 'string' then 32000 "
                "      when TYPE_NAME = 'int64' then 19 "
                "      else -1 end BUFFER_LENGTH"                                     //8
                ",0 DECIMAL_DIGITS"                                                   //9
                ",10 NUM_PREC_RADIX"                                                  //10
                ",1 NULLABLE"                                                         //11
                ",'' REMARKS"                                                         //12
                ",'' COLUMN_DEF"                                                      //13
                ",case when TYPE_NAME = 'int64' then -5 "
                "      when TYPE_NAME = 'double' then 8 "
                "      when TYPE_NAME = 'string' then 12 "
                "      when TYPE_NAME = 'date' then 9 "
                "      when TYPE_NAME = 'time' then 9 "
                "      when TYPE_NAME = 'datetime' then 9 "
                "      else -1 end SQL_DATA_TYPE"                                     //14
                ",case when TYPE_NAME = 'date' then 1 "
                "      when TYPE_NAME = 'time' then 2 "
                "      when TYPE_NAME = 'datetime' then 3 "
                "      else 0 end SQL_DATETIME_SUB"                                   //15
                ",case when TYPE_NAME = 'string' then 32000 "
                "      else null end CHAR_OCTET_LENGTH"                               //16
                ",1 ORDINAL_POSITION"                                                 //17
                ",'YES' IS_NULLABLE"                                                  //18
                " FROM Metadata.`Dataset` ds"
                " JOIN Metadata.`Datatype` dt ON ds.DatatypeDataverseName = dt.DataverseName"
                " AND ds.DatatypeName = dt.DatatypeName"
                " UNNEST dt.Derived.Record.Fields AS field AT fieldpos LEFT"
                " JOIN Metadata.`Datatype` dt2 ON field.FieldType = dt2.DatatypeName"
                " AND ds.DataverseName = dt2.DataverseName"
                " AND dt2.Derived IS KNOWN"
                " LET dvname = decode_dataverse_name(ds.DataverseName),"
                " TABLE_CAT = dvname[0],"
                " TABLE_SCHEM = case array_length(dvname) when 1 then null else dvname[1] end,"
                " TABLE_NAME = ds.DatasetName,"
                " TYPE_NAME =field.FieldType,"
                " COLUMN_NAME = field.FieldName"
                " WHERE (ARRAY_LENGTH(dt.Derived.Record.Fields) > 0)";