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