#include "SceneInfoPanel.h"

#include "UiLayoutUtils.h"
#include "Widgets.h"

#include <vsgImGui/imgui.h>

#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace
{
std::string vec3Text(const vsg::dvec3& value)
{
    std::ostringstream out;
    out << value.x << ", " << value.y << ", " << value.z;
    return out.str();
}

std::string fpsText(double fps)
{
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(1);
    out << "FPS: " << fps;
    return out.str();
}

std::string objectCountText(const AppState& state)
{
    return "Objects: " + std::to_string(state.scene.objects.size());
}

using TextBindingReader = std::string (*)(const AppState&);
using StringBindingAccessor = std::string* (*)(AppState&);
using BoolBindingAccessor = bool* (*)(AppState&);

std::string fpsBinding(const AppState& state) { return fpsText(state.view.fps); }
std::string objectCountBinding(const AppState& state) { return objectCountText(state); }
std::string* backgroundColorAccessor(AppState& state) { return &state.view.backgroundColorHex; }
std::string* sceneNameAccessor(AppState& state) { return &state.scene.name; }
bool* displayGridAccessor(AppState& state) { return &state.ui.displayGrid; }

const std::unordered_map<std::string_view, TextBindingReader>& textBindings()
{
    static const std::unordered_map<std::string_view, TextBindingReader> bindings{
        {"view.fps.text", &fpsBinding},
        {"scene.objectCount.text", &objectCountBinding},
    };
    return bindings;
}

const std::unordered_map<std::string_view, StringBindingAccessor>& stringBindings()
{
    static const std::unordered_map<std::string_view, StringBindingAccessor> bindings{
        {"view.backgroundColorHex", &backgroundColorAccessor},
        {"scene.name", &sceneNameAccessor},
    };
    return bindings;
}

const std::unordered_map<std::string_view, BoolBindingAccessor>& boolBindings()
{
    static const std::unordered_map<std::string_view, BoolBindingAccessor> bindings{
        {"ui.displayGrid", &displayGridAccessor},
    };
    return bindings;
}

std::string readTextBinding(const AppState& state, std::string_view bind)
{
    const auto& bindings = textBindings();
    const auto it = bindings.find(bind);
    if (it == bindings.end()) return {};
    return it->second(state);
}

std::string* resolveStringBinding(AppState& state, std::string_view bind)
{
    const auto& bindings = stringBindings();
    const auto it = bindings.find(bind);
    if (it == bindings.end()) return nullptr;
    return it->second(state);
}

bool* resolveBoolBinding(AppState& state, std::string_view bind)
{
    const auto& bindings = boolBindings();
    const auto it = bindings.find(bind);
    if (it == bindings.end()) return nullptr;
    return it->second(state);
}

bool isThemeRadio(const UiPanelWidgetNode& node)
{
    return node.spec.type == "radio" && node.spec.bind == "ui.themeMode";
}

bool isSceneSummaryPopup(const UiPanelWidgetNode& node)
{
    return node.spec.type == "popup" && node.spec.id == "popup-scene-summary";
}
}

std::string_view SceneInfoPanel::id() const
{
    return "panel-scene-info";
}

PanelMinSize SceneInfoPanel::minSize(const UiPanelState& panelState) const
{
    (void)panelState;
    return PanelMinSize{.width = 360.0f, .height = 492.0f, .enabled = true};
}

