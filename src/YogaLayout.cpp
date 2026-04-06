#include "YogaLayout.h"

#include <algorithm>
#include <stdexcept>

std::size_t YogaLayout::Builder::addNode(std::string_view name, const Style& style, bool pushToStack)
{
    _spec.nodes.push_back(Node{std::string(name), style, {}});
    const auto index = _spec.nodes.size() - 1;
    if (!_stack.empty()) _spec.nodes[_stack.back()].children.push_back(index);
    if (pushToStack) _stack.push_back(index);
    return index;
}

YogaLayout::Builder& YogaLayout::Builder::root(std::string_view name, const Style& style)
{
    _spec = {};
    _stack.clear();
    _spec.root = addNode(name, style, true);
    return *this;
}

YogaLayout::Builder& YogaLayout::Builder::begin(std::string_view name, const Style& style)
{
    addNode(name, style, true);
    return *this;
}

YogaLayout::Builder& YogaLayout::Builder::item(std::string_view name, const Style& style)
{
    addNode(name, style, false);
    return *this;
}

YogaLayout::Builder& YogaLayout::Builder::end()
{
    if (_stack.size() > 1) _stack.pop_back();
    return *this;
}

YogaLayout::Spec YogaLayout::Builder::build()
{
    return _spec;
}

YogaLayout::~YogaLayout()
{
    destroyYogaTree();
}

void YogaLayout::destroyYogaTree()
{
    for (auto node : _yogaNodes)
    {
        if (node) YGNodeFree(node);
    }
    _yogaNodes.clear();
    if (_config)
    {
        YGConfigFree(_config);
        _config = nullptr;
    }
}

void YogaLayout::applyStyle(YGNodeRef node, const Style& style)
{
    YGNodeStyleSetFlexDirection(node, style.direction == Axis::Row ? YGFlexDirectionRow : YGFlexDirectionColumn);
    YGNodeStyleSetGap(node, YGGutterAll, std::max(0.0f, style.gap));

    const auto setDim = [node](Length len, bool isWidth)
    {
        switch (len.kind)
        {
        case LengthKind::Px:
            if (isWidth) YGNodeStyleSetWidth(node, std::max(0.0f, len.value));
            else YGNodeStyleSetHeight(node, std::max(0.0f, len.value));
            break;
        case LengthKind::Percent:
            if (isWidth) YGNodeStyleSetWidthPercent(node, len.value);
            else YGNodeStyleSetHeightPercent(node, len.value);
            break;
        case LengthKind::Auto:
            if (isWidth) YGNodeStyleSetWidthAuto(node);
            else YGNodeStyleSetHeightAuto(node);
            break;
        case LengthKind::Flex:
            YGNodeStyleSetFlexGrow(node, std::max(0.0f, len.value));
            YGNodeStyleSetFlexShrink(node, 1.0f);
            YGNodeStyleSetFlexBasis(node, 0.0f);
            break;
        }
    };

    setDim(style.width, true);
    setDim(style.height, false);
}

void YogaLayout::setLayout(Spec spec)
{
    destroyYogaTree();
    _spec = std::move(spec);
    _rects.clear();
    if (_spec.nodes.empty()) return;

    _config = YGConfigNew();
    YGConfigSetUseWebDefaults(_config, true);

    _yogaNodes.resize(_spec.nodes.size(), nullptr);
    for (std::size_t index = 0; index < _spec.nodes.size(); ++index)
    {
        _yogaNodes[index] = YGNodeNewWithConfig(_config);
        applyStyle(_yogaNodes[index], _spec.nodes[index].style);
    }

    for (std::size_t index = 0; index < _spec.nodes.size(); ++index)
    {
        for (auto childIndex : _spec.nodes[index].children)
        {
            YGNodeInsertChild(_yogaNodes[index], _yogaNodes[childIndex], YGNodeGetChildCount(_yogaNodes[index]));
        }
    }
}

void YogaLayout::resize(float x, float y, float width, float height)
{
    if (_spec.nodes.empty() || _yogaNodes.empty()) return;
    _rects.clear();
    YGNodeCalculateLayout(_yogaNodes[_spec.root], std::max(0.0f, width), std::max(0.0f, height), YGDirectionLTR);
    computeRects(_spec.root, x, y);
}

bool YogaLayout::has(std::string_view name) const
{
    return _rects.find(std::string(name)) != _rects.end();
}

const UiLayoutRectState& YogaLayout::rect(std::string_view name) const
{
    const auto it = _rects.find(std::string(name));
    if (it == _rects.end()) throw std::runtime_error("YogaLayout missing rect: " + std::string(name));
    return it->second;
}

void YogaLayout::computeRects(std::size_t nodeIndex, float absX, float absY)
{
    const auto& node = _spec.nodes[nodeIndex];
    const auto yogaNode = _yogaNodes[nodeIndex];
    const float left = YGNodeLayoutGetLeft(yogaNode);
    const float top = YGNodeLayoutGetTop(yogaNode);
    const float width = YGNodeLayoutGetWidth(yogaNode);
    const float height = YGNodeLayoutGetHeight(yogaNode);

    const float nodeAbsX = absX + left;
    const float nodeAbsY = absY + top;
    _rects[node.name] = UiLayoutRectState{
        .x = nodeAbsX,
        .y = nodeAbsY,
        .width = width,
        .height = height,
        .enabled = true,
    };

    for (const auto childIndex : node.children)
    {
        computeRects(childIndex, nodeAbsX, nodeAbsY);
    }
}
