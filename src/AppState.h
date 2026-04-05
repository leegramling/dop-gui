#pragma once

#include <vsg/all.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct SceneObjectState
{
    std::string id;
    std::string kind;
    uint32_t vertexCount = 0;
    vsg::dvec3 position{0.0, 0.0, 0.0};
    vsg::dvec3 scale{1.0, 1.0, 1.0};
};

struct SceneState
{
    std::vector<SceneObjectState> objects;
};

struct CameraPoseState
{
    vsg::dvec3 eye{0.0, -2.5, 1.5};
    vsg::dvec3 center{0.0, 0.0, 0.0};
    vsg::dvec3 up{0.0, 0.0, 1.0};
};

struct ViewState
{
    CameraPoseState cameraPose;
    vsg::vec4 backgroundColor{0.2f, 0.2f, 0.4f, 1.0f};
    std::string backgroundColorHex = "#333366";
    double fps = 0.0;
};

struct WidgetState
{
    std::string label;
    std::string type;
    std::string textValue;
    bool boolValue = false;
};

struct UiTestAction
{
    std::string label;
    std::string kind;
    std::string textValue;
    bool boolValue = false;
};

struct UiMenuItemState
{
    std::string label;
    std::string command;
};

struct UiMenuState
{
    std::string label;
    std::vector<UiMenuItemState> items;
};

struct UiPanelState
{
    std::string label;
};

struct UiLayoutState
{
    std::vector<UiMenuState> menus;
    std::vector<UiPanelState> panels;
};

struct UiState
{
    bool testMode = false;
    bool displayGrid = true;
    bool exitRequested = false;
    std::optional<std::string> requestedSceneFile;
    UiLayoutState layout;
    std::vector<WidgetState> registry;
    std::vector<UiTestAction> pendingActions;
};

struct AppState
{
    SceneState scene;
    ViewState view;
    UiState ui;
};

AppState createBootstrapAppState();
AppState loadAppStateFromSceneFile(const std::string& filename);
UiLayoutState loadUiLayoutFromFile(const std::string& filename);
SceneObjectState* findSceneObject(SceneState& scene, const std::string& id);
const SceneObjectState* findSceneObject(const SceneState& scene, const std::string& id);
WidgetState* findWidget(UiState& ui, const std::string& label);
const WidgetState* findWidget(const UiState& ui, const std::string& label);
UiTestAction* findPendingUiAction(UiState& ui, const std::string& label, const std::string& kind);
std::string colorHexFromVec4(const vsg::vec4& color);
bool tryParseHexColor(const std::string& text, vsg::vec4& color);
