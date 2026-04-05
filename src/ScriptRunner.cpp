#include "ScriptRunner.h"

#include "App.h"
#include "Command.h"
#include "Query.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
std::string escapeJson(const std::string& value)
{
    std::ostringstream out;
    for (char ch : value)
    {
        switch (ch)
        {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out << ch; break;
        }
    }
    return out.str();
}
}

ScriptRequest ScriptRunner::parse(const std::string& filename) const
{
    auto text = stripComments(readFile(filename));
    return {
        .commands = parseStringArray(text, "commands"),
        .queries = parseStringArray(text, "queries"),
    };
}

int ScriptRunner::execute(App& app, const ScriptRequest& request) const
{
    std::ostringstream json;
    json << "{"
         << "\"ok\":true,"
         << "\"commands\":[";

    bool first = true;
    for (const auto& commandName : request.commands)
    {
        if (!first) json << ",";
        first = false;
        auto command = parseCommandRequest(commandName);
        if (!command) continue;
        auto result = executeCommand(app, *command);
        json << serializeCommandResult(result);
    }

    json << "],\"queries\":[";
    first = true;
    for (const auto& queryName : request.queries)
    {
        if (!first) json << ",";
        first = false;
        auto query = parseQueryRequest(queryName);
        if (!query) continue;
        auto result = executeQuery(app, *query);
        json << serializeQueryResult(result);
    }
    json << "]}";

    std::cout << json.str() << '\n';
    return 0;
}

std::string ScriptRunner::readFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Failed to open script file: " + filename);

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ScriptRunner::stripComments(const std::string& text)
{
    std::string result;
    result.reserve(text.size());

    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        char ch = text[i];
        char next = (i + 1 < text.size()) ? text[i + 1] : '\0';

        if (!inSingleQuote && !inDoubleQuote && ch == '/' && next == '/')
        {
            while (i < text.size() && text[i] != '\n') ++i;
            if (i < text.size()) result.push_back(text[i]);
            continue;
        }

        if (!inSingleQuote && !inDoubleQuote && ch == '/' && next == '*')
        {
            i += 2;
            while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/')) ++i;
            ++i;
            continue;
        }

        if (ch == '\'' && !inDoubleQuote) inSingleQuote = !inSingleQuote;
        if (ch == '"' && !inSingleQuote) inDoubleQuote = !inDoubleQuote;

        result.push_back(ch);
    }

    return result;
}

std::vector<std::string> ScriptRunner::parseStringArray(const std::string& text, const std::string& key)
{
    auto keyPos = text.find(key);
    if (keyPos == std::string::npos) return {};

    auto arrayStart = text.find('[', keyPos);
    auto arrayEnd = text.find(']', arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) return {};

    std::vector<std::string> values;
    std::string current;
    char quote = '\0';

    for (std::size_t i = arrayStart + 1; i < arrayEnd; ++i)
    {
        char ch = text[i];

        if (quote == '\0')
        {
            if (ch == '"' || ch == '\'')
            {
                quote = ch;
                current.clear();
            }
        }
        else
        {
            if (ch == quote)
            {
                values.push_back(current);
                quote = '\0';
            }
            else
            {
                current.push_back(ch);
            }
        }
    }

    return values;
}
