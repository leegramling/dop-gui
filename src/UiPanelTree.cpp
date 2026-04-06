#include "UiPanelTree.h"

#include "Panel.h"
#include "Widgets.h"

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

UiPanelTree::BuiltWidget* UiPanelTree::findBuiltWidget(std::string_view widgetId)
{
    for (auto& widget : _widgets)
    {
        if (widget.node.spec.id == widgetId) return &widget;
    }
    return nullptr;
}

void UiPanelTree::setWidgetRenderer(std::string_view widgetId, WidgetRenderer renderer)
{
    if (auto* widget = findBuiltWidget(widgetId))
    {
        widget->renderer = std::move(renderer);
    }
}

void UiPanelTree::bindText(std::string_view widgetId, TextBindingReader reader)
{
    setWidgetRenderer(widgetId, [reader = std::move(reader)](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
    {
        auto& state = context.panelContext.state;
        const auto value = reader(state);
        if (!value) return;
        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.valueSlotId);
        Text(state.ui, node.spec.id.c_str(), *value);
    });
}

void UiPanelTree::bindCheckbox(std::string_view widgetId, BoolBindingAccessor accessor)
{
    setWidgetRenderer(widgetId, [accessor = std::move(accessor)](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
    {
        auto& state = context.panelContext.state;
        auto* value = accessor(state);
        if (!value) return;
        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.valueSlotId);
        const bool changed = Checkbox(state.ui, node.spec.id.c_str(), node.spec.label.c_str(), *value);
        if (changed && !node.spec.onChange.empty())
        {
            queueUiCommand(state.ui, node.spec.onChange, *value ? "true" : "false");
        }
    });
}

void UiPanelTree::bindStringInput(std::string_view widgetId, StringBindingAccessor accessor)
{
    setWidgetRenderer(widgetId, [accessor = std::move(accessor)](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
    {
        auto& state = context.panelContext.state;
        auto* value = accessor(state);
        if (!value) return;
        const auto previousValue = *value;
        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.valueSlotId);
        *value = Input(state.ui, node.spec.id.c_str(), node.spec.label.c_str(), *value);
        if (*value != previousValue && !node.spec.onChange.empty())
        {
            queueUiCommand(state.ui, node.spec.onChange, *value);
        }
        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.labelSlotId);
        Text(state.ui, node.slots.labelSlotId.c_str(), node.spec.label);
    });
}

void UiPanelTree::bindStringCombo(std::string_view widgetId, StringBindingAccessor accessor)
{
    setWidgetRenderer(widgetId, [accessor = std::move(accessor)](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
    {
        auto& state = context.panelContext.state;
        auto* value = accessor(state);
        if (!value) return;
        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.valueSlotId);
        const auto selected = ComboBox(state.ui, node.spec.id.c_str(), node.spec.label.c_str(), *value, node.spec.options);
        if (selected != *value && !node.spec.onChange.empty())
        {
            queueUiCommand(state.ui, node.spec.onChange, selected);
        }
        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.labelSlotId);
        Text(state.ui, node.slots.labelSlotId.c_str(), node.spec.label);
    });
}

void UiPanelTree::bindRadioChoice(std::string_view widgetId, StringBindingAccessor accessor)
{
    setWidgetRenderer(widgetId, [accessor = std::move(accessor)](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
    {
        auto& state = context.panelContext.state;
        auto* value = accessor(state);
        if (!value) return;
        const bool selected = *value == node.spec.arg;
        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.valueSlotId);
        if (RadioButton(state.ui, node.spec.id.c_str(), node.spec.label.c_str(), selected) && !node.spec.onClick.empty())
        {
            queueUiCommand(state.ui, node.spec.onClick, node.spec.arg);
        }
        if (auto* widgetState = ::findWidget(state.ui, state.ui.currentPanelId, node.spec.id))
        {
            widgetState->boolValue = selected;
        }
    });
}

void UiPanelTree::bindDoubleInput(std::string_view widgetId, DoubleBindingAccessor accessor)
{
    setWidgetRenderer(widgetId, [accessor = std::move(accessor)](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
    {
        auto& state = context.panelContext.state;
        auto* value = accessor(state);
        if (!value) return;
        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.labelSlotId);
        Text(state.ui, node.slots.labelSlotId.c_str(), node.spec.label);

        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.valueSlotId);
        *value = InputDouble(
            state.ui,
            node.spec.id.c_str(),
            "",
            *value,
            node.spec.precision,
            node.spec.unit.empty() ? nullptr : node.spec.unit.c_str());
    });
}

void UiPanelTree::render(UiPanelRenderContext& context) const
{
    for (const auto& widget : _widgets)
    {
        if (widget.renderer) widget.renderer(context, widget.node);
    }
}
