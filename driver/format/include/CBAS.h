#pragma once

#include "driver/platform/platform.h"
#include "driver/include/result_set.h"

// Pre-computed per-column type tag so readNextRow() dispatches with a switch
enum class ColTypeTag : uint8_t {
    String_,    // lossless-adm string: strip leading ':' if present
    Float64_,   // lossless-adm double: strip "0C:" prefix
    Date_,      // lossless-adm date:   strip "11:", convert days→date string
    Time_,      // lossless-adm time:   strip "12:", convert ms→time string
    DateTime_,  // lossless-adm dt:     strip "10:", convert ms→datetime string
    Integer_,   // any integer type: read from col->valueint
    Other_      // boolean (UInt8) or unknown: handled generically
};

class CBASResultSet : public ResultSet {
public:
    explicit CBASResultSet(const std::string & timezone,
        AmortizedIStreamReader & stream,
        std::unique_ptr<ResultMutator> && mutator,
        CallbackCookie & cbCookie,
        std::vector<std::string>* expected_column_order);
    virtual ~CBASResultSet() override = default;

    CallbackCookie & cookie;
    std::vector<int> indexMapper;
    // Indexed by ODBC column slot (same index space as columns_info).
    // Populated once in the constructor, used every row in readNextRow().
    std::vector<uint8_t> col_tags;


protected:
    virtual bool readNextRow(Row & row) override;
};

class CBASResultReader : public ResultReader {
public:
    explicit CBASResultReader(
        const std::string & timezone_, std::istream & raw_stream, std::unique_ptr<ResultMutator> && mutator, CallbackCookie & cbCookie, std::vector<std::string>* expected_column_order);
    virtual ~CBASResultReader() override = default;

    virtual bool advanceToNextResultSet() override;
};
