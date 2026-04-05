#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

class App;

struct QueryValue;
using QueryValuePtr = std::shared_ptr<QueryValue>;

struct QueryArray
{
    std::vector<QueryValuePtr> elements;
};

struct QueryField
{
    std::string key;
    QueryValuePtr value;
};

struct QueryObject
{
    std::vector<QueryField> fields;
};

struct QueryValue
{
    using Variant = std::variant<std::nullptr_t, bool, int64_t, uint64_t, double, std::string, QueryArray, QueryObject>;
    Variant data;
};

struct QueryByPath
{
    std::string queryName;
    std::string canonicalPath;
};

using QueryRequest = std::variant<QueryByPath>;

struct QuerySuccess
{
    std::string queryName;
    QueryValue value;
};

struct QueryError
{
    std::string queryName;
    std::string message;
};

using QueryResult = std::variant<QuerySuccess, QueryError>;

std::optional<QueryRequest> parseQueryRequest(const std::string& name);
QueryResult executeQuery(const App& app, const QueryRequest& request);
std::string serializeQueryResult(const QueryResult& result);
std::vector<std::string> queryNames();

QueryValue makeNullValue();
QueryValue makeBoolValue(bool value);
QueryValue makeIntValue(int64_t value);
QueryValue makeUIntValue(uint64_t value);
QueryValue makeDoubleValue(double value);
QueryValue makeStringValue(std::string value);
QueryValue makeArrayValue(std::initializer_list<QueryValue> values);
QueryValue makeObjectValue(std::initializer_list<QueryField> fields);
QueryField makeField(std::string key, QueryValue value);
