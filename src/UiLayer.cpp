#include "UiLayer.h"

#include "Panel.h"
#include "Theme.h"
#include "Widgets.h"
#include "WindowManager.h"
#include "YogaLayout.h"

#include <vsgImGui/SendEventsToImGui.h>

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

std::string sanitizeLabel(std::string text)
{
    for (auto& ch : text)
    {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        else if (ch == ' ' || ch == '.') ch = '-';
    }
    return text;
}

bool menuHasPendingClick(const UiState& uiState, const std::string& menuLabel, const UiMenuState& menu)
{
    for (const auto& item : menu.items)
    {
        const auto itemLabel = "menuitem-" + sanitizeLabel(menuLabel) + "-" + sanitizeLabel(item.label);
        for (const auto& action : uiState.pendingActions)
        {
            if (action.label == itemLabel && action.kind == "click") return true;
        }
    }

    return false;
}

void queueUiCommand(UiState& uiState, const std::string& commandName, const std::string& value = {})
{
    if (commandName.empty()) return;
    uiState.requestedCommands.push_back(value.empty() ? commandName : (commandName + "=" + value));
}

void registerLayoutSlot(UiState& uiState, const std::string& panelId, const std::string& slotId, const UiLayoutRectState& layout)
{
    for (auto& slot : uiState.layoutSlots)
    {
        if (slot.panelId == panelId && slot.slotId == slotId)
        {
            slot.layout = layout;
            return;
        }
    }

    uiState.layoutSlots.push_back(UiLayoutSlotState{
        .panelId = panelId,
        .slotId = slotId,
        .layout = layout,
    });
}

void registerLayoutSlots(UiState& uiState, const std::string& panelId, const YogaLayout& layout, std::initializer_list<std::string_view> slotIds)
{
    for (const auto slotId : slotIds)
    {
        if (layout.has(slotId))
        {
            registerLayoutSlot(uiState, panelId, std::string(slotId), layout.rect(slotId));
        }
    }
}

void registerLayoutSlots(UiState& uiState, const std::string& panelId, const YogaLayout& layout, const std::vector<std::string_view>& slotIds)
{
    for (const auto slotId : slotIds)
    {
        if (layout.has(slotId))
        {
            registerLayoutSlot(uiState, panelId, std::string(slotId), layout.rect(slotId));
        }
    }
}

void registerLayoutSlots(UiState& uiState, const std::string& panelId, const YogaLayout& layout, const std::vector<std::string>& slotIds)
{
    for (const auto& slotId : slotIds)
    {
        if (layout.has(slotId))
        {
            registerLayoutSlot(uiState, panelId, slotId, layout.rect(slotId));
        }
    }
}

struct YogaPanelDefinition
{
    YogaLayout::Spec spec;
    PanelMinSize minSize;
};

struct WidgetSlotBinding
{
    std::string valueSlotId;
    std::string labelSlotId;
};

std::string propertiesLabelSlotId(std::string_view widgetId)
{
    return std::string(widgetId) + "-label";
}

std::string sceneInfoLabelSlotId(std::string_view widgetId)
{
    if (widgetId == "panel-bgcolor") return "panel-bgcolor-label";
    if (widgetId == "panel-scene-select") return "panel-scene-select-label";
    if (widgetId == "panel-scene-selected-object") return "panel-scene-selected-object-label";
    return std::string(widgetId) + "-label";
}

std::vector<std::string_view> sceneInfoSlotIds()
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

std::vector<std::string> propertiesSlotIds(const UiPanelState& panelState)
{
    std::vector<std::string> slotIds{
        "panel-properties-selected-object-label",
        "panel-selected-object",
        "panel-properties-selected-object",
    };

    for (const auto& widgetSpec : panelState.widgets)
    {
        if (widgetSpec.type != "input_double") continue;
        slotIds.push_back(propertiesLabelSlotId(widgetSpec.id));
        slotIds.push_back(widgetSpec.id);
    }

    return slotIds;
}

