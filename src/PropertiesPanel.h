#pragma once

#include "Panel.h"

/** Simple ImGui-flow selected-object properties panel. */
class PropertiesPanel final : public Panel
{
public:
    std::string_view id() const override;
    PanelMinSize minSize(const UiPanelState& panelState) const override;
    void render(PanelContext& context, const UiPanelState& panelState) override;
};
