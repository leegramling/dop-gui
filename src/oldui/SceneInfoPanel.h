#pragma once

#include "Panel.h"
#include "UiPanelTree.h"

/**
 * @brief Scene information panel controller.
 */
class SceneInfoPanel final : public Panel
{
public:
    /**
     * @brief Return the stable authored panel identifier.
     * @return Scene info panel id.
     */
    std::string_view id() const override;
    /**
     * @brief Return the panel minimum size requirement.
     * @param panelState Authored panel specification.
     * @return Minimum window size for the scene info panel.
     */
    PanelMinSize minSize(const UiPanelState& panelState) const override;
    /**
     * @brief Render the scene info panel.
     * @param context Shared app/UI context.
     * @param panelState Authored panel specification.
     */
    void render(PanelContext& context, const UiPanelState& panelState) override;

protected:
    /**
     * @brief Build the declarative runtime tree the first time the panel is used.
     * @param panelState Authored panel specification.
     */
    void init(const UiPanelState& panelState) override;

private:
    UiPanelTree _root;
};
