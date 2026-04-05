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
class WindowManager;

/**
 * @brief Top-level application object that wires startup, runtime state, and the main loop.
 */
class App
{
public:
    /**
     * @brief Construct an application from raw process arguments.
     * @param argc Argument count from `main`.
     * @param argv Argument vector from `main`.
     */
    App(int argc, char** argv);
    /**
     * @brief Destroy the application and release owned subsystems.
     */
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /**
     * @brief Run the application until shutdown or an exit condition is reached.
     * @return Process-style exit code.
     */
    int run();

    /**
     * @brief Access the visualizer subsystem.
     * @return Mutable visualizer reference.
     */
    VsgVisualizer& visualizer();
    /**
     * @brief Access the visualizer subsystem.
     * @return Immutable visualizer reference.
     */
    const VsgVisualizer& visualizer() const;
    /**
     * @brief Access the input manager subsystem.
     * @return Mutable input manager reference.
     */
    InputManager& inputManager();
    /**
     * @brief Access the input manager subsystem.
     * @return Immutable input manager reference.
     */
    const InputManager& inputManager() const;
    /**
     * @brief Access the window manager subsystem.
     * @return Mutable window manager reference.
     */
    WindowManager& windowManager();
    /**
     * @brief Access the window manager subsystem.
     * @return Immutable window manager reference.
     */
    const WindowManager& windowManager() const;
    /**
     * @brief Access mutable application state.
     * @return Mutable application state reference.
     */
    AppState& state();
    /**
     * @brief Access immutable application state.
     * @return Immutable application state reference.
     */
    const AppState& state() const;
    /**
     * @brief Refresh UI-local evaluation state from the current application state.
     */
    void refreshUiState();
    /**
     * @brief Load a scene file while preserving current UI-local state.
     * @param filename Scene file path to load.
     */
    void loadSceneFile(const std::string& filename);

    /**
     * @brief Execute a command request and emit the result as structured output.
     * @param command Parsed command request to execute.
     * @return Process-style exit code for the command.
     */
    int executeCommand(const CommandRequest& command);
    /**
     * @brief Execute a query request and emit the result as structured output.
     * @param query Parsed query request to execute.
     * @return Process-style exit code for the query.
     */
    int executeQuery(const QueryRequest& query);

private:
    bool applyStateRequests();
    int executeTimedAction(const TimedScriptAction& action);

    int _argc;
    char** _argv;
    int _numFrames = -1;
    AppState _state;

    std::unique_ptr<InputManager> _inputManager;
    std::unique_ptr<WindowManager> _windowManager;
    std::unique_ptr<VsgVisualizer> _visualizer;
    std::unique_ptr<UiLayer> _uiLayer;
    std::optional<CommandRequest> _startupCommand;
    std::optional<QueryRequest> _startupQuery;
    std::unique_ptr<ScriptRunner> _scriptRunner;
    std::vector<TimedScriptAction> _scheduledScriptActions;
    std::size_t _scheduledScriptIndex = 0;
};
