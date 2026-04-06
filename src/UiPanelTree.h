#pragma once

#include "AppState.h"
#include "UiLayoutUtils.h"
#include "YogaLayout.h"

#include <string_view>
#include <vector>

/**
 * @brief Built widget node resolved from an authored panel spec.
 */
struct UiPanelWidgetNode
{
    UiWidgetSpecState spec;
    WidgetSlotBinding slots;
};

/**
 * @brief Built runtime tree metadata for a declarative authored panel.
 */
class UiPanelTree
{
public:
    /**
     * @brief Build a runtime panel tree from an authored panel specification.
     * @param panelState Authored panel specification.
     * @return Built runtime tree for the panel.
     */
    static UiPanelTree build(const UiPanelState& panelState);

    /**
     * @brief Return the Yoga layout spec for the built panel tree.
     * @return Built Yoga layout specification.
     */
    const YogaLayout::Spec& layoutSpec() const;

    /**
     * @brief Return the slot ids the built panel tree expects to register.
     * @return Ordered layout slot ids.
     */
    const std::vector<std::string>& slotIds() const;

    /**
     * @brief Return the built widget nodes for the panel.
     * @return Built widget nodes.
     */
    const std::vector<UiPanelWidgetNode>& widgets() const;

    /**
     * @brief Find a built widget node by widget id.
     * @param widgetId Stable widget identifier.
     * @return Matching built widget node, or null when not found.
     */
    const UiPanelWidgetNode* findWidget(std::string_view widgetId) const;

private:
    YogaLayout::Spec _layoutSpec;
    std::vector<std::string> _slotIds;
    std::vector<UiPanelWidgetNode> _widgets;
};
