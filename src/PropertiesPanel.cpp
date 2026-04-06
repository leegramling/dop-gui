#include "PropertiesPanel.h"

#include "UiLayoutUtils.h"
#include "Widgets.h"

#include <vsgImGui/imgui.h>

namespace
{
const UiPanelWidgetNode* findWidget(const UiPanelTree& root, std::string_view widgetId)
{
    return root.findWidget(widgetId);
}
}

std::string_view PropertiesPanel::id() const
{
    return "panel-properties";
}

PanelMinSize PropertiesPanel::minSize(const UiPanelState& panelState) const
{
    (void)panelState;
    return PanelMinSize{.width = 360.0f, .height = 356.0f, .enabled = true};
}

void PropertiesPanel::init(const UiPanelState& panelState)
{
    _root = UiPanelTree::build(panelState);
}

void PropertiesPanel::render(PanelContext& context, const UiPanelState& panelState)
{
    auto& state = context.state;
    auto* selectedObject = findSceneObject(state.scene, state.scene.selectedObjectId);
    YogaLayout propertiesLayout;
    propertiesLayout.setLayout(_root.layoutSpec());
    ImVec2 origin{0.0f, 0.0f};
    ImVec2 avail{
        panelState.layout.width > 0.0 ? static_cast<float>(panelState.layout.width) : 320.0f,
        panelState.layout.height > 0.0 ? static_cast<float>(panelState.layout.height) : 520.0f};
    if (!state.ui.testMode)
    {
        origin = ImGui::GetCursorPos();
        avail = ImGui::GetContentRegionAvail();
    }
    propertiesLayout.resize(origin.x, origin.y, avail.x, avail.y);
    registerLayoutSlots(state.ui, std::string(id()), propertiesLayout, _root.slotIds());

    const auto* selectedObjectWidget = findWidget(_root, "selected-object");
    const auto selectedObjectSlots = selectedObjectWidget ? selectedObjectWidget->slots : makeWidgetSlotBinding("selected-object", [](std::string_view id) { return std::string(id) + "-label"; });
    const auto objectIds = collectSceneObjectIds(state.scene);
    const auto selectedValue = renderSelectedObjectControl(
        state.ui,
        propertiesLayout,
        selectedObjectSlots,
        selectedObjectWidget ? selectedObjectWidget->spec.id.c_str() : "selected-object",
        "panel-properties-selected-object-label",
        "Selected Object",
        state.scene.selectedObjectId,
        objectIds);
    if (!selectedValue.empty() && selectedValue != state.scene.selectedObjectId)
    {
        queueUiCommand(state.ui, "scene.select_object", selectedValue);
    }

    setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, "panel-properties-selected-object");
    Text(state.ui, "panel-properties-selected-object",
        state.scene.selectedObjectId.empty() ? "Selected: none" : "Selected: " + state.scene.selectedObjectId);

    auto emitPropertyRow = [&](const std::string& key, const std::string& value)
    {
        Text(state.ui, ("row-" + key + "-label").c_str(), key);
        Text(state.ui, ("row-" + key + "-value").c_str(), value);
    };

    auto emitEditablePropertyRow = [&](const UiWidgetSpecState& widgetSpec, double& value, int precision, const char* unit)
    {
        const auto* node = findWidget(_root, widgetSpec.id);
        const auto slots = node ? node->slots : makeWidgetSlotBinding(widgetSpec.id, [](std::string_view id) { return std::string(id) + "-label"; });
        setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, slots.labelSlotId);
        Text(state.ui, slots.labelSlotId.c_str(), widgetSpec.label);

        setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, slots.valueSlotId);
        value = InputDouble(state.ui, widgetSpec.id.c_str(), "", value, precision, unit);
    };

    if (!selectedObject)
    {
        emitPropertyRow("Selected Object", "none");
        return;
    }

    for (const auto& node : _root.widgets())
    {
        const auto& widgetSpec = node.spec;
        if (widgetSpec.type == "combo" && widgetSpec.bind == "scene.selectedObjectId")
        {
            continue;
        }
        if (widgetSpec.type != "input_double") continue;

        if (widgetSpec.bind == "scene.selected.position.x")
        {
            emitEditablePropertyRow(widgetSpec, selectedObject->position.x, widgetSpec.precision, widgetSpec.unit.c_str());
        }
        else if (widgetSpec.bind == "scene.selected.position.y")
        {
            emitEditablePropertyRow(widgetSpec, selectedObject->position.y, widgetSpec.precision, widgetSpec.unit.c_str());
        }
        else if (widgetSpec.bind == "scene.selected.position.z")
        {
            emitEditablePropertyRow(widgetSpec, selectedObject->position.z, widgetSpec.precision, widgetSpec.unit.c_str());
        }
        else if (widgetSpec.bind == "scene.selected.rotation.x")
        {
            emitEditablePropertyRow(widgetSpec, selectedObject->rotation.x, widgetSpec.precision, widgetSpec.unit.c_str());
        }
        else if (widgetSpec.bind == "scene.selected.rotation.y")
        {
            emitEditablePropertyRow(widgetSpec, selectedObject->rotation.y, widgetSpec.precision, widgetSpec.unit.c_str());
        }
        else if (widgetSpec.bind == "scene.selected.rotation.z")
        {
            emitEditablePropertyRow(widgetSpec, selectedObject->rotation.z, widgetSpec.precision, widgetSpec.unit.c_str());
        }
        else if (widgetSpec.bind == "scene.selected.scale.x")
        {
            emitEditablePropertyRow(widgetSpec, selectedObject->scale.x, widgetSpec.precision, nullptr);
        }
        else if (widgetSpec.bind == "scene.selected.scale.y")
        {
            emitEditablePropertyRow(widgetSpec, selectedObject->scale.y, widgetSpec.precision, nullptr);
        }
        else if (widgetSpec.bind == "scene.selected.scale.z")
        {
            emitEditablePropertyRow(widgetSpec, selectedObject->scale.z, widgetSpec.precision, nullptr);
        }
    }
}
