#include "Theme.h"

#include <vsgImGui/imgui.h>

void Theme::applyDefault()
{
    auto& style = ImGui::GetStyle();
    ImGui::StyleColorsDark(&style);
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
}
