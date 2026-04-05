#pragma once

#include "AppState.h"

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
     */
    Panel(UiState& uiState, const char* id, const char* title);
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
    bool _opened = false;
};
