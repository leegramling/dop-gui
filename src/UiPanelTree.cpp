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
        tree._widgets.push_back(BuiltWidget{
            .node = UiPanelWidgetNode{
                .spec = widget,
                .slots = bindingForWidget(widget),
            },
            .renderer = {},
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

std::vector<UiPanelWidgetNode> UiPanelTree::widgets() const
{
    std::vector<UiPanelWidgetNode> widgets;
    widgets.reserve(_widgets.size());
    for (const auto& widget : _widgets) widgets.push_back(widget.node);
    return widgets;
}

const UiPanelWidgetNode* UiPanelTree::findWidget(std::string_view widgetId) const
{
    for (const auto& widget : _widgets)
    {
        if (widget.node.spec.id == widgetId) return &widget.node;
    }
    return nullptr;
}

void UiPanelTree::setWidgetRenderer(std::string_view widgetId, WidgetRenderer renderer)
{
    for (auto& widget : _widgets)
    {
        if (widget.node.spec.id == widgetId)
        {
            widget.renderer = std::move(renderer);
            return;
        }
    }
}

void UiPanelTree::render(UiPanelRenderContext& context) const
{
    for (const auto& widget : _widgets)
    {
        if (widget.renderer) widget.renderer(context, widget.node);
    }
}
