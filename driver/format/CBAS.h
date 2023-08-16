#pragma once

#include "driver/platform/platform.h"
#include "driver/result_set.h"

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
