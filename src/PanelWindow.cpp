#include "PanelWindow.h"

#include "Widgets.h"

#include <vsgImGui/imgui.h>

#include <cfloat>

namespace
{
ImGuiWindowFlags decodeFlags(const std::vector<std::string>& flags)
{
    ImGuiWindowFlags decoded = 0;
    for (const auto& flag : flags)
    {
        if (flag == "NoResize") decoded |= ImGuiWindowFlags_NoResize;
        else if (flag == "NoCollapse") decoded |= ImGuiWindowFlags_NoCollapse;
        else if (flag == "NoMove") decoded |= ImGuiWindowFlags_NoMove;
    }
    return decoded;
}
}

PanelWindow::PanelWindow(
    UiState& uiState,
    const char* id,
    const char* title,
    bool& isOpen,
    std::uint64_t hostViewportId,
    bool closable,
    const std::vector<std::string>& flags,
    const UiLayoutRectState& layout,
    const PanelMinSize& minSize) :
    _uiState(uiState),
    _id(id),
    _title(title),
    _isOpen(isOpen),
    _hostViewportId(hostViewportId),
    _closable(closable),
    _flags(decodeFlags(flags)),
    _layout(layout),
    _minSize(minSize)
{
}

PanelWindow::~PanelWindow()
{
    if (_began && !_uiState.testMode) ImGui::End();
}

bool PanelWindow::begin()
{
    registerWidget(_uiState, _id, "panel");
    if (_uiState.testMode)
    {
        _began = false;
        _opened = _isOpen;
        return _opened;
    }

    const auto* mainViewport = ImGui::GetMainViewport();
    if (_layout.enabled && (_hostViewportId != 0 || !_uiState.dockingEnabled))
    {
        ImVec2 windowPos(static_cast<float>(_layout.x), static_cast<float>(_layout.y));
        if (_hostViewportId == 0 && mainViewport)
        {
            windowPos.x += mainViewport->Pos.x;
            windowPos.y += mainViewport->Pos.y;
        }
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        if (_layout.width > 0.0 && _layout.height > 0.0)
        {
            ImGui::SetNextWindowSize(
                ImVec2(static_cast<float>(_layout.width), static_cast<float>(_layout.height)),
                ImGuiCond_Always);
        }
    }

    if (_minSize.enabled)
    {
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(_minSize.width, _minSize.height),
            ImVec2(FLT_MAX, FLT_MAX));
    }

    if (mainViewport)
    {
        const auto viewportId = _hostViewportId != 0
            ? static_cast<ImGuiID>(_hostViewportId)
            : mainViewport->ID;
        ImGui::SetNextWindowViewport(viewportId);
    }

    bool* openPtr = _closable ? &_isOpen : nullptr;
    _began = _isOpen;
    _opened = _isOpen && ImGui::Begin(_title, openPtr, static_cast<ImGuiWindowFlags>(_flags));
    return _opened;
}
