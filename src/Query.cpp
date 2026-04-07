#include "Query.h"

#include "App.h"
#include "Command.h"
#include "InputManager.h"
#include "VsgVisualizer.h"
#include "WindowManager.h"

#include <functional>
#include <sstream>
#include <stdexcept>

namespace
{
using Segments = std::vector<std::string>;
using QueryReader = std::function<QueryValue(const App&, const Segments&)>;

struct QueryRoute
{
    std::string prefix;
    QueryReader read;
};

std::string escapeJson(const std::string& value)
{
    std::ostringstream out;
    for (char ch : value)
    {
        switch (ch)
        {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out << ch; break;
        }
    }
    return out.str();
}

std::string sanitizePanelId(std::string label)
{
    for (auto& ch : label)
    {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        else if (ch == ' ' || ch == '.') ch = '-';
    }
    return "panel-" + label;
}

QueryValuePtr makeValuePtr(QueryValue value)
{
    return std::make_shared<QueryValue>(std::move(value));
}

Segments splitPath(std::string_view path)
{
    Segments segments;
    std::string current;

    for (char ch : path)
    {
        if (ch == '.')
        {
            if (!current.empty())
            {
                segments.push_back(current);
                current.clear();
            }
        }
        else
        {
            current.push_back(ch);
        }
    }

    if (!current.empty()) segments.push_back(current);
    return segments;
}

bool isPrefixMatch(std::string_view prefix, std::string_view path)
{
    if (path == prefix) return true;
    if (path.size() <= prefix.size()) return false;
    if (path.compare(0, prefix.size(), prefix) != 0) return false;
    return path[prefix.size()] == '.';
}

Segments remainingSegments(std::string_view prefix, std::string_view path)
{
    if (path == prefix) return {};
    return splitPath(path.substr(prefix.size() + 1));
}

QueryValue makeVec3Value(const vsg::dvec3& value)
{
    return makeArrayValue({
        makeDoubleValue(value.x),
        makeDoubleValue(value.y),
        makeDoubleValue(value.z),
    });
}

QueryValue makeSceneObjectValue(const SceneObjectState& object, bool includeTransform, bool includeProperties)
{
    std::vector<QueryField> fields;
    fields.push_back(makeField("id", makeStringValue(object.id)));

    if (includeProperties)
    {
        fields.push_back(makeField("kind", makeStringValue(object.kind)));
        fields.push_back(makeField("vertexCount", makeUIntValue(object.vertexCount)));
        fields.push_back(makeField("colorHex", makeStringValue(object.colorHex)));
        fields.push_back(makeField("color", makeArrayValue({
            makeDoubleValue(object.color.r),
            makeDoubleValue(object.color.g),
            makeDoubleValue(object.color.b),
            makeDoubleValue(object.color.a),
        })));
    }

    if (includeTransform)
    {
        fields.push_back(makeField("position", makeVec3Value(object.position)));
        fields.push_back(makeField("rotation", makeVec3Value(object.rotation)));
        fields.push_back(makeField("scale", makeVec3Value(object.scale)));
    }

    return QueryValue{QueryObject{std::move(fields)}};
}

QueryValue makeWidgetValue(const WidgetState& widget)
{
    return makeObjectValue({
        makeField("label", makeStringValue(widget.label)),
        makeField("panelId", makeStringValue(widget.panelId)),
        makeField("widgetId", makeStringValue(widget.widgetId)),
        makeField("type", makeStringValue(widget.type)),
        makeField("textValue", makeStringValue(widget.textValue)),
        makeField("boolValue", makeBoolValue(widget.boolValue)),
        makeField(
            "layout",
            makeObjectValue({
                makeField("enabled", makeBoolValue(widget.layout.enabled)),
                makeField("x", makeDoubleValue(widget.layout.x)),
                makeField("y", makeDoubleValue(widget.layout.y)),
                makeField("width", makeDoubleValue(widget.layout.width)),
                makeField("height", makeDoubleValue(widget.layout.height)),
            })),
    });
}

QueryValue makeLayoutRectValue(const UiLayoutRectState& layout)
{
    return makeObjectValue({
        makeField("enabled", makeBoolValue(layout.enabled)),
        makeField("x", makeDoubleValue(layout.x)),
        makeField("y", makeDoubleValue(layout.y)),
        makeField("width", makeDoubleValue(layout.width)),
        makeField("height", makeDoubleValue(layout.height)),
    });
}

QueryValue makeManagedWindowValue(const WindowManager::ManagedWindowRecord& window)
{
    return makeObjectValue({
        makeField("viewportId", makeUIntValue(window.viewportId)),
        makeField("title", makeStringValue(window.title)),
        makeField("x", makeDoubleValue(window.x)),
        makeField("y", makeDoubleValue(window.y)),
        makeField("width", makeDoubleValue(window.width)),
        makeField("height", makeDoubleValue(window.height)),
        makeField("visible", makeBoolValue(window.visible)),
        makeField("focused", makeBoolValue(window.focused)),
        makeField("minimized", makeBoolValue(window.minimized)),
        makeField("destroyed", makeBoolValue(window.destroyed)),
        makeField("platformWindowCreated", makeBoolValue(window.platformWindowCreated)),
        makeField("rendererWindowCreated", makeBoolValue(window.rendererWindowCreated)),
        makeField("ownedByApp", makeBoolValue(window.ownedByApp)),
        makeField("traits", makeObjectValue({
            makeField("windowTitle", makeStringValue(window.traitsWindowTitle)),
            makeField("x", makeIntValue(window.traitsX)),
            makeField("y", makeIntValue(window.traitsY)),
            makeField("width", makeUIntValue(window.traitsWidth)),
            makeField("height", makeUIntValue(window.traitsHeight)),
            makeField("decoration", makeBoolValue(window.traitsDecoration)),
            makeField("hdpi", makeBoolValue(window.traitsHdpi)),
            makeField("debugLayer", makeBoolValue(window.traitsDebugLayer)),
            makeField("apiDumpLayer", makeBoolValue(window.traitsApiDumpLayer)),
            makeField("samples", makeUIntValue(window.traitsSamples)),
        })),
    });
}

QueryValue makeWidgetSpecValue(const UiWidgetSpecState& widget)
{
    auto layoutValue = makeLayoutRectValue(widget.layout);

    QueryArray columns;
    for (const auto& column : widget.columns)
    {
        columns.elements.push_back(makeValuePtr(makeStringValue(column)));
    }

    QueryArray options;
    for (const auto& option : widget.options)
    {
        options.elements.push_back(makeValuePtr(makeStringValue(option)));
    }

    return makeObjectValue({
        makeField("id", makeStringValue(widget.id)),
        makeField("slotId", makeStringValue(widget.slotId)),
        makeField("labelSlot", makeStringValue(widget.labelSlot)),
        makeField("type", makeStringValue(widget.type)),
        makeField("label", makeStringValue(widget.label)),
        makeField("bind", makeStringValue(widget.bind)),
        makeField("arg", makeStringValue(widget.arg)),
        makeField("onClick", makeStringValue(widget.onClick)),
        makeField("onChange", makeStringValue(widget.onChange)),
        makeField("unit", makeStringValue(widget.unit)),
        makeField("precision", makeIntValue(widget.precision)),
        makeField("layout", layoutValue),
        makeField("options", QueryValue{std::move(options)}),
        makeField("columns", QueryValue{std::move(columns)}),
    });
}

QueryValue makeFlexNodeValue(const UiFlexNodeState& node)
{
    QueryArray children;
    for (const auto& child : node.children)
    {
        children.elements.push_back(makeValuePtr(makeFlexNodeValue(child)));
    }

    return makeObjectValue({
        makeField("type", makeStringValue(node.type)),
        makeField("slot", makeStringValue(node.slot)),
        makeField("widget", makeStringValue(node.widget)),
        makeField("labelFor", makeStringValue(node.labelFor)),
        makeField("gap", makeDoubleValue(node.gap)),
        makeField("width", node.width ? makeDoubleValue(*node.width) : makeNullValue()),
        makeField("height", node.height ? makeDoubleValue(*node.height) : makeNullValue()),
        makeField("flex", node.flex ? makeDoubleValue(*node.flex) : makeNullValue()),
        makeField("children", QueryValue{std::move(children)}),
    });
}

QueryValue makePanelSpecValue(const UiPanelState& panel)
{
    auto layoutValue = makeLayoutRectValue(panel.layout);

    QueryArray flags;
    for (const auto& flag : panel.flags)
    {
        flags.elements.push_back(makeValuePtr(makeStringValue(flag)));
    }

    QueryArray widgets;
    for (const auto& widget : panel.widgets)
    {
        widgets.elements.push_back(makeValuePtr(makeWidgetSpecValue(widget)));
    }

    return makeObjectValue({
        makeField("label", makeStringValue(panel.label)),
        makeField("id", makeStringValue(sanitizePanelId(panel.label))),
        makeField("open", makeBoolValue(panel.open)),
        makeField("closable", makeBoolValue(panel.closable)),
        makeField("flags", QueryValue{std::move(flags)}),
        makeField("layout", layoutValue),
        makeField("flexLayout", panel.flexLayout ? makeFlexNodeValue(*panel.flexLayout) : makeNullValue()),
        makeField("widgets", QueryValue{std::move(widgets)}),
    });
}

std::string canonicalizeQueryPath(const std::string& name)
{
    if (name == "window.size") return "view.window.size";
    if (name == "view.window.size") return name;
    if (name == "camera.pose") return "view.camera.pose";
    if (name == "view.camera.pose") return name;
    if (name == "view.background.color") return name;
    if (name == "scene.objects") return "data.scene.objects";
    if (name == "data.scene.objects") return name;
    if (name == "scene.selection") return "data.scene.selection";
    if (name == "data.scene.selection") return name;
    if (name == "runtime.capabilities") return name;
    if (name == "ui.widgets") return name;
    if (name == "ui.layout") return name;
    if (name.rfind("ui.layout.slot.", 0) == 0) return name;
    if (name == "ui.docking.status") return name;
    if (name == "ui.tearout.status") return name;
    if (name == "ui.windows") return name;
    if (name.rfind("ui.window.", 0) == 0) return name;
    if (name == "help") return "runtime.capabilities";

    constexpr std::string_view uiWidgetPrefix = "ui.widget.";
    if (name.rfind(uiWidgetPrefix.data(), 0) == 0)
    {
        auto label = name.substr(uiWidgetPrefix.size());
        if (label.empty()) throw std::runtime_error("Unknown query: " + name);
        return name;
    }
    constexpr std::string_view uiPanelPrefix = "ui.panel.";
    if (name.rfind(uiPanelPrefix.data(), 0) == 0)
    {
        auto suffix = name.substr(uiPanelPrefix.size());
        if (suffix.empty()) throw std::runtime_error("Unknown query: " + name);
        return name;
    }

    constexpr std::string_view sceneObjectPrefix = "scene.object.";
    constexpr std::string_view dataSceneObjectPrefix = "data.scene.object.";
    constexpr std::string_view sceneObjectTransformPrefix = "scene.object.transform.";
    constexpr std::string_view dataSceneObjectTransformPrefix = "data.scene.object.transform.";
    constexpr std::string_view sceneObjectPropertiesPrefix = "scene.object.properties.";
    constexpr std::string_view dataSceneObjectPropertiesPrefix = "data.scene.object.properties.";

    if (name.rfind(sceneObjectTransformPrefix.data(), 0) == 0)
    {
        auto id = name.substr(sceneObjectTransformPrefix.size());
        return "data.scene.object." + id + ".transform";
    }
    if (name.rfind(dataSceneObjectTransformPrefix.data(), 0) == 0)
    {
        auto id = name.substr(dataSceneObjectTransformPrefix.size());
        return "data.scene.object." + id + ".transform";
    }
    if (name.rfind(sceneObjectPropertiesPrefix.data(), 0) == 0)
    {
        auto id = name.substr(sceneObjectPropertiesPrefix.size());
        return "data.scene.object." + id + ".properties";
    }
    if (name.rfind(dataSceneObjectPropertiesPrefix.data(), 0) == 0)
    {
        auto id = name.substr(dataSceneObjectPropertiesPrefix.size());
        return "data.scene.object." + id + ".properties";
    }
    if (name.rfind(sceneObjectPrefix.data(), 0) == 0)
    {
        auto id = name.substr(sceneObjectPrefix.size());
        return "data.scene.object." + id;
    }
    if (name.rfind(dataSceneObjectPrefix.data(), 0) == 0)
    {
        auto suffix = name.substr(dataSceneObjectPrefix.size());
        if (suffix.empty()) throw std::runtime_error("Unknown query: " + name);
        return "data.scene.object." + suffix;
    }

    throw std::runtime_error("Unknown query: " + name);
}

QueryValue readWindowQuery(const App& app, const Segments& segments)
{
    if (segments.size() == 1 && segments[0] == "size")
    {
        return makeObjectValue({
            makeField("width", makeUIntValue(app.inputManager().configuredWidth())),
            makeField("height", makeUIntValue(app.inputManager().configuredHeight())),
        });
    }

    throw std::runtime_error("Unknown view.window query path.");
}

QueryValue readCameraQuery(const App& app, const Segments& segments)
{
    if (segments.size() == 1 && segments[0] == "pose")
    {
        const auto& pose = app.state().view.cameraPose;
        return makeObjectValue({
            makeField("eye", makeVec3Value(pose.eye)),
            makeField("center", makeVec3Value(pose.center)),
            makeField("up", makeVec3Value(pose.up)),
        });
    }

    throw std::runtime_error("Unknown view.camera query path.");
}

QueryValue readViewQuery(const App& app, const Segments& segments)
{
    if (segments.size() == 2 && segments[0] == "background" && segments[1] == "color")
    {
        return makeObjectValue({
            makeField("hex", makeStringValue(app.state().view.backgroundColorHex)),
            makeField("rgba", makeArrayValue({
                makeDoubleValue(app.state().view.backgroundColor.r),
                makeDoubleValue(app.state().view.backgroundColor.g),
                makeDoubleValue(app.state().view.backgroundColor.b),
                makeDoubleValue(app.state().view.backgroundColor.a),
            })),
        });
    }

    throw std::runtime_error("Unknown view query path.");
}

QueryValue readSceneQuery(const App& app, const Segments& segments)
{
    if (segments.size() == 1 && segments[0] == "objects")
    {
        QueryArray array;
        for (const auto& object : app.state().scene.objects)
        {
            array.elements.push_back(makeValuePtr(makeSceneObjectValue(object, true, true)));
        }
        return QueryValue{std::move(array)};
    }

    if (segments.size() == 1 && segments[0] == "selection")
    {
        return makeObjectValue({
            makeField("selectedObjectId", makeStringValue(app.state().scene.selectedObjectId)),
        });
    }

    if (segments.size() >= 2 && segments[0] == "object")
    {
        const auto& id = segments[1];
        auto object = findSceneObject(app.state().scene, id);
        if (!object) throw std::runtime_error("Unknown scene object: " + id);

        if (segments.size() == 2) return makeSceneObjectValue(*object, true, true);
        if (segments.size() == 3 && segments[2] == "transform") return makeSceneObjectValue(*object, true, false);
        if (segments.size() == 3 && segments[2] == "properties") return makeSceneObjectValue(*object, false, true);
    }

    throw std::runtime_error("Unknown data.scene query path.");
}

QueryValue readRuntimeQuery(const App&, const Segments& segments)
{
    if (segments.size() == 1 && segments[0] == "capabilities")
    {
        QueryArray commandArray;
        for (const auto& name : commandNames())
        {
            commandArray.elements.push_back(makeValuePtr(makeStringValue(name)));
        }

        QueryArray queryArray;
        for (const auto& name : queryNames())
        {
            queryArray.elements.push_back(makeValuePtr(makeStringValue(name)));
        }

        return makeObjectValue({
            makeField("commands", QueryValue{std::move(commandArray)}),
            makeField("queries", QueryValue{std::move(queryArray)}),
            makeField("scriptFormat", makeStringValue("json5")),
            makeField("machineOutput", makeStringValue("json")),
        });
    }

    throw std::runtime_error("Unknown runtime query path.");
}

QueryValue readUiQuery(const App& app, const Segments& segments)
{
    if (segments.size() == 1 && segments[0] == "layout")
    {
        QueryArray menus;
        for (const auto& menu : app.state().ui.layout.menus)
        {
            QueryArray items;
            for (const auto& item : menu.items)
            {
                items.elements.push_back(makeValuePtr(makeObjectValue({
                    makeField("label", makeStringValue(item.label)),
                    makeField("command", makeStringValue(item.command)),
                })));
            }

            menus.elements.push_back(makeValuePtr(makeObjectValue({
                makeField("label", makeStringValue(menu.label)),
                makeField("items", QueryValue{std::move(items)}),
            })));
        }

        QueryArray panels;
        for (const auto& panel : app.state().ui.layout.panels)
        {
            panels.elements.push_back(makeValuePtr(makePanelSpecValue(panel)));
        }

        return makeObjectValue({
            makeField("menus", QueryValue{std::move(menus)}),
            makeField("panels", QueryValue{std::move(panels)}),
        });
    }

    if (segments.size() >= 4 && segments[0] == "layout" && segments[1] == "slot")
    {
        std::string panelId = segments[2];
        std::string slotId = segments[3];
        for (std::size_t i = 4; i < segments.size(); ++i) slotId += "." + segments[i];
        const auto* slot = findLayoutSlot(app.state().ui, panelId, slotId);
        if (!slot) throw std::runtime_error("Unknown UI layout slot: " + panelId + "." + slotId);
        return makeObjectValue({
            makeField("panelId", makeStringValue(slot->panelId)),
            makeField("slotId", makeStringValue(slot->slotId)),
            makeField("layout", makeLayoutRectValue(slot->layout)),
        });
    }

    if (segments.size() == 2 && segments[0] == "docking" && segments[1] == "status")
    {
        return makeObjectValue({
            makeField("dockingEnabled", makeBoolValue(app.state().ui.dockingEnabled)),
            makeField("viewportsEnabled", makeBoolValue(app.state().ui.viewportsEnabled)),
            makeField("backendPlatformHasViewports", makeBoolValue(app.state().ui.backendPlatformHasViewports)),
            makeField("backendRendererHasViewports", makeBoolValue(app.state().ui.backendRendererHasViewports)),
            makeField("platformCallbacksInstalled", makeBoolValue(app.state().ui.platformCallbacksInstalled)),
            makeField("rendererCallbacksInstalled", makeBoolValue(app.state().ui.rendererCallbacksInstalled)),
            makeField("platformCreateWindowCallback", makeBoolValue(app.state().ui.platformCreateWindowCallback)),
            makeField("platformDestroyWindowCallback", makeBoolValue(app.state().ui.platformDestroyWindowCallback)),
            makeField("rendererCreateWindowCallback", makeBoolValue(app.state().ui.rendererCreateWindowCallback)),
            makeField("rendererDestroyWindowCallback", makeBoolValue(app.state().ui.rendererDestroyWindowCallback)),
        });
    }

    if (segments.size() == 2 && segments[0] == "tearout" && segments[1] == "status")
    {
        return makeObjectValue({
            makeField("primaryWindowRegistered", makeBoolValue(app.state().ui.primaryWindowRegistered)),
            makeField("dockingEnabled", makeBoolValue(app.state().ui.dockingEnabled)),
            makeField("viewportsEnabled", makeBoolValue(app.state().ui.viewportsEnabled)),
            makeField("backendPlatformHasViewports", makeBoolValue(app.state().ui.backendPlatformHasViewports)),
            makeField("backendRendererHasViewports", makeBoolValue(app.state().ui.backendRendererHasViewports)),
            makeField("platformCallbacksInstalled", makeBoolValue(app.state().ui.platformCallbacksInstalled)),
            makeField("rendererCallbacksInstalled", makeBoolValue(app.state().ui.rendererCallbacksInstalled)),
            makeField("platformCreateWindowCallback", makeBoolValue(app.state().ui.platformCreateWindowCallback)),
            makeField("platformDestroyWindowCallback", makeBoolValue(app.state().ui.platformDestroyWindowCallback)),
            makeField("rendererCreateWindowCallback", makeBoolValue(app.state().ui.rendererCreateWindowCallback)),
            makeField("rendererDestroyWindowCallback", makeBoolValue(app.state().ui.rendererDestroyWindowCallback)),
            makeField("tearOutCallbacksSupported", makeBoolValue(app.state().ui.tearOutCallbacksSupported)),
            makeField("hasMainViewport", makeBoolValue(app.state().ui.hasMainViewport)),
            makeField("viewportCount", makeIntValue(app.state().ui.viewportCount)),
            makeField("monitorCount", makeIntValue(app.state().ui.monitorCount)),
            makeField("platformCreateRequestCount", makeIntValue(app.state().ui.platformCreateRequestCount)),
            makeField("platformDestroyRequestCount", makeIntValue(app.state().ui.platformDestroyRequestCount)),
            makeField("rendererCreateRequestCount", makeIntValue(app.state().ui.rendererCreateRequestCount)),
            makeField("rendererDestroyRequestCount", makeIntValue(app.state().ui.rendererDestroyRequestCount)),
            makeField("managedWindowCount", makeIntValue(static_cast<int>(app.windowManager().managedWindows().size()))),
            makeField("lastTearOutEvent", makeStringValue(app.state().ui.lastTearOutEvent)),
            makeField("lastTearOutViewportId", makeUIntValue(app.state().ui.lastTearOutViewportId)),
        });
    }

    if (segments.size() == 1 && segments[0] == "windows")
    {
        QueryArray array;
        for (const auto& window : app.windowManager().managedWindows())
        {
            array.elements.push_back(makeValuePtr(makeManagedWindowValue(window)));
        }
        return QueryValue{std::move(array)};
    }

    if (segments.size() >= 2 && segments[0] == "window")
    {
        std::string joined = segments[1];
        for (std::size_t i = 2; i < segments.size(); ++i) joined += "." + segments[i];
        const auto viewportId = static_cast<std::uint64_t>(std::stoull(joined));
        const auto* window = app.windowManager().findManagedWindow(viewportId);
        if (!window) throw std::runtime_error("Unknown UI managed window: " + joined);
        return makeManagedWindowValue(*window);
    }

    if (segments.size() == 1 && segments[0] == "widgets")
    {
        QueryArray array;
        for (const auto& widget : app.state().ui.registry)
        {
            array.elements.push_back(makeValuePtr(makeWidgetValue(widget)));
        }
        return QueryValue{std::move(array)};
    }

    if (segments.size() >= 2 && segments[0] == "widget")
    {
        std::string label = segments[1];
        for (std::size_t i = 2; i < segments.size(); ++i) label += "." + segments[i];
        auto widget = findWidget(app.state().ui, label);
        if (!widget)
        {
            if (auto alias = resolveLegacyWidgetAlias(label))
            {
                widget = findWidget(app.state().ui, alias->first, alias->second);
            }
        }
        if (!widget) throw std::runtime_error("Unknown UI widget: " + label);
        return makeWidgetValue(*widget);
    }

    if (segments.size() >= 2 && segments[0] == "panel")
    {
        std::string panelId = segments[1];
        for (std::size_t i = 2; i < segments.size() && segments[i] != "widgets" && segments[i] != "widget"; ++i)
        {
            panelId += "." + segments[i];
        }

        const UiPanelState* panel = nullptr;
        for (const auto& candidate : app.state().ui.layout.panels)
        {
            std::string candidateId = candidate.label;
            for (auto& ch : candidateId)
            {
                if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
                else if (ch == ' ' || ch == '.') ch = '-';
            }
            candidateId = "panel-" + candidateId;
            if (candidateId == panelId)
            {
                panel = &candidate;
                break;
            }
        }

        if (!panel) throw std::runtime_error("Unknown UI panel: " + panelId);
        if (segments.size() == 2) return makePanelSpecValue(*panel);
        if (segments.size() == 3 && segments[2] == "widgets")
        {
            QueryArray widgets;
            for (const auto& widget : panel->widgets)
            {
                widgets.elements.push_back(makeValuePtr(makeWidgetSpecValue(widget)));
            }
            return QueryValue{std::move(widgets)};
        }
        if (segments.size() >= 4 && segments[2] == "widget")
        {
            std::string widgetId = segments[3];
            for (std::size_t i = 4; i < segments.size(); ++i) widgetId += "." + segments[i];
            const auto* widget = findWidget(app.state().ui, panelId, widgetId);
            if (!widget) throw std::runtime_error("Unknown UI panel widget: " + panelId + "." + widgetId);
            return makeWidgetValue(*widget);
        }
    }

    throw std::runtime_error("Unknown ui query path.");
}

const std::vector<QueryRoute>& queryRoutes()
{
    static const std::vector<QueryRoute> routes{
        QueryRoute{.prefix = "view.window", .read = readWindowQuery},
        QueryRoute{.prefix = "view.camera", .read = readCameraQuery},
        QueryRoute{.prefix = "view", .read = readViewQuery},
        QueryRoute{.prefix = "data.scene", .read = readSceneQuery},
        QueryRoute{.prefix = "ui", .read = readUiQuery},
        QueryRoute{.prefix = "runtime", .read = readRuntimeQuery},
    };
    return routes;
}

void appendJsonValue(std::ostringstream& json, const QueryValue& value)
{
    std::visit(
        [&](const auto& data)
        {
            using T = std::decay_t<decltype(data)>;

            if constexpr (std::is_same_v<T, std::nullptr_t>)
            {
                json << "null";
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                json << (data ? "true" : "false");
            }
            else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, double>)
            {
                json << data;
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                json << "\"" << escapeJson(data) << "\"";
            }
            else if constexpr (std::is_same_v<T, QueryArray>)
            {
                json << "[";
                for (std::size_t i = 0; i < data.elements.size(); ++i)
                {
                    if (i > 0) json << ",";
                    appendJsonValue(json, *data.elements[i]);
                }
                json << "]";
            }
            else if constexpr (std::is_same_v<T, QueryObject>)
            {
                json << "{";
                for (std::size_t i = 0; i < data.fields.size(); ++i)
                {
                    if (i > 0) json << ",";
                    json << "\"" << escapeJson(data.fields[i].key) << "\":";
                    appendJsonValue(json, *data.fields[i].value);
                }
                json << "}";
            }
        },
        value.data);
}
}

