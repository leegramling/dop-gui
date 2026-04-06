#pragma once

#include "AppState.h"

#include <vsg/all.h>

#include <vector>

/**
 * @brief VSG scene creation and synchronization boundary.
 */
class VsgVisualizer
{
public:
    /**
     * Default construct the visualizer.
     */
    VsgVisualizer() = default;

    VsgVisualizer(const VsgVisualizer&) = delete;
    VsgVisualizer& operator=(const VsgVisualizer&) = delete;

    /**
     * @brief Initialize VSG resources from application state.
     * @param state Application state used to build the initial scene and camera.
     * @param window Target window used for render resources.
     */
    void initialize(const AppState& state, vsg::ref_ptr<vsg::Window> window);
    /**
     * @brief Connect the visualizer to a viewer.
     * @param viewer Viewer that should execute the visualizer command graph.
     */
    void connect(vsg::ref_ptr<vsg::Viewer> viewer);
    /**
     * @brief Sync camera and scene state from the application model.
     * @param state Application state to mirror into render resources.
     */
    void syncFromState(const AppState& state);
    /**
     * @brief Sync only scene state from the application model.
     * @param state Application state to mirror into scene render resources.
     */
    void syncSceneFromState(const AppState& state);
    /**
     * @brief Sync only camera state from the application model.
     * @param state Application state to mirror into camera resources.
     */
    void syncCameraFromState(const AppState& state);
    /**
     * @brief Return whether the visualizer has been initialized.
     * @return True when render resources are ready.
     */
    bool isInitialized() const;

    /**
     * @brief Access the active camera.
     * @return Active camera handle.
     */
    vsg::ref_ptr<vsg::Camera> camera() const;
    /**
     * @brief Access the command graph.
     * @return Command graph used for rendering.
     */
    vsg::ref_ptr<vsg::CommandGraph> commandGraph() const;
    /**
     * @brief Access the render graph.
     * @return Render graph used for scene and UI rendering.
     */
    vsg::ref_ptr<vsg::RenderGraph> renderGraph() const;

private:
    void rebuildScene(const AppState& state);
    void populateScene(const AppState& state);
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
    vsg::ref_ptr<vsg::BindGraphicsPipeline> _bindPipeline;
    vsg::ref_ptr<vsg::MatrixTransform> _gridTransform;
    std::vector<vsg::ref_ptr<vsg::MatrixTransform>> _objectTransforms;
};
