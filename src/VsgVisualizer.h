#pragma once

#include "AppState.h"

#include <vsg/all.h>

#include <vector>

class VsgVisualizer
{
public:
    VsgVisualizer() = default;

    VsgVisualizer(const VsgVisualizer&) = delete;
    VsgVisualizer& operator=(const VsgVisualizer&) = delete;

    void initialize(const AppState& state, vsg::ref_ptr<vsg::Window> window);
    void connect(vsg::ref_ptr<vsg::Viewer> viewer);
    void syncFromState(const AppState& state);
    void syncSceneFromState(const AppState& state);
    void syncCameraFromState(const AppState& state);
    bool isInitialized() const;

    vsg::ref_ptr<vsg::Camera> camera() const;
    vsg::ref_ptr<vsg::CommandGraph> commandGraph() const;
    vsg::ref_ptr<vsg::RenderGraph> renderGraph() const;

private:
    vsg::ref_ptr<vsg::BindGraphicsPipeline> createBindGraphicsPipeline() const;
    vsg::ref_ptr<vsg::VertexIndexDraw> createDrawCommands(const SceneObjectState& object) const;
    vsg::ref_ptr<vsg::vec4Array> createColors(const SceneObjectState& object, std::size_t count) const;
    vsg::ref_ptr<vsg::Group> createScene(const AppState& state);

    vsg::ref_ptr<vsg::Group> _scene;
    vsg::ref_ptr<vsg::Camera> _camera;
    vsg::ref_ptr<vsg::LookAt> _lookAt;
    vsg::ref_ptr<vsg::CommandGraph> _commandGraph;
    vsg::ref_ptr<vsg::View> _view;
    vsg::ref_ptr<vsg::RenderGraph> _renderGraph;
    vsg::ref_ptr<vsg::MatrixTransform> _gridTransform;
    std::vector<vsg::ref_ptr<vsg::MatrixTransform>> _objectTransforms;
};
