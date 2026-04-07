#include "WindowManager.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <xcb/xcb.h>

namespace
{
WindowManager* g_callbackOwner = nullptr;

double positiveOrZero(float value)
{
    return value > 0.0f ? static_cast<double>(value) : 0.0;
}

xcb_connection_t* nativeConnection(vsg::Window* window)
{
    if (!window || !window->traits()) return nullptr;
    auto& connection = window->traits()->systemConnection;
    if (!connection.has_value()) return nullptr;
    if (auto* ptr = std::any_cast<xcb_connection_t*>(&connection)) return *ptr;
    return nullptr;
}

std::optional<xcb_window_t> nativeWindowId(vsg::Window* window)
{
    if (!window || !window->traits()) return std::nullopt;
    auto& nativeWindow = window->traits()->nativeWindow;
    if (!nativeWindow.has_value()) return std::nullopt;
    if (auto* value = std::any_cast<xcb_window_t>(&nativeWindow)) return *value;
    if (auto* value = std::any_cast<std::uint32_t>(&nativeWindow)) return static_cast<xcb_window_t>(*value);
    return std::nullopt;
}

void applyPlatformHandles(ImGuiViewport* viewport, vsg::Window* window, void* userData)
{
    if (!viewport) return;

    viewport->PlatformUserData = userData;
    viewport->PlatformHandle = window;
    if (auto native = nativeWindowId(window))
    {
        viewport->PlatformHandleRaw = reinterpret_cast<void*>(static_cast<uintptr_t>(*native));
    }
    else
    {
        viewport->PlatformHandleRaw = window;
    }
}

void clearPlatformHandles(ImGuiViewport* viewport)
{
    if (!viewport) return;
    viewport->PlatformUserData = nullptr;
    viewport->PlatformHandle = nullptr;
    viewport->PlatformHandleRaw = nullptr;
}

void mapNativeWindow(vsg::Window* window)
{
    auto* connection = nativeConnection(window);
    auto native = nativeWindowId(window);
    if (!connection || !native) return;
    xcb_map_window(connection, *native);
    xcb_flush(connection);
}

void configureNativeWindow(vsg::Window* window, const std::optional<int32_t>& x, const std::optional<int32_t>& y,
                           const std::optional<uint32_t>& width, const std::optional<uint32_t>& height)
{
    auto* connection = nativeConnection(window);
    auto native = nativeWindowId(window);
    if (!connection || !native) return;

    uint16_t mask = 0;
    uint32_t values[4];
    int index = 0;
    if (x)
    {
        mask |= XCB_CONFIG_WINDOW_X;
        values[index++] = static_cast<uint32_t>(*x);
    }
    if (y)
    {
        mask |= XCB_CONFIG_WINDOW_Y;
        values[index++] = static_cast<uint32_t>(*y);
    }
    if (width)
    {
        mask |= XCB_CONFIG_WINDOW_WIDTH;
        values[index++] = *width;
    }
    if (height)
    {
        mask |= XCB_CONFIG_WINDOW_HEIGHT;
        values[index++] = *height;
    }

    if (mask == 0) return;
    xcb_configure_window(connection, *native, mask, values);
    xcb_flush(connection);
}

ImVec2 queryNativeWindowPos(vsg::Window* window, ImVec2 fallback)
{
    auto* connection = nativeConnection(window);
    auto native = nativeWindowId(window);
    if (!connection || !native) return fallback;

    auto cookie = xcb_get_geometry(connection, *native);
    auto geometry = xcb_get_geometry_reply(connection, cookie, nullptr);
    if (!geometry) return fallback;

    ImVec2 value(static_cast<float>(geometry->x), static_cast<float>(geometry->y));
    if (geometry->root != XCB_NONE)
    {
        auto translateCookie = xcb_translate_coordinates(connection, *native, geometry->root, 0, 0);
        auto translateReply = xcb_translate_coordinates_reply(connection, translateCookie, nullptr);
        if (translateReply)
        {
            value.x = static_cast<float>(translateReply->dst_x);
            value.y = static_cast<float>(translateReply->dst_y);
            free(translateReply);
        }
    }

    free(geometry);
    return value;
}

ImVec2 queryNativeWindowSize(vsg::Window* window, ImVec2 fallback)
{
    auto* connection = nativeConnection(window);
    auto native = nativeWindowId(window);
    if (!connection || !native) return fallback;

    auto cookie = xcb_get_geometry(connection, *native);
    auto geometry = xcb_get_geometry_reply(connection, cookie, nullptr);
    if (!geometry) return fallback;
    const ImVec2 value(static_cast<float>(geometry->width), static_cast<float>(geometry->height));
    free(geometry);
    return value;
}

void focusNativeWindow(vsg::Window* window)
{
    auto* connection = nativeConnection(window);
    auto native = nativeWindowId(window);
    if (!connection || !native) return;
    xcb_set_input_focus(connection, XCB_INPUT_FOCUS_POINTER_ROOT, *native, XCB_CURRENT_TIME);
    xcb_flush(connection);
}

void syncNativeWindowToViewport(vsg::Window* window, ImGuiViewport* viewport)
{
    if (!window || !viewport) return;
    configureNativeWindow(
        window,
        static_cast<int32_t>(viewport->Pos.x),
        static_cast<int32_t>(viewport->Pos.y),
        std::max(1u, static_cast<unsigned int>(viewport->Size.x)),
        std::max(1u, static_cast<unsigned int>(viewport->Size.y)));
}
}

