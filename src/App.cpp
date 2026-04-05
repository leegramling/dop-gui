#include "App.h"

#include "Command.h"
#include "InputManager.h"
#include "Query.h"
#include "ScriptRunner.h"
#include "VsgVisualizer.h"

#include <vsg/all.h>

#include <iostream>
#include <stdexcept>
#include <string>

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
}

App::App(int argc, char** argv) :
    _argc(argc),
    _argv(argv),
    _state(createBootstrapAppState()),
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

    try
    {
        vsg::CommandLine arguments(&_argc, _argv);

        _numFrames = arguments.value(-1, "-f");
        commandName = arguments.value(std::string{}, "--command");
        queryName = arguments.value(std::string{}, "--query");
        scriptFilename = arguments.value(std::string{}, "--script");

        _inputManager->configure(arguments);
        _startupCommand = parseCommandRequest(commandName);
        _startupQuery = parseQueryRequest(queryName);

        if (arguments.errors())
        {
            return arguments.writeErrorMessages(std::cerr);
        }

        if (!scriptFilename.empty())
        {
            auto request = _scriptRunner->parse(scriptFilename);
            return _scriptRunner->execute(*this, request);
        }

        if (_startupCommand && !requiresInitializedApp(*_startupCommand)) return executeCommand(*_startupCommand);
        if (_startupQuery) return executeQuery(*_startupQuery);

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

        if (_startupCommand) return executeCommand(*_startupCommand);
        if (_startupQuery) return executeQuery(*_startupQuery);

        viewer->compile();

        while (viewer->advanceToNextFrame() && (_numFrames < 0 || (_numFrames--) > 0))
        {
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
