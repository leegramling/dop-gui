#pragma once

#include "Panel.h"

/**
 * @brief Selected-object properties panel controller.
 */
class PropertiesPanel final : public Panel
{
public:
    /**
     * @brief Return the stable authored panel identifier.
     * @return Properties panel id.
     */
    std::string_view id() const override;
    /**
     * @brief Return the panel minimum size requirement.
     * @param panelState Authored panel specification.
     * @return Minimum window size for the properties panel.
     */
    PanelMinSize minSize(const UiPanelState& panelState) const override;
    /**
     * @brief Render the properties panel.
     * @param context Shared app/UI context.
     * @param panelState Authored panel specification.
     */
    void render(PanelContext& context, const UiPanelState& panelState) override;
};
