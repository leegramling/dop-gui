#pragma once

#include "AppState.h"

#include <vsg/all.h>

class VsgVisualizer
{
public:
    VsgVisualizer() = default;

    VsgVisualizer(const VsgVisualizer&) = delete;
    VsgVisualizer& operator=(const VsgVisualizer&) = delete;

    void initialize(const AppState& state, vsg::ref_ptr<vsg::Window> window);
    void connect(vsg::ref_ptr<vsg::Viewer> viewer);

    vsg::ref_ptr<vsg::Camera> camera() const;
    vsg::ref_ptr<vsg::CommandGraph> commandGraph() const;

private:
    vsg::ref_ptr<vsg::BindGraphicsPipeline> createBindGraphicsPipeline() const;
    vsg::ref_ptr<vsg::VertexIndexDraw> createDrawCommands() const;
    vsg::ref_ptr<vsg::Group> createScene() const;

    vsg::ref_ptr<vsg::Group> _scene;
    vsg::ref_ptr<vsg::Camera> _camera;
    vsg::ref_ptr<vsg::CommandGraph> _commandGraph;
    vsg::ref_ptr<vsg::View> _view;
    vsg::ref_ptr<vsg::RenderGraph> _renderGraph;
};
