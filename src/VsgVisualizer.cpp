#include "VsgVisualizer.h"

#include <stdexcept>
#include <string>

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

vsg::ref_ptr<vsg::VertexIndexDraw> VsgVisualizer::createDrawCommands() const
{
    auto vertices = vsg::vec3Array::create({
        {-0.8f, 0.0f, 0.0f},
        {0.8f, 0.0f, 0.0f},
        {0.0f, 0.8f, 0.0f},
    });

    auto colors = vsg::vec4Array::create({
        {1.0f, 0.2f, 0.2f, 1.0f},
        {0.2f, 1.0f, 0.2f, 1.0f},
        {0.2f, 0.4f, 1.0f, 1.0f},
    });

    auto indices = vsg::uintArray::create({0, 1, 2});

    vsg::DataList vertexArrays{vertices, colors};

    auto drawCommands = vsg::VertexIndexDraw::create();
    drawCommands->assignArrays(vertexArrays);
    drawCommands->assignIndices(indices);
    drawCommands->indexCount = indices->width();
    drawCommands->instanceCount = 1;
    return drawCommands;
}

vsg::ref_ptr<vsg::Group> VsgVisualizer::createScene() const
{
    auto scene = vsg::Group::create();
    auto stateGroup = vsg::StateGroup::create();
    stateGroup->add(createBindGraphicsPipeline());
    stateGroup->addChild(createDrawCommands());
    scene->addChild(stateGroup);
    return scene;
}

void VsgVisualizer::initialize(const AppState& state, vsg::ref_ptr<vsg::Window> window)
{
    _scene = createScene();

    const auto& pose = state.view.cameraPose;
    auto lookAt = vsg::LookAt::create(pose.eye, pose.center, pose.up);
    auto perspective = vsg::Perspective::create(
        30.0,
        static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height),
        0.01,
        10.0);
    _camera = vsg::Camera::create( perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    _commandGraph = vsg::CommandGraph::create(window);
    _view = vsg::View::create(_camera);
    _renderGraph = vsg::RenderGraph::create(window, _view);
    _commandGraph->addChild(_renderGraph);
    _view->addChild(_scene);
}

void VsgVisualizer::connect(vsg::ref_ptr<vsg::Viewer> viewer)
{
    viewer->assignRecordAndSubmitTaskAndPresentation({_commandGraph});
}

vsg::ref_ptr<vsg::Camera> VsgVisualizer::camera() const
{
    return _camera;
}

vsg::ref_ptr<vsg::CommandGraph> VsgVisualizer::commandGraph() const
{
    return _commandGraph;
}
