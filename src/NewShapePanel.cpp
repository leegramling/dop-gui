#include "NewShapePanel.h"

#include "UiLayoutUtils.h"
#include "Widgets.h"

#include <vsgImGui/imgui.h>

#include <algorithm>
#include <cctype>

namespace
{
std::string toSceneKind(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
    {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::uint32_t vertexCountForKind(std::string_view kind)
{
    if (kind == "sphere") return 12u * 24u * 6u;
    if (kind == "torus") return 24u * 16u * 6u;
    if (kind == "pyramid") return 18u;
    return 0;
}

std::string makeUniqueObjectId(const SceneState& scene, const std::string& kind)
{
    for (std::size_t index = 1;; ++index)
    {
        const auto candidate = kind + "_" + std::to_string(index);
        if (!findSceneObject(scene, candidate)) return candidate;
    }
}
}

std::string_view NewShapePanel::id() const
{
    return "panel-new-shape";
}

PanelMinSize NewShapePanel::minSize(const UiPanelState& panelState) const
{
    (void)panelState;
    return PanelMinSize{.width = 360.0f, .height = 460.0f, .enabled = true};
}

void NewShapePanel::init(const UiPanelState& panelState)
{
    (void)panelState;
    YogaLayout::Builder builder;
    using Style = YogaLayout::Style;
    using Length = YogaLayout::Length;

    auto rowStyle = Style{.direction = YogaLayout::Axis::Row, .gap = 8.0f, .width = Length::px(360.0f), .height = Length::px(24.0f)};
    auto labelStyle = Style{.direction = YogaLayout::Axis::Column, .width = Length::px(116.0f), .height = Length::px(24.0f)};
    auto valueStyle = Style{.direction = YogaLayout::Axis::Column, .width = Length::flex(1.0f), .height = Length::px(24.0f)};

    builder.root("new-shape-root", Style{.direction = YogaLayout::Axis::Column, .gap = 8.0f, .width = Length::px(360.0f), .height = Length::autoV()});

    const auto addLabeledRow = [&](std::string_view widgetId)
    {
        builder.begin(std::string(widgetId) + "-row", rowStyle)
            .item(std::string(widgetId) + "-label", labelStyle)
            .item(widgetId, valueStyle)
            .end();
    };

    addLabeledRow("shape-kind");
    addLabeledRow("position-x");
    addLabeledRow("position-y");
    addLabeledRow("position-z");
    addLabeledRow("rotation-x");
    addLabeledRow("rotation-y");
    addLabeledRow("rotation-z");
    addLabeledRow("scale-x");
    addLabeledRow("scale-y");
    addLabeledRow("scale-z");
    addLabeledRow("color");
    builder.item("create-shape", Style{.direction = YogaLayout::Axis::Column, .width = Length::px(168.0f), .height = Length::px(24.0f)});
    builder.item("status", Style{.direction = YogaLayout::Axis::Column, .width = Length::px(360.0f), .height = Length::px(20.0f)});

    _layoutSpec = builder.build();
    _slotIds = {
        "shape-kind-label", "shape-kind",
        "position-x-label", "position-x",
        "position-y-label", "position-y",
        "position-z-label", "position-z",
        "rotation-x-label", "rotation-x",
        "rotation-y-label", "rotation-y",
        "rotation-z-label", "rotation-z",
        "scale-x-label", "scale-x",
        "scale-y-label", "scale-y",
        "scale-z-label", "scale-z",
        "color-label", "color",
        "create-shape",
        "status",
    };
}

void NewShapePanel::render(PanelContext& context, const UiPanelState& panelState)
{
    auto& state = context.state;
    YogaLayout layout;
    layout.setLayout(_layoutSpec);

    ImVec2 origin{0.0f, 0.0f};
    ImVec2 avail{
        panelState.layout.width > 0.0 ? static_cast<float>(panelState.layout.width) : 360.0f,
        panelState.layout.height > 0.0 ? static_cast<float>(panelState.layout.height) : 480.0f};
    if (!state.ui.testMode)
    {
        origin = ImGui::GetCursorPos();
        avail = ImGui::GetContentRegionAvail();
    }

    layout.resize(origin.x, origin.y, avail.x, avail.y);
    registerLayoutSlots(state.ui, std::string(id()), layout, _slotIds);

    const std::vector<std::string> shapeOptions{"Sphere", "Torus", "Pyramid"};

    setNextWidgetLayoutIfPresent(state.ui, layout, "shape-kind-label");
    Text(state.ui, "shape-kind-label", "Shape");
    setNextWidgetLayoutIfPresent(state.ui, layout, "shape-kind");
    _shapeKind = ComboBox(state.ui, "shape-kind", "", _shapeKind, shapeOptions);

    const auto renderDoubleField = [&](const char* labelSlot, const char* valueId, const char* labelText, double& value, const char* unit = nullptr, int precision = 2)
    {
        setNextWidgetLayoutIfPresent(state.ui, layout, labelSlot);
        Text(state.ui, labelSlot, labelText);
        setNextWidgetLayoutIfPresent(state.ui, layout, valueId);
        value = InputDouble(state.ui, valueId, "", value, precision, unit);
    };

    renderDoubleField("position-x-label", "position-x", "Location X", _positionX, "m");
    renderDoubleField("position-y-label", "position-y", "Location Y", _positionY, "m");
    renderDoubleField("position-z-label", "position-z", "Location Z", _positionZ, "m");
    renderDoubleField("rotation-x-label", "rotation-x", "Rotation X", _rotationX, "deg");
    renderDoubleField("rotation-y-label", "rotation-y", "Rotation Y", _rotationY, "deg");
    renderDoubleField("rotation-z-label", "rotation-z", "Rotation Z", _rotationZ, "deg");
    renderDoubleField("scale-x-label", "scale-x", "Scale X", _scaleX, nullptr, 3);
    renderDoubleField("scale-y-label", "scale-y", "Scale Y", _scaleY, nullptr, 3);
    renderDoubleField("scale-z-label", "scale-z", "Scale Z", _scaleZ, nullptr, 3);

    setNextWidgetLayoutIfPresent(state.ui, layout, "color-label");
    Text(state.ui, "color-label", "Color");
    setNextWidgetLayoutIfPresent(state.ui, layout, "color");
    _colorHex = Input(state.ui, "color", "", _colorHex);

    setNextWidgetLayoutIfPresent(state.ui, layout, "create-shape");
    if (Button(state.ui, "create-shape", "Create Shape"))
    {
        vsg::vec4 parsedColor{};
        if (!tryParseHexColor(_colorHex, parsedColor))
        {
            _status = "Invalid color hex";
        }
        else
        {
            const auto kind = toSceneKind(_shapeKind);
            SceneObjectState object{
                .id = makeUniqueObjectId(state.scene, kind),
                .kind = kind,
                .vertexCount = vertexCountForKind(kind),
                .position = {_positionX, _positionY, _positionZ},
                .rotation = {_rotationX, _rotationY, _rotationZ},
                .scale = {_scaleX, _scaleY, _scaleZ},
                .colorHex = _colorHex,
                .color = parsedColor,
            };
            state.scene.objects.push_back(object);
            state.scene.selectedObjectId = object.id;
            _status = "Created " + object.id;
        }
    }

    setNextWidgetLayoutIfPresent(state.ui, layout, "status");
    Text(state.ui, "status", _status);
}
