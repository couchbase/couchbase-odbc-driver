#pragma once

#include "driver/platform/platform.h"
#include "driver/utils/include/utils.h"
#include "driver/include/attributes.h"

#include <string>
#include <vector>

class DiagnosticsRecord
    : public AttributeContainer
{
public:
};

// A class that enables other classes derived from it
// to hold diagnostics information in a common form:
//   a diagnostics header (record with index 0), and
//   diagnostics records (records starting from index 1).
// Each record is basically an AttributeContainer instance,
// i.e., it is able to hold an integer or string values
// under arbitrary integer keys.
class DiagnosticsContainer {
public:
    void fillDiag(SQLRETURN rc, const std::string& sql_status, const std::string& message, SQLINTEGER native_error_code);
    void fillDiag(const std::string& sql_status, const std::string& message, SQLINTEGER native_error_code);
    void fillDiag(SQLRETURN rc, const std::string& sql_status, const std::string& message);
    void fillDiag(const std::string& sql_status, const std::string& message);

    DiagnosticsRecord & getDiagHeader();

    void setReturnCode(SQLRETURN rc);
    SQLRETURN getReturnCode();

    std::size_t getDiagStatusCount();
    DiagnosticsRecord & getDiagStatus(std::size_t num);
    void insertDiagStatus(DiagnosticsRecord && rec);

    void resetDiag();

private:
    std::vector<DiagnosticsRecord> records;
};
