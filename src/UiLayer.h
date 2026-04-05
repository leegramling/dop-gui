#pragma once

#include "AppState.h"

#include <vsg/all.h>
#include <vsgImGui/RenderImGui.h>

class UiLayer
{
public:
    UiLayer() = default;

    UiLayer(const UiLayer&) = delete;
    UiLayer& operator=(const UiLayer&) = delete;

    void initialize(vsg::ref_ptr<vsg::Window> window, vsg::ref_ptr<vsg::RenderGraph> renderGraph, AppState& state);
    void evaluate(AppState& state);
    bool isInitialized() const;
    vsg::ref_ptr<vsg::Visitor> eventHandler() const;

private:
    void render(AppState& state);

    AppState* _state = nullptr;
    vsg::ref_ptr<vsgImGui::RenderImGui> _renderImGui;
    vsg::ref_ptr<vsg::Visitor> _sendEventsToImGui;
};
