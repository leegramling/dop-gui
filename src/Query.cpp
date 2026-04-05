#include "Query.h"

#include "App.h"
#include "Command.h"
#include "InputManager.h"
#include "VsgVisualizer.h"

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
    }

    if (includeTransform)
    {
        fields.push_back(makeField("position", makeVec3Value(object.position)));
        fields.push_back(makeField("scale", makeVec3Value(object.scale)));
    }

    return QueryValue{QueryObject{std::move(fields)}};
}

std::string canonicalizeQueryPath(const std::string& name)
{
    if (name == "window.size") return "view.window.size";
    if (name == "view.window.size") return name;
    if (name == "camera.pose") return "view.camera.pose";
    if (name == "view.camera.pose") return name;
    if (name == "scene.objects") return "data.scene.objects";
    if (name == "data.scene.objects") return name;
    if (name == "runtime.capabilities") return name;
    if (name == "help") return "runtime.capabilities";

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

const std::vector<QueryRoute>& queryRoutes()
{
    static const std::vector<QueryRoute> routes{
        QueryRoute{.prefix = "view.window", .read = readWindowQuery},
        QueryRoute{.prefix = "view.camera", .read = readCameraQuery},
        QueryRoute{.prefix = "data.scene", .read = readSceneQuery},
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
        "data.scene.objects",
        "camera.pose",
        "view.camera.pose",
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
