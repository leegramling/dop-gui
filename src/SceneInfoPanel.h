#pragma once

#include "Panel.h"

/** Simple ImGui-flow scene information panel. */
class SceneInfoPanel final : public Panel
{
public:
    std::string_view id() const override;
    PanelMinSize minSize(const UiPanelState& panelState) const override;
    void render(PanelContext& context, const UiPanelState& panelState) override;
};
