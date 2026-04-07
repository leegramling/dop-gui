#include "App.h"

#include "Command.h"
#include "InputManager.h"
#include "Query.h"
#include "ScriptRunner.h"
#include "UiManager.h"
#include "VsgVisualizer.h"
#include "WindowManager.h"

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
    _windowManager(std::make_unique<WindowManager>()),
    _visualizer(std::make_unique<VsgVisualizer>()),
    _uiManager(std::make_unique<UiManager>()),
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
    bool uiTestMode = false;
    int startupDelayMs = 0;

    try
    {
        const auto scenePath = findScenePathArgument(_argc, _argv);
        _state = scenePath.empty() ? createBootstrapAppState() : loadAppStateFromSceneFile(scenePath);
        _state.ui.layout = loadUiLayoutFromFile(std::string(DOP_GUI_SOURCE_DIR) + "/ui/layout.json5");

        vsg::CommandLine arguments(&_argc, _argv);

        _numFrames = arguments.value(-1, "-f");
        startupDelayMs = arguments.value(0, "--startup-delay-ms");
        commandName = arguments.value(std::string{}, "--command");
        queryName = arguments.value(std::string{}, "--query");
        scriptFilename = arguments.value(std::string{}, "--script");
        stayOpen = arguments.read("--stay-open");
        uiTestMode = arguments.read("--ui-test-mode");

        _inputManager->configure(arguments);
        _startupCommand = parseCommandRequest(commandName);
        _startupQuery = parseQueryRequest(queryName);
        _state.ui.testMode = uiTestMode;
        if (_state.ui.testMode) _uiManager->evaluate(_state);

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
        _windowManager->registerPrimaryWindow(window);
        _windowManager->registerViewer(viewer);

        _visualizer->initialize(_state, window);
        _uiManager->initialize(window, _visualizer->renderGraph(), _state, *_windowManager);
        if (auto uiHandler = _uiManager->eventHandler()) viewer->addEventHandler(uiHandler);
        _inputManager->attachDefaultHandlers(viewer, _visualizer->camera());
        _visualizer->connect(viewer);

        auto executeStartupActions = [&]() -> int
        {
            if (startupScript)
            {
                if (stayOpen)
                {
                    _scheduledScriptActions = _scriptRunner->createTimedActions(*startupScript);
                    _scheduledScriptIndex = 0;
                    startupScript.reset();
                    return 0;
                }

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
        auto scheduledScriptStart = std::chrono::steady_clock::time_point{};

        if (hasStartupActions && !hasDelayedStartupActions)
        {
            auto exitCode = executeStartupActions();
            if (!_scheduledScriptActions.empty()) scheduledScriptStart = std::chrono::steady_clock::now();
            if (!stayOpen) return exitCode;
        }

        viewer->compile();

        auto previousFrameTime = std::chrono::steady_clock::now();
        while (viewer->advanceToNextFrame() && (_numFrames < 0 || (_numFrames--) > 0))
        {
            const auto now = std::chrono::steady_clock::now();
            const auto frameSeconds = std::chrono::duration<double>(now - previousFrameTime).count();
            previousFrameTime = now;
            _state.view.fps = frameSeconds > 0.0 ? (1.0 / frameSeconds) : 0.0;

            if (applyStateRequests() && _visualizer->isInitialized())
            {
                _visualizer->syncFromState(_state);
            }

            if (_state.ui.exitRequested) return 0;
            _visualizer->syncSceneFromState(_state);

            if (hasDelayedStartupActions && (startupScript || _startupCommand || _startupQuery) &&
                std::chrono::steady_clock::now() >= startupDeadline)
            {
                auto exitCode = executeStartupActions();
                if (!_scheduledScriptActions.empty()) scheduledScriptStart = std::chrono::steady_clock::now();
                if (!stayOpen) return exitCode;
            }

            if (!_scheduledScriptActions.empty() && _scheduledScriptIndex < _scheduledScriptActions.size())
            {
                const auto elapsedMs = static_cast<std::uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - scheduledScriptStart)
                        .count());

                while (_scheduledScriptIndex < _scheduledScriptActions.size() &&
                       _scheduledScriptActions[_scheduledScriptIndex].offsetMs <= elapsedMs)
                {
                    auto exitCode = executeTimedAction(_scheduledScriptActions[_scheduledScriptIndex]);
                    ++_scheduledScriptIndex;
                    if (exitCode != 0) return exitCode;
                    if (_state.ui.exitRequested) return 0;
                }
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

WindowManager& App::windowManager()
{
    return *_windowManager;
}

const WindowManager& App::windowManager() const
{
    return *_windowManager;
}

AppState& App::state()
{
    return _state;
}

const AppState& App::state() const
{
    return _state;
}

void App::refreshUiState()
{
    if (_state.ui.testMode)
    {
        _uiManager->evaluate(_state);
        if (applyStateRequests())
        {
            _uiManager->evaluate(_state);
        }
    }
}

bool App::applyStateRequests()
{
    bool changed = false;

    if (!_state.ui.requestedCommands.empty())
    {
        auto commands = _state.ui.requestedCommands;
        _state.ui.requestedCommands.clear();
        for (const auto& commandText : commands)
        {
            auto command = parseCommandRequest(commandText);
            if (!command) continue;
            auto result = ::executeCommand(*this, *command);
            if (const auto* error = std::get_if<CommandError>(&result))
            {
                throw std::runtime_error(error->message);
            }
            changed = true;
        }
    }

    if (_state.ui.requestedSceneFile)
    {
        loadSceneFile(*_state.ui.requestedSceneFile);
        _state.ui.requestedSceneFile.reset();
        changed = true;
    }

    return changed;
}

void App::loadSceneFile(const std::string& filename)
{
    auto preservedUi = _state.ui;
    _state = loadAppStateFromSceneFile(filename);
    _state.ui = preservedUi;
}

int App::executeTimedAction(const TimedScriptAction& action)
{
    if (action.kind == TimedScriptAction::Kind::command)
    {
        auto command = parseCommandRequest(action.text);
        if (!command) return 0;
        return executeCommand(*command);
    }

    auto query = parseQueryRequest(action.text);
    if (!query) return 0;
    return executeQuery(*query);
}

int App::executeCommand(const CommandRequest& command)
{
    auto result = ::executeCommand(*this, command);
    refreshUiState();
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
