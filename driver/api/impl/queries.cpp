#include "driver/api/impl/queries.h"

std::stringstream build_query_conditionally(Statement& statement, const std::string& tableCat, const std::string& tableSchem, const std::string& ds, const std::string& DataverseName, const std::string& DatabaseName) {
    std::stringstream query;
    switch (statement.getParent().database_entity_support) {
        case 0:
            switch (statement.getParent().two_part_scope_name) {
                case 0:
                    query << " " << tableCat << " = " << ds << "." << DataverseName << ",";
                    query << " " << tableSchem << " = NULL,";
                    break;
                case 1:
                    query << " dvname = decode_dataverse_name(" << ds << "." << DataverseName << "),";
                    query << " " << tableCat << " = dvname[0],";
                    query << " " << tableSchem << " = dvname[1],";
                    break;
            }
            break;
        case 1:
            query << " " << tableCat << " = " << ds << "." << DatabaseName << ",";
            query << " " << tableSchem << " = " << ds << "." << DataverseName << ",";
            break;
    }
    return query;
}

std::stringstream get_query_sql_columns(Statement& statement){
    std::stringstream query_sql_columns;
    query_sql_columns<< "SELECT TABLE_CAT"                                                    //1
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
                        " AND ds.DatatypeName = dt.DatatypeName";
    if(statement.getParent().database_entity_support){
        query_sql_columns << " AND ds.DatatypeDatabaseName = dt.DatabaseName ";
    }
    query_sql_columns <<" UNNEST dt.Derived.Record.Fields AS field AT fieldpos LEFT"
                        " JOIN Metadata.`Datatype` dt2 ON field.FieldType = dt2.DatatypeName"
                        " AND ds.DataverseName = dt2.DataverseName"
                        " AND dt2.Derived IS KNOWN";
    if(statement.getParent().database_entity_support){
        query_sql_columns << " AND ds.DatabaseName = dt2.DatabaseName ";
    }
    query_sql_columns <<" LET ";
    query_sql_columns << build_query_conditionally(statement, "TABLE_CAT", "TABLE_SCHEM", "ds", "DataverseName", "DatabaseName").str();
    query_sql_columns << " TABLE_NAME = ds.DatasetName, ";
    query_sql_columns << " TYPE_NAME =field.FieldType, ";
    query_sql_columns << " isView = ds.DatasetType = 'VIEW',";
    query_sql_columns << " COLUMN_NAME = field.FieldName ";
    query_sql_columns << " WHERE (ARRAY_LENGTH(dt.Derived.Record.Fields) > 0) ";

    return query_sql_columns;
}

std::stringstream get_query_primary_keys(Statement& statement){
    std::stringstream query_primary_keys;
    query_primary_keys << "SELECT TABLE_CAT,"
                          "TABLE_SCHEM, "
                          "TABLE_NAME, "
                          "primaryKey[0] AS COLUMN_NAME, "
                          "KEY_SEQ, "
                          "NULL PK_NAME "
                          "FROM Metadata.`Dataset` ds "
                          "JOIN Metadata.`Datatype` dt ON ds.DatatypeDataverseName = dt.DataverseName "
                          "AND ds.DatatypeName = dt.DatatypeName ";
    if(statement.getParent().database_entity_support){
        query_primary_keys << " AND ds.DatatypeDatabaseName = dt.DatabaseName ";
    }
    query_primary_keys  << " UNNEST ds.ViewDetails.PrimaryKey as primaryKey at p "
                          "LET ";
    query_primary_keys << build_query_conditionally(statement, "TABLE_CAT", "TABLE_SCHEM", "ds", "DataverseName", "DatabaseName").str();;
    query_primary_keys << "TABLE_NAME = ds.DatasetName, ";
    query_primary_keys << "KEY_SEQ = p ";
    query_primary_keys << "WHERE ";
    return query_primary_keys;
}


std::stringstream get_query_procedure_columns(Statement& statement){
    std::cout<<"\nLOG: inside get_query_procedure_columns";
    std::stringstream query_procedure_columns;
    query_procedure_columns << "SELECT "
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
        "LET ";
    query_procedure_columns << build_query_conditionally(statement, "PROCEDURE_CAT", "PROCEDURE_SCHEM", "fn", "DataverseName", "DatabaseName").str();
    query_procedure_columns << "    PROCEDURE_NAME = fn.Name, "
        "    COLUMN_TYPE = ";
    return query_procedure_columns;
}


