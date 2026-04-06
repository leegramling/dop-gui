#include "PropertiesPanel.h"

#include "UiLayoutUtils.h"
#include "Widgets.h"

#include <vsgImGui/imgui.h>

#include <unordered_map>

namespace
{
const UiPanelWidgetNode* findWidget(const UiPanelTree& root, std::string_view widgetId)
{
    return root.findWidget(widgetId);
}

using DoubleAccessor = double* (*)(SceneObjectState&);

double* positionX(SceneObjectState& object) { return &object.position.x; }
double* positionY(SceneObjectState& object) { return &object.position.y; }
double* positionZ(SceneObjectState& object) { return &object.position.z; }
double* rotationX(SceneObjectState& object) { return &object.rotation.x; }
double* rotationY(SceneObjectState& object) { return &object.rotation.y; }
double* rotationZ(SceneObjectState& object) { return &object.rotation.z; }
double* scaleX(SceneObjectState& object) { return &object.scale.x; }
double* scaleY(SceneObjectState& object) { return &object.scale.y; }
double* scaleZ(SceneObjectState& object) { return &object.scale.z; }

const std::unordered_map<std::string_view, DoubleAccessor>& selectedObjectDoubleBindings()
{
    static const std::unordered_map<std::string_view, DoubleAccessor> bindings{
        {"scene.selected.position.x", &positionX},
        {"scene.selected.position.y", &positionY},
        {"scene.selected.position.z", &positionZ},
        {"scene.selected.rotation.x", &rotationX},
        {"scene.selected.rotation.y", &rotationY},
        {"scene.selected.rotation.z", &rotationZ},
        {"scene.selected.scale.x", &scaleX},
        {"scene.selected.scale.y", &scaleY},
        {"scene.selected.scale.z", &scaleZ},
    };
    return bindings;
}

double* resolveSelectedObjectDouble(AppState& state, std::string_view bind)
{
    auto* selectedObject = findSceneObject(state.scene, state.scene.selectedObjectId);
    if (!selectedObject) return nullptr;

    const auto& bindings = selectedObjectDoubleBindings();
    const auto it = bindings.find(bind);
    if (it == bindings.end()) return nullptr;

    return it->second(*selectedObject);
}
}

std::string_view PropertiesPanel::id() const
{
    return "panel-properties";
}

PanelMinSize PropertiesPanel::minSize(const UiPanelState& panelState) const
{
    (void)panelState;
    return PanelMinSize{.width = 360.0f, .height = 356.0f, .enabled = true};
}

void PropertiesPanel::init(const UiPanelState& panelState)
{
    _root = UiPanelTree::build(panelState);

    _root.setWidgetRenderer("selected-object", [](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
    {
        auto& state = context.panelContext.state;
        const auto objectIds = collectSceneObjectIds(state.scene);
        const auto selectedValue = renderSelectedObjectControl(
            state.ui,
            context.layout,
            node.slots,
            node.spec.id.c_str(),
            "panel-properties-selected-object-label",
            "Selected Object",
            state.scene.selectedObjectId,
            objectIds);
        if (!selectedValue.empty() && selectedValue != state.scene.selectedObjectId)
        {
            queueUiCommand(state.ui, "scene.select_object", selectedValue);
        }
    });

    _root.setWidgetRenderer("selected-object-summary", [](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
    {
        auto& state = context.panelContext.state;
        setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.valueSlotId);
        Text(
            state.ui,
            node.spec.slotId.c_str(),
            state.scene.selectedObjectId.empty() ? "Selected: none" : "Selected: " + state.scene.selectedObjectId);
    });

    for (const auto& widget : _root.widgets())
    {
        if (widget.spec.type != "input_double") continue;

        _root.setWidgetRenderer(widget.spec.id, [](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
        {
            auto& state = context.panelContext.state;
            auto* value = resolveSelectedObjectDouble(state, node.spec.bind);
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
}

void PropertiesPanel::render(PanelContext& context, const UiPanelState& panelState)
{
    auto& state = context.state;
    YogaLayout propertiesLayout;
    propertiesLayout.setLayout(_root.layoutSpec());
    ImVec2 origin{0.0f, 0.0f};
    ImVec2 avail{
        panelState.layout.width > 0.0 ? static_cast<float>(panelState.layout.width) : 320.0f,
        panelState.layout.height > 0.0 ? static_cast<float>(panelState.layout.height) : 520.0f};
    if (!state.ui.testMode)
    {
        origin = ImGui::GetCursorPos();
        avail = ImGui::GetContentRegionAvail();
    }
    propertiesLayout.resize(origin.x, origin.y, avail.x, avail.y);
    registerLayoutSlots(state.ui, std::string(id()), propertiesLayout, _root.slotIds());
    UiPanelRenderContext renderContext{
        .panelContext = context,
        .panelState = panelState,
        .layout = propertiesLayout,
    };
    _root.render(renderContext);
}
