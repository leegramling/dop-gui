#include "SceneInfoPanel.h"

#include "UiLayoutUtils.h"
#include "Widgets.h"
#include "YogaLayout.h"

#include <vsgImGui/imgui.h>

#include <algorithm>
#include <sstream>

namespace
{
std::string vec3Text(const vsg::dvec3& value)
{
    std::ostringstream out;
    out << value.x << ", " << value.y << ", " << value.z;
    return out.str();
}

std::string fpsText(double fps)
{
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(1);
    out << "FPS: " << fps;
    return out.str();
}

std::string objectCountText(const AppState& state)
{
    return "Objects: " + std::to_string(state.scene.objects.size());
}

std::string labelSlotId(std::string_view widgetId)
{
    if (widgetId == "panel-bgcolor") return "panel-bgcolor-label";
    if (widgetId == "panel-scene-select") return "panel-scene-select-label";
    if (widgetId == "panel-scene-selected-object") return "panel-scene-selected-object-label";
    return std::string(widgetId) + "-label";
}

WidgetSlotBinding binding(std::string_view widgetId)
{
    return makeWidgetSlotBinding(widgetId, labelSlotId);
}

std::vector<std::string_view> slotIds()
{
    return {
        "panel-fps",
        "panel-object-count",
        "panel-display-grid",
        "panel-bgcolor",
        "panel-bgcolor-label",
        "panel-scene-select",
        "panel-scene-select-label",
        "panel-theme-label",
        "panel-theme-dark",
        "panel-theme-light",
        "panel-scene-summary-open",
        "panel-scene-selected-object-label",
        "panel-scene-selected-object",
        "panel-scene-table-label",
        "panel-scene-table",
    };
}

YogaLayout::Spec buildLayout()
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

    Style labelRow;
    labelRow.width = Length::percent(100.0f);
    labelRow.height = Length::px(20.0f);

    Style metricRow = labelRow;

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

    Style radio;
    radio.width = Length::px(120.0f);
    radio.height = Length::px(24.0f);

    Style button;
    button.width = Length::px(144.0f);
    button.height = Length::px(24.0f);

    Style table;
    table.width = Length::percent(100.0f);
    table.height = Length::px(180.0f);

    return Builder{}
        .root("scene-info-root", root)
            .item("panel-fps", metricRow)
            .item("panel-object-count", metricRow)
            .item("panel-display-grid", button)
            .begin("panel-bgcolor-row", row)
                .item("panel-bgcolor", input)
                .item("panel-bgcolor-label", label)
            .end()
            .begin("panel-scene-select-row", row)
                .item("panel-scene-select", input)
                .item("panel-scene-select-label", label)
            .end()
            .item("panel-theme-label", labelRow)
            .item("panel-theme-dark", radio)
            .item("panel-theme-light", radio)
            .item("panel-scene-summary-open", button)
            .begin("panel-scene-selected-object-row", row)
                .item("panel-scene-selected-object-label", label)
                .item("panel-scene-selected-object", input)
            .end()
            .item("panel-scene-table-label", labelRow)
            .item("panel-scene-table", table)
        .build();
}
}

std::string_view SceneInfoPanel::id() const
{
    return "panel-scene-info";
}

PanelMinSize SceneInfoPanel::minSize(const UiPanelState& panelState) const
{
    (void)panelState;
    return PanelMinSize{.width = 360.0f, .height = 492.0f, .enabled = true};
}

