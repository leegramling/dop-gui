#include "VsgVisualizer.h"

#include <cmath>
#include <array>
#include <stdexcept>
#include <string>

namespace
{
double degreesToRadians(double degrees)
{
    return degrees * 3.14159265358979323846 / 180.0;
}

vsg::vec4 colorForKind(const std::string& kind)
{
    if (kind == "triangle") return {0.95f, 0.30f, 0.25f, 1.0f};
    if (kind == "rectangle") return {0.20f, 0.70f, 0.95f, 1.0f};
    if (kind == "tristrip") return {0.20f, 0.85f, 0.45f, 1.0f};
    if (kind == "cube") return {0.95f, 0.80f, 0.25f, 1.0f};
    return {0.85f, 0.85f, 0.85f, 1.0f};
}

vsg::ref_ptr<vsg::VertexIndexDraw> createMesh(
    vsg::ref_ptr<vsg::vec3Array> vertices,
    vsg::ref_ptr<vsg::uintArray> indices,
    vsg::ref_ptr<vsg::vec4Array> colors)
{
    vsg::DataList vertexArrays{vertices, colors};
    auto drawCommands = vsg::VertexIndexDraw::create();
    drawCommands->assignArrays(vertexArrays);
    drawCommands->assignIndices(indices);
    drawCommands->indexCount = indices->width();
    drawCommands->instanceCount = 1;
    return drawCommands;
}

vsg::ref_ptr<vsg::Node> createGridGeometry()
{
    auto vertices = vsg::vec3Array::create({
        {-5.0f, -0.03f, -0.01f}, {5.0f, -0.03f, -0.01f}, {5.0f, 0.03f, -0.01f}, {-5.0f, 0.03f, -0.01f},
        {-0.03f, -5.0f, -0.01f}, {0.03f, -5.0f, -0.01f}, {0.03f, 5.0f, -0.01f}, {-0.03f, 5.0f, -0.01f},
    });
    auto colors = vsg::vec4Array::create(8);
    for (std::size_t i = 0; i < 8; ++i) (*colors)[i] = vsg::vec4(0.35f, 0.35f, 0.35f, 1.0f);

    auto indices = vsg::uintArray::create({0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7});
    auto draw = createMesh(vertices, indices, colors);

    auto stateGroup = vsg::StateGroup::create();
    stateGroup->addChild(draw);
    return stateGroup;
}
}

vsg::ref_ptr<vsg::BindGraphicsPipeline> VsgVisualizer::createBindGraphicsPipeline() const
{
    vsg::DescriptorSetLayouts descriptorSetLayouts;
    vsg::PushConstantRanges pushConstantRanges{{VK_SHADER_STAGE_VERTEX_BIT, 0, 128}};
    auto pipelineLayout = vsg::PipelineLayout::create(descriptorSetLayouts, pushConstantRanges);

    auto vertexShader = vsg::ShaderStage::read(
        VK_SHADER_STAGE_VERTEX_BIT,
        "main",
        std::string(DOP_GUI_SHADER_DIR) + "/triangle.vert.spv");
    auto fragmentShader = vsg::ShaderStage::read(
        VK_SHADER_STAGE_FRAGMENT_BIT,
        "main",
        std::string(DOP_GUI_SHADER_DIR) + "/triangle.frag.spv");

    if (!vertexShader || !fragmentShader)
    {
        throw std::runtime_error("Failed to load compiled SPIR-V shaders from build output.");
    }

    auto shaderStages = vsg::ShaderStages{vertexShader, fragmentShader};

    auto vertexInputState = vsg::VertexInputState::create();
    auto& bindings = vertexInputState->vertexBindingDescriptions;
    auto& attributes = vertexInputState->vertexAttributeDescriptions;

    constexpr uint32_t offset = 0;

    bindings.emplace_back(VkVertexInputBindingDescription{0, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX});
    attributes.emplace_back(VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offset});

    bindings.emplace_back(VkVertexInputBindingDescription{1, sizeof(vsg::vec4), VK_VERTEX_INPUT_RATE_VERTEX});
    attributes.emplace_back(VkVertexInputAttributeDescription{1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offset});

    auto inputAssemblyState = vsg::InputAssemblyState::create();
    inputAssemblyState->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    auto rasterizationState = vsg::RasterizationState::create();
    rasterizationState->cullMode = VK_CULL_MODE_NONE;

    auto depthStencilState = vsg::DepthStencilState::create();
    depthStencilState->depthTestEnable = VK_TRUE;

    auto graphicsPipelineStates = vsg::GraphicsPipelineStates{
        vertexInputState,
        inputAssemblyState,
        rasterizationState,
        vsg::ColorBlendState::create(),
        vsg::MultisampleState::create(),
        depthStencilState};

    auto graphicsPipeline = vsg::GraphicsPipeline::create(pipelineLayout, shaderStages, graphicsPipelineStates);
    return vsg::BindGraphicsPipeline::create(graphicsPipeline);
}

