#include "UiPanelTree.h"

namespace
{
WidgetSlotBinding bindingForWidget(const UiWidgetSpecState& widget)
{
    return WidgetSlotBinding{
        .valueSlotId = valueSlotForWidget(widget),
        .labelSlotId = labelSlotForWidget(widget),
    };
}
}

UiPanelTree UiPanelTree::build(const UiPanelState& panelState)
{
    UiPanelTree tree;
    tree._layoutSpec = panelState.flexLayout ? buildYogaLayoutSpec(*panelState.flexLayout, panelState.widgets) : YogaLayout::Spec{};
    tree._slotIds = panelState.flexLayout ? collectYogaSlotIds(*panelState.flexLayout, panelState.widgets) : std::vector<std::string>{};
    tree._widgets.reserve(panelState.widgets.size());

    for (const auto& widget : panelState.widgets)
    {
        tree._widgets.push_back(UiPanelWidgetNode{
            .spec = widget,
            .slots = bindingForWidget(widget),
        });
    }

    return tree;
}

const YogaLayout::Spec& UiPanelTree::layoutSpec() const
{
    return _layoutSpec;
}

const std::vector<std::string>& UiPanelTree::slotIds() const
{
    return _slotIds;
}

const std::vector<UiPanelWidgetNode>& UiPanelTree::widgets() const
{
    return _widgets;
}

const UiPanelWidgetNode* UiPanelTree::findWidget(std::string_view widgetId) const
{
    for (const auto& widget : _widgets)
    {
        if (widget.spec.id == widgetId) return &widget;
    }
    return nullptr;
}
