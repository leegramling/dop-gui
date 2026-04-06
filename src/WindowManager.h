#pragma once

#include "AppState.h"

#include <vsg/all.h>

/**
 * @brief Lightweight window lifecycle boundary for current and future ImGui/VSG windows.
 */
class WindowManager
{
public:
    /**
     * @brief Default construct a window manager.
     */
    WindowManager() = default;

    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;

    /**
     * @brief Register the primary application window.
     * @param window Primary VSG window.
     */
    void registerPrimaryWindow(vsg::ref_ptr<vsg::Window> window);
    /**
     * @brief Refresh ImGui docking and platform-window callback status into UI state.
     * @param uiState UI state that mirrors the current callback availability.
     */
    void syncImGuiStatus(UiState& uiState) const;
    /**
     * @brief Return the registered primary window.
     * @return Primary VSG window, if any.
     */
    vsg::ref_ptr<vsg::Window> primaryWindow() const;
    /**
     * @brief Return whether the current ImGui backend exposes tear-out window callbacks.
     * @return True when the relevant ImGui platform/renderer callbacks are installed.
     */
    bool canSupportTearOutCallbacks() const;

private:
    vsg::ref_ptr<vsg::Window> _primaryWindow;
};
