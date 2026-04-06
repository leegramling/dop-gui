#include "WindowManager.h"

#include <vsgImGui/imgui.h>

void WindowManager::registerPrimaryWindow(vsg::ref_ptr<vsg::Window> window)
{
    _primaryWindow = window;
}

void WindowManager::syncImGuiStatus(UiState& uiState) const
{
    uiState.primaryWindowRegistered = _primaryWindow.valid();

    if (!ImGui::GetCurrentContext())
    {
        uiState.dockingEnabled = false;
        uiState.viewportsEnabled = false;
        uiState.platformCreateWindowCallback = false;
        uiState.platformDestroyWindowCallback = false;
        uiState.rendererCreateWindowCallback = false;
        uiState.rendererDestroyWindowCallback = false;
        uiState.tearOutCallbacksSupported = false;
        uiState.hasMainViewport = false;
        uiState.viewportCount = 0;
        uiState.monitorCount = 0;
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
    uiState.tearOutCallbacksSupported = canSupportTearOutCallbacks();
    uiState.hasMainViewport = ImGui::GetMainViewport() != nullptr;
    uiState.viewportCount = platformIo.Viewports.Size;
    uiState.monitorCount = platformIo.Monitors.Size;
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

bool WindowManager::hasPrimaryWindow() const
{
    return _primaryWindow.valid();
}