WindowManager* WindowManager::callbackOwner()
{
    return g_callbackOwner;
}

void WindowManager::setCallbackOwner(WindowManager* owner)
{
    g_callbackOwner = owner;
}

void WindowManager::registerPrimaryWindow(vsg::ref_ptr<vsg::Window> window)
{
    _primaryWindow = window;
}

void WindowManager::registerViewer(vsg::ref_ptr<vsg::Viewer> viewer)
{
    _viewer = viewer;
}

void WindowManager::installImGuiPlatformCallbacks()
{
    if (!ImGui::GetCurrentContext()) return;

    setCallbackOwner(this);

    auto& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;

    auto& platformIo = ImGui::GetPlatformIO();
    _callbackState.previousRendererCreateWindow = platformIo.Renderer_CreateWindow;
    _callbackState.previousRendererDestroyWindow = platformIo.Renderer_DestroyWindow;
    _callbackState.previousRendererSetWindowSize = platformIo.Renderer_SetWindowSize;
    _callbackState.previousRendererRenderWindow = platformIo.Renderer_RenderWindow;
    _callbackState.previousRendererSwapBuffers = platformIo.Renderer_SwapBuffers;
    platformIo.Platform_CreateWindow = &WindowManager::platformCreateWindow;
    platformIo.Platform_DestroyWindow = &WindowManager::platformDestroyWindow;
    platformIo.Platform_ShowWindow = &WindowManager::platformShowWindow;
    platformIo.Platform_SetWindowPos = &WindowManager::platformSetWindowPos;
    platformIo.Platform_GetWindowPos = &WindowManager::platformGetWindowPos;
    platformIo.Platform_SetWindowSize = &WindowManager::platformSetWindowSize;
    platformIo.Platform_GetWindowSize = &WindowManager::platformGetWindowSize;
    platformIo.Platform_SetWindowFocus = &WindowManager::platformSetWindowFocus;
    platformIo.Platform_GetWindowFocus = &WindowManager::platformGetWindowFocus;
    platformIo.Platform_GetWindowMinimized = &WindowManager::platformGetWindowMinimized;
    platformIo.Platform_SetWindowTitle = &WindowManager::platformSetWindowTitle;
    platformIo.Platform_CreateVkSurface = &WindowManager::platformCreateVkSurface;
    platformIo.Renderer_CreateWindow = &WindowManager::rendererCreateWindow;
    platformIo.Renderer_DestroyWindow = &WindowManager::rendererDestroyWindow;
    platformIo.Renderer_SetWindowSize = &WindowManager::rendererSetWindowSize;

    _callbackState.platformCallbacksInstalled = true;
    _callbackState.rendererCallbacksInstalled = true;
    _callbackState.lastEvent = "callbacks_installed";
    _callbackState.lastViewportId = 0;

    installPlatformMonitorSnapshot();
    installMainViewportHandles();
}

