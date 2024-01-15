#include <iostream>
#include <string.h>
#include <sstream>
#include "driver/include/statement.h"

std::stringstream build_query_conditionally(Statement& statement, const std::string& tableCat, const std::string& tableSchem, const std::string& ds, const std::string& DataverseName, const std::string& DatabaseName);
std::stringstream get_query_sql_columns(Statement& statement);
std::stringstream get_query_primary_keys(Statement& statement);
std::stringstream get_query_procedure_columns(Statement& statement);
std::stringstream get_query_procedures(Statement& statement);
std::stringstream get_query_foreign_keys_fk_with_pk(Statement& statement);
std::stringstream get_query_foreign_keys_pk(Statement& statement);
std::stringstream get_query_foreign_keys_fk(Statement& statement);


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
            "                                     DEFERRABILITY ORDER BY FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, KEY_SEQ) AS subquery";

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
            "                                     DEFERRABILITY ORDER BY PKTABLE_CAT, PKTABLE_SCHEM, PKTABLE_NAME, KEY_SEQ) AS subquery ";