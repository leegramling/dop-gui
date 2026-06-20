#include "NewShapePanel.h"

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

void renderNumber(
    UiState& ui,
    const char* labelId,
    const char* valueId,
    const char* label,
    double& value,
    int precision,
    const char* unit = nullptr)
{
    DisplayText(ui, labelId, label);
    value = NumericField(ui, valueId, "", value, precision, unit);
}
}

std::string_view NewShapePanel::id() const
{
    return "panel-new-shape";
}

void NewShapePanel::resetForm()
{
    _shapeKind = "Sphere";
    _positionX = 0.0;
    _positionY = 0.0;
    _positionZ = 0.0;
    _rotationX = 0.0;
    _rotationY = 0.0;
    _rotationZ = 0.0;
    _scaleX = 1.0;
    _scaleY = 1.0;
    _scaleZ = 1.0;
    _colorHex = "#00FF00";
    _status = "Ready";
}

PanelMinSize NewShapePanel::minSize(const UiPanelState& panelState) const
{
    (void)panelState;
    return PanelMinSize{.width = 320.0f, .height = 460.0f, .enabled = true};
}

void NewShapePanel::render(PanelContext& context, const UiPanelState& panelState)
{
    (void)panelState;
    auto& state = context.state;
    const std::vector<std::string> shapeOptions{"Sphere", "Torus", "Pyramid"};

    DisplayText(state.ui, "shape-kind-label", "Shape");
    _shapeKind = SelectField(state.ui, "shape-kind", "", _shapeKind, shapeOptions);
    renderNumber(state.ui, "position-x-label", "position-x", "Location X", _positionX, 2, "m");
    renderNumber(state.ui, "position-y-label", "position-y", "Location Y", _positionY, 2, "m");
    renderNumber(state.ui, "position-z-label", "position-z", "Location Z", _positionZ, 2, "m");
    renderNumber(state.ui, "rotation-x-label", "rotation-x", "Rotation X", _rotationX, 2, "deg");
    renderNumber(state.ui, "rotation-y-label", "rotation-y", "Rotation Y", _rotationY, 2, "deg");
    renderNumber(state.ui, "rotation-z-label", "rotation-z", "Rotation Z", _rotationZ, 2, "deg");
    renderNumber(state.ui, "scale-x-label", "scale-x", "Scale X", _scaleX, 3);
    renderNumber(state.ui, "scale-y-label", "scale-y", "Scale Y", _scaleY, 3);
    renderNumber(state.ui, "scale-z-label", "scale-z", "Scale Z", _scaleZ, 3);

    DisplayText(state.ui, "color-label", "Color");
    _colorHex = TextField(state.ui, "color", "", _colorHex);

    if (ActionButton(state.ui, "create-shape", "Create Shape"))
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
            state.ui.requestedCommands.push_back("ui.panel.close=panel-new-shape");
            resetForm();
        }
    }

    if (!state.ui.testMode) ImGui::SameLine();
    if (ActionButton(state.ui, "cancel", "Cancel"))
    {
        state.ui.requestedCommands.push_back("ui.panel.close=panel-new-shape");
        resetForm();
    }

    DisplayText(state.ui, "status", _status);
}