void WindowManager::syncImGuiStatus(UiState& uiState) const
{
    uiState.primaryWindowRegistered = _primaryWindow.valid();

    if (!ImGui::GetCurrentContext())
    {
        uiState.dockingEnabled = false;
        uiState.viewportsEnabled = false;
        uiState.backendPlatformHasViewports = false;
        uiState.backendRendererHasViewports = false;
        uiState.platformCallbacksInstalled = false;
        uiState.rendererCallbacksInstalled = false;
        uiState.platformCreateWindowCallback = false;
        uiState.platformDestroyWindowCallback = false;
        uiState.rendererCreateWindowCallback = false;
        uiState.rendererDestroyWindowCallback = false;
        uiState.tearOutCallbacksSupported = false;
        uiState.hasMainViewport = false;
        uiState.viewportCount = 0;
        uiState.monitorCount = 0;
        uiState.platformCreateRequestCount = 0;
        uiState.platformDestroyRequestCount = 0;
        uiState.rendererCreateRequestCount = 0;
        uiState.rendererDestroyRequestCount = 0;
        uiState.lastTearOutEvent.clear();
        uiState.lastTearOutViewportId = 0;
        const_cast<WindowManager*>(this)->_managedWindows.clear();
        return;
    }

    const auto& io = ImGui::GetIO();
    const auto& platformIo = ImGui::GetPlatformIO();
    uiState.dockingEnabled = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0;
    uiState.viewportsEnabled = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
    uiState.backendPlatformHasViewports = (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) != 0;
    uiState.backendRendererHasViewports = (io.BackendFlags & ImGuiBackendFlags_RendererHasViewports) != 0;
    uiState.platformCallbacksInstalled = _callbackState.platformCallbacksInstalled;
    uiState.rendererCallbacksInstalled = _callbackState.rendererCallbacksInstalled;
    uiState.platformCreateWindowCallback = platformIo.Platform_CreateWindow != nullptr;
    uiState.platformDestroyWindowCallback = platformIo.Platform_DestroyWindow != nullptr;
    uiState.rendererCreateWindowCallback = platformIo.Renderer_CreateWindow != nullptr;
    uiState.rendererDestroyWindowCallback = platformIo.Renderer_DestroyWindow != nullptr;
    uiState.tearOutCallbacksSupported = canSupportTearOutCallbacks();
    uiState.hasMainViewport = ImGui::GetMainViewport() != nullptr;
    uiState.viewportCount = platformIo.Viewports.Size;
    uiState.monitorCount = platformIo.Monitors.Size;
    uiState.platformCreateRequestCount = _callbackState.platformCreateRequestCount;
    uiState.platformDestroyRequestCount = _callbackState.platformDestroyRequestCount;
    uiState.rendererCreateRequestCount = _callbackState.rendererCreateRequestCount;
    uiState.rendererDestroyRequestCount = _callbackState.rendererDestroyRequestCount;
    uiState.lastTearOutEvent = _callbackState.lastEvent;
    uiState.lastTearOutViewportId = static_cast<std::uint64_t>(_callbackState.lastViewportId);
    const_cast<WindowManager*>(this)->emitStatusToStderrIfChanged(uiState);
}

void WindowManager::realizeManagedWindows()
{
    if (!ImGui::GetCurrentContext()) return;

    for (auto& record : _managedWindows)
    {
        if (!record.pendingCreate || record.window || record.destroyed) continue;

        record.traits = createSecondaryWindowTraits(ImGui::FindViewportByID(static_cast<ImGuiID>(record.viewportId)));
        if (!record.traits) continue;

        record.window = vsg::Window::create(record.traits);
        record.hasVsgWindow = record.window.valid();
        if (!record.window) continue;

        if (auto* viewport = ImGui::FindViewportByID(static_cast<ImGuiID>(record.viewportId)))
        {
            applyPlatformHandles(viewport, record.window.get(), record.window.get());
            syncNativeWindowToViewport(record.window.get(), viewport);
        }

        record.pendingCreate = false;
    }
}

