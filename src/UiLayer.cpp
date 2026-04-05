#include "UiLayer.h"

#include "Panel.h"
#include "Theme.h"
#include "Widgets.h"

#include <vsgImGui/SendEventsToImGui.h>

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
    if (!state.ui.testMode) Theme::applyDefault();
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
                        if (item.command == "app.exit")
                        {
                            state.ui.exitRequested = true;
                        }
                        else if (item.command == "scene.load.cubes")
                        {
                            state.ui.requestedSceneFile = std::string(DOP_GUI_SOURCE_DIR) + "/scenes/cubes.json5";
                        }
                        else if (item.command == "scene.load.shapes")
                        {
                            state.ui.requestedSceneFile = std::string(DOP_GUI_SOURCE_DIR) + "/scenes/shapes.json5";
                        }
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

        Text(state.ui, "panel-fps", fpsText(state.view.fps));
        Text(state.ui, "panel-object-count", objectCountText(state));
        Checkbox(state.ui, "panel-display-grid", "Display Grid", state.ui.displayGrid);

        const auto backgroundValue = Input(
            state.ui,
            "panel-bgcolor",
            "Background Color",
            state.view.backgroundColorHex);
        state.view.backgroundColorHex = backgroundValue;

        vsg::vec4 parsedColor;
        if (tryParseHexColor(backgroundValue, parsedColor))
        {
            state.view.backgroundColor = parsedColor;
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
