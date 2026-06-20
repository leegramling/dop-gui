#include "Theme.h"

#include <vsgImGui/imgui.h>

void Theme::applyDefault(const std::string& mode, float scale)
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
    style.WindowRounding = 6.0f * scale;
    style.FrameRounding = 4.0f * scale;
    style.GrabRounding = 4.0f * scale;
}