WidgetSlotBinding sceneInfoBinding(std::string_view widgetId)
{
    return WidgetSlotBinding{
        .valueSlotId = std::string(widgetId),
        .labelSlotId = sceneInfoLabelSlotId(widgetId),
    };
}

WidgetSlotBinding propertiesBinding(std::string_view widgetId)
{
    return WidgetSlotBinding{
        .valueSlotId = std::string(widgetId),
        .labelSlotId = propertiesLabelSlotId(widgetId),
    };
}

void setNextWidgetLayoutIfPresent(UiState& uiState, const YogaLayout& layout, std::string_view slotId)
{
    if (layout.has(slotId))
    {
        setNextWidgetLayout(uiState, layout.rect(slotId));
    }
}

YogaPanelDefinition buildPropertiesPanelLayout(const UiPanelState& panelState)
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

    Style valueOnlyRow = row;
    valueOnlyRow.height = Length::px(20.0f);

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
        if (!(widgetSpec.type == "input_double")) continue;

        const auto rowId = "row-" + widgetSpec.id;
        const auto labelId = propertiesLabelSlotId(widgetSpec.id);
        builder.begin(rowId, row)
            .item(labelId, label)
            .item(widgetSpec.id, input)
        .end();
    }

    return YogaPanelDefinition{
        .spec = builder.build(),
        .minSize = PanelMinSize{.width = 360.0f, .height = 356.0f, .enabled = true},
    };
}

YogaPanelDefinition buildSceneInfoPanelLayout()
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

    return YogaPanelDefinition{
        .spec = Builder{}
        .root("scene-info-lower-root", root)
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
        .build(),
        .minSize = PanelMinSize{.width = 360.0f, .height = 492.0f, .enabled = true},
    };
}

