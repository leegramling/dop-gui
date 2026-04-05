#include "Command.h"

#include "App.h"
#include "AppState.h"

#include <functional>
#include <sstream>
#include <stdexcept>

namespace
{
using CommandHandler = std::function<std::string(App&, const CommandByPath&)>;

struct CommandRoute
{
    std::string prefix;
    CommandHandler execute;
};

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

std::vector<std::string> splitString(std::string_view text, char delimiter)
{
    std::vector<std::string> parts;
    std::string current;

    for (char ch : text)
    {
        if (ch == delimiter)
        {
            parts.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }

    parts.push_back(current);
    return parts;
}

std::vector<double> parseNumericArgs(std::string_view text)
{
    std::vector<double> values;
    if (text.empty()) return values;

    for (const auto& part : splitString(text, ','))
    {
        if (part.empty()) throw std::runtime_error("Invalid empty numeric command argument.");
        values.push_back(std::stod(part));
    }

    return values;
}

bool isPrefixMatch(std::string_view prefix, std::string_view path)
{
    if (path == prefix) return true;
    if (!prefix.empty() && prefix.back() == '.')
    {
        return path.size() > prefix.size() && path.compare(0, prefix.size(), prefix) == 0;
    }
    if (path.size() <= prefix.size()) return false;
    if (path.compare(0, prefix.size(), prefix) != 0) return false;
    return path[prefix.size()] == '.';
}

std::string commandPath(const std::string& name)
{
    auto separator = name.find('=');
    if (separator == std::string::npos) return name;
    return name.substr(0, separator);
}

std::vector<double> commandArgs(const std::string& name)
{
    auto separator = name.find('=');
    if (separator == std::string::npos) return {};
    return parseNumericArgs(name.substr(separator + 1));
}

std::string canonicalizeCommandPath(const std::string& name)
{
    auto path = commandPath(name);

    if (path == "noop") return path;
    if (path == "help") return path;
    if (path == "state.reset.bootstrap") return path;

    constexpr std::string_view translatePrefix = "data.scene.object.";
    if (path.rfind(translatePrefix.data(), 0) == 0)
    {
        auto suffix = path.substr(translatePrefix.size());
        auto translateAt = suffix.rfind(".translate");
        if (translateAt != std::string::npos && translateAt + std::string(".translate").size() == suffix.size())
        {
            return path;
        }
    }

    if (path == "view.camera.set_pose") return path;

    throw std::runtime_error("Unknown command: " + name);
}

std::string executeHelp(App&, const CommandByPath&)
{
    std::ostringstream json;
    json << "{\"commands\":[";

    auto names = commandNames();
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        if (i > 0) json << ",";
        json << "\"" << escapeJson(names[i]) << "\"";
    }

    json << "]}";
    return json.str();
}

std::string executeNoOp(App&, const CommandByPath&)
{
    return "{}";
}

std::string executeResetBootstrap(App& app, const CommandByPath&)
{
    app.state() = createBootstrapAppState();
    return "{\"state\":\"bootstrap_reset\"}";
}

std::string executeSceneTranslate(App& app, const CommandByPath& command)
{
    constexpr std::string_view prefix = "data.scene.object.";
    auto suffix = command.canonicalPath.substr(prefix.size());
    auto translateAt = suffix.rfind(".translate");
    auto id = suffix.substr(0, translateAt);

    if (command.numericArgs.size() != 3)
    {
        throw std::runtime_error("Scene object translate command requires exactly 3 numeric arguments.");
    }

    auto* object = findSceneObject(app.state().scene, id);
    if (!object) throw std::runtime_error("Unknown scene object: " + id);

    object->position.x += command.numericArgs[0];
    object->position.y += command.numericArgs[1];
    object->position.z += command.numericArgs[2];

    std::ostringstream json;
    json << "{"
         << "\"id\":\"" << escapeJson(object->id) << "\","
         << "\"position\":[" << object->position.x << "," << object->position.y << "," << object->position.z << "]"
         << "}";
    return json.str();
}

std::string executeSetCameraPose(App& app, const CommandByPath& command)
{
    if (command.numericArgs.size() != 9)
    {
        throw std::runtime_error("view.camera.set_pose requires exactly 9 numeric arguments.");
    }

    auto& pose = app.state().view.cameraPose;
    pose.eye = {command.numericArgs[0], command.numericArgs[1], command.numericArgs[2]};
    pose.center = {command.numericArgs[3], command.numericArgs[4], command.numericArgs[5]};
    pose.up = {command.numericArgs[6], command.numericArgs[7], command.numericArgs[8]};

    std::ostringstream json;
    json << "{"
         << "\"eye\":[" << pose.eye.x << "," << pose.eye.y << "," << pose.eye.z << "],"
         << "\"center\":[" << pose.center.x << "," << pose.center.y << "," << pose.center.z << "],"
         << "\"up\":[" << pose.up.x << "," << pose.up.y << "," << pose.up.z << "]"
         << "}";
    return json.str();
}

const std::vector<CommandRoute>& commandRoutes()
{
    static const std::vector<CommandRoute> routes{
        CommandRoute{.prefix = "help", .execute = executeHelp},
        CommandRoute{.prefix = "noop", .execute = executeNoOp},
        CommandRoute{.prefix = "state.reset.bootstrap", .execute = executeResetBootstrap},
        CommandRoute{.prefix = "data.scene.object.", .execute = executeSceneTranslate},
        CommandRoute{.prefix = "view.camera.set_pose", .execute = executeSetCameraPose},
    };
    return routes;
}
}

