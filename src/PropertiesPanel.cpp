#include "PropertiesPanel.h"

#include "UiLayoutUtils.h"
#include "Widgets.h"
#include "YogaLayout.h"

#include <vsgImGui/imgui.h>

namespace
{
std::string labelSlotId(std::string_view widgetId)
{
    return std::string(widgetId) + "-label";
}

WidgetSlotBinding binding(std::string_view widgetId)
{
    return makeWidgetSlotBinding(widgetId, labelSlotId);
}

std::vector<std::string> slotIds(const UiPanelState& panelState)
{
    std::vector<std::string> ids{
        "panel-properties-selected-object-label",
        "panel-selected-object",
        "panel-properties-selected-object",
    };

    for (const auto& widgetSpec : panelState.widgets)
    {
        if (widgetSpec.type != "input_double") continue;
        ids.push_back(labelSlotId(widgetSpec.id));
        ids.push_back(widgetSpec.id);
    }

    return ids;
}

YogaLayout::Spec buildLayout(const UiPanelState& panelState)
{
    using Axis = YogaLayout::Axis;
    using Builder = YogaLayout::Builder;
    using Length = YogaLayout::Length;
    using Style = YogaLayout::Style;

    Style root;
    root.direction = Axis::Column;
    root.gap = 8.0f;
    root.width = Length::percent(100.0f);
    root.height = Length::autoV();

    Style row;
    row.direction = Axis::Row;
    row.gap = 8.0f;
    row.width = Length::percent(100.0f);
    row.height = Length::px(24.0f);

    Style label;
    label.width = Length::px(116.0f);
    label.height = Length::px(24.0f);

    Style input;
    input.width = Length::flex(1.0f);
    input.height = Length::px(24.0f);

    Style fullWidth;
    fullWidth.width = Length::percent(100.0f);
    fullWidth.height = Length::px(20.0f);

    Builder builder;
    builder.root("properties-root", root)
        .begin("properties-selection-row", row)
            .item("panel-properties-selected-object-label", label)
            .item("panel-selected-object", input)
        .end()
        .item("panel-properties-selected-object", fullWidth);

    for (const auto& widgetSpec : panelState.widgets)
    {
        if (widgetSpec.type != "input_double") continue;
        builder.begin("row-" + widgetSpec.id, row)
            .item(labelSlotId(widgetSpec.id), label)
            .item(widgetSpec.id, input)
        .end();
    }

    return builder.build();
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

void PropertiesPanel::render(PanelContext& context, const UiPanelState& panelState)
{
    auto& state = context.state;
    auto* selectedObject = findSceneObject(state.scene, state.scene.selectedObjectId);
    YogaLayout propertiesLayout;
    propertiesLayout.setLayout(buildLayout(panelState));
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
    registerLayoutSlots(state.ui, std::string(id()), propertiesLayout, slotIds(panelState));

    const auto selectedObjectSlots = binding("panel-selected-object");
    const auto objectIds = collectSceneObjectIds(state.scene);
    const auto selectedValue = renderSelectedObjectControl(
        state.ui,
        propertiesLayout,
        selectedObjectSlots,
        "panel-selected-object",
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
        const auto slots = binding(widgetSpec.id);
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

    for (const auto& widgetSpec : panelState.widgets)
    {
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