vsg::ref_ptr<vsg::vec4Array> VsgVisualizer::createColors(const SceneObjectState& object, std::size_t count) const
{
    auto colors = vsg::vec4Array::create(count);
    const auto color = object.colorHex.empty() ? colorForKind(object.kind) : object.color;
    for (std::size_t i = 0; i < count; ++i) (*colors)[i] = color;
    return colors;
}

vsg::ref_ptr<vsg::VertexIndexDraw> VsgVisualizer::createDrawCommands(const SceneObjectState& object) const
{
    vsg::ref_ptr<vsg::vec3Array> vertices;
    vsg::ref_ptr<vsg::uintArray> indices;

    if (object.kind == "triangle")
    {
        vertices = vsg::vec3Array::create({
            {-0.8f, 0.0f, 0.0f},
            {0.8f, 0.0f, 0.0f},
            {0.0f, 0.8f, 0.0f},
        });
        indices = vsg::uintArray::create({0, 1, 2});
    }
    else if (object.kind == "rectangle")
    {
        vertices = vsg::vec3Array::create({
            {-0.9f, -0.45f, 0.0f},
            {0.9f, -0.45f, 0.0f},
            {0.9f, 0.45f, 0.0f},
            {-0.9f, 0.45f, 0.0f},
        });
        indices = vsg::uintArray::create({0, 1, 2, 0, 2, 3});
    }
    else if (object.kind == "tristrip")
    {
        vertices = vsg::vec3Array::create({
            {-1.0f, -0.45f, 0.0f},
            {-0.45f, 0.45f, 0.0f},
            {0.0f, -0.35f, 0.0f},
            {0.45f, 0.45f, 0.0f},
            {1.0f, -0.25f, 0.0f},
        });
        indices = vsg::uintArray::create({0, 1, 2, 1, 3, 2, 2, 3, 4});
    }
    else if (object.kind == "cube")
    {
        vertices = vsg::vec3Array::create({
            {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
            {-0.5f, -0.5f, 0.5f},  {0.5f, -0.5f, 0.5f},  {0.5f, 0.5f, 0.5f},  {-0.5f, 0.5f, 0.5f},
        });
        indices = vsg::uintArray::create({
            0, 1, 2, 0, 2, 3,
            4, 6, 5, 4, 7, 6,
            0, 4, 5, 0, 5, 1,
            1, 5, 6, 1, 6, 2,
            2, 6, 7, 2, 7, 3,
            3, 7, 4, 3, 4, 0,
        });
    }
    else if (object.kind == "pyramid")
    {
        vertices = vsg::vec3Array::create({
            {-0.6f, -0.6f, 0.0f},
            {0.6f, -0.6f, 0.0f},
            {0.6f, 0.6f, 0.0f},
            {-0.6f, 0.6f, 0.0f},
            {0.0f, 0.0f, 1.0f},
        });
        indices = vsg::uintArray::create({
            0, 1, 2, 0, 2, 3,
            0, 1, 4,
            1, 2, 4,
            2, 3, 4,
            3, 0, 4,
        });
    }
    else if (object.kind == "sphere")
    {
        constexpr int rings = 12;
        constexpr int sectors = 24;
        std::vector<vsg::vec3> generatedVertices;
        std::vector<uint32_t> generatedIndices;
        generatedVertices.reserve((rings + 1) * (sectors + 1));
        generatedIndices.reserve(rings * sectors * 6);
        for (int ring = 0; ring <= rings; ++ring)
        {
            const double v = static_cast<double>(ring) / static_cast<double>(rings);
            const double phi = v * 3.14159265358979323846;
            const float z = static_cast<float>(std::cos(phi));
            const float r = static_cast<float>(std::sin(phi));
            for (int sector = 0; sector <= sectors; ++sector)
            {
                const double u = static_cast<double>(sector) / static_cast<double>(sectors);
                const double theta = u * 2.0 * 3.14159265358979323846;
                generatedVertices.push_back(vsg::vec3(
                    r * static_cast<float>(std::cos(theta)),
                    r * static_cast<float>(std::sin(theta)),
                    z));
            }
        }
        for (int ring = 0; ring < rings; ++ring)
        {
            for (int sector = 0; sector < sectors; ++sector)
            {
                const uint32_t a = ring * (sectors + 1) + sector;
                const uint32_t b = a + sectors + 1;
                const uint32_t c = a + 1;
                const uint32_t d = b + 1;
                generatedIndices.push_back(a); generatedIndices.push_back(b); generatedIndices.push_back(c);
                generatedIndices.push_back(c); generatedIndices.push_back(b); generatedIndices.push_back(d);
            }
        }
        vertices = vsg::vec3Array::create(generatedVertices.size());
        for (std::size_t i = 0; i < generatedVertices.size(); ++i) (*vertices)[i] = generatedVertices[i];
        indices = vsg::uintArray::create(generatedIndices.size());
        for (std::size_t i = 0; i < generatedIndices.size(); ++i) (*indices)[i] = generatedIndices[i];
    }
    else if (object.kind == "torus")
    {
        constexpr int majorSegments = 24;
        constexpr int minorSegments = 16;
        constexpr float majorRadius = 0.85f;
        constexpr float minorRadius = 0.28f;
        std::vector<vsg::vec3> generatedVertices;
        std::vector<uint32_t> generatedIndices;
        generatedVertices.reserve((majorSegments + 1) * (minorSegments + 1));
        generatedIndices.reserve(majorSegments * minorSegments * 6);
        for (int major = 0; major <= majorSegments; ++major)
        {
            const double u = static_cast<double>(major) / static_cast<double>(majorSegments);
            const double theta = u * 2.0 * 3.14159265358979323846;
            const float cosTheta = static_cast<float>(std::cos(theta));
            const float sinTheta = static_cast<float>(std::sin(theta));
            for (int minor = 0; minor <= minorSegments; ++minor)
            {
                const double v = static_cast<double>(minor) / static_cast<double>(minorSegments);
                const double phi = v * 2.0 * 3.14159265358979323846;
                const float cosPhi = static_cast<float>(std::cos(phi));
                const float sinPhi = static_cast<float>(std::sin(phi));
                const float radius = majorRadius + minorRadius * cosPhi;
                generatedVertices.push_back(vsg::vec3(
                    radius * cosTheta,
                    radius * sinTheta,
                    minorRadius * sinPhi));
            }
        }
        for (int major = 0; major < majorSegments; ++major)
        {
            for (int minor = 0; minor < minorSegments; ++minor)
            {
                const uint32_t a = major * (minorSegments + 1) + minor;
                const uint32_t b = (major + 1) * (minorSegments + 1) + minor;
                const uint32_t c = a + 1;
                const uint32_t d = b + 1;
                generatedIndices.push_back(a); generatedIndices.push_back(b); generatedIndices.push_back(c);
                generatedIndices.push_back(c); generatedIndices.push_back(b); generatedIndices.push_back(d);
            }
        }
        vertices = vsg::vec3Array::create(generatedVertices.size());
        for (std::size_t i = 0; i < generatedVertices.size(); ++i) (*vertices)[i] = generatedVertices[i];
        indices = vsg::uintArray::create(generatedIndices.size());
        for (std::size_t i = 0; i < generatedIndices.size(); ++i) (*indices)[i] = generatedIndices[i];
    }
    else
    {
        throw std::runtime_error("Unsupported scene object kind: " + object.kind);
    }

    auto colors = createColors(object, vertices->size());
    return createMesh(vertices, indices, colors);
}

void VsgVisualizer::populateScene(const AppState& state)
{
    if (!_scene) _scene = vsg::Group::create();
    _scene->children.clear();
    _objectTransforms.clear();
    _objectTransforms.reserve(state.scene.objects.size());

    for (const auto& object : state.scene.objects)
    {
        auto objectTransform = vsg::MatrixTransform::create();
        auto stateGroup = vsg::StateGroup::create();
        stateGroup->add(_bindPipeline);
        stateGroup->addChild(createDrawCommands(object));
        objectTransform->addChild(stateGroup);
        _scene->addChild(objectTransform);
        _objectTransforms.push_back(objectTransform);
    }

    _gridTransform = vsg::MatrixTransform::create();
    auto gridStateGroup = vsg::StateGroup::create();
    gridStateGroup->add(_bindPipeline);
    gridStateGroup->addChild(createGridGeometry());
    _gridTransform->addChild(gridStateGroup);
    _scene->addChild(_gridTransform);
}

vsg::ref_ptr<vsg::Group> VsgVisualizer::createScene(const AppState& state)
{
    _scene = vsg::Group::create();
    populateScene(state);
    return _scene;
}

void VsgVisualizer::rebuildScene(const AppState& state)
{
    populateScene(state);
    compileSceneIfNeeded();
}

void VsgVisualizer::compileSceneIfNeeded()
{
    if (!_viewer || !_viewer->compileManager || !_scene) return;
    auto compileResult = _viewer->compileManager->compile(_scene);
    if (compileResult.requiresViewerUpdate())
    {
        vsg::updateViewer(*_viewer, compileResult);
    }
}

void VsgVisualizer::initialize(const AppState& state, vsg::ref_ptr<vsg::Window> window)
{
    _bindPipeline = createBindGraphicsPipeline();
    _scene = createScene(state);

    const auto& pose = state.view.cameraPose;
    _lookAt = vsg::LookAt::create(pose.eye, pose.center, pose.up);
    auto perspective = vsg::Perspective::create(
        30.0,
        static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height),
        0.01,
        50.0);
    _camera = vsg::Camera::create(perspective, _lookAt, vsg::ViewportState::create(window->extent2D()));

    _commandGraph = vsg::CommandGraph::create(window);
    _view = vsg::View::create(_camera);
    _renderGraph = vsg::RenderGraph::create(window, _view);
    _commandGraph->addChild(_renderGraph);
    _view->addChild(_scene);
    syncFromState(state);
}

