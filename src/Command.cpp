#include "Command.h"

#include "App.h"
#include "AppState.h"

#include <chrono>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <thread>

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
    try
    {
        return parseNumericArgs(name.substr(separator + 1));
    }
    catch (const std::exception&)
    {
        return {};
    }
}

std::string commandRawArg(const std::string& name)
{
    auto separator = name.find('=');
    if (separator == std::string::npos) return {};
    return name.substr(separator + 1);
}

std::string canonicalizeCommandPath(const std::string& name)
{
    auto path = commandPath(name);

    if (path == "noop") return path;
    if (path == "help") return path;
    if (path == "app.exit") return path;
    if (path == "state.reset.bootstrap") return path;
    if (path == "scene.load") return path;
    if (path == "scene.load.cubes") return path;
    if (path == "scene.load.shapes") return path;
    if (path == "scene.select_object") return path;
    if (path == "ui.grid.set_visible") return path;
    if (path == "view.background.set_hex") return path;
    if (path == "sleep.ms") return path;

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

    constexpr std::string_view uiClickPrefix = "ui.test.click.";
    constexpr std::string_view uiSetBoolPrefix = "ui.test.set_bool.";
    constexpr std::string_view uiSetTextPrefix = "ui.test.set_text.";

    if (path.rfind(uiClickPrefix.data(), 0) == 0 && path.size() > uiClickPrefix.size()) return path;
    if (path.rfind(uiSetBoolPrefix.data(), 0) == 0 && path.size() > uiSetBoolPrefix.size()) return path;
    if (path.rfind(uiSetTextPrefix.data(), 0) == 0 && path.size() > uiSetTextPrefix.size()) return path;

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

std::string executeAppExit(App& app, const CommandByPath&)
{
    app.state().ui.exitRequested = true;
    return "{\"exitRequested\":true}";
}

std::string executeResetBootstrap(App& app, const CommandByPath&)
{
    app.state() = createBootstrapAppState();
    return "{\"state\":\"bootstrap_reset\"}";
}

std::string sceneLoadResult(const App& app, const std::string& sceneName)
{
    std::ostringstream json;
    json << "{"
         << "\"scene\":\"" << escapeJson(sceneName) << "\","
         << "\"objectCount\":" << app.state().scene.objects.size() << ","
         << "\"objectIds\":[";
    for (std::size_t i = 0; i < app.state().scene.objects.size(); ++i)
    {
        if (i > 0) json << ",";
        json << "\"" << escapeJson(app.state().scene.objects[i].id) << "\"";
    }
    json << "]}";
    return json.str();
}

std::string executeLoadCubes(App& app, const CommandByPath&)
{
    app.loadSceneFile(std::string(DOP_GUI_SOURCE_DIR) + "/scenes/cubes.json5");
    return sceneLoadResult(app, "cubes");
}

std::string executeLoadShapes(App& app, const CommandByPath&)
{
    app.loadSceneFile(std::string(DOP_GUI_SOURCE_DIR) + "/scenes/shapes.json5");
    return sceneLoadResult(app, "shapes");
}

std::string executeLoadScene(App& app, const CommandByPath& command)
{
    if (command.rawArg.empty()) throw std::runtime_error("scene.load requires a scene name argument.");
    if (command.rawArg == "bootstrap")
    {
        app.loadSceneFile(std::string(DOP_GUI_SOURCE_DIR) + "/scenes/bootstrap_scene.json5");
        return sceneLoadResult(app, "bootstrap");
    }
    if (command.rawArg == "cubes")
    {
        app.loadSceneFile(std::string(DOP_GUI_SOURCE_DIR) + "/scenes/cubes.json5");
        return sceneLoadResult(app, "cubes");
    }
    if (command.rawArg == "shapes")
    {
        app.loadSceneFile(std::string(DOP_GUI_SOURCE_DIR) + "/scenes/shapes.json5");
        return sceneLoadResult(app, "shapes");
    }

    throw std::runtime_error("Unknown scene.load target: " + command.rawArg);
}

std::string executeSelectObject(App& app, const CommandByPath& command)
{
    if (command.rawArg.empty()) throw std::runtime_error("scene.select_object requires an object id argument.");
    auto* object = findSceneObject(app.state().scene, command.rawArg);
    if (!object) throw std::runtime_error("Unknown scene object: " + command.rawArg);
    app.state().scene.selectedObjectId = object->id;
    return "{\"selectedObjectId\":\"" + escapeJson(object->id) + "\"}";
}

std::string executeSetGridVisible(App& app, const CommandByPath& command)
{
    bool visible = false;
    if (command.rawArg == "1" || command.rawArg == "true") visible = true;
    else if (command.rawArg == "0" || command.rawArg == "false") visible = false;
    else throw std::runtime_error("ui.grid.set_visible requires true/false or 1/0.");
    app.state().ui.displayGrid = visible;
    return std::string("{\"displayGrid\":") + (visible ? "true}" : "false}");
}

std::string executeSetBackgroundHex(App& app, const CommandByPath& command)
{
    if (command.rawArg.empty()) throw std::runtime_error("view.background.set_hex requires a hex string.");
    vsg::vec4 parsed{};
    if (!tryParseHexColor(command.rawArg, parsed))
    {
        throw std::runtime_error("view.background.set_hex requires a valid hex color.");
    }
    app.state().view.backgroundColorHex = command.rawArg;
    app.state().view.backgroundColor = parsed;
    return "{\"hex\":\"" + escapeJson(command.rawArg) + "\"}";
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

std::string executeSleep(App&, const CommandByPath& command)
{
    if (command.numericArgs.size() != 1)
    {
        throw std::runtime_error("sleep.ms requires exactly 1 numeric argument.");
    }

    const auto sleepMs = static_cast<int>(command.numericArgs[0]);
    if (sleepMs < 0) throw std::runtime_error("sleep.ms requires a non-negative duration.");
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));

    return "{\"sleptMs\":" + std::to_string(sleepMs) + "}";
}

