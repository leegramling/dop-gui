#include "Panel.h"

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

Panel::Panel(
    UiState& uiState,
    const char* id,
    const char* title,
    bool& isOpen,
    bool closable,
    const std::vector<std::string>& flags,
    const UiLayoutRectState& layout,
    const PanelMinSize& minSize) :
    _uiState(uiState),
    _id(id),
    _title(title),
    _isOpen(isOpen),
    _closable(closable),
    _flags(decodeFlags(flags)),
    _layout(layout),
    _minSize(minSize)
{
}

Panel::~Panel()
{
    if (_opened && !_uiState.testMode) ImGui::End();
}

bool Panel::begin()
{
    registerWidget(_uiState, _id, "panel");
    if (_uiState.testMode)
    {
        _opened = _isOpen;
        return _opened;
    }

    if (_layout.enabled)
    {
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(_layout.x), static_cast<float>(_layout.y)), ImGuiCond_Once);
        if (_layout.width > 0.0 && _layout.height > 0.0)
        {
            ImGui::SetNextWindowSize(ImVec2(static_cast<float>(_layout.width), static_cast<float>(_layout.height)), ImGuiCond_Once);
        }
    }

    if (_minSize.enabled)
    {
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(_minSize.width, _minSize.height),
            ImVec2(FLT_MAX, FLT_MAX));
    }

    bool* openPtr = _closable ? &_isOpen : nullptr;
    _opened = _isOpen && ImGui::Begin(_title, openPtr, static_cast<ImGuiWindowFlags>(_flags));
    return _opened;
}
