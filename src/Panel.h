#pragma once

#include "AppState.h"

#include <cstdint>

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
 * @brief Narrow ImGui panel wrapper with test-friendly labeling.
 */
class Panel
{
public:
    /**
     * @brief Construct a panel wrapper.
     * @param uiState UI-local state used for registry and test evaluation.
     * @param id Stable panel identifier used for testing and registry lookups.
     * @param title Visible ImGui panel title.
     * @param isOpen Panel open state.
     * @param closable Whether the panel should expose a close button.
     * @param flags Authored ImGui window flags.
     * @param layout Authored panel layout rectangle.
     * @param minSize Minimum panel size constraint.
     */
    Panel(
        UiState& uiState,
        const char* id,
        const char* title,
        bool& isOpen,
        bool closable,
        const std::vector<std::string>& flags,
        const UiLayoutRectState& layout,
        const PanelMinSize& minSize = {});
    /**
     * @brief Destroy the panel wrapper and close any opened ImGui panel.
     */
    ~Panel();

    /**
     * @brief Begin the panel for the current frame.
     * @return True when the panel contents should be emitted this frame.
     */
    bool begin();

private:
    UiState& _uiState;
    const char* _id;
    const char* _title;
    bool& _isOpen;
    bool _closable = true;
    std::uint32_t _flags = 0;
    UiLayoutRectState _layout;
    PanelMinSize _minSize;
    bool _opened = false;
};
