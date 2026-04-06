#pragma once

#include "Panel.h"

#include <cstdint>

/**
 * @brief Narrow ImGui window wrapper with test-friendly labeling.
 */
class PanelWindow
{
public:
    /**
     * @brief Construct a panel window wrapper.
     * @param uiState UI-local state used for registry and test evaluation.
     * @param id Stable panel identifier used for testing and registry lookups.
     * @param title Visible ImGui panel title.
     * @param isOpen Panel open state.
     * @param closable Whether the panel should expose a close button.
     * @param flags Authored ImGui window flags.
     * @param layout Authored panel layout rectangle.
     * @param minSize Minimum panel size constraint.
     */
    PanelWindow(
        UiState& uiState,
        const char* id,
        const char* title,
        bool& isOpen,
        bool closable,
        const std::vector<std::string>& flags,
        const UiLayoutRectState& layout,
        const PanelMinSize& minSize = {});
    /**
     * @brief Destroy the panel window wrapper and close any opened ImGui window.
     */
    ~PanelWindow();

    /**
     * @brief Begin the window for the current frame.
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
