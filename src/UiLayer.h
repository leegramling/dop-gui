#pragma once

#include "AppState.h"

#include <vsg/all.h>
#include <vsgImGui/RenderImGui.h>

/**
 * @brief Live ImGui overlay integration.
 */
class UiLayer
{
public:
    /**
     * Default construct a UI layer.
     */
    UiLayer() = default;

    UiLayer(const UiLayer&) = delete;
    UiLayer& operator=(const UiLayer&) = delete;

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
        class WindowManager& windowManager);
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
    class WindowManager* _windowManager = nullptr;
    vsg::ref_ptr<vsgImGui::RenderImGui> _renderImGui;
    vsg::ref_ptr<vsg::Visitor> _sendEventsToImGui;
};