void SceneInfoPanel::render(PanelContext& context, const UiPanelState& panelState)
{
    auto& state = context.state;
    static const auto layoutSpec = buildLayout();
    YogaLayout sceneInfoLayout;
    sceneInfoLayout.setLayout(layoutSpec);
    ImVec2 origin{16.0f, 16.0f};
    ImVec2 avail{
        panelState.layout.width > 0.0 ? static_cast<float>(panelState.layout.width) - 32.0f : 328.0f,
        panelState.layout.height > 0.0 ? static_cast<float>(panelState.layout.height) - 32.0f : 488.0f};
    if (!state.ui.testMode)
    {
        avail = ImVec2(
            std::max(0.0f, ImGui::GetContentRegionAvail().x - 16.0f),
            std::max(0.0f, ImGui::GetContentRegionAvail().y));
    }
    sceneInfoLayout.resize(origin.x, origin.y, avail.x, avail.y);
    registerLayoutSlots(state.ui, std::string(id()), sceneInfoLayout, slotIds());

    for (const auto& widgetSpec : panelState.widgets)
    {
        if (widgetSpec.type == "text")
        {
            const auto slots = binding(widgetSpec.id);
            if (widgetSpec.bind == "view.fps.text")
            {
                setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.valueSlotId);
                Text(state.ui, widgetSpec.id.c_str(), fpsText(state.view.fps));
            }
            else if (widgetSpec.bind == "scene.objectCount.text")
            {
                setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.valueSlotId);
                Text(state.ui, widgetSpec.id.c_str(), objectCountText(state));
            }
        }
        else if (widgetSpec.type == "checkbox" && widgetSpec.bind == "ui.displayGrid")
        {
            bool value = state.ui.displayGrid;
            const auto slots = binding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.valueSlotId);
            const bool changed = Checkbox(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str(), value);
            state.ui.displayGrid = value;
            if (changed && !widgetSpec.onChange.empty())
            {
                queueUiCommand(state.ui, widgetSpec.onChange, value ? "true" : "false");
            }
        }
        else if (widgetSpec.type == "input" && widgetSpec.bind == "view.backgroundColorHex")
        {
            const auto previousValue = state.view.backgroundColorHex;
            const auto slots = binding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.valueSlotId);
            const auto value = Input(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str(), state.view.backgroundColorHex);
            state.view.backgroundColorHex = value;
            if (value != previousValue && !widgetSpec.onChange.empty()) queueUiCommand(state.ui, widgetSpec.onChange, value);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.labelSlotId);
            Text(state.ui, slots.labelSlotId.c_str(), widgetSpec.label);
        }
        else if (widgetSpec.type == "combo" && widgetSpec.bind == "scene.name")
        {
            const auto slots = binding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.valueSlotId);
            const auto value = ComboBox(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str(), state.scene.name, widgetSpec.options);
            if (value != state.scene.name && !widgetSpec.onChange.empty())
            {
                queueUiCommand(state.ui, widgetSpec.onChange, value);
            }
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.labelSlotId);
            Text(state.ui, slots.labelSlotId.c_str(), widgetSpec.label);
        }
        else if (widgetSpec.type == "radio" && widgetSpec.bind == "ui.themeMode")
        {
            const bool selected = state.ui.themeMode == widgetSpec.arg;
            const auto slots = binding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.valueSlotId);
            if (RadioButton(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str(), selected) && !widgetSpec.onClick.empty())
            {
                queueUiCommand(state.ui, widgetSpec.onClick, widgetSpec.arg);
            }
            if (auto* widget = findWidget(state.ui, widgetSpec.id))
            {
                widget->boolValue = selected;
            }
        }
    }

    std::vector<std::string> objectIds;
    objectIds.reserve(state.scene.objects.size());
    for (const auto& object : state.scene.objects) objectIds.push_back(object.id);

    const auto selectedObjectSlots = binding("panel-scene-selected-object");
    setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, selectedObjectSlots.labelSlotId);
    Text(state.ui, selectedObjectSlots.labelSlotId.c_str(), "Selected Object");

    if (!objectIds.empty())
    {
        setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, selectedObjectSlots.valueSlotId);
        const auto selectedObject = ComboBox(state.ui, "panel-scene-selected-object", "", state.scene.selectedObjectId, objectIds);
        if (!selectedObject.empty() && selectedObject != state.scene.selectedObjectId)
        {
            queueUiCommand(state.ui, "scene.select_object", selectedObject);
        }
    }

    setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, "panel-theme-label");
    Text(state.ui, "panel-theme-label", "Theme");

    bool openSceneSummary = false;
    for (const auto& widgetSpec : panelState.widgets)
    {
        if (widgetSpec.type == "button" && widgetSpec.id == "panel-scene-summary-open")
        {
            const auto slots = binding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.valueSlotId);
            openSceneSummary = Button(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str());
        }
        else if (widgetSpec.type == "popup" && widgetSpec.id == "popup-scene-summary")
        {
            setNextWidgetLayout(state.ui, widgetSpec.layout);
            Popup(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str(), openSceneSummary, [&]()
            {
                Text(state.ui, "popup-scene-summary-name", "Scene: " + state.scene.name);
                Text(state.ui, "popup-scene-summary-object-count", objectCountText(state));
            });
        }
        else if (widgetSpec.type == "table" && widgetSpec.bind == "scene.objects")
        {
            const auto slots = binding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.labelSlotId);
            Text(state.ui, "panel-scene-table-label", widgetSpec.label);
            const int columnCount = widgetSpec.columns.empty() ? 4 : static_cast<int>(widgetSpec.columns.size());
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, slots.valueSlotId);
            Table(state.ui, widgetSpec.id.c_str(), columnCount, state.scene.objects.size(), [&]()
            {
                if (!state.ui.testMode)
                {
                    for (const auto& column : widgetSpec.columns) ImGui::TableSetupColumn(column.c_str());
                    if (!widgetSpec.columns.empty()) ImGui::TableHeadersRow();
                }

                for (const auto& object : state.scene.objects)
                {
                    const auto rowPrefix = "table-scene-objects-row-" + object.id;
                    if (!state.ui.testMode)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                    }
                    if (Button(state.ui, (rowPrefix + "-select").c_str(), "Select"))
                    {
                        queueUiCommand(state.ui, "scene.select_object", object.id);
                    }
                    if (!state.ui.testMode) ImGui::TableSetColumnIndex(1);
                    Text(state.ui, (rowPrefix + "-id").c_str(), object.id);
                    if (!state.ui.testMode) ImGui::TableSetColumnIndex(2);
                    Text(state.ui, (rowPrefix + "-kind").c_str(), object.kind);
                    if (!state.ui.testMode) ImGui::TableSetColumnIndex(3);
                    Text(state.ui, (rowPrefix + "-position").c_str(), vec3Text(object.position));
                }
            });
        }
    }

    if (auto* selectedWidget = findWidget(state.ui, "panel-scene-selected-object"))
    {
        selectedWidget->textValue = state.scene.selectedObjectId;
    }
}
