#include "Panel.h"

#include "Widgets.h"

#include <vsgImGui/imgui.h>

Panel::Panel(UiState& uiState, const char* id, const char* title) :
    _uiState(uiState),
    _id(id),
    _title(title)
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
        _opened = true;
        return true;
    }

    _opened = ImGui::Begin(_title);
    return _opened;
}