void renderSceneInfoPanel(AppState& state, const std::string& panelId, const UiPanelState& panelState)
{
    static const auto sceneInfoPanelDefinition = buildSceneInfoPanelLayout();
    YogaLayout sceneInfoLayout;
    sceneInfoLayout.setLayout(sceneInfoPanelDefinition.spec);
    ImVec2 lowerOrigin{16.0f, 16.0f};
    ImVec2 lowerAvail{
        panelState.layout.width > 0.0 ? static_cast<float>(panelState.layout.width) - 32.0f : 328.0f,
        panelState.layout.height > 0.0 ? static_cast<float>(panelState.layout.height) - 32.0f : 488.0f};
    if (!state.ui.testMode)
    {
        lowerAvail = ImVec2(
            std::max(0.0f, ImGui::GetContentRegionAvail().x - 16.0f),
            std::max(0.0f, ImGui::GetContentRegionAvail().y));
    }
    sceneInfoLayout.resize(lowerOrigin.x, lowerOrigin.y, lowerAvail.x, lowerAvail.y);
    const auto slotIds = sceneInfoSlotIds();
    registerLayoutSlots(state.ui, panelId, sceneInfoLayout, slotIds);

    for (const auto& widgetSpec : panelState.widgets)
    {
        if (widgetSpec.type == "text")
        {
            const auto binding = sceneInfoBinding(widgetSpec.id);
            if (widgetSpec.bind == "view.fps.text")
            {
                setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.valueSlotId);
                Text(state.ui, widgetSpec.id.c_str(), fpsText(state.view.fps));
            }
            else if (widgetSpec.bind == "scene.objectCount.text")
            {
                setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.valueSlotId);
                Text(state.ui, widgetSpec.id.c_str(), objectCountText(state));
            }
        }
        else if (widgetSpec.type == "checkbox" && widgetSpec.bind == "ui.displayGrid")
        {
            bool value = state.ui.displayGrid;
            const auto binding = sceneInfoBinding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.valueSlotId);
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
            const auto binding = sceneInfoBinding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.valueSlotId);
            const auto value = Input(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str(), state.view.backgroundColorHex);
            state.view.backgroundColorHex = value;
            if (value != previousValue && !widgetSpec.onChange.empty()) queueUiCommand(state.ui, widgetSpec.onChange, value);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.labelSlotId);
            Text(state.ui, binding.labelSlotId.c_str(), widgetSpec.label);
        }
        else if (widgetSpec.type == "combo" && widgetSpec.bind == "scene.name")
        {
            const auto binding = sceneInfoBinding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.valueSlotId);
            const auto value = ComboBox(
                state.ui,
                widgetSpec.id.c_str(),
                widgetSpec.label.c_str(),
                state.scene.name,
                widgetSpec.options);
            if (value != state.scene.name && !widgetSpec.onChange.empty())
            {
                queueUiCommand(state.ui, widgetSpec.onChange, value);
            }
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.labelSlotId);
            Text(state.ui, binding.labelSlotId.c_str(), widgetSpec.label);
        }
        else if (widgetSpec.type == "radio" && widgetSpec.bind == "ui.themeMode")
        {
            const bool selected = state.ui.themeMode == widgetSpec.arg;
            const auto binding = sceneInfoBinding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.valueSlotId);
            if (RadioButton(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str(), selected) &&
                !widgetSpec.onClick.empty())
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
    const auto selectedObjectBinding = sceneInfoBinding("panel-scene-selected-object");
    setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, selectedObjectBinding.labelSlotId);
    Text(state.ui, selectedObjectBinding.labelSlotId.c_str(), "Selected Object");

    if (!objectIds.empty())
    {
        setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, selectedObjectBinding.valueSlotId);
        const auto selectedObject = ComboBox(
            state.ui,
            "panel-scene-selected-object",
            "",
            state.scene.selectedObjectId,
            objectIds);
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
            const auto binding = sceneInfoBinding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.valueSlotId);
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
            const auto binding = sceneInfoBinding(widgetSpec.id);
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.labelSlotId);
            Text(state.ui, "panel-scene-table-label", widgetSpec.label);
            const int columnCount = widgetSpec.columns.empty() ? 4 : static_cast<int>(widgetSpec.columns.size());
            setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, binding.valueSlotId);
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

