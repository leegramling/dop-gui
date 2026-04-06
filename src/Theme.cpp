#include "Theme.h"

#include <vsgImGui/imgui.h>

void Theme::applyDefault(const std::string& mode)
{
    auto& style = ImGui::GetStyle();
    if (mode == "light")
    {
        ImGui::StyleColorsLight(&style);
    }
    else
    {
        ImGui::StyleColorsDark(&style);
    }
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
}
