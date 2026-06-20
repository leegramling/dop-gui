#include "PropertiesPanel.h"

#include "Widgets.h"

namespace
{
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

std::string_view PropertiesPanel::id() const
{
    return "panel-properties";
}

PanelMinSize PropertiesPanel::minSize(const UiPanelState& panelState) const
{
    (void)panelState;
    return PanelMinSize{.width = 320.0f, .height = 356.0f, .enabled = true};
}

void PropertiesPanel::render(PanelContext& context, const UiPanelState& panelState)
{
    (void)panelState;
    auto& state = context.state;
    const auto objectIds = collectSceneObjectIds(state.scene);

    DisplayText(state.ui, "panel-properties-selected-object-label", "Selected Object");
    const auto selected = SelectField(
        state.ui, "selected-object", "", state.scene.selectedObjectId, objectIds);
    if (selected != state.scene.selectedObjectId)
    {
        queueUiCommand(state.ui, "scene.select_object", selected);
    }

    DisplayText(
        state.ui,
        "panel-properties-selected-object",
        state.scene.selectedObjectId.empty() ? "Selected: none" : "Selected: " + state.scene.selectedObjectId);

    auto* object = findSceneObject(state.scene, state.scene.selectedObjectId);
    if (!object) return;

    renderNumber(state.ui, "position-x-label", "position-x", "Location X", object->position.x, 2, "m");
    renderNumber(state.ui, "position-y-label", "position-y", "Location Y", object->position.y, 2, "m");
    renderNumber(state.ui, "position-z-label", "position-z", "Location Z", object->position.z, 2, "m");
    renderNumber(state.ui, "rotation-x-label", "rotation-x", "Rotation X", object->rotation.x, 2, "deg");
    renderNumber(state.ui, "rotation-y-label", "rotation-y", "Rotation Y", object->rotation.y, 2, "deg");
    renderNumber(state.ui, "rotation-z-label", "rotation-z", "Rotation Z", object->rotation.z, 2, "deg");
    renderNumber(state.ui, "scale-x-label", "scale-x", "Scale X", object->scale.x, 6);
    renderNumber(state.ui, "scale-y-label", "scale-y", "Scale Y", object->scale.y, 6);
    renderNumber(state.ui, "scale-z-label", "scale-z", "Scale Z", object->scale.z, 6);
}