void renderPropertiesPanel(AppState& state, const std::string& panelId, const UiPanelState& panelState)
{
    auto* selectedObject = findSceneObject(state.scene, state.scene.selectedObjectId);
    const auto propertiesDefinition = buildPropertiesPanelLayout(panelState);
    YogaLayout propertiesLayout;
    propertiesLayout.setLayout(propertiesDefinition.spec);
    ImVec2 propertiesOrigin{0.0f, 0.0f};
    ImVec2 propertiesAvail{
        panelState.layout.width > 0.0 ? static_cast<float>(panelState.layout.width) : 320.0f,
        panelState.layout.height > 0.0 ? static_cast<float>(panelState.layout.height) : 520.0f};
    if (!state.ui.testMode)
    {
        propertiesOrigin = ImGui::GetCursorPos();
        propertiesAvail = ImGui::GetContentRegionAvail();
    }
    propertiesLayout.resize(propertiesOrigin.x, propertiesOrigin.y, propertiesAvail.x, propertiesAvail.y);
    const auto slotIds = propertiesSlotIds(panelState);
    registerLayoutSlots(state.ui, panelId, propertiesLayout, slotIds);

    const auto selectedObjectBinding = propertiesBinding("panel-selected-object");
    setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, "panel-properties-selected-object-label");
    Text(state.ui, "panel-properties-selected-object-label", "Selected Object");

    std::vector<std::string> objectIds;
    objectIds.reserve(state.scene.objects.size());
    for (const auto& object : state.scene.objects) objectIds.push_back(object.id);

    setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, selectedObjectBinding.valueSlotId);
    const auto selectedValue = ComboBox(
        state.ui,
        "panel-selected-object",
        "",
        state.scene.selectedObjectId,
        objectIds);
    if (!selectedValue.empty() && selectedValue != state.scene.selectedObjectId)
    {
        queueUiCommand(state.ui, "scene.select_object", selectedValue);
    }

    setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, "panel-properties-selected-object");
    Text(
        state.ui,
        "panel-properties-selected-object",
        state.scene.selectedObjectId.empty() ? "Selected: none" : "Selected: " + state.scene.selectedObjectId);

    auto emitPropertyRow = [&](const std::string& key, const std::string& value)
    {
        const auto labelId = "row-" + sanitizeLabel(key) + "-label";
        const auto valueId = "row-" + sanitizeLabel(key) + "-value";
        Text(state.ui, labelId.c_str(), key);
        Text(state.ui, valueId.c_str(), value);
    };

    auto emitEditablePropertyRow = [&](const UiWidgetSpecState& widgetSpec, double& value, int precision)
    {
        const auto binding = propertiesBinding(widgetSpec.id);
        setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, binding.labelSlotId);
        Text(state.ui, binding.labelSlotId.c_str(), widgetSpec.label);

        setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, binding.valueSlotId);
        value = InputDouble(
            state.ui,
            widgetSpec.id.c_str(),
            "",
            value,
            precision);
    };

    auto emitUnitEditablePropertyRow =
        [&](const UiWidgetSpecState& widgetSpec, double& value, int precision, const char* unit)
    {
        const auto binding = propertiesBinding(widgetSpec.id);
        setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, binding.labelSlotId);
        Text(state.ui, binding.labelSlotId.c_str(), widgetSpec.label);

        setNextWidgetLayoutIfPresent(state.ui, propertiesLayout, binding.valueSlotId);
        value = InputDouble(
            state.ui,
            widgetSpec.id.c_str(),
            "",
            value,
            precision,
            unit);
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
        else if (widgetSpec.type == "input_double")
        {
            if (widgetSpec.bind == "scene.selected.position.x")
            {
                emitUnitEditablePropertyRow(widgetSpec, selectedObject->position.x, widgetSpec.precision, widgetSpec.unit.c_str());
            }
            else if (widgetSpec.bind == "scene.selected.position.y")
            {
                emitUnitEditablePropertyRow(widgetSpec, selectedObject->position.y, widgetSpec.precision, widgetSpec.unit.c_str());
            }
            else if (widgetSpec.bind == "scene.selected.position.z")
            {
                emitUnitEditablePropertyRow(widgetSpec, selectedObject->position.z, widgetSpec.precision, widgetSpec.unit.c_str());
            }
            else if (widgetSpec.bind == "scene.selected.rotation.x")
            {
                emitUnitEditablePropertyRow(widgetSpec, selectedObject->rotation.x, widgetSpec.precision, widgetSpec.unit.c_str());
            }
            else if (widgetSpec.bind == "scene.selected.rotation.y")
            {
                emitUnitEditablePropertyRow(widgetSpec, selectedObject->rotation.y, widgetSpec.precision, widgetSpec.unit.c_str());
            }
            else if (widgetSpec.bind == "scene.selected.rotation.z")
            {
                emitUnitEditablePropertyRow(widgetSpec, selectedObject->rotation.z, widgetSpec.precision, widgetSpec.unit.c_str());
            }
            else if (widgetSpec.bind == "scene.selected.scale.x")
            {
                emitEditablePropertyRow(widgetSpec, selectedObject->scale.x, widgetSpec.precision);
            }
            else if (widgetSpec.bind == "scene.selected.scale.y")
            {
                emitEditablePropertyRow(widgetSpec, selectedObject->scale.y, widgetSpec.precision);
            }
            else if (widgetSpec.bind == "scene.selected.scale.z")
            {
                emitEditablePropertyRow(widgetSpec, selectedObject->scale.z, widgetSpec.precision);
            }
        }
    }
}
}

