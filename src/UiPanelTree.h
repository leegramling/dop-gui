#pragma once

#include "AppState.h"
#include "UiLayoutUtils.h"
#include "YogaLayout.h"

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

struct PanelContext;

/**
 * @brief Built widget node resolved from an authored panel spec.
 */
struct UiPanelWidgetNode
{
    UiWidgetSpecState spec;
    WidgetSlotBinding slots;
};

/**
 * @brief Shared render context passed to built panel-tree widget renderers.
 */
struct UiPanelRenderContext
{
    PanelContext& panelContext;
    const UiPanelState& panelState;
    const YogaLayout& layout;
};

/**
 * @brief Built runtime tree metadata for a declarative authored panel.
 */
class UiPanelTree
{
public:
    using WidgetRenderer = std::function<void(UiPanelRenderContext&, const UiPanelWidgetNode&)>;
    using TextBindingReader = std::function<std::optional<std::string>(AppState&)>;
    using StringBindingAccessor = std::function<std::string*(AppState&)>;
    using BoolBindingAccessor = std::function<bool*(AppState&)>;
    using DoubleBindingAccessor = std::function<double*(AppState&)>;

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
    std::vector<UiPanelWidgetNode> widgets() const;

    /**
     * @brief Find a built widget node by widget id.
     * @param widgetId Stable widget identifier.
     * @return Matching built widget node, or null when not found.
     */
    const UiPanelWidgetNode* findWidget(std::string_view widgetId) const;

    /**
     * @brief Assign a renderer callback for a built widget node.
     * @param widgetId Stable widget identifier.
     * @param renderer Widget renderer callback.
     */
    void setWidgetRenderer(std::string_view widgetId, WidgetRenderer renderer);

    /**
     * @brief Bind a text widget to a computed string value.
     * @param widgetId Stable widget identifier.
     * @param reader Function that computes the current text value from application state.
     */
    void bindText(std::string_view widgetId, TextBindingReader reader);
    /**
     * @brief Bind a checkbox widget to a mutable boolean state value.
     * @param widgetId Stable widget identifier.
     * @param accessor Function that returns the bound boolean state pointer.
     */
    void bindCheckbox(std::string_view widgetId, BoolBindingAccessor accessor);
    /**
     * @brief Bind a text input widget to a mutable string state value.
     * @param widgetId Stable widget identifier.
     * @param accessor Function that returns the bound string state pointer.
     */
    void bindStringInput(std::string_view widgetId, StringBindingAccessor accessor);
    /**
     * @brief Bind a combo widget to a mutable string state value.
     * @param widgetId Stable widget identifier.
     * @param accessor Function that returns the bound string state pointer.
     */
    void bindStringCombo(std::string_view widgetId, StringBindingAccessor accessor);
    /**
     * @brief Bind a radio widget to a mutable string state value that matches the widget arg.
     * @param widgetId Stable widget identifier.
     * @param accessor Function that returns the bound string state pointer.
     */
    void bindRadioChoice(std::string_view widgetId, StringBindingAccessor accessor);
    /**
     * @brief Bind a floating-point input widget to a mutable numeric state value.
     * @param widgetId Stable widget identifier.
     * @param accessor Function that returns the bound numeric state pointer.
     */
    void bindDoubleInput(std::string_view widgetId, DoubleBindingAccessor accessor);

    /**
     * @brief Render the widget nodes that currently have registered renderers.
     * @param context Shared panel render context.
     */
    void render(UiPanelRenderContext& context) const;

private:
    struct BuiltWidget
    {
        UiPanelWidgetNode node;
        WidgetRenderer renderer;
    };

    BuiltWidget* findBuiltWidget(std::string_view widgetId);

    YogaLayout::Spec _layoutSpec;
    std::vector<std::string> _slotIds;
    std::vector<BuiltWidget> _widgets;
};
