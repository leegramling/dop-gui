#pragma once

#include "AppState.h"

#include <memory>
#include <vector>
#include <vsg/all.h>
#include <vsgImGui/RenderImGui.h>

class Panel;
class WindowManager;

/**
 * @brief Top-level ImGui manager that owns authored panel controllers.
 */
class UiManager
{
public:
    /**
     * @brief Construct a UI manager with its default panel set.
     */
    UiManager();
    /**
     * @brief Destroy the UI manager and owned panel controllers.
     */
    ~UiManager();

    UiManager(const UiManager&) = delete;
    UiManager& operator=(const UiManager&) = delete;

    /**
     * @brief Initialize the live ImGui overlay.
     * @param window Target window for ImGui rendering.
     * @param renderGraph Render graph that receives the ImGui node.
     * @param state Application state used by the overlay.
     * @param windowManager Window lifecycle manager used to observe ImGui platform callbacks.
     */
    void initialize(
        vsg::ref_ptr<vsg::Window> window,
        vsg::ref_ptr<vsg::RenderGraph> renderGraph,
        AppState& state,
        WindowManager& windowManager);
    /**
     * @brief Evaluate the UI in test mode without a live window.
     * @param state Application state to evaluate against.
     */
    void evaluate(AppState& state);
    /**
     * @brief Return whether the live overlay has been initialized.
     * @return True when the live overlay is ready.
     */
    bool isInitialized() const;
    /**
     * @brief Return the ImGui event handler.
     * @return Event handler that forwards input to ImGui.
     */
    vsg::ref_ptr<vsg::Visitor> eventHandler() const;

private:
    void render(AppState& state);

    AppState* _state = nullptr;
    WindowManager* _windowManager = nullptr;
    std::vector<std::unique_ptr<Panel>> _panels;
    vsg::ref_ptr<vsgImGui::RenderImGui> _renderImGui;
    vsg::ref_ptr<vsg::Visitor> _sendEventsToImGui;
};
