#include "WindowManager.h"

#include <vsgImGui/imgui.h>

void WindowManager::registerPrimaryWindow(vsg::ref_ptr<vsg::Window> window)
{
    _primaryWindow = window;
}

void WindowManager::syncImGuiStatus(UiState& uiState) const
{
    if (!ImGui::GetCurrentContext())
    {
        uiState.dockingEnabled = false;
        uiState.viewportsEnabled = false;
        uiState.platformCreateWindowCallback = false;
        uiState.platformDestroyWindowCallback = false;
        uiState.rendererCreateWindowCallback = false;
        uiState.rendererDestroyWindowCallback = false;
        return;
    }

    const auto& io = ImGui::GetIO();
    const auto& platformIo = ImGui::GetPlatformIO();
    uiState.dockingEnabled = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0;
    uiState.viewportsEnabled = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
    uiState.platformCreateWindowCallback = platformIo.Platform_CreateWindow != nullptr;
    uiState.platformDestroyWindowCallback = platformIo.Platform_DestroyWindow != nullptr;
    uiState.rendererCreateWindowCallback = platformIo.Renderer_CreateWindow != nullptr;
    uiState.rendererDestroyWindowCallback = platformIo.Renderer_DestroyWindow != nullptr;
}

vsg::ref_ptr<vsg::Window> WindowManager::primaryWindow() const
{
    return _primaryWindow;
}

bool WindowManager::canSupportTearOutCallbacks() const
{
    if (!ImGui::GetCurrentContext()) return false;
    const auto& platformIo = ImGui::GetPlatformIO();
    return platformIo.Platform_CreateWindow != nullptr &&
           platformIo.Platform_DestroyWindow != nullptr &&
           platformIo.Renderer_CreateWindow != nullptr &&
           platformIo.Renderer_DestroyWindow != nullptr;
}
