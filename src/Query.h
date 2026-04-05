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

/**
 * @brief JSON-like array value used by query results.
 */
struct QueryArray
{
    std::vector<QueryValuePtr> elements;
};

/**
 * @brief JSON-like object field used by query results.
 */
struct QueryField
{
    std::string key;
    QueryValuePtr value;
};

/**
 * @brief JSON-like object value used by query results.
 */
struct QueryObject
{
    std::vector<QueryField> fields;
};

/**
 * @brief Recursive query result value.
 */
struct QueryValue
{
    using Variant = std::variant<std::nullptr_t, bool, int64_t, uint64_t, double, std::string, QueryArray, QueryObject>;
    Variant data;
};

/**
 * @brief Path-based query request with original and canonicalized names.
 */
struct QueryByPath
{
    std::string queryName;
    std::string canonicalPath;
};

/**
 * @brief Tagged query request data.
 */
using QueryRequest = std::variant<QueryByPath>;

/**
 * @brief Successful query execution details.
 */
struct QuerySuccess
{
    std::string queryName;
    QueryValue value;
};

/**
 * @brief Failed query execution details.
 */
struct QueryError
{
    std::string queryName;
    std::string message;
};

/**
 * @brief Tagged query result data.
 */
using QueryResult = std::variant<QuerySuccess, QueryError>;

/**
 * @brief Parse a query name into typed query request data.
 * @param name Raw query string from CLI or script input.
 * @return Parsed query request when the input is non-empty.
 */
std::optional<QueryRequest> parseQueryRequest(const std::string& name);
/**
 * @brief Execute a query request against the application.
 * @param app Application to inspect.
 * @param request Parsed query request.
 * @return Tagged query result.
 */
QueryResult executeQuery(const App& app, const QueryRequest& request);
/**
 * @brief Serialize a query result as JSON-compatible text.
 * @param result Tagged query result.
 * @return JSON-compatible serialized result text.
 */
std::string serializeQueryResult(const QueryResult& result);
/**
 * @brief Return the list of supported query names.
 * @return Supported query surface names.
 */
std::vector<std::string> queryNames();

/**
 * @brief Create a null query value.
 * @return Null query value.
 */
QueryValue makeNullValue();
/**
 * @brief Create a boolean query value.
 * @param value Boolean payload.
 * @return Boolean query value.
 */
QueryValue makeBoolValue(bool value);
/**
 * @brief Create a signed integer query value.
 * @param value Signed integer payload.
 * @return Signed integer query value.
 */
QueryValue makeIntValue(int64_t value);
/**
 * @brief Create an unsigned integer query value.
 * @param value Unsigned integer payload.
 * @return Unsigned integer query value.
 */
QueryValue makeUIntValue(uint64_t value);
/**
 * @brief Create a floating-point query value.
 * @param value Floating-point payload.
 * @return Floating-point query value.
 */
QueryValue makeDoubleValue(double value);
/**
 * @brief Create a string query value.
 * @param value String payload.
 * @return String query value.
 */
QueryValue makeStringValue(std::string value);
/**
 * @brief Create an array query value.
 * @param values Array elements.
 * @return Array query value.
 */
QueryValue makeArrayValue(std::initializer_list<QueryValue> values);
/**
 * @brief Create an object query value.
 * @param fields Object fields.
 * @return Object query value.
 */
QueryValue makeObjectValue(std::initializer_list<QueryField> fields);
/**
 * @brief Create a query object field.
 * @param key Field key.
 * @param value Field value.
 * @return Query object field.
 */
QueryField makeField(std::string key, QueryValue value);
