#pragma once

#include "AppState.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <vsg/all.h>
#include <vsgImGui/imgui.h>

/**
 * @brief Lightweight window lifecycle boundary for current and future ImGui/VSG windows.
 */
class WindowManager
{
public:
    /**
     * @brief Snapshot of a managed tear-out viewport and its derived secondary window traits.
     */
    struct ManagedWindowRecord
    {
        std::uint64_t viewportId = 0;
        std::string title;
        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;
        bool visible = false;
        bool focused = false;
        bool minimized = false;
        bool destroyed = false;
        bool platformWindowCreated = false;
        bool rendererWindowCreated = false;
        bool ownedByApp = false;
        std::string traitsWindowTitle;
        int traitsX = 0;
        int traitsY = 0;
        unsigned int traitsWidth = 0;
        unsigned int traitsHeight = 0;
        bool traitsDecoration = true;
        bool traitsHdpi = true;
        bool traitsDebugLayer = false;
        bool traitsApiDumpLayer = false;
        unsigned int traitsSamples = 0;
    };

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
     * @brief Install WindowManager-owned ImGui platform/renderer callback trampolines.
     *
     * This installs callback seams without yet advertising full backend viewport support.
     * That allows the application to observe and count callback traffic before enabling
     * the complete ImGui multi-viewport contract.
     */
    void installImGuiPlatformCallbacks();
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
    /**
     * @brief Return whether a primary VSG window is currently registered.
     * @return True when the primary window is available.
     */
    bool hasPrimaryWindow() const;
    /**
     * @brief Return the currently tracked secondary viewport windows.
     * @return Immutable managed-window records.
     */
    const std::vector<ManagedWindowRecord>& managedWindows() const;
    /**
     * @brief Find a managed window by viewport id.
     * @param viewportId ImGui viewport identifier.
     * @return Managed window snapshot when known.
     */
    const ManagedWindowRecord* findManagedWindow(std::uint64_t viewportId) const;

private:
    struct CallbackState
    {
        bool platformCallbacksInstalled = false;
        bool rendererCallbacksInstalled = false;
        int platformCreateRequestCount = 0;
        int platformDestroyRequestCount = 0;
        int rendererCreateRequestCount = 0;
        int rendererDestroyRequestCount = 0;
        std::string lastEvent;
        ImGuiID lastViewportId = 0;
    };

    void recordPlatformCreateWindow(ImGuiViewport* viewport);
    void recordPlatformDestroyWindow(ImGuiViewport* viewport);
    void recordRendererCreateWindow(ImGuiViewport* viewport);
    void recordRendererDestroyWindow(ImGuiViewport* viewport);
    void installPlatformMonitorSnapshot();
    void installMainViewportHandles();
    void emitStatusToStderrIfChanged(const UiState& uiState);
    ManagedWindowRecord& upsertManagedWindow(ImGuiViewport* viewport);
    ManagedWindowRecord* findManagedWindow(ImGuiID viewportId);
    void syncManagedWindowFromViewport(ManagedWindowRecord& record, ImGuiViewport* viewport);
    vsg::ref_ptr<vsg::WindowTraits> createSecondaryWindowTraits(ImGuiViewport* viewport) const;

    static WindowManager* callbackOwner();
    static void setCallbackOwner(WindowManager* owner);
    static void platformCreateWindow(ImGuiViewport* viewport);
    static void platformDestroyWindow(ImGuiViewport* viewport);
    static void platformShowWindow(ImGuiViewport* viewport);
    static void platformSetWindowPos(ImGuiViewport* viewport, ImVec2 pos);
    static ImVec2 platformGetWindowPos(ImGuiViewport* viewport);
    static void platformSetWindowSize(ImGuiViewport* viewport, ImVec2 size);
    static ImVec2 platformGetWindowSize(ImGuiViewport* viewport);
    static void platformSetWindowFocus(ImGuiViewport* viewport);
    static bool platformGetWindowFocus(ImGuiViewport* viewport);
    static bool platformGetWindowMinimized(ImGuiViewport* viewport);
    static void platformSetWindowTitle(ImGuiViewport* viewport, const char* title);
    static void rendererCreateWindow(ImGuiViewport* viewport);
    static void rendererDestroyWindow(ImGuiViewport* viewport);
    static void rendererSetWindowSize(ImGuiViewport* viewport, ImVec2 size);

    vsg::ref_ptr<vsg::Window> _primaryWindow;
    CallbackState _callbackState;
    std::vector<ManagedWindowRecord> _managedWindows;
    std::string _lastStatusLog;
};