void VsgVisualizer::connect(vsg::ref_ptr<vsg::Viewer> viewer)
{
    _viewer = viewer;
    viewer->assignRecordAndSubmitTaskAndPresentation({_commandGraph});
}

void VsgVisualizer::syncFromState(const AppState& state)
{
    syncCameraFromState(state);
    syncSceneFromState(state);
}

void VsgVisualizer::syncCameraFromState(const AppState& state)
{
    if (_lookAt)
    {
        const auto& pose = state.view.cameraPose;
        _lookAt->eye = pose.eye;
        _lookAt->center = pose.center;
        _lookAt->up = pose.up;
    }
}

void VsgVisualizer::syncSceneFromState(const AppState& state)
{
    if (_objectTransforms.size() != state.scene.objects.size())
    {
        rebuildScene(state);
        if (_view && _view->children.empty())
        {
            _view->addChild(_scene);
        }
    }

    const auto count = std::min(_objectTransforms.size(), state.scene.objects.size());
    for (std::size_t i = 0; i < count; ++i)
    {
        const auto& object = state.scene.objects[i];
        const auto rotation =
            vsg::rotate(degreesToRadians(object.rotation.x), 1.0, 0.0, 0.0) *
            vsg::rotate(degreesToRadians(object.rotation.y), 0.0, 1.0, 0.0) *
            vsg::rotate(degreesToRadians(object.rotation.z), 0.0, 0.0, 1.0);
        _objectTransforms[i]->matrix =
            vsg::translate(object.position) *
            rotation *
            vsg::scale(object.scale.x, object.scale.y, object.scale.z);
    }
    for (std::size_t i = count; i < _objectTransforms.size(); ++i)
    {
        _objectTransforms[i]->matrix = vsg::scale(0.0, 0.0, 0.0);
    }

    if (_gridTransform)
    {
        _gridTransform->matrix = state.ui.displayGrid ? vsg::scale(1.0, 1.0, 1.0) : vsg::scale(0.0, 0.0, 0.0);
    }

    if (_renderGraph)
    {
        VkClearColorValue clearColor = {{
            state.view.backgroundColor.r,
            state.view.backgroundColor.g,
            state.view.backgroundColor.b,
            state.view.backgroundColor.a,
        }};
        _renderGraph->setClearValues(clearColor, {0.0f, 0});
    }
}

bool VsgVisualizer::isInitialized() const
{
    return _camera.valid() && _commandGraph.valid() && _scene.valid();
}

vsg::ref_ptr<vsg::Camera> VsgVisualizer::camera() const
{
    return _camera;
}

vsg::ref_ptr<vsg::CommandGraph> VsgVisualizer::commandGraph() const
{
    return _commandGraph;
}

vsg::ref_ptr<vsg::RenderGraph> VsgVisualizer::renderGraph() const
{
    return _renderGraph;
}