void WindowManager::retireManagedWindows()
{
    auto it = _managedWindows.begin();
    while (it != _managedWindows.end())
    {
        if (it->pendingDestroy && !it->addedToViewer)
        {
            it = _managedWindows.erase(it);
            continue;
        }
        ++it;
    }
}

vsg::ref_ptr<vsg::Window> WindowManager::primaryWindow() const
{
    return _primaryWindow;
}

bool WindowManager::canSupportTearOutCallbacks() const
{
    if (!ImGui::GetCurrentContext()) return false;

    const auto& io = ImGui::GetIO();
    const auto& platformIo = ImGui::GetPlatformIO();
    return (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) != 0 &&
           (io.BackendFlags & ImGuiBackendFlags_RendererHasViewports) != 0 &&
           platformIo.Platform_CreateWindow != nullptr &&
           platformIo.Platform_DestroyWindow != nullptr &&
           platformIo.Renderer_CreateWindow != nullptr &&
           platformIo.Renderer_DestroyWindow != nullptr;
}

bool WindowManager::hasPrimaryWindow() const
{
    return _primaryWindow.valid();
}

const std::vector<WindowManager::ManagedWindowRecord>& WindowManager::managedWindows() const
{
    return _managedWindows;
}

const WindowManager::ManagedWindowRecord* WindowManager::findManagedWindow(std::uint64_t viewportId) const
{
    for (const auto& record : _managedWindows)
    {
        if (record.viewportId == viewportId) return &record;
    }
    return nullptr;
}

void WindowManager::recordPlatformCreateWindow(ImGuiViewport* viewport)
{
    ++_callbackState.platformCreateRequestCount;
    _callbackState.lastEvent = "platform_create_window";
    _callbackState.lastViewportId = viewport ? viewport->ID : 0;
    if (viewport)
    {
        auto& record = upsertManagedWindow(viewport);
        record.platformWindowCreated = true;
        record.destroyed = false;
        syncManagedWindowFromViewport(record, viewport);
        const auto* mainViewport = ImGui::GetMainViewport();
        if (!record.window && (!mainViewport || viewport->ID != mainViewport->ID))
        {
            record.traits = createSecondaryWindowTraits(viewport);
            if (record.traits)
            {
                record.window = vsg::Window::create(record.traits);
                record.hasVsgWindow = record.window.valid();
                record.pendingCreate = false;
                if (record.window.valid())
                {
                    applyPlatformHandles(viewport, record.window.get(), record.window.get());
                    syncNativeWindowToViewport(record.window.get(), viewport);
                }
            }
        }
    }
}

void WindowManager::recordPlatformDestroyWindow(ImGuiViewport* viewport)
{
    ++_callbackState.platformDestroyRequestCount;
    _callbackState.lastEvent = "platform_destroy_window";
    _callbackState.lastViewportId = viewport ? viewport->ID : 0;
    if (viewport)
    {
        if (auto* record = findManagedWindow(viewport->ID))
        {
            record->platformWindowCreated = false;
            record->visible = false;
            record->focused = false;
            record->destroyed = true;
            record->pendingDestroy = true;
            clearPlatformHandles(viewport);
            syncManagedWindowFromViewport(*record, viewport);
        }
    }
}

void WindowManager::recordRendererCreateWindow(ImGuiViewport* viewport)
{
    ++_callbackState.rendererCreateRequestCount;
    _callbackState.lastEvent = "renderer_create_window";
    _callbackState.lastViewportId = viewport ? viewport->ID : 0;
    if (viewport)
    {
        auto& record = upsertManagedWindow(viewport);
        record.rendererWindowCreated = true;
        record.destroyed = false;
        record.hasVsgWindow = record.window.valid();
        syncManagedWindowFromViewport(record, viewport);
    }
}