std::stringstream get_query_procedures(Statement& statement){
    std::cout<<"\nLOG: inside get_query_procedures";
    std::stringstream query_procedures;
    query_procedures << "SELECT PROCEDURE_CAT, "
        "       PROCEDURE_SCHEM, "
        "       PROCEDURE_NAME, "
        "       NULL NUM_INPUT_PARAMS, "
        "       NULL NUM_OUTPUT_PARAMS, "
        "       NULL NUM_RESULT_SETS, "
        "       REMARKS, "
        "       PROCEDURE_TYPE "
        "FROM Metadata.`Function` AS fn "
        "LET ";
    query_procedures << build_query_conditionally(statement, "PROCEDURE_CAT", "PROCEDURE_SCHEM", "fn", "DataverseName", "DatabaseName").str();
    query_procedures << "    PROCEDURE_NAME = fn.Name, "
        "    REMARKS = 'Default Remarks', "
        "    PROCEDURE_TYPE = 2 "
        "WHERE" ;
    return query_procedures;
}


std::stringstream get_query_foreign_keys_fk_with_pk(Statement& statement){
    std::cout<<"\nLOG: inside get_query_foreign_keys_fk_with_pk";
    std::stringstream query_foreign_keys_fk_with_pk;
    query_foreign_keys_fk_with_pk << "SELECT subquery.* "
            "FROM Metadata.`Dataset` ds1 "
            "  JOIN Metadata.`Datatype` dt1 ON ds1.DatatypeDataverseName = dt1.DataverseName "
            "  AND ds1.DatatypeName = dt1.DatatypeName ";
    if(statement.getParent().database_entity_support){
        query_foreign_keys_fk_with_pk << " AND ds1.DatatypeDatabaseName = dt1.DatabaseName ";
    }
    query_foreign_keys_fk_with_pk << "UNNEST ds1.ViewDetails.ForeignKeys AS ForeignKeys "
            "UNNEST ForeignKeys.ForeignKey AS ForeignKey AT p "
            "UNNEST ( "
            "  FROM Metadata.`Dataset` ds2 "
            "    JOIN Metadata.`Datatype` dt2 ON ds2.DatatypeDataverseName = dt2.DataverseName "
            "    AND ds2.DatatypeName = dt2.DatatypeName ";
    if(statement.getParent().database_entity_support){
        query_foreign_keys_fk_with_pk << " AND ds2.DatatypeDatabaseName = dt2.DatabaseName ";
    }
    query_foreign_keys_fk_with_pk << "  UNNEST ds2.ViewDetails.PrimaryKey AS primaryKey "
            "  LET ";
    query_foreign_keys_fk_with_pk << build_query_conditionally(statement, "FKTABLE_CAT", "FKTABLE_SCHEM", "ds1", "DataverseName", "DatabaseName").str();
    query_foreign_keys_fk_with_pk << build_query_conditionally(statement, "PKTABLE_CAT", "PKTABLE_SCHEM", "ds2", "DataverseName", "DatabaseName").str();
    query_foreign_keys_fk_with_pk << build_query_conditionally(statement, "fkTable_pk_CAT", "fkTable_pk_SCHEM", "ForeignKeys", "RefDataverseName", "RefDatabaseName").str();
    query_foreign_keys_fk_with_pk << "      FKTABLE_NAME = ds1.DatasetName, "
            "      fkTable_fk = ForeignKey[0], "
            "      fkTable_pk_TABLE = ForeignKeys.RefDatasetName, "
            "      PKTABLE_NAME = ds2.DatasetName, "
            "      pkTable_pk = primaryKey[0], "
            "    UPDATE_RULE =  ";
    return query_foreign_keys_fk_with_pk;
}


