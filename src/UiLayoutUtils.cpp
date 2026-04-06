#include "UiLayoutUtils.h"

#include "Widgets.h"

namespace
{
const UiWidgetSpecState* findWidgetSpec(const std::vector<UiWidgetSpecState>& widgets, std::string_view widgetId)
{
    for (const auto& widget : widgets)
    {
        if (widget.id == widgetId) return &widget;
    }
    return nullptr;
}

YogaLayout::Length flexLength(const std::optional<double>& pxValue, const std::optional<double>& flexValue)
{
    if (flexValue && *flexValue > 0.0) return YogaLayout::Length::flex(static_cast<float>(*flexValue));
    if (pxValue) return YogaLayout::Length::px(static_cast<float>(*pxValue));
    return YogaLayout::Length::autoV();
}

YogaLayout::Style styleFromFlexNode(const UiFlexNodeState& node)
{
    YogaLayout::Style style;
    style.direction = node.type == "row" ? YogaLayout::Axis::Row : YogaLayout::Axis::Column;
    style.gap = static_cast<float>(node.gap);
    style.width = flexLength(node.width, node.flex);
    style.height = node.height ? YogaLayout::Length::px(static_cast<float>(*node.height)) : YogaLayout::Length::autoV();
    return style;
}

std::string resolvedNodeSlotId(const UiFlexNodeState& node, const std::vector<UiWidgetSpecState>& widgets, std::size_t& generatedIndex)
{
    if (!node.slot.empty()) return node.slot;
    if (!node.widget.empty()) return node.widget;
    if (!node.labelFor.empty())
    {
        if (const auto* widget = findWidgetSpec(widgets, node.labelFor))
        {
            return labelSlotForWidget(*widget);
        }
        return node.labelFor + "-label";
    }
    return "flex-node-" + std::to_string(generatedIndex++);
}

std::size_t appendFlexNode(
    YogaLayout::Spec& spec,
    const UiFlexNodeState& node,
    const std::vector<UiWidgetSpecState>& widgets,
    std::optional<std::size_t> parentIndex,
    std::size_t& generatedIndex)
{
    const auto name = resolvedNodeSlotId(node, widgets, generatedIndex);
    spec.nodes.push_back(YogaLayout::Node{
        .name = name,
        .style = styleFromFlexNode(node),
        .children = {},
    });
    const auto index = spec.nodes.size() - 1;
    if (parentIndex) spec.nodes[*parentIndex].children.push_back(index);
    for (const auto& child : node.children)
    {
        appendFlexNode(spec, child, widgets, index, generatedIndex);
    }
    return index;
}
}

void queueUiCommand(UiState& uiState, const std::string& commandName, const std::string& value)
{
    if (commandName.empty()) return;
    uiState.requestedCommands.push_back(value.empty() ? commandName : (commandName + "=" + value));
}

void registerLayoutSlot(UiState& uiState, const std::string& panelId, const std::string& slotId, const UiLayoutRectState& layout)
{
    for (auto& slot : uiState.layoutSlots)
    {
        if (slot.panelId == panelId && slot.slotId == slotId)
        {
            slot.layout = layout;
            return;
        }
    }

    uiState.layoutSlots.push_back(UiLayoutSlotState{
        .panelId = panelId,
        .slotId = slotId,
        .layout = layout,
    });
}

void registerLayoutSlots(UiState& uiState, const std::string& panelId, const YogaLayout& layout, const std::vector<std::string_view>& slotIds)
{
    for (const auto slotId : slotIds)
    {
        if (layout.has(slotId))
        {
            registerLayoutSlot(uiState, panelId, std::string(slotId), layout.rect(slotId));
        }
    }
}

void registerLayoutSlots(UiState& uiState, const std::string& panelId, const YogaLayout& layout, const std::vector<std::string>& slotIds)
{
    for (const auto& slotId : slotIds)
    {
        if (layout.has(slotId))
        {
            registerLayoutSlot(uiState, panelId, slotId, layout.rect(slotId));
        }
    }
}

void setNextWidgetLayoutIfPresent(UiState& uiState, const YogaLayout& layout, std::string_view slotId)
{
    if (layout.has(slotId))
    {
        setNextWidgetLayout(uiState, layout.rect(slotId));
    }
}

std::string labelSlotForWidget(const UiWidgetSpecState& widget)
{
    if (!widget.labelSlot.empty()) return widget.labelSlot;
    return widget.id + "-label";
}

WidgetSlotBinding makeWidgetSlotBinding(
    std::string_view widgetId,
    const std::function<std::string(std::string_view)>& labelResolver)
{
    return WidgetSlotBinding{
        .valueSlotId = std::string(widgetId),
        .labelSlotId = labelResolver(widgetId),
    };
}

std::vector<std::string> collectSceneObjectIds(const SceneState& scene)
{
    std::vector<std::string> objectIds;
    objectIds.reserve(scene.objects.size());
    for (const auto& object : scene.objects) objectIds.push_back(object.id);
    return objectIds;
}

std::string renderSelectedObjectControl(
    UiState& uiState,
    const YogaLayout& layout,
    const WidgetSlotBinding& slots,
    const char* widgetId,
    const char* labelWidgetId,
    const char* labelText,
    const std::string& selectedObjectId,
    const std::vector<std::string>& objectIds)
{
    setNextWidgetLayoutIfPresent(uiState, layout, slots.labelSlotId);
    Text(uiState, labelWidgetId, labelText);

    if (objectIds.empty()) return selectedObjectId;

    setNextWidgetLayoutIfPresent(uiState, layout, slots.valueSlotId);
    const auto selectedValue = ComboBox(uiState, widgetId, "", selectedObjectId, objectIds);
    if (auto* widget = findWidget(uiState, widgetId))
    {
        widget->textValue = selectedValue;
    }
    return selectedValue;
}

YogaLayout::Spec buildYogaLayoutSpec(const UiFlexNodeState& rootNode)
{
    return buildYogaLayoutSpec(rootNode, {});
}

YogaLayout::Spec buildYogaLayoutSpec(const UiFlexNodeState& rootNode, const std::vector<UiWidgetSpecState>& widgets)
{
    YogaLayout::Spec spec;
    std::size_t generatedIndex = 0;
    spec.root = appendFlexNode(spec, rootNode, widgets, std::nullopt, generatedIndex);
    return spec;
}
