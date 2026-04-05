#include "Panel.h"

#include "Widgets.h"

#include <vsgImGui/imgui.h>

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

Panel::Panel(UiState& uiState, const char* id, const char* title, bool& isOpen, bool closable, const std::vector<std::string>& flags) :
    _uiState(uiState),
    _id(id),
    _title(title),
    _isOpen(isOpen),
    _closable(closable),
    _flags(decodeFlags(flags))
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

    bool* openPtr = _closable ? &_isOpen : nullptr;
    _opened = _isOpen && ImGui::Begin(_title, openPtr, static_cast<ImGuiWindowFlags>(_flags));
    return _opened;
}