std::stringstream get_query_foreign_keys_pk(Statement& statement){
    std::cout<<"\nLOG: inside get_query_foreign_keys_pk";
    std::stringstream query_foreign_keys_pk;
    query_foreign_keys_pk << "SELECT subquery.* "
            "FROM Metadata.`Dataset` ds1 "
            "  JOIN Metadata.`Datatype` dt1 ON ds1.DatatypeDataverseName = dt1.DataverseName "
            "  AND ds1.DatatypeName = dt1.DatatypeName ";
    if(statement.getParent().database_entity_support){
        query_foreign_keys_pk << " AND ds1.DatatypeDatabaseName = dt1.DatabaseName ";
    }
    query_foreign_keys_pk << "UNNEST ds1.ViewDetails.PrimaryKey AS primaryKey "
            "UNNEST ( "
            "  FROM Metadata.`Dataset` ds2 "
            "    JOIN Metadata.`Datatype` dt2 ON ds2.DatatypeDataverseName = dt2.DataverseName "
            "    AND ds2.DatatypeName = dt2.DatatypeName ";
    if(statement.getParent().database_entity_support){
        query_foreign_keys_pk << " AND ds2.DatatypeDatabaseName = dt2.DatabaseName ";
    }
    query_foreign_keys_pk << "  UNNEST ds2.ViewDetails.ForeignKeys AS ForeignKeys "
            "  UNNEST ForeignKeys.ForeignKey AS ForeignKey AT p "
            "  LET ";
    query_foreign_keys_pk << build_query_conditionally(statement, "FKTABLE_CAT", "FKTABLE_SCHEM", "ds2", "DataverseName", "DatabaseName").str();
    query_foreign_keys_pk << build_query_conditionally(statement, "PKTABLE_CAT", "PKTABLE_SCHEM", "ds1", "DataverseName", "DatabaseName").str();
    query_foreign_keys_pk << build_query_conditionally(statement, "fkTable_pk_CAT", "fkTable_pk_SCHEM", "ForeignKeys", "RefDataverseName", "RefDatabaseName").str();
    query_foreign_keys_pk << "      FKTABLE_NAME = ds2.DatasetName, "
            "      fkTable_fk = ForeignKey[0], "
            "      fkTable_pk_TABLE = ForeignKeys.RefDatasetName, "
            "      PKTABLE_NAME = ds1.DatasetName, "
            "      pkTable_pk = primaryKey[0], "
            "    UPDATE_RULE = ";
    return query_foreign_keys_pk;
}


std::stringstream get_query_foreign_keys_fk(Statement& statement){
    std::cout<<"\nLOG: inside get_query_foreign_keys_fk";
    std::stringstream query_foreign_keys_fk;
    query_foreign_keys_fk << "SELECT subquery.*  "
            "FROM Metadata.`Dataset` ds1 "
            "  JOIN Metadata.`Datatype` dt1 ON ds1.DatatypeDataverseName = dt1.DataverseName "
            "  AND ds1.DatatypeName = dt1.DatatypeName ";
    if(statement.getParent().database_entity_support){
        query_foreign_keys_fk << " AND ds1.DatatypeDatabaseName = dt1.DatabaseName ";
    }
    query_foreign_keys_fk << "UNNEST ds1.ViewDetails.ForeignKeys AS ForeignKeys "
            "UNNEST ForeignKeys.ForeignKey AS ForeignKey AT p "
            "UNNEST ( "
            "  FROM Metadata.`Dataset` ds2 "
            "    JOIN Metadata.`Datatype` dt2 ON ds2.DatatypeDataverseName = dt2.DataverseName "
            "    AND ds2.DatatypeName = dt2.DatatypeName ";
    if(statement.getParent().database_entity_support){
        query_foreign_keys_fk << " AND ds2.DatatypeDatabaseName = dt2.DatabaseName ";
    }
    query_foreign_keys_fk << "  UNNEST ds2.ViewDetails.PrimaryKey AS primaryKey "
            "  LET ";
    query_foreign_keys_fk << build_query_conditionally(statement, "FKTABLE_CAT", "FKTABLE_SCHEM", "ds1", "DataverseName", "DatabaseName").str();
    query_foreign_keys_fk << build_query_conditionally(statement, "PKTABLE_CAT", "PKTABLE_SCHEM", "ds2", "DataverseName", "DatabaseName").str();
    query_foreign_keys_fk << build_query_conditionally(statement, "fkTable_pk_CAT", "fkTable_pk_SCHEM", "ForeignKeys", "RefDataverseName", "RefDatabaseName").str();
    query_foreign_keys_fk << "      FKTABLE_NAME = ds1.DatasetName, "
            "      fkTable_fk = ForeignKey[0], "
            "      fkTable_pk_TABLE = ForeignKeys.RefDatasetName, "
            "      PKTABLE_NAME = ds2.DatasetName, "
            "      pkTable_pk = primaryKey[0], "
            "    UPDATE_RULE = ";
    return query_foreign_keys_fk;
}