void UiLayer::initialize(
    vsg::ref_ptr<vsg::Window> window,
    vsg::ref_ptr<vsg::RenderGraph> renderGraph,
    AppState& state,
    WindowManager& windowManager)
{
    _state = &state;
    _windowManager = &windowManager;

    _renderImGui = vsgImGui::RenderImGui::create(window, [this]() -> bool
    {
        if (!_state) return true;
        render(*_state);
        return true;
    });

    renderGraph->addChild(_renderImGui);
    _sendEventsToImGui = vsgImGui::SendEventsToImGui::create();

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (_windowManager) _windowManager->syncImGuiStatus(state.ui);
}

void UiLayer::evaluate(AppState& state)
{
    const bool previousTestMode = state.ui.testMode;
    state.ui.testMode = true;
    if (_windowManager) _windowManager->syncImGuiStatus(state.ui);
    render(state);
    state.ui.testMode = previousTestMode;
}

void UiLayer::render(AppState& state)
{
    if (!state.ui.testMode) Theme::applyDefault(state.ui.themeMode);
    if (_windowManager) _windowManager->syncImGuiStatus(state.ui);
    state.ui.registry.clear();
    state.ui.layoutSlots.clear();

    if (!state.ui.testMode && state.ui.dockingEnabled)
    {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    }

    if (state.ui.testMode || ImGui::BeginMainMenuBar())
    {
        registerWidget(state.ui, "menubar-main", "menubar");

        for (const auto& menu : state.ui.layout.menus)
        {
            const auto menuLabel = "menu-" + sanitizeLabel(menu.label);
            registerWidget(state.ui, menuLabel, "menu");
            const bool hasPendingClick = menuHasPendingClick(state.ui, menu.label, menu);
            const bool menuOpened = !state.ui.testMode && !hasPendingClick ? ImGui::BeginMenu(menu.label.c_str()) : false;
            if (state.ui.testMode || hasPendingClick || menuOpened)
            {
                for (const auto& item : menu.items)
                {
                    const auto itemLabel = "menuitem-" + sanitizeLabel(menu.label) + "-" + sanitizeLabel(item.label);
                    registerWidget(state.ui, itemLabel, "menuitem");
                    bool clicked = false;
                    if (auto* action = findPendingUiAction(state.ui, itemLabel, "click"))
                    {
                        clicked = true;
                        action->kind.clear();
                    }
                    if (!state.ui.testMode) clicked = clicked || ImGui::MenuItem(item.label.c_str());
                    if (clicked)
                    {
                        queueUiCommand(state.ui, item.command);
                    }
                }
                if (menuOpened) ImGui::EndMenu();
            }
        }

        if (!state.ui.testMode) ImGui::EndMainMenuBar();
    }

    for (const auto& panelState : state.ui.layout.panels)
    {
        const auto panelId = "panel-" + sanitizeLabel(panelState.label);
        bool panelOpen = panelState.open;
        PanelMinSize minSize;
        if (panelId == "panel-scene-info")
        {
            minSize = buildSceneInfoPanelLayout().minSize;
        }
        else if (panelId == "panel-properties")
        {
            minSize = buildPropertiesPanelLayout(panelState).minSize;
        }
        Panel panel(
            state.ui,
            panelId.c_str(),
            panelState.label.c_str(),
            panelOpen,
            panelState.closable,
            panelState.flags,
            panelState.layout,
            minSize);
        if (!panel.begin()) continue;

        if (panelId == "panel-scene-info")
        {
            renderSceneInfoPanel(state, panelId, panelState);
        }
        else if (panelId == "panel-properties")
        {
            renderPropertiesPanel(state, panelId, panelState);
        }
    }

    state.ui.pendingActions.clear();
}

bool UiLayer::isInitialized() const
{
    return _renderImGui.valid();
}

vsg::ref_ptr<vsg::Visitor> UiLayer::eventHandler() const
{
    return _sendEventsToImGui;
}
