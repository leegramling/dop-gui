#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace dopgui::test
{
struct Json
{
    using Object = std::map<std::string, Json>;
    using Array = std::vector<Json>;
    using Value = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    Value value;

    const Json& at(const std::string& key) const;
    const Json& at(std::size_t index) const;
    bool boolean() const;
    double number() const;
    const std::string& string() const;
};

/** Parse a complete JSON document into typed values. */
Json parseJson(std::string text);

/** Launch dop-gui, execute one query, and return its parsed JSON response. */
Json runDopGuiQuery(
    const std::filesystem::path& executable,
    const std::string& query);
}