QueryValue makeNullValue()
{
    return QueryValue{nullptr};
}

QueryValue makeBoolValue(bool value)
{
    return QueryValue{value};
}

QueryValue makeIntValue(int64_t value)
{
    return QueryValue{value};
}

QueryValue makeUIntValue(uint64_t value)
{
    return QueryValue{value};
}

QueryValue makeDoubleValue(double value)
{
    return QueryValue{value};
}

QueryValue makeStringValue(std::string value)
{
    return QueryValue{std::move(value)};
}

QueryValue makeArrayValue(std::initializer_list<QueryValue> values)
{
    QueryArray array;
    for (const auto& value : values)
    {
        array.elements.push_back(makeValuePtr(value));
    }
    return QueryValue{std::move(array)};
}

QueryField makeField(std::string key, QueryValue value)
{
    return QueryField{.key = std::move(key), .value = makeValuePtr(std::move(value))};
}

QueryValue makeObjectValue(std::initializer_list<QueryField> fields)
{
    QueryObject object;
    object.fields.assign(fields.begin(), fields.end());
    return QueryValue{std::move(object)};
}

std::optional<QueryRequest> parseQueryRequest(const std::string& name)
{
    if (name.empty()) return std::nullopt;
    return QueryByPath{
        .queryName = name,
        .canonicalPath = canonicalizeQueryPath(name),
    };
}

