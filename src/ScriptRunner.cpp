#include "ScriptRunner.h"

#include "App.h"
#include "Command.h"
#include "Query.h"
#include "VsgVisualizer.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
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

std::size_t findKey(const std::string& text, const std::string& key)
{
    const auto position = text.find(key);
    if (position == std::string::npos) throw std::runtime_error("Missing script key: " + key);
    return position;
}

std::string extractBalanced(const std::string& text, std::size_t start, char openChar, char closeChar)
{
    if (start >= text.size() || text[start] != openChar)
    {
        throw std::runtime_error("Invalid balanced block extraction.");
    }

    int depth = 0;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (std::size_t i = start; i < text.size(); ++i)
    {
        const char ch = text[i];

        if (ch == '\'' && !inDoubleQuote) inSingleQuote = !inSingleQuote;
        if (ch == '"' && !inSingleQuote) inDoubleQuote = !inDoubleQuote;
        if (inSingleQuote || inDoubleQuote) continue;

        if (ch == openChar) ++depth;
        if (ch == closeChar)
        {
            --depth;
            if (depth == 0) return text.substr(start, i - start + 1);
        }
    }

    throw std::runtime_error("Unbalanced script block.");
}

std::string extractArrayBlock(const std::string& text, const std::string& key)
{
    const auto keyPos = findKey(text, key);
    const auto openPos = text.find('[', keyPos);
    if (openPos == std::string::npos) throw std::runtime_error("Missing array block for key: " + key);
    return extractBalanced(text, openPos, '[', ']');
}

std::vector<std::string> extractObjectEntries(const std::string& arrayBlock)
{
    std::vector<std::string> objects;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    int depth = 0;
    std::size_t objectStart = std::string::npos;

    for (std::size_t i = 0; i < arrayBlock.size(); ++i)
    {
        const char ch = arrayBlock[i];
        if (ch == '\'' && !inDoubleQuote) inSingleQuote = !inSingleQuote;
        if (ch == '"' && !inSingleQuote) inDoubleQuote = !inDoubleQuote;
        if (inSingleQuote || inDoubleQuote) continue;

        if (ch == '{')
        {
            if (depth == 0) objectStart = i;
            ++depth;
        }
        else if (ch == '}')
        {
            --depth;
            if (depth == 0 && objectStart != std::string::npos)
            {
                objects.push_back(arrayBlock.substr(objectStart, i - objectStart + 1));
                objectStart = std::string::npos;
            }
        }
    }

    return objects;
}

std::optional<std::string> parseStringField(const std::string& block, const std::string& key)
{
    const std::regex pattern(key + R"__JSON5__(\s*:\s*(?:"([^"]*)"|'([^']*)'))__JSON5__");
    std::smatch match;
    if (!std::regex_search(block, match, pattern)) return std::nullopt;
    return match[1].matched ? match[1].str() : match[2].str();
}

std::optional<std::uint64_t> parseUIntField(const std::string& block, const std::string& key)
{
    const std::regex pattern(key + R"(\s*:\s*([0-9]+))");
    std::smatch match;
    if (!std::regex_search(block, match, pattern)) return std::nullopt;
    return static_cast<std::uint64_t>(std::stoull(match[1].str()));
}
}

ScriptRequest ScriptRunner::parse(const std::string& filename) const
{
    auto text = stripComments(readFile(filename));
    ScriptRequest request{
        .commands = parseStringArray(text, "commands"),
        .queries = parseStringArray(text, "queries"),
    };

    if (text.find("actions") != std::string::npos)
    {
        for (const auto& block : extractObjectEntries(extractArrayBlock(text, "actions")))
        {
            if (auto command = parseStringField(block, "command"))
            {
                request.actions.push_back(TimedScriptAction{
                    .kind = TimedScriptAction::Kind::command,
                    .text = *command,
                });
                continue;
            }

            if (auto query = parseStringField(block, "query"))
            {
                request.actions.push_back(TimedScriptAction{
                    .kind = TimedScriptAction::Kind::query,
                    .text = *query,
                });
                continue;
            }

            if (auto sleepMs = parseUIntField(block, "sleepMs"))
            {
                request.actions.push_back(TimedScriptAction{
                    .kind = TimedScriptAction::Kind::command,
                    .text = "sleep.ms=" + std::to_string(*sleepMs),
                });
                continue;
            }

            throw std::runtime_error("Unsupported script action block: " + block);
        }
    }

    return request;
}

