#pragma once

#include "AppState.h"
#include "Command.h"
#include "Query.h"

#include <memory>
#include <optional>
#include <vector>

namespace vsg
{}

class InputManager;
class ScriptRunner;
struct TimedScriptAction;
class UiLayer;
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
    void refreshUiState();
    void loadSceneFile(const std::string& filename);

    int executeCommand(const CommandRequest& command);
    int executeQuery(const QueryRequest& query);

private:
    bool applyStateRequests();
    int executeTimedAction(const TimedScriptAction& action);

    int _argc;
    char** _argv;
    int _numFrames = -1;
    AppState _state;

    std::unique_ptr<InputManager> _inputManager;
    std::unique_ptr<VsgVisualizer> _visualizer;
    std::unique_ptr<UiLayer> _uiLayer;
    std::optional<CommandRequest> _startupCommand;
    std::optional<QueryRequest> _startupQuery;
    std::unique_ptr<ScriptRunner> _scriptRunner;
    std::vector<TimedScriptAction> _scheduledScriptActions;
    std::size_t _scheduledScriptIndex = 0;
};