void WindowManager::recordRendererDestroyWindow(ImGuiViewport* viewport)
{
    ++_callbackState.rendererDestroyRequestCount;
    _callbackState.lastEvent = "renderer_destroy_window";
    _callbackState.lastViewportId = viewport ? viewport->ID : 0;
    if (viewport)
    {
        if (auto* record = findManagedWindow(viewport->ID))
        {
            record->rendererWindowCreated = false;
            record->destroyed = true;
            record->pendingDestroy = true;
            syncManagedWindowFromViewport(*record, viewport);
        }
    }
}

void WindowManager::installPlatformMonitorSnapshot()
{
    if (!ImGui::GetCurrentContext()) return;

    auto& io = ImGui::GetIO();
    auto& platformIo = ImGui::GetPlatformIO();
    platformIo.Monitors.clear();

    ImGuiPlatformMonitor monitor;
    monitor.MainPos = ImVec2(0.0f, 0.0f);
    monitor.MainSize = io.DisplaySize;
    monitor.WorkPos = monitor.MainPos;
    monitor.WorkSize = io.DisplaySize;
    monitor.DpiScale = 1.0f;
    platformIo.Monitors.push_back(monitor);
}

void WindowManager::installMainViewportHandles()
{
    if (!ImGui::GetCurrentContext()) return;
    auto* mainViewport = ImGui::GetMainViewport();
    if (!mainViewport) return;

    applyPlatformHandles(mainViewport, _primaryWindow.get(), this);
}

WindowManager::ManagedWindowRecord& WindowManager::upsertManagedWindow(ImGuiViewport* viewport)
{
    auto* existing = findManagedWindow(viewport ? viewport->ID : 0);
    if (existing) return *existing;

    ManagedWindowRecord record;
    if (viewport) record.viewportId = viewport->ID;
    _managedWindows.push_back(std::move(record));
    return _managedWindows.back();
}

WindowManager::ManagedWindowRecord* WindowManager::findManagedWindow(ImGuiID viewportId)
{
    for (auto& record : _managedWindows)
    {
        if (record.viewportId == static_cast<std::uint64_t>(viewportId)) return &record;
    }
    return nullptr;
}

vsg::Window* WindowManager::resolveWindowForViewport(ImGuiViewport* viewport) const
{
    if (!viewport) return nullptr;
    if (const auto* mainViewport = ImGui::GetMainViewport(); mainViewport && viewport->ID == mainViewport->ID)
    {
        return _primaryWindow.get();
    }
    if (const auto* record = findManagedWindow(static_cast<std::uint64_t>(viewport->ID)))
    {
        return record->window.get();
    }
    return nullptr;
}

void WindowManager::syncManagedWindowFromViewport(ManagedWindowRecord& record, ImGuiViewport* viewport)
{
    if (!viewport) return;

    record.viewportId = viewport->ID;
    record.x = viewport->Pos.x;
    record.y = viewport->Pos.y;
    record.width = positiveOrZero(viewport->Size.x);
    record.height = positiveOrZero(viewport->Size.y);
    record.focused = (viewport->Flags & ImGuiViewportFlags_IsFocused) != 0;
    record.minimized = (viewport->Flags & ImGuiViewportFlags_IsMinimized) != 0;
    record.visible = record.platformWindowCreated && !record.minimized;
    record.ownedByApp = (viewport->Flags & ImGuiViewportFlags_OwnedByApp) != 0;
    record.hasVsgWindow = record.window.valid();
    if (record.title.empty())
    {
        record.title = "Viewport " + std::to_string(static_cast<std::uint64_t>(viewport->ID));
    }

    if (auto traits = createSecondaryWindowTraits(viewport))
    {
        record.traits = traits;
        record.traitsWindowTitle = traits->windowTitle;
        record.traitsX = traits->x;
        record.traitsY = traits->y;
        record.traitsWidth = traits->width;
        record.traitsHeight = traits->height;
        record.traitsDecoration = traits->decoration;
        record.traitsHdpi = traits->hdpi;
        record.traitsDebugLayer = traits->debugLayer;
        record.traitsApiDumpLayer = traits->apiDumpLayer;
        record.traitsSamples = traits->samples;
    }
}

