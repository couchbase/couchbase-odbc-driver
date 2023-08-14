#pragma once

#include "driver/include/driver.h"
#include "driver/include/connection.h"
#include "driver/include/descriptor.h"
#include "driver/include/result_set.h"

#include <Poco/Net/HTTPResponse.h>

#include <memory>
#include <sstream>
#include <string>
#include <vector>

class Statement
    : public Child<Connection, Statement>
{
private:
    using ChildType = Child<Connection, Statement>;

public:
    explicit Statement(Connection & connection);
    virtual ~Statement();
    // Lookup TypeInfo for given name of type.
    const TypeInfo & getTypeInfo(const std::string & type_name, const std::string & type_name_without_parameters) const;
    bool isPrepared() const;
    bool isExecuted() const;
    // Prepare query for execution.
    void prepareQuery(const std::string & q);
    std::string nativeSql(const std::string & q);
    // Execute previously prepared query, or no-op if previous execution was done using forwardExecuteQuery().
    void executeQuery(std::unique_ptr<ResultMutator> && mutator = std::unique_ptr<ResultMutator>{});
    // Prepare and execute query.
    void executeQuery(const std::string & q, std::unique_ptr<ResultMutator> && mutator = std::unique_ptr<ResultMutator> {});
    // Execute previously prepared query.
    void forwardExecuteQuery(std::unique_ptr<ResultMutator> && mutator = std::unique_ptr<ResultMutator> {});
    // Indicates whether there is an result set available for reading.
    bool hasResultSet() const;
    // Get the current resukt set.
    ResultSet & getResultSet();
    // Make the next result set current, if any.
    bool advanceToNextResultSet();
    // Reset statement to initial state.
    void closeCursor();
    // Reset/release row/column buffer bindings.
    void resetColBindings();
    // Reset/release parameter buffer bindings.
    void resetParamBindings();
    // Access the effective descriptor by its role (type).
    Descriptor & getEffectiveDescriptor(SQLINTEGER type);
    // Set an explicit descriptor for a role (type).
    void setExplicitDescriptor(SQLINTEGER type, std::shared_ptr<Descriptor> desc);
    // Make an implicit descriptor active again.
    void setImplicitDescriptor(SQLINTEGER type);
    CallbackCookie cbCookie;
    std::uint32_t stmt_query_timeout;
    bool is_set_stmt_query_timeout;
    void handleGetTypeInfo(std::unique_ptr<ResultMutator> && mutator = std::unique_ptr<ResultMutator>{});
    long long getMillisecondsFromODBCtimestamp(const std::string& timestamp_exp);
    std::vector<std::string>* expected_column_order = nullptr;
private:
    void requestNextPackOfResultSets(std::unique_ptr<ResultMutator> && mutator);
    static void queryCallback(lcb_INSTANCE * instance, int type, const lcb_RESPANALYTICS * resp);

    void processEscapeSequences();
    void extractParametersinfo();
    std::string getParamFinalName(std::size_t param_idx);
    std::vector<ParamBindingInfo> getParamsBindingInfo(std::size_t param_set_idx);

    Descriptor & choose(std::shared_ptr<Descriptor> & implicit_desc, std::weak_ptr<Descriptor> & explicit_desc);

    void allocateImplicitDescriptors();
    void deallocateImplicitDescriptors();

    std::shared_ptr<Descriptor> allocateDescriptor();
    void deallocateDescriptor(std::shared_ptr<Descriptor> & desc);

private:
    std::shared_ptr<Descriptor> implicit_ard;
    std::shared_ptr<Descriptor> implicit_apd;
    std::shared_ptr<Descriptor> implicit_ird;
    std::shared_ptr<Descriptor> implicit_ipd;

    std::weak_ptr<Descriptor> explicit_ard;
    std::weak_ptr<Descriptor> explicit_apd;
    std::weak_ptr<Descriptor> explicit_ird;
    std::weak_ptr<Descriptor> explicit_ipd;

    bool is_prepared = false;
    bool is_forward_executed = false;
    bool is_executed = false;
    std::string query;
    std::vector<ParamInfo> parameters;
    std::vector<std::string> cbas_params_args;

    std::unique_ptr<Poco::Net::HTTPResponse> response;
    std::istream* in = nullptr;
    std::unique_ptr<ResultReader> result_reader;
    std::size_t next_param_set_idx = 0;
};
