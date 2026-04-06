#pragma once

#include "AppState.h"

#include <string_view>

class WindowManager;

/**
 * @brief Minimum panel size constraint applied through ImGui window sizing.
 */
struct PanelMinSize
{
    float width = 0.0f;
    float height = 0.0f;
    bool enabled = false;
};

/**
 * @brief Shared application-facing context passed to panel controllers.
 */
struct PanelContext
{
    AppState& state;
    WindowManager* windowManager = nullptr;
};

/**
 * @brief Base class for authored UI panels.
 */
class Panel
{
public:
    virtual ~Panel() = default;

    /**
     * @brief Return the stable authored panel identifier.
     * @return Stable panel id used to match authored panel state.
     */
    virtual std::string_view id() const = 0;

    /**
     * @brief Return the current minimum size requirement for the panel.
     * @param panelState Authored panel specification for this panel.
     * @return Minimum window size constraint for the panel.
     */
    virtual PanelMinSize minSize(const UiPanelState& panelState) const = 0;

    /**
     * @brief Ensure the panel has initialized any long-lived layout/controller state.
     * @param panelState Authored panel specification for this panel.
     */
    void ensureInitialized(const UiPanelState& panelState)
    {
        if (_initialized) return;
        init(panelState);
        _initialized = true;
    }

    /**
     * @brief Render the panel contents for the current frame.
     * @param context Shared app/UI context.
     * @param panelState Authored panel specification for this panel.
     */
    virtual void render(PanelContext& context, const UiPanelState& panelState) = 0;

protected:
    /**
     * @brief Initialize any panel-local state the first time the panel is used.
     * @param panelState Authored panel specification for this panel.
     */
    virtual void init(const UiPanelState& panelState)
    {
        (void)panelState;
    }

private:
    bool _initialized = false;
};
