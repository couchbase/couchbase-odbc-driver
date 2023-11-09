#include "driver/utils/include/database_entity_support.h"

bool checkDatabaseEntitySupport(Statement& statement){
    std::string sql = "select count(*) from Metadata.`Dataset` where DataverseName='Metadata' and DatasetName='Database'";
    statement.executeQuery(sql);

    // fetch the result
    SQLHSTMT statementHandle = statement.getHandle();
    SQLRETURN ret;
    ret = SQLFetch(statementHandle);
    if (ret == SQL_NO_DATA) {
        std::cout << "LOG: No data" << std::endl;
    } else if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        // Process the row's data here.
        SQLINTEGER intValue;
        SQLLEN indicator;
        SQLGetData(statementHandle, 1, SQL_C_LONG, &intValue, sizeof(intValue), &indicator);
        if (indicator == SQL_NULL_DATA) {
            std::cout << "LOG: Value is: NULL" << std::endl;
        } else {
            std::cout << "LOG: Value is: " << intValue << std::endl;
            return (intValue == 1);
        }
    } else {
        std::cout<<"LOG: Error Occurred while Fetching" << std::endl;
    }
}