std::string executeUiClick(App& app, const CommandByPath& command)
{
    constexpr std::string_view prefix = "ui.test.click.";
    const auto label = command.canonicalPath.substr(prefix.size());
    app.state().ui.pendingActions.push_back(UiTestAction{
        .label = label,
        .kind = "click",
    });

    return "{\"label\":\"" + escapeJson(label) + "\",\"kind\":\"click\"}";
}

std::string executeUiSetBool(App& app, const CommandByPath& command)
{
    constexpr std::string_view prefix = "ui.test.set_bool.";
    const auto label = command.canonicalPath.substr(prefix.size());

    bool boolValue = false;
    if (command.rawArg == "1" || command.rawArg == "true") boolValue = true;
    else if (command.rawArg == "0" || command.rawArg == "false") boolValue = false;
    else throw std::runtime_error("ui.test.set_bool requires true/false or 1/0.");

    app.state().ui.pendingActions.push_back(UiTestAction{
        .label = label,
        .kind = "set_bool",
        .boolValue = boolValue,
    });

    return "{\"label\":\"" + escapeJson(label) + "\",\"kind\":\"set_bool\",\"value\":" + (boolValue ? "true" : "false") + "}";
}

std::string executeUiSetText(App& app, const CommandByPath& command)
{
    constexpr std::string_view prefix = "ui.test.set_text.";
    const auto label = command.canonicalPath.substr(prefix.size());
    app.state().ui.pendingActions.push_back(UiTestAction{
        .label = label,
        .kind = "set_text",
        .textValue = command.rawArg,
    });

    return "{\"label\":\"" + escapeJson(label) + "\",\"kind\":\"set_text\",\"value\":\"" + escapeJson(command.rawArg) + "\"}";
}

const std::vector<CommandRoute>& commandRoutes()
{
    static const std::vector<CommandRoute> routes{
        CommandRoute{.prefix = "help", .execute = executeHelp},
        CommandRoute{.prefix = "noop", .execute = executeNoOp},
        CommandRoute{.prefix = "app.exit", .execute = executeAppExit},
        CommandRoute{.prefix = "state.reset.bootstrap", .execute = executeResetBootstrap},
        CommandRoute{.prefix = "scene.load", .execute = executeLoadScene},
        CommandRoute{.prefix = "scene.load.cubes", .execute = executeLoadCubes},
        CommandRoute{.prefix = "scene.load.shapes", .execute = executeLoadShapes},
        CommandRoute{.prefix = "scene.select_object", .execute = executeSelectObject},
        CommandRoute{.prefix = "ui.grid.set_visible", .execute = executeSetGridVisible},
        CommandRoute{.prefix = "view.background.set_hex", .execute = executeSetBackgroundHex},
        CommandRoute{.prefix = "data.scene.object.", .execute = executeSceneTranslate},
        CommandRoute{.prefix = "view.camera.set_pose", .execute = executeSetCameraPose},
        CommandRoute{.prefix = "sleep.ms", .execute = executeSleep},
        CommandRoute{.prefix = "ui.test.click.", .execute = executeUiClick},
        CommandRoute{.prefix = "ui.test.set_bool.", .execute = executeUiSetBool},
        CommandRoute{.prefix = "ui.test.set_text.", .execute = executeUiSetText},
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
        .rawArg = commandRawArg(name),
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
        "app.exit",
        "state.reset.bootstrap",
        "scene.load=<name>",
        "scene.load.cubes",
        "scene.load.shapes",
        "scene.select_object=<id>",
        "ui.grid.set_visible=<true|false>",
        "view.background.set_hex=<hex>",
        "data.scene.object.<id>.translate=<dx>,<dy>,<dz>",
        "view.camera.set_pose=<eyeX>,<eyeY>,<eyeZ>,<centerX>,<centerY>,<centerZ>,<upX>,<upY>,<upZ>",
        "sleep.ms=<milliseconds>",
        "ui.test.click.<label>",
        "ui.test.set_bool.<label>=<true|false>",
        "ui.test.set_text.<label>=<value>",
    };
}
