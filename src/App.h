#pragma once

#include "AppState.h"
#include "Command.h"
#include "Query.h"

#include <memory>
#include <optional>

namespace vsg
{}

class InputManager;
class ScriptRunner;
class VsgVisualizer;

class App
{
public:
    App(int argc, char** argv);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    int run();

    VsgVisualizer& visualizer();
    const VsgVisualizer& visualizer() const;
    InputManager& inputManager();
    const InputManager& inputManager() const;
    AppState& state();
    const AppState& state() const;

    int executeCommand(const CommandRequest& command);
    int executeQuery(const QueryRequest& query);

private:
    int _argc;
    char** _argv;
    int _numFrames = -1;
    AppState _state;

    std::unique_ptr<InputManager> _inputManager;
    std::unique_ptr<VsgVisualizer> _visualizer;
    std::optional<CommandRequest> _startupCommand;
    std::optional<QueryRequest> _startupQuery;
    std::unique_ptr<ScriptRunner> _scriptRunner;
};
