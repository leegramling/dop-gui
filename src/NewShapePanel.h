#pragma once

#include "Panel.h"

#include <string>

/** Simple ImGui-flow panel for creating new shapes. */
class NewShapePanel final : public Panel
{
public:
    std::string_view id() const override;
    PanelMinSize minSize(const UiPanelState& panelState) const override;
    void render(PanelContext& context, const UiPanelState& panelState) override;

private:
    void resetForm();

    std::string _shapeKind = "Sphere";
    double _positionX = 0.0;
    double _positionY = 0.0;
    double _positionZ = 0.0;
    double _rotationX = 0.0;
    double _rotationY = 0.0;
    double _rotationZ = 0.0;
    double _scaleX = 1.0;
    double _scaleY = 1.0;
    double _scaleZ = 1.0;
    std::string _colorHex = "#00FF00";
    std::string _status = "Ready";
};