vsg::ref_ptr<vsg::WindowTraits> WindowManager::createSecondaryWindowTraits(ImGuiViewport* viewport) const
{
    if (!_primaryWindow || !_primaryWindow->traits()) return {};

    auto traits = vsg::WindowTraits::create(*_primaryWindow->traits());
    if (!traits) return {};

    if (viewport)
    {
        traits->x = static_cast<int32_t>(viewport->Pos.x);
        traits->y = static_cast<int32_t>(viewport->Pos.y);
        traits->width = std::max(1u, static_cast<unsigned int>(viewport->Size.x));
        traits->height = std::max(1u, static_cast<unsigned int>(viewport->Size.y));
        traits->windowTitle = "TearOut " + std::to_string(static_cast<std::uint64_t>(viewport->ID));
    }
    traits->nativeWindow.reset();
    traits->systemConnection.reset();
    traits->device = _primaryWindow->getDevice();
    return traits;
}

void WindowManager::platformCreateWindow(ImGuiViewport* viewport)
{
    if (auto* owner = callbackOwner()) owner->recordPlatformCreateWindow(viewport);
}

void WindowManager::platformDestroyWindow(ImGuiViewport* viewport)
{
    if (auto* owner = callbackOwner()) owner->recordPlatformDestroyWindow(viewport);
}

void WindowManager::platformShowWindow(ImGuiViewport* viewport)
{
    if (auto* owner = callbackOwner(); owner)
    {
        auto* window = owner->resolveWindowForViewport(viewport);
        syncNativeWindowToViewport(window, viewport);
        mapNativeWindow(window);
    }
}

void WindowManager::platformSetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    if (viewport) viewport->Pos = pos;
    if (auto* owner = callbackOwner(); owner && viewport)
    {
        auto& record = owner->upsertManagedWindow(viewport);
        if (record.traits)
        {
            record.traits->x = static_cast<int32_t>(pos.x);
            record.traits->y = static_cast<int32_t>(pos.y);
        }
        configureNativeWindow(record.window.get(), static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y), std::nullopt, std::nullopt);
        owner->syncManagedWindowFromViewport(record, viewport);
    }
}

ImVec2 WindowManager::platformGetWindowPos(ImGuiViewport* viewport)
{
    if (auto* owner = callbackOwner(); owner && viewport)
    {
        return queryNativeWindowPos(owner->resolveWindowForViewport(viewport), viewport->Pos);
    }
    return viewport ? viewport->Pos : ImVec2(0.0f, 0.0f);
}

void WindowManager::platformSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    if (viewport) viewport->Size = size;
    if (auto* owner = callbackOwner(); owner && viewport)
    {
        auto& record = owner->upsertManagedWindow(viewport);
        if (record.traits)
        {
            record.traits->width = std::max(1u, static_cast<unsigned int>(size.x));
            record.traits->height = std::max(1u, static_cast<unsigned int>(size.y));
        }
        configureNativeWindow(record.window.get(), std::nullopt, std::nullopt,
                              std::max(1u, static_cast<unsigned int>(size.x)),
                              std::max(1u, static_cast<unsigned int>(size.y)));
        owner->syncManagedWindowFromViewport(record, viewport);
    }
}

ImVec2 WindowManager::platformGetWindowSize(ImGuiViewport* viewport)
{
    if (auto* owner = callbackOwner(); owner && viewport)
    {
        return queryNativeWindowSize(owner->resolveWindowForViewport(viewport), viewport->Size);
    }
    return viewport ? viewport->Size : ImVec2(0.0f, 0.0f);
}

void WindowManager::platformSetWindowFocus(ImGuiViewport* viewport)
{
    if (viewport) viewport->Flags |= ImGuiViewportFlags_IsFocused;
    if (auto* owner = callbackOwner(); owner && viewport)
    {
        auto& record = owner->upsertManagedWindow(viewport);
        focusNativeWindow(owner->resolveWindowForViewport(viewport));
        owner->syncManagedWindowFromViewport(record, viewport);
    }
}

bool WindowManager::platformGetWindowFocus(ImGuiViewport* viewport)
{
    return viewport ? ((viewport->Flags & ImGuiViewportFlags_IsFocused) != 0) : true;
}