void SceneInfoPanel::init(const UiPanelState& panelState)
{
    _root = UiPanelTree::build(panelState);

    for (const auto& widget : _root.widgets())
    {
        if (widget.spec.type == "text")
        {
            _root.bindText(widget.spec.id, [bind = widget.spec.bind](AppState& state) -> std::optional<std::string>
            {
                const auto value = readTextBinding(state, bind);
                if (value.empty()) return std::nullopt;
                return value;
            });
        }
        else if (widget.spec.type == "checkbox")
        {
            _root.bindCheckbox(widget.spec.id, [bind = widget.spec.bind](AppState& state) { return resolveBoolBinding(state, bind); });
        }
        else if (widget.spec.type == "input")
        {
            _root.bindStringInput(widget.spec.id, [bind = widget.spec.bind](AppState& state) { return resolveStringBinding(state, bind); });
        }
        else if (widget.spec.type == "combo" && widget.spec.bind == "scene.name")
        {
            _root.bindStringCombo(widget.spec.id, [bind = widget.spec.bind](AppState& state) { return resolveStringBinding(state, bind); });
        }
        else if (isThemeRadio(widget))
        {
            _root.bindRadioChoice(widget.spec.id, [](AppState& state) { return &state.ui.themeMode; });
        }
        else if (widget.spec.id == "selected-object")
        {
            _root.setWidgetRenderer(widget.spec.id, [](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
            {
                auto& state = context.panelContext.state;
                const auto objectIds = collectSceneObjectIds(state.scene);
                const auto selectedValue = renderSelectedObjectControl(
                    state.ui,
                    context.layout,
                    node.slots,
                    node.spec.id.c_str(),
                    "panel-scene-selected-object-label",
                    "Selected Object",
                    state.scene.selectedObjectId,
                    objectIds);
                if (!selectedValue.empty() && selectedValue != state.scene.selectedObjectId)
                {
                    queueUiCommand(state.ui, "scene.select_object", selectedValue);
                }
            });
        }
        else if (widget.spec.id == "scene-summary-open")
        {
            _root.setWidgetRenderer(widget.spec.id, [](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
            {
                auto& state = context.panelContext.state;
                setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.valueSlotId);
                const bool opened = Button(state.ui, node.spec.id.c_str(), node.spec.label.c_str());
                if (auto* widgetState = findWidget(state.ui, std::string("panel-scene-info"), node.spec.id))
                {
                    widgetState->boolValue = opened;
                }
            });
        }
        else if (isSceneSummaryPopup(widget))
        {
            _root.setWidgetRenderer(widget.spec.id, [](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
            {
                auto& state = context.panelContext.state;
                bool requestOpen = false;
                if (const auto* button = findWidget(state.ui, std::string("panel-scene-info"), "scene-summary-open"))
                {
                    requestOpen = button->boolValue;
                }

                setNextWidgetLayout(state.ui, node.spec.layout);
                Popup(state.ui, node.spec.id.c_str(), node.spec.label.c_str(), requestOpen, [&]()
                {
                    Text(state.ui, "popup-scene-summary-name", "Scene: " + state.scene.name);
                    Text(state.ui, "popup-scene-summary-object-count", objectCountText(state));
                });
            });
        }
        else if (widget.spec.type == "table" && widget.spec.bind == "scene.objects")
        {
            _root.setWidgetRenderer(widget.spec.id, [](UiPanelRenderContext& context, const UiPanelWidgetNode& node)
            {
                auto& state = context.panelContext.state;
                setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.labelSlotId);
                Text(state.ui, "panel-scene-table-label", node.spec.label);

                const int columnCount = node.spec.columns.empty() ? 4 : static_cast<int>(node.spec.columns.size());
                setNextWidgetLayoutIfPresent(state.ui, context.layout, node.slots.valueSlotId);
                Table(state.ui, node.spec.id.c_str(), columnCount, state.scene.objects.size(), [&]()
                {
                    if (!state.ui.testMode)
                    {
                        for (const auto& column : node.spec.columns) ImGui::TableSetupColumn(column.c_str());
                        if (!node.spec.columns.empty()) ImGui::TableHeadersRow();
                    }

                    for (const auto& object : state.scene.objects)
                    {
                        const auto rowPrefix = "table-scene-objects-row-" + object.id;
                        if (!state.ui.testMode)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                        }
                        if (Button(state.ui, (rowPrefix + "-select").c_str(), "Select"))
                        {
                            queueUiCommand(state.ui, "scene.select_object", object.id);
                        }
                        if (!state.ui.testMode) ImGui::TableSetColumnIndex(1);
                        Text(state.ui, (rowPrefix + "-id").c_str(), object.id);
                        if (!state.ui.testMode) ImGui::TableSetColumnIndex(2);
                        Text(state.ui, (rowPrefix + "-kind").c_str(), object.kind);
                        if (!state.ui.testMode) ImGui::TableSetColumnIndex(3);
                        Text(state.ui, (rowPrefix + "-position").c_str(), vec3Text(object.position));
                    }
                });
            });
        }
    }
}

void SceneInfoPanel::render(PanelContext& context, const UiPanelState& panelState)
{
    auto& state = context.state;
    YogaLayout sceneInfoLayout;
    sceneInfoLayout.setLayout(_root.layoutSpec());

    ImVec2 origin{16.0f, 16.0f};
    ImVec2 avail{
        panelState.layout.width > 0.0 ? static_cast<float>(panelState.layout.width) - 32.0f : 328.0f,
        panelState.layout.height > 0.0 ? static_cast<float>(panelState.layout.height) - 32.0f : 488.0f};
    if (!state.ui.testMode)
    {
        avail = ImVec2(
            std::max(0.0f, ImGui::GetContentRegionAvail().x - 16.0f),
            std::max(0.0f, ImGui::GetContentRegionAvail().y));
    }
    sceneInfoLayout.resize(origin.x, origin.y, avail.x, avail.y);
    registerLayoutSlots(state.ui, std::string(id()), sceneInfoLayout, _root.slotIds());

    setNextWidgetLayoutIfPresent(state.ui, sceneInfoLayout, "panel-theme-label");
    Text(state.ui, "panel-theme-label", "Theme");

    UiPanelRenderContext renderContext{
        .panelContext = context,
        .panelState = panelState,
        .layout = sceneInfoLayout,
    };
    _root.render(renderContext);
}