std::optional<CommandRequest> parseCommandRequest(const std::string& name)
{
    if (name.empty()) return std::nullopt;
    return CommandByPath{
        .commandName = name,
        .canonicalPath = canonicalizeCommandPath(name),
        .numericArgs = commandArgs(name),
    };
}

bool requiresInitializedApp(const CommandRequest&)
{
    return false;
}

CommandResult executeCommand(App& app, const CommandRequest& request)
{
    return std::visit(
        [&](const auto& command) -> CommandResult
        {
            using T = std::decay_t<decltype(command)>;

            if constexpr (std::is_same_v<T, CommandByPath>)
            {
                for (const auto& route : commandRoutes())
                {
                    if (isPrefixMatch(route.prefix, command.canonicalPath))
                    {
                        return CommandSuccess{
                            .commandName = command.commandName,
                            .continueRunning = false,
                            .exitCode = 0,
                            .jsonValue = route.execute(app, command),
                        };
                    }
                }

                throw std::runtime_error("Unknown command path: " + command.canonicalPath);
            }
        },
        request);
}

std::string serializeCommandResult(const CommandResult& result)
{
    return std::visit(
        [](const auto& value) -> std::string
        {
            using T = std::decay_t<decltype(value)>;
            std::ostringstream json;

            if constexpr (std::is_same_v<T, CommandSuccess>)
            {
                json << "{"
                     << "\"ok\":true,"
                     << "\"command\":\"" << escapeJson(value.commandName) << "\","
                     << "\"value\":" << value.jsonValue
                     << "}";
            }
            else if constexpr (std::is_same_v<T, CommandError>)
            {
                json << "{"
                     << "\"ok\":false,"
                     << "\"command\":\"" << escapeJson(value.commandName) << "\","
                     << "\"error\":{"
                     << "\"message\":\"" << escapeJson(value.message) << "\""
                     << "}}";
            }

            return json.str();
        },
        result);
}

std::vector<std::string> commandNames()
{
    return {
        "noop",
        "help",
        "state.reset.bootstrap",
        "data.scene.object.<id>.translate=<dx>,<dy>,<dz>",
        "view.camera.set_pose=<eyeX>,<eyeY>,<eyeZ>,<centerX>,<centerY>,<centerZ>,<upX>,<upY>,<upZ>",
    };
}
