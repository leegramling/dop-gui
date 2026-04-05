#include "UiLayer.h"

#include "Panel.h"
#include "Theme.h"
#include "Widgets.h"

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
}

void UiLayer::initialize(vsg::ref_ptr<vsg::Window> window, vsg::ref_ptr<vsg::RenderGraph> renderGraph, AppState& state)
{
    _state = &state;

    _renderImGui = vsgImGui::RenderImGui::create(window, [this]() -> bool
    {
        if (!_state) return true;
        render(*_state);
        return true;
    });

    renderGraph->addChild(_renderImGui);
    _sendEventsToImGui = vsgImGui::SendEventsToImGui::create();
}

void UiLayer::evaluate(AppState& state)
{
    const bool previousTestMode = state.ui.testMode;
    state.ui.testMode = true;
    render(state);
    state.ui.testMode = previousTestMode;
}

void UiLayer::render(AppState& state)
{
    if (!state.ui.testMode) Theme::applyDefault(state.ui.themeMode);
    state.ui.registry.clear();

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
        Panel panel(state.ui, panelId.c_str(), panelState.label.c_str());
        if (!panel.begin()) continue;

        if (panelId == "panel-scene-info")
        {
            for (const auto& widgetSpec : panelState.widgets)
            {
                if (widgetSpec.type == "text")
                {
                    if (widgetSpec.bind == "view.fps.text")
                    {
                        Text(state.ui, widgetSpec.id.c_str(), fpsText(state.view.fps));
                    }
                    else if (widgetSpec.bind == "scene.objectCount.text")
                    {
                        Text(state.ui, widgetSpec.id.c_str(), objectCountText(state));
                    }
                }
                else if (widgetSpec.type == "checkbox" && widgetSpec.bind == "ui.displayGrid")
                {
                    bool value = state.ui.displayGrid;
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
                    const auto value = Input(state.ui, widgetSpec.id.c_str(), widgetSpec.label.c_str(), state.view.backgroundColorHex);
                    state.view.backgroundColorHex = value;
                    if (value != previousValue && !widgetSpec.onChange.empty()) queueUiCommand(state.ui, widgetSpec.onChange, value);
                }
                else if (widgetSpec.type == "combo" && widgetSpec.bind == "scene.name")
                {
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
            }

            std::vector<std::string> objectIds;
            objectIds.reserve(state.scene.objects.size());
            for (const auto& object : state.scene.objects) objectIds.push_back(object.id);
            if (!objectIds.empty())
            {
                const auto selectedObject = ComboBox(
                    state.ui,
                    "panel-selected-object",
                    "Selected Object",
                    state.scene.selectedObjectId,
                    objectIds);
                if (!selectedObject.empty() && selectedObject != state.scene.selectedObjectId)
                {
                    queueUiCommand(state.ui, "scene.select_object", selectedObject);
                }
            }

            Text(state.ui, "panel-theme-label", "Theme");
            if (RadioButton(state.ui, "panel-theme-dark", "Dark", state.ui.themeMode == "dark"))
            {
                state.ui.themeMode = "dark";
            }
            if (RadioButton(state.ui, "panel-theme-light", "Light", state.ui.themeMode == "light"))
            {
                state.ui.themeMode = "light";
            }
            if (auto* darkWidget = findWidget(state.ui, "panel-theme-dark"))
            {
                darkWidget->boolValue = state.ui.themeMode == "dark";
            }
            if (auto* lightWidget = findWidget(state.ui, "panel-theme-light"))
            {
                lightWidget->boolValue = state.ui.themeMode == "light";
            }

            const bool openSceneSummary = Button(state.ui, "panel-scene-summary-open", "Scene Summary");
            Popup(state.ui, "popup-scene-summary", "Scene Summary", openSceneSummary, [&]()
            {
                Text(state.ui, "popup-scene-summary-name", "Scene: " + state.scene.name);
                Text(state.ui, "popup-scene-summary-object-count", objectCountText(state));
            });

            Text(state.ui, "panel-scene-table-label", "Scene Objects");
            Table(state.ui, "panel-scene-table", 4, state.scene.objects.size(), [&]()
            {
                if (!state.ui.testMode)
                {
                    ImGui::TableSetupColumn("Select");
                    ImGui::TableSetupColumn("Id");
                    ImGui::TableSetupColumn("Kind");
                    ImGui::TableSetupColumn("Position");
                    ImGui::TableHeadersRow();
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
            if (auto* selectedWidget = findWidget(state.ui, "panel-selected-object"))
            {
                selectedWidget->textValue = state.scene.selectedObjectId;
            }
        }
        else if (panelId == "panel-properties")
        {
            auto* selectedObject = findSceneObject(state.scene, state.scene.selectedObjectId);
            Text(
                state.ui,
                "panel-properties-selected-object",
                state.scene.selectedObjectId.empty() ? "Selected: none" : "Selected: " + state.scene.selectedObjectId);
            const std::size_t widgetRowCount = panelState.widgets.empty() ? 1u : panelState.widgets.size();
            Table(state.ui, "panel-properties-table", 2, widgetRowCount, [&]()
            {
                if (!state.ui.testMode)
                {
                    ImGui::TableSetupColumn("Property");
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableHeadersRow();
                }

                auto emitPropertyRow = [&](const std::string& key, const std::string& value)
                {
                    const auto rowPrefix = "table-properties-row-" + sanitizeLabel(key);
                    if (!state.ui.testMode)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                    }
                    Text(state.ui, (rowPrefix + "-name").c_str(), key);
                    if (!state.ui.testMode) ImGui::TableSetColumnIndex(1);
                    Text(state.ui, (rowPrefix + "-value").c_str(), value);
                };

                auto emitEditablePropertyRow = [&](const std::string& key, const std::string& inputId, double& value)
                {
                    const auto rowPrefix = "table-properties-row-" + sanitizeLabel(key);
                    if (!state.ui.testMode)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                    }
                    Text(state.ui, (rowPrefix + "-name").c_str(), key);
                    if (!state.ui.testMode) ImGui::TableSetColumnIndex(1);
                    value = InputDouble(
                        state.ui,
                        inputId.c_str(),
                        "",
                        value,
                        6);
                };

                auto emitUnitEditablePropertyRow =
                    [&](const std::string& key, const std::string& inputId, double& value, const char* unit)
                {
                    const auto rowPrefix = "table-properties-row-" + sanitizeLabel(key);
                    if (!state.ui.testMode)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                    }
                    Text(state.ui, (rowPrefix + "-name").c_str(), key);
                    if (!state.ui.testMode) ImGui::TableSetColumnIndex(1);
                    value = InputDouble(
                        state.ui,
                        inputId.c_str(),
                        "",
                        value,
                        2,
                        unit);
                };

                auto emitComboPropertyRow =
                    [&](const std::string& key, const UiWidgetSpecState& widgetSpec, const std::vector<std::string>& options)
                {
                    if (!state.ui.testMode)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                    }
                    Text(state.ui, ("table-properties-row-" + sanitizeLabel(key) + "-name").c_str(), key);
                    if (!state.ui.testMode) ImGui::TableSetColumnIndex(1);
                    const auto selectedValue = ComboBox(
                        state.ui,
                        widgetSpec.id.c_str(),
                        "",
                        state.scene.selectedObjectId,
                        options);
                    if (!selectedValue.empty() && selectedValue != state.scene.selectedObjectId && !widgetSpec.onChange.empty())
                    {
                        queueUiCommand(state.ui, widgetSpec.onChange, selectedValue);
                    }
                };

                if (!selectedObject)
                {
                    emitPropertyRow("Selected Object", "none");
                    return;
                }

                std::vector<std::string> objectIds;
                objectIds.reserve(state.scene.objects.size());
                for (const auto& object : state.scene.objects) objectIds.push_back(object.id);

                for (const auto& widgetSpec : panelState.widgets)
                {
                    if (widgetSpec.type == "combo" && widgetSpec.bind == "scene.selectedObjectId")
                    {
                        emitComboPropertyRow(widgetSpec.label, widgetSpec, objectIds);
                    }
                    else if (widgetSpec.type == "input_double")
                    {
                        if (widgetSpec.bind == "scene.selected.position.x")
                        {
                            emitUnitEditablePropertyRow(widgetSpec.label, widgetSpec.id, selectedObject->position.x, "m");
                        }
                        else if (widgetSpec.bind == "scene.selected.position.y")
                        {
                            emitUnitEditablePropertyRow(widgetSpec.label, widgetSpec.id, selectedObject->position.y, "m");
                        }
                        else if (widgetSpec.bind == "scene.selected.position.z")
                        {
                            emitUnitEditablePropertyRow(widgetSpec.label, widgetSpec.id, selectedObject->position.z, "m");
                        }
                        else if (widgetSpec.bind == "scene.selected.rotation.x")
                        {
                            emitUnitEditablePropertyRow(widgetSpec.label, widgetSpec.id, selectedObject->rotation.x, "deg");
                        }
                        else if (widgetSpec.bind == "scene.selected.rotation.y")
                        {
                            emitUnitEditablePropertyRow(widgetSpec.label, widgetSpec.id, selectedObject->rotation.y, "deg");
                        }
                        else if (widgetSpec.bind == "scene.selected.rotation.z")
                        {
                            emitUnitEditablePropertyRow(widgetSpec.label, widgetSpec.id, selectedObject->rotation.z, "deg");
                        }
                        else if (widgetSpec.bind == "scene.selected.scale.x")
                        {
                            emitEditablePropertyRow(widgetSpec.label, widgetSpec.id, selectedObject->scale.x);
                        }
                        else if (widgetSpec.bind == "scene.selected.scale.y")
                        {
                            emitEditablePropertyRow(widgetSpec.label, widgetSpec.id, selectedObject->scale.y);
                        }
                        else if (widgetSpec.bind == "scene.selected.scale.z")
                        {
                            emitEditablePropertyRow(widgetSpec.label, widgetSpec.id, selectedObject->scale.z);
                        }
                    }
                }
            });
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
