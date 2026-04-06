#pragma once

#include "AppState.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <yoga/Yoga.h>

/**
 * @brief Minimal Yoga-backed panel-local layout adapter.
 */
class YogaLayout
{
public:
    /**
     * @brief Primary axis for a layout node.
     */
    enum class Axis
    {
        Row,
        Column
    };

    /**
     * @brief Supported sizing modes for the first Yoga slice.
     */
    enum class LengthKind
    {
        Auto,
        Px,
        Percent,
        Flex
    };

    /**
     * @brief Simple length value for Yoga-backed nodes.
     */
    struct Length
    {
        LengthKind kind = LengthKind::Auto;
        float value = 0.0f;

        static Length autoV() { return {LengthKind::Auto, 0.0f}; }
        static Length px(float v) { return {LengthKind::Px, v}; }
        static Length percent(float v) { return {LengthKind::Percent, v}; }
        static Length flex(float v = 1.0f) { return {LengthKind::Flex, v}; }
    };

    /**
     * @brief Basic style attributes for a Yoga layout node.
     */
    struct Style
    {
        Axis direction = Axis::Column;
        float gap = 0.0f;
        Length width = Length::autoV();
        Length height = Length::autoV();
    };

    /**
     * @brief One named node in the authored Yoga layout tree.
     */
    struct Node
    {
        std::string name;
        Style style;
        std::vector<std::size_t> children;
    };

    /**
     * @brief Plain layout tree specification.
     */
    struct Spec
    {
        std::vector<Node> nodes;
        std::size_t root = 0;
    };

    /**
     * @brief Builder for the first Yoga layout tree shape.
     */
    class Builder
    {
    public:
        /**
         * @brief Start a new layout tree with the named root node.
         * @param name Stable node name.
         * @param style Style for the root node.
         * @return Builder reference.
         */
        Builder& root(std::string_view name, const Style& style);
        /**
         * @brief Add a container node and push it as the current parent.
         * @param name Stable node name.
         * @param style Style for the container.
         * @return Builder reference.
         */
        Builder& begin(std::string_view name, const Style& style);
        /**
         * @brief Add a leaf node under the current parent.
         * @param name Stable node name.
         * @param style Style for the leaf node.
         * @return Builder reference.
         */
        Builder& item(std::string_view name, const Style& style);
        /**
         * @brief Pop the current parent container.
         * @return Builder reference.
         */
        Builder& end();
        /**
         * @brief Finalize the current layout specification.
         * @return Completed Yoga layout spec.
         */
        Spec build();

    private:
        std::size_t addNode(std::string_view name, const Style& style, bool pushToStack);

        Spec _spec;
        std::vector<std::size_t> _stack;
    };

    /**
     * @brief Default construct an empty Yoga layout.
     */
    YogaLayout() = default;
    /**
     * @brief Destroy Yoga resources owned by this layout.
     */
    ~YogaLayout();

    YogaLayout(const YogaLayout&) = delete;
    YogaLayout& operator=(const YogaLayout&) = delete;

    /**
     * @brief Set the layout tree specification.
     * @param spec New layout specification.
     */
    void setLayout(Spec spec);
    /**
     * @brief Recompute layout rects for a panel-local area.
     * @param x Local origin X.
     * @param y Local origin Y.
     * @param width Available panel width.
     * @param height Available panel height.
     */
    void resize(float x, float y, float width, float height);
    /**
     * @brief Return whether a named layout slot exists.
     * @param name Stable layout node name.
     * @return True when the named node has a computed rect.
     */
    bool has(std::string_view name) const;
    /**
     * @brief Return the computed rect for a named layout slot.
     * @param name Stable layout node name.
     * @return Computed layout rectangle.
     */
    const UiLayoutRectState& rect(std::string_view name) const;

private:
    void destroyYogaTree();
    void applyStyle(YGNodeRef node, const Style& style);
    void computeRects(std::size_t nodeIndex, float absX, float absY);

    Spec _spec;
    std::unordered_map<std::string, UiLayoutRectState> _rects;
    YGConfigRef _config = nullptr;
    std::vector<YGNodeRef> _yogaNodes;
};
