#pragma once

#include "Panel.h"
#include "YogaLayout.h"

#include <string>
#include <vector>

/**
 * @brief Hand-coded development example panel for creating new shapes.
 */
class NewShapePanel final : public Panel
{
public:
    /**
     * @brief Return the stable authored panel identifier.
     * @return New shape panel id.
     */
    std::string_view id() const override;
    /**
     * @brief Return the panel minimum size requirement.
     * @param panelState Authored panel specification.
     * @return Minimum window size for the new-shape panel.
     */
    PanelMinSize minSize(const UiPanelState& panelState) const override;
    /**
     * @brief Render the new-shape development panel.
     * @param context Shared app/UI context.
     * @param panelState Authored panel specification.
     */
    void render(PanelContext& context, const UiPanelState& panelState) override;

protected:
    /**
     * @brief Build the hand-coded Yoga layout for the panel.
     * @param panelState Authored panel specification.
     */
    void init(const UiPanelState& panelState) override;

private:
    /**
     * @brief Restore the form fields to their default creation values.
     */
    void resetForm();

    YogaLayout::Spec _layoutSpec;
    std::vector<std::string> _slotIds;
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
