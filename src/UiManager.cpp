#include "UiManager.h"

#include "Panel.h"
#include "PanelWindow.h"
#include "NewShapePanel.h"
#include "PropertiesPanel.h"
#include "SceneInfoPanel.h"
#include "Theme.h"
#include "Widgets.h"
#include "WindowManager.h"

#include <vsgImGui/SendEventsToImGui.h>
#include <vsgImGui/imgui_internal.h>

namespace
{
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

void createDefaultDockLayout(ImGuiID dockspaceId, const ImGuiViewport& viewport)
{
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodePos(dockspaceId, viewport.WorkPos);
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport.WorkSize);

    ImGuiID leftColumn = 0;
    ImGuiID remaining = 0;
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.28f, &leftColumn, &remaining);

    ImGuiID rightColumn = 0;
    ImGuiID center = 0;
    ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Right, 0.28f, &rightColumn, &center);
    ImGui::DockBuilderDockWindow("Scene Info", leftColumn);
    ImGui::DockBuilderDockWindow("Properties", rightColumn);
    ImGui::DockBuilderFinish(dockspaceId);
}
}

UiManager::UiManager()
{
    registerPanel(std::make_unique<SceneInfoPanel>());
    registerPanel(std::make_unique<PropertiesPanel>());
    registerPanel(std::make_unique<NewShapePanel>());
}

UiManager::~UiManager() = default;

void UiManager::registerPanel(std::unique_ptr<Panel> panel)
{
    if (!panel) return;
    _panels.push_back(PanelRegistration{
        .id = std::string(panel->id()),
        .controller = std::move(panel),
    });
}

Panel* UiManager::findPanel(std::string_view id)
{
    for (auto& panel : _panels)
    {
        if (panel.id == id) return panel.controller.get();
    }

    return nullptr;
}

void UiManager::initialize(
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
    io.FontGlobalScale = state.ui.scale;
    ImGui::GetStyle().ScaleAllSizes(state.ui.scale);
    if (_windowManager) _windowManager->syncImGuiStatus(state.ui);
}

void UiManager::evaluate(AppState& state)
{
    const bool previousTestMode = state.ui.testMode;
    state.ui.testMode = true;
    if (_windowManager) _windowManager->syncImGuiStatus(state.ui);
    render(state);
    state.ui.testMode = previousTestMode;
}

void UiManager::render(AppState& state)
{
    if (!state.ui.testMode) Theme::applyDefault(state.ui.themeMode, state.ui.scale);
    if (_windowManager) _windowManager->syncImGuiStatus(state.ui);
    state.ui.registry.clear();
    state.ui.layoutSlots.clear();

    if (!state.ui.testMode && state.ui.dockingEnabled)
    {
        const auto* viewport = ImGui::GetMainViewport();
        const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(
            0, viewport, ImGuiDockNodeFlags_PassthruCentralNode);
        if (!_defaultDockLayoutApplied)
        {
            createDefaultDockLayout(dockspaceId, *viewport);
            _defaultDockLayoutApplied = true;
        }
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
                    if (clicked && !item.command.empty())
                    {
                        state.ui.requestedCommands.push_back(item.command);
                    }
                }
                if (menuOpened) ImGui::EndMenu();
            }
        }

        if (!state.ui.testMode) ImGui::EndMainMenuBar();
    }

    PanelContext context{
        .state = state,
        .windowManager = _windowManager,
    };

    for (auto& panelState : state.ui.layout.panels)
    {
        const auto panelId = "panel-" + sanitizeLabel(panelState.label);
        auto* panelController = findPanel(panelId);
        if (!panelController) continue;

        panelController->ensureInitialized(panelState);
        PanelWindow panelWindow(
            state.ui,
            panelId.c_str(),
            panelState.label.c_str(),
            panelState.open,
            panelState.closable,
            panelState.flags,
            panelState.layout,
            panelController->minSize(panelState));
        if (!panelWindow.begin())
        {
            state.ui.currentPanelId.clear();
            continue;
        }
        state.ui.currentPanelId = panelId;
        panelController->render(context, panelState);
        state.ui.currentPanelId.clear();
    }

    state.ui.pendingActions.clear();
}

bool UiManager::isInitialized() const
{
    return _renderImGui.valid();
}

vsg::ref_ptr<vsg::Visitor> UiManager::eventHandler() const
{
    return _sendEventsToImGui;
}
