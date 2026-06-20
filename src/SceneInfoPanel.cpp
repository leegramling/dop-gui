#include "SceneInfoPanel.h"

#include "Widgets.h"
#include "WindowManager.h"

#include <vsgImGui/imgui.h>

#include <sstream>

namespace
{
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

std::string vec3Text(const vsg::dvec3& value)
{
    std::ostringstream out;
    out << value.x << ", " << value.y << ", " << value.z;
    return out.str();
}

std::string windowSizeText(const WindowManager* windowManager)
{
    if (!windowManager || !windowManager->primaryWindow()) return "Window Size: unavailable";
    const auto extent = windowManager->primaryWindow()->extent2D();
    return "Window Size: " + std::to_string(extent.width) + " x " + std::to_string(extent.height);
}
}

std::string_view SceneInfoPanel::id() const
{
    return "panel-scene-info";
}

PanelMinSize SceneInfoPanel::minSize(const UiPanelState& panelState) const
{
    (void)panelState;
    return PanelMinSize{.width = 320.0f, .height = 420.0f, .enabled = true};
}

void SceneInfoPanel::render(PanelContext& context, const UiPanelState& panelState)
{
    (void)panelState;
    auto& state = context.state;

    DisplayText(state.ui, "fps", fpsText(state.view.fps));
    DisplayText(state.ui, "window-size", windowSizeText(context.windowManager));
    DisplayText(state.ui, "object-count", objectCountText(state));

    if (Checkbox(state.ui, "display-grid", "Display Grid", state.ui.displayGrid))
    {
        queueUiCommand(state.ui, "ui.grid.set_visible", state.ui.displayGrid ? "true" : "false");
    }

    const auto backgroundColor = TextField(
        state.ui, "background-color", "Background Color", state.view.backgroundColorHex);
    if (backgroundColor != state.view.backgroundColorHex)
    {
        state.view.backgroundColorHex = backgroundColor;
        queueUiCommand(state.ui, "view.background.set_hex", backgroundColor);
    }

    const std::vector<std::string> scenes{"bootstrap", "cubes", "shapes"};
    const auto selectedScene = SelectField(state.ui, "scene-select", "Scene", state.scene.name, scenes);
    if (selectedScene != state.scene.name)
    {
        queueUiCommand(state.ui, "scene.load", selectedScene);
    }

    DisplayText(state.ui, "panel-theme-label", "Theme");
    if (RadioButton(state.ui, "theme-dark", "Dark", state.ui.themeMode == "dark"))
    {
        queueUiCommand(state.ui, "ui.theme.set", "dark");
    }
    if (!state.ui.testMode) ImGui::SameLine();
    if (RadioButton(state.ui, "theme-light", "Light", state.ui.themeMode == "light"))
    {
        queueUiCommand(state.ui, "ui.theme.set", "light");
    }

    const bool openSummary = ActionButton(state.ui, "scene-summary-open", "Scene Summary");
    Popup(state.ui, "popup-scene-summary", "Scene Summary", openSummary, [&]()
    {
        DisplayText(state.ui, "popup-scene-summary-name", "Scene: " + state.scene.name);
        DisplayText(state.ui, "popup-scene-summary-object-count", objectCountText(state));
    });

    const auto objectIds = collectSceneObjectIds(state.scene);
    const auto selectedObject = SelectField(
        state.ui, "selected-object", "Selected Object", state.scene.selectedObjectId, objectIds);
    if (selectedObject != state.scene.selectedObjectId)
    {
        queueUiCommand(state.ui, "scene.select_object", selectedObject);
    }

    DisplayText(state.ui, "panel-scene-table-label", "Scene Objects");
    Table(state.ui, "scene-table", 4, state.scene.objects.size(), [&]()
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
            if (ActionButton(state.ui, (rowPrefix + "-select").c_str(), "Select"))
            {
                queueUiCommand(state.ui, "scene.select_object", object.id);
            }
            if (!state.ui.testMode) ImGui::TableSetColumnIndex(1);
            DisplayText(state.ui, (rowPrefix + "-id").c_str(), object.id);
            if (!state.ui.testMode) ImGui::TableSetColumnIndex(2);
            DisplayText(state.ui, (rowPrefix + "-kind").c_str(), object.kind);
            if (!state.ui.testMode) ImGui::TableSetColumnIndex(3);
            DisplayText(state.ui, (rowPrefix + "-position").c_str(), vec3Text(object.position));
        }
    });
}
