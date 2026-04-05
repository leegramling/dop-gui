#include "App.h"

#include "Command.h"
#include "InputManager.h"
#include "Query.h"
#include "ScriptRunner.h"
#include "VsgVisualizer.h"

#include <vsg/all.h>

#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
std::string escapeJson(const std::string& value)
{
    std::string out;
    out.reserve(value.size());
    for (char ch : value)
    {
        switch (ch)
        {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(ch); break;
        }
    }
    return out;
}

bool isMachineMode(const std::string& commandName, const std::string& queryName, const std::string& scriptFilename)
{
    return !commandName.empty() || !queryName.empty() || !scriptFilename.empty();
}

void printJsonError(const std::string& type, const std::string& message)
{
    std::cerr << "{\"ok\":false,\"error\":{\"type\":\"" << escapeJson(type)
              << "\",\"message\":\"" << escapeJson(message) << "\"}}\n";
}

bool consumesNextArgument(std::string_view argument)
{
    return argument == "-f" ||
           argument == "--command" ||
           argument == "--query" ||
           argument == "--script" ||
           argument == "--startup-delay-ms";
}

std::string findScenePathArgument(int argc, char** argv)
{
    bool skipNext = false;
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];

        if (skipNext)
        {
            skipNext = false;
            continue;
        }

        if (consumesNextArgument(argument))
        {
            skipNext = true;
            continue;
        }

        if (!argument.empty() && argument[0] != '-')
        {
            return argument;
        }
    }

    return {};
}
}

App::App(int argc, char** argv) :
    _argc(argc),
    _argv(argv),
    _state(),
    _inputManager(std::make_unique<InputManager>()),
    _visualizer(std::make_unique<VsgVisualizer>()),
    _scriptRunner(std::make_unique<ScriptRunner>())
{
}

App::~App() = default;

int App::run()
{
    std::string commandName;
    std::string queryName;
    std::string scriptFilename;
    bool stayOpen = false;
    int startupDelayMs = 0;

    try
    {
        const auto scenePath = findScenePathArgument(_argc, _argv);
        _state = scenePath.empty() ? createBootstrapAppState() : loadAppStateFromSceneFile(scenePath);

        vsg::CommandLine arguments(&_argc, _argv);

        _numFrames = arguments.value(-1, "-f");
        startupDelayMs = arguments.value(0, "--startup-delay-ms");
        commandName = arguments.value(std::string{}, "--command");
        queryName = arguments.value(std::string{}, "--query");
        scriptFilename = arguments.value(std::string{}, "--script");
        stayOpen = arguments.read("--stay-open");

        _inputManager->configure(arguments);
        _startupCommand = parseCommandRequest(commandName);
        _startupQuery = parseQueryRequest(queryName);

        if (arguments.errors())
        {
            return arguments.writeErrorMessages(std::cerr);
        }

        std::optional<ScriptRequest> startupScript;
        if (!scriptFilename.empty())
        {
            startupScript = _scriptRunner->parse(scriptFilename);
            if (!stayOpen) return _scriptRunner->execute(*this, *startupScript);
        }

        if (!stayOpen)
        {
            if (_startupCommand && !requiresInitializedApp(*_startupCommand)) return executeCommand(*_startupCommand);
            if (_startupQuery) return executeQuery(*_startupQuery);
        }

        auto viewer = vsg::Viewer::create();
        auto window = _inputManager->createWindow();
        if (!window)
        {
            std::cerr << "Unable to create VSG window.\n";
            return 1;
        }

        viewer->addWindow(window);

        _visualizer->initialize(_state, window);
        _inputManager->attachDefaultHandlers(viewer, _visualizer->camera());
        _visualizer->connect(viewer);

        auto executeStartupActions = [&]() -> int
        {
            if (startupScript)
            {
                auto exitCode = _scriptRunner->execute(*this, *startupScript);
                startupScript.reset();
                if (!stayOpen) return exitCode;
            }
            if (_startupCommand)
            {
                auto exitCode = executeCommand(*_startupCommand);
                _startupCommand.reset();
                if (!stayOpen) return exitCode;
            }
            if (_startupQuery)
            {
                auto exitCode = executeQuery(*_startupQuery);
                _startupQuery.reset();
                if (!stayOpen) return exitCode;
            }
            return 0;
        };

        const bool hasStartupActions = startupScript.has_value() || _startupCommand.has_value() || _startupQuery.has_value();
        const bool hasDelayedStartupActions = stayOpen && startupDelayMs > 0 &&
            hasStartupActions;
        auto startupDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(startupDelayMs);

        if (hasStartupActions && !hasDelayedStartupActions)
        {
            auto exitCode = executeStartupActions();
            if (!stayOpen) return exitCode;
        }

        viewer->compile();

        while (viewer->advanceToNextFrame() && (_numFrames < 0 || (_numFrames--) > 0))
        {
            if (hasDelayedStartupActions && (startupScript || _startupCommand || _startupQuery) &&
                std::chrono::steady_clock::now() >= startupDeadline)
            {
                auto exitCode = executeStartupActions();
                if (!stayOpen) return exitCode;
            }
            viewer->handleEvents();
            viewer->update();
            viewer->recordAndSubmit();
            viewer->present();
        }

        return 0;
    }
    catch (const vsg::Exception& exception)
    {
        if (isMachineMode(commandName, queryName, scriptFilename))
        {
            printJsonError("vsg_exception", exception.message + " result=" + std::to_string(exception.result));
        }
        else
        {
            std::cerr << "[vsg::Exception] " << exception.message << " result=" << exception.result << '\n';
        }
        return 1;
    }
    catch (const std::exception& exception)
    {
        if (isMachineMode(commandName, queryName, scriptFilename))
        {
            printJsonError("std_exception", exception.what());
        }
        else
        {
            std::cerr << "[std::exception] " << exception.what() << '\n';
        }
        return 1;
    }
}

VsgVisualizer& App::visualizer()
{
    return *_visualizer;
}

const VsgVisualizer& App::visualizer() const
{
    return *_visualizer;
}

InputManager& App::inputManager()
{
    return *_inputManager;
}

const InputManager& App::inputManager() const
{
    return *_inputManager;
}

AppState& App::state()
{
    return _state;
}

const AppState& App::state() const
{
    return _state;
}

int App::executeCommand(const CommandRequest& command)
{
    auto result = ::executeCommand(*this, command);
    if (std::holds_alternative<CommandSuccess>(result) && _visualizer->isInitialized())
    {
        _visualizer->syncFromState(_state);
    }
    std::cout << serializeCommandResult(result) << '\n';
    if (const auto* error = std::get_if<CommandError>(&result)) return error->exitCode;
    if (const auto* success = std::get_if<CommandSuccess>(&result))
    {
        if (!success->continueRunning) return success->exitCode;
    }
    return 0;
}

int App::executeQuery(const QueryRequest& query)
{
    auto result = ::executeQuery(*this, query);
    std::cout << serializeQueryResult(result) << '\n';
    if (const auto* error = std::get_if<QueryError>(&result)) return 1;
    return 0;
}