QueryResult executeQuery(const App& app, const QueryRequest& request)
{
    return std::visit(
        [&](const auto& query) -> QueryResult
        {
            using T = std::decay_t<decltype(query)>;

            if constexpr (std::is_same_v<T, QueryByPath>)
            {
                for (const auto& route : queryRoutes())
                {
                    if (isPrefixMatch(route.prefix, query.canonicalPath))
                    {
                        return QuerySuccess{
                            .queryName = query.queryName,
                            .value = route.read(app, remainingSegments(route.prefix, query.canonicalPath)),
                        };
                    }
                }

                throw std::runtime_error("Unknown query path: " + query.canonicalPath);
            }
        },
        request);
}

std::string serializeQueryResult(const QueryResult& result)
{
    return std::visit(
        [&](const auto& value) -> std::string
        {
            std::ostringstream json;
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, QuerySuccess>)
            {
                json << "{"
                     << "\"ok\":true,"
                     << "\"query\":\"" << escapeJson(value.queryName) << "\","
                     << "\"value\":";
                appendJsonValue(json, value.value);
                json << "}";
            }
            else if constexpr (std::is_same_v<T, QueryError>)
            {
                json << "{"
                     << "\"ok\":false,"
                     << "\"query\":\"" << escapeJson(value.queryName) << "\","
                     << "\"error\":{\"message\":\"" << escapeJson(value.message) << "\"}}";
            }

            return json.str();
        },
        result);
}

std::vector<std::string> queryNames()
{
    return {
        "window.size",
        "view.window.size",
        "scene.objects",
        "scene.selection",
        "data.scene.objects",
        "data.scene.selection",
        "ui.layout",
        "ui.layout.slot.<panel>.<slot>",
        "ui.docking.status",
        "ui.tearout.status",
        "ui.windows",
        "ui.window.<viewportId>",
        "ui.panel.<id>",
        "ui.panel.<id>.widgets",
        "ui.widgets",
        "ui.widget.<label>",
        "camera.pose",
        "view.camera.pose",
        "view.background.color",
        "runtime.capabilities",
        "help",
        "scene.object.<id>",
        "scene.object.transform.<id>",
        "scene.object.properties.<id>",
        "data.scene.object.<id>",
        "data.scene.object.transform.<id>",
        "data.scene.object.properties.<id>",
    };
}
