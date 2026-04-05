#pragma once

#include <vsg/all.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Plain data for a renderable scene object.
 */
struct SceneObjectState
{
    std::string id;
    std::string kind;
    uint32_t vertexCount = 0;
    vsg::dvec3 position{0.0, 0.0, 0.0};
    vsg::dvec3 rotation{0.0, 0.0, 0.0};
    vsg::dvec3 scale{1.0, 1.0, 1.0};
};

/**
 * @brief Plain data for the current scene.
 */
struct SceneState
{
    std::string name = "bootstrap";
    std::string selectedObjectId;
    std::vector<SceneObjectState> objects;
};

/**
 * @brief Plain data for the camera pose.
 */
struct CameraPoseState
{
    vsg::dvec3 eye{0.0, -2.5, 1.5};
    vsg::dvec3 center{0.0, 0.0, 0.0};
    vsg::dvec3 up{0.0, 0.0, 1.0};
};

/**
 * @brief Plain data for application-facing view state.
 */
struct ViewState
{
    CameraPoseState cameraPose;
    vsg::vec4 backgroundColor{0.2f, 0.2f, 0.4f, 1.0f};
    std::string backgroundColorHex = "#333366";
    double fps = 0.0;
};

/**
 * @brief Registered state for a wrapped widget.
 */
struct WidgetState
{
    std::string label;
    std::string type;
    std::string textValue;
    bool boolValue = false;
};

/**
 * @brief Pending simulated UI action keyed by widget label.
 */
struct UiTestAction
{
    std::string label;
    std::string kind;
    std::string textValue;
    bool boolValue = false;
};

/**
 * @brief Menu item from the authored UI layout.
 */
struct UiMenuItemState
{
    std::string label;
    std::string command;
};

/**
 * @brief Authored widget specification loaded from the UI layout.
 */
struct UiWidgetSpecState
{
    std::string id;
    std::string type;
    std::string label;
    std::string bind;
    std::string arg;
    std::string onClick;
    std::string onChange;
    std::string unit;
    int precision = 3;
    std::vector<std::string> options;
};

/**
 * @brief Top-level menu from the authored UI layout.
 */
struct UiMenuState
{
    std::string label;
    std::vector<UiMenuItemState> items;
};

/**
 * @brief Panel from the authored UI layout.
 */
struct UiPanelState
{
    std::string label;
    bool open = true;
    bool closable = true;
    std::vector<std::string> flags;
    std::vector<UiWidgetSpecState> widgets;
};

/**
 * @brief Parsed authored UI layout.
 */
struct UiLayoutState
{
    std::vector<UiMenuState> menus;
    std::vector<UiPanelState> panels;
};

/**
 * @brief UI-local state and interaction plumbing.
 */
struct UiState
{
    bool testMode = false;
    bool displayGrid = true;
    bool exitRequested = false;
    std::string themeMode = "dark";
    std::optional<std::string> requestedSceneFile;
    std::vector<std::string> requestedCommands;
    UiLayoutState layout;
    std::vector<WidgetState> registry;
    std::vector<UiTestAction> pendingActions;
};

/**
 * @brief Full application state.
 */
struct AppState
{
    SceneState scene;
    ViewState view;
    UiState ui;
};

/**
 * @brief Build the default bootstrap application state.
 * @return Default bootstrap application state.
 */
AppState createBootstrapAppState();
/**
 * @brief Load application state from a scene file.
 * @param filename Scene file path.
 * @return Loaded application state.
 */
AppState loadAppStateFromSceneFile(const std::string& filename);
/**
 * @brief Load the authored UI layout from a JSON5 file.
 * @param filename UI layout file path.
 * @return Parsed UI layout state.
 */
UiLayoutState loadUiLayoutFromFile(const std::string& filename);
/**
 * @brief Find a mutable scene object by id.
 * @param scene Scene to search.
 * @param id Scene object identifier.
 * @return Mutable scene object pointer, or null when not found.
 */
SceneObjectState* findSceneObject(SceneState& scene, const std::string& id);
/**
 * @brief Find an immutable scene object by id.
 * @param scene Scene to search.
 * @param id Scene object identifier.
 * @return Immutable scene object pointer, or null when not found.
 */
const SceneObjectState* findSceneObject(const SceneState& scene, const std::string& id);
/**
 * @brief Find a mutable registered widget by label.
 * @param ui UI state to search.
 * @param label Stable widget label.
 * @return Mutable widget pointer, or null when not found.
 */
WidgetState* findWidget(UiState& ui, const std::string& label);
/**
 * @brief Find an immutable registered widget by label.
 * @param ui UI state to search.
 * @param label Stable widget label.
 * @return Immutable widget pointer, or null when not found.
 */
const WidgetState* findWidget(const UiState& ui, const std::string& label);
/**
 * @brief Find a pending simulated UI action by label and kind.
 * @param ui UI state to search.
 * @param label Stable widget label.
 * @param kind Pending action kind.
 * @return Mutable pending action pointer, or null when not found.
 */
UiTestAction* findPendingUiAction(UiState& ui, const std::string& label, const std::string& kind);
/**
 * @brief Format a linear color as a hex string.
 * @param color Linear RGBA color.
 * @return Hex color string.
 */
std::string colorHexFromVec4(const vsg::vec4& color);
/**
 * @brief Parse a hex color string into a linear RGBA value.
 * @param text Hex color string.
 * @param color Output linear RGBA color.
 * @return True when parsing succeeds.
 */
bool tryParseHexColor(const std::string& text, vsg::vec4& color);
