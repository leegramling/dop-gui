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
};

struct AppState
{
    SceneState scene;
    ViewState view;
};

AppState createBootstrapAppState();
AppState loadAppStateFromSceneFile(const std::string& filename);
SceneObjectState* findSceneObject(SceneState& scene, const std::string& id);
const SceneObjectState* findSceneObject(const SceneState& scene, const std::string& id);