bool WindowManager::platformGetWindowMinimized(ImGuiViewport* viewport)
{
    return viewport ? ((viewport->Flags & ImGuiViewportFlags_IsMinimized) != 0) : false;
}

void WindowManager::platformSetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    if (viewport && title)
    {
        if (auto* owner = callbackOwner())
        {
            auto& record = owner->upsertManagedWindow(viewport);
            record.title = title;
            if (record.traits) record.traits->windowTitle = title;
            owner->syncManagedWindowFromViewport(record, viewport);
        }
    }
}

void WindowManager::rendererCreateWindow(ImGuiViewport* viewport)
{
    if (auto* owner = callbackOwner())
    {
        owner->recordRendererCreateWindow(viewport);
        if (owner->_callbackState.previousRendererCreateWindow)
        {
            owner->_callbackState.previousRendererCreateWindow(viewport);
        }
    }
}

void WindowManager::rendererDestroyWindow(ImGuiViewport* viewport)
{
    if (auto* owner = callbackOwner())
    {
        owner->recordRendererDestroyWindow(viewport);
        if (owner->_callbackState.previousRendererDestroyWindow)
        {
            owner->_callbackState.previousRendererDestroyWindow(viewport);
        }
    }
}

void WindowManager::rendererSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    if (viewport) viewport->Size = size;
    if (auto* owner = callbackOwner(); owner && viewport)
    {
        auto& record = owner->upsertManagedWindow(viewport);
        owner->syncManagedWindowFromViewport(record, viewport);
        if (owner->_callbackState.previousRendererSetWindowSize)
        {
            owner->_callbackState.previousRendererSetWindowSize(viewport, size);
        }
    }
}

int WindowManager::platformCreateVkSurface(ImGuiViewport* viewport, ImU64, const void*, ImU64* outVkSurface)
{
    if (!viewport || !outVkSurface) return VK_ERROR_INITIALIZATION_FAILED;

    if (auto* owner = callbackOwner())
    {
        if (auto* record = owner->findManagedWindow(viewport->ID))
        {
            if (record->window && record->window->getSurface())
            {
                *outVkSurface = reinterpret_cast<ImU64>(static_cast<VkSurfaceKHR>(*(record->window->getSurface())));
                return VK_SUCCESS;
            }
        }
    }

    return VK_ERROR_SURFACE_LOST_KHR;
}

void WindowManager::emitStatusToStderrIfChanged(const UiState& uiState)
{
    if (uiState.testMode) return;

    std::ostringstream out;
    out << "[tearout] "
        << "primary=" << (uiState.primaryWindowRegistered ? "1" : "0")
        << " docking=" << (uiState.dockingEnabled ? "1" : "0")
        << " viewports=" << (uiState.viewportsEnabled ? "1" : "0")
        << " backend_platform=" << (uiState.backendPlatformHasViewports ? "1" : "0")
        << " backend_renderer=" << (uiState.backendRendererHasViewports ? "1" : "0")
        << " callbacks_installed=" << (uiState.platformCallbacksInstalled && uiState.rendererCallbacksInstalled ? "1" : "0")
        << " platform_create_cb=" << (uiState.platformCreateWindowCallback ? "1" : "0")
        << " renderer_create_cb=" << (uiState.rendererCreateWindowCallback ? "1" : "0")
        << " supported=" << (uiState.tearOutCallbacksSupported ? "1" : "0")
        << " viewports_count=" << uiState.viewportCount
        << " monitors=" << uiState.monitorCount
        << " platform_create_requests=" << uiState.platformCreateRequestCount
        << " renderer_create_requests=" << uiState.rendererCreateRequestCount
        << " managed_windows=" << _managedWindows.size()
        << " last_event=" << (uiState.lastTearOutEvent.empty() ? "none" : uiState.lastTearOutEvent)
        << " last_viewport=" << uiState.lastTearOutViewportId;

    const auto message = out.str();
    if (message == _lastStatusLog) return;
    _lastStatusLog = message;
    std::cerr << message << '\n';
}