int ScriptRunner::execute(App& app, const ScriptRequest& request) const
{
    if (!request.actions.empty())
    {
        std::ostringstream json;
        json << "{"
             << "\"ok\":true,"
             << "\"actions\":[";

        bool first = true;
        for (const auto& action : createTimedActions(request))
        {
            if (!first) json << ",";
            first = false;

            if (action.kind == TimedScriptAction::Kind::command)
            {
                auto command = parseCommandRequest(action.text);
                if (!command) continue;
                auto result = executeCommand(app, *command);
                app.refreshUiState();
                if (std::holds_alternative<CommandSuccess>(result) && app.visualizer().isInitialized())
                {
                    app.visualizer().syncFromState(app.state());
                }
                json << serializeCommandResult(result);
                continue;
            }

            auto query = parseQueryRequest(action.text);
            if (!query) continue;
            auto result = executeQuery(app, *query);
            json << serializeQueryResult(result);
        }

        json << "]}";
        std::cout << json.str() << '\n';
        return 0;
    }

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
        app.refreshUiState();
        if (std::holds_alternative<CommandSuccess>(result) && app.visualizer().isInitialized())
        {
            app.visualizer().syncFromState(app.state());
        }
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

std::vector<TimedScriptAction> ScriptRunner::createTimedActions(const ScriptRequest& request) const
{
    if (!request.actions.empty())
    {
        std::vector<TimedScriptAction> actions;
        std::uint64_t offsetMs = 0;

        for (const auto& action : request.actions)
        {
            if (action.kind == TimedScriptAction::Kind::command)
            {
                auto command = parseCommandRequest(action.text);
                if (!command) continue;
                const auto* byPath = std::get_if<CommandByPath>(&*command);
                if (!byPath) continue;

                if (byPath->canonicalPath == "sleep.ms")
                {
                    if (byPath->numericArgs.size() != 1)
                    {
                        throw std::runtime_error("sleep.ms requires exactly 1 numeric argument in scripts.");
                    }

                    const auto sleepMs = static_cast<int>(byPath->numericArgs[0]);
                    if (sleepMs < 0) throw std::runtime_error("sleep.ms requires a non-negative duration.");
                    offsetMs += static_cast<std::uint64_t>(sleepMs);
                    continue;
                }
            }

            auto scheduled = action;
            scheduled.offsetMs = offsetMs;
            actions.push_back(std::move(scheduled));
        }

        return actions;
    }

    std::vector<TimedScriptAction> actions;
    std::uint64_t offsetMs = 0;

    for (const auto& commandText : request.commands)
    {
        auto command = parseCommandRequest(commandText);
        if (!command) continue;

        const auto* byPath = std::get_if<CommandByPath>(&*command);
        if (!byPath) continue;

        if (byPath->canonicalPath == "sleep.ms")
        {
            if (byPath->numericArgs.size() != 1)
            {
                throw std::runtime_error("sleep.ms requires exactly 1 numeric argument in scripts.");
            }

            const auto sleepMs = static_cast<int>(byPath->numericArgs[0]);
            if (sleepMs < 0) throw std::runtime_error("sleep.ms requires a non-negative duration.");
            offsetMs += static_cast<std::uint64_t>(sleepMs);
            continue;
        }

        actions.push_back(TimedScriptAction{
            .kind = TimedScriptAction::Kind::command,
            .text = commandText,
            .offsetMs = offsetMs,
        });
    }

    for (const auto& queryText : request.queries)
    {
        actions.push_back(TimedScriptAction{
            .kind = TimedScriptAction::Kind::query,
            .text = queryText,
            .offsetMs = offsetMs,
        });
    }

    return actions;
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
