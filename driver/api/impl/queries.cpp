#include "driver/api/impl/queries.h"

std::stringstream build_query_conditionally(Statement& statement){
    std::stringstream query;
    switch (statement.getParent().database_entity_support)
            {
            case 0:
                query << " TABLE_CAT = ds.DataverseName, ";
                query << " TABLE_SCHEM = null, ";
                break;
            case 1:
                query << " TABLE_CAT = ds.DatabaseName,";
                query << " sch = ds.DataverseName,";
                query << " TABLE_SCHEM = sch, ";
                break;
            }
    return query;
}

std::stringstream get_query_sql_columns(Statement& statement){
    std::stringstream query_sql_columns;
    query_sql_columns << "SELECT TABLE_CAT"                                                   //1
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
                        " LET ";
                        query_sql_columns << build_query_conditionally(statement).str();
                        query_sql_columns << " TABLE_NAME = ds.DatasetName, ";
                        query_sql_columns << " TYPE_NAME =field.FieldType, ";
                        query_sql_columns << " isView = ds.DatasetType = 'VIEW',";
                        query_sql_columns << " COLUMN_NAME = field.FieldName ";
                        query_sql_columns << " WHERE (ARRAY_LENGTH(dt.Derived.Record.Fields) > 0) ";

    return query_sql_columns;
}