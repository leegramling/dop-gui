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

YogaLayout::Spec buildPropertiesPanelLayout(const UiPanelState& panelState)
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
        const auto labelId = widgetSpec.id + "-label";
        builder.begin(rowId, row)
            .item(labelId, label)
            .item(widgetSpec.id, input)
        .end();
    }

    return builder.build();
}

YogaLayout::Spec buildSceneInfoLowerLayout()
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
        .root("scene-info-lower-root", root)
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
        Panel panel(
            state.ui,
            panelId.c_str(),
            panelState.label.c_str(),
            panelOpen,
            panelState.closable,
            panelState.flags,
            panelState.layout);
        if (!panel.begin()) continue;

        if (panelId == "panel-scene-info")
        {
            static const auto sceneInfoLowerSpec = buildSceneInfoLowerLayout();
            YogaLayout sceneInfoLowerLayout;
            sceneInfoLowerLayout.setLayout(sceneInfoLowerSpec);
            ImVec2 lowerOrigin{16.0f, 156.0f};
            ImVec2 lowerAvail{
                panelState.layout.width > 0.0 ? static_cast<float>(panelState.layout.width) - 32.0f : 328.0f,
                320.0f};
            if (!state.ui.testMode)
            {
                lowerAvail = ImVec2(
                    std::max(0.0f, ImGui::GetContentRegionAvail().x - 16.0f),
                    std::max(0.0f, ImGui::GetContentRegionAvail().y));
            }
            sceneInfoLowerLayout.resize(lowerOrigin.x, lowerOrigin.y, lowerAvail.x, lowerAvail.y);

            for (const auto& widgetSpec : panelState.widgets)
            {
                if (widgetSpec.type == "text")
                {
                    if (widgetSpec.bind == "view.fps.text")
                    {
                        setNextWidgetLayout(state.ui, widgetSpec.layout);
                        Text(state.ui, widgetSpec.id.c_str(), fpsText(state.view.fps));
                    }
                    else if (widgetSpec.bind == "scene.objectCount.text")
                    {
                        setNextWidgetLayout(state.ui, widgetSpec.layout);
                        Text(state.ui, widgetSpec.id.c_str(), objectCountText(state));
                    }
                }
                else if (widgetSpec.type == "checkbox" && widgetSpec.bind == "ui.displayGrid")
                {
                    bool value = state.ui.displayGrid;
                    setNextWidgetLayout(state.ui, widgetSpec.layout);
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
                    setNextWidgetLayout(state.ui, widgetSpec.layout);
                    const auto value = Input(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str(), state.view.backgroundColorHex);
                    state.view.backgroundColorHex = value;
                    if (value != previousValue && !widgetSpec.onChange.empty()) queueUiCommand(state.ui, widgetSpec.onChange, value);
                }
                else if (widgetSpec.type == "combo" && widgetSpec.bind == "scene.name")
                {
                    setNextWidgetLayout(state.ui, widgetSpec.layout);
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
                }
                else if (widgetSpec.type == "radio" && widgetSpec.bind == "ui.themeMode")
                {
                    const bool selected = state.ui.themeMode == widgetSpec.arg;
                    if (sceneInfoLowerLayout.has(widgetSpec.id))
                    {
                        setNextWidgetLayout(state.ui, sceneInfoLowerLayout.rect(widgetSpec.id));
                    }
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
            if (sceneInfoLowerLayout.has("panel-scene-selected-object-label"))
            {
                setNextWidgetLayout(state.ui, sceneInfoLowerLayout.rect("panel-scene-selected-object-label"));
            }
            Text(state.ui, "panel-scene-selected-object-label", "Selected Object");

            if (!objectIds.empty())
            {
                if (sceneInfoLowerLayout.has("panel-scene-selected-object"))
                {
                    setNextWidgetLayout(state.ui, sceneInfoLowerLayout.rect("panel-scene-selected-object"));
                }
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

            if (sceneInfoLowerLayout.has("panel-theme-label"))
            {
                setNextWidgetLayout(state.ui, sceneInfoLowerLayout.rect("panel-theme-label"));
            }
            Text(state.ui, "panel-theme-label", "Theme");

            bool openSceneSummary = false;
            for (const auto& widgetSpec : panelState.widgets)
            {
                if (widgetSpec.type == "button" && widgetSpec.id == "panel-scene-summary-open")
                {
                    if (sceneInfoLowerLayout.has(widgetSpec.id))
                    {
                        setNextWidgetLayout(state.ui, sceneInfoLowerLayout.rect(widgetSpec.id));
                    }
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
                    if (sceneInfoLowerLayout.has("panel-scene-table-label"))
                    {
                        setNextWidgetLayout(state.ui, sceneInfoLowerLayout.rect("panel-scene-table-label"));
                    }
                    Text(state.ui, "panel-scene-table-label", widgetSpec.label);
                    const int columnCount = widgetSpec.columns.empty() ? 4 : static_cast<int>(widgetSpec.columns.size());
                    if (sceneInfoLowerLayout.has(widgetSpec.id))
                    {
                        setNextWidgetLayout(state.ui, sceneInfoLowerLayout.rect(widgetSpec.id));
                    }
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
        else if (panelId == "panel-properties")
        {
            auto* selectedObject = findSceneObject(state.scene, state.scene.selectedObjectId);
            const auto propertiesSpec = buildPropertiesPanelLayout(panelState);
            YogaLayout propertiesLayout;
            propertiesLayout.setLayout(propertiesSpec);
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

            if (propertiesLayout.has("panel-properties-selected-object-label"))
            {
                setNextWidgetLayout(state.ui, propertiesLayout.rect("panel-properties-selected-object-label"));
            }
            Text(state.ui, "panel-properties-selected-object-label", "Selected Object");

            std::vector<std::string> objectIds;
            objectIds.reserve(state.scene.objects.size());
            for (const auto& object : state.scene.objects) objectIds.push_back(object.id);

            if (propertiesLayout.has("panel-selected-object"))
            {
                setNextWidgetLayout(state.ui, propertiesLayout.rect("panel-selected-object"));
            }
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

            if (propertiesLayout.has("panel-properties-selected-object"))
            {
                setNextWidgetLayout(state.ui, propertiesLayout.rect("panel-properties-selected-object"));
            }
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
                const auto labelSlot = widgetSpec.id + "-label";
                if (propertiesLayout.has(labelSlot))
                {
                    setNextWidgetLayout(state.ui, propertiesLayout.rect(labelSlot));
                }
                Text(state.ui, labelSlot.c_str(), widgetSpec.label);

                if (propertiesLayout.has(widgetSpec.id))
                {
                    setNextWidgetLayout(state.ui, propertiesLayout.rect(widgetSpec.id));
                }
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
                const auto labelSlot = widgetSpec.id + "-label";
                if (propertiesLayout.has(labelSlot))
                {
                    setNextWidgetLayout(state.ui, propertiesLayout.rect(labelSlot));
                }
                Text(state.ui, labelSlot.c_str(), widgetSpec.label);

                if (propertiesLayout.has(widgetSpec.id))
                {
                    setNextWidgetLayout(state.ui, propertiesLayout.rect(widgetSpec.id));
                }
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
                continue;
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
