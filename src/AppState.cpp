#include "AppState.h"

AppState createBootstrapAppState()
{
    return AppState{
        .scene =
            SceneState{
                .objects =
                    {
                        SceneObjectState{
                            .id = "bootstrap_triangle",
                            .kind = "triangle",
                            .vertexCount = 3,
                            .position = {0.0, 0.0, 0.0},
                            .scale = {1.0, 1.0, 1.0},
                        },
                    },
            },
        .view =
            ViewState{
                .cameraPose =
                    CameraPoseState{
                        .eye = {0.0, -2.5, 1.5},
                        .center = {0.0, 0.0, 0.0},
                        .up = {0.0, 0.0, 1.0},
                    },
            },
    };
}

const SceneObjectState* findSceneObject(const SceneState& scene, const std::string& id)
{
    for (const auto& object : scene.objects)
    {
        if (object.id == id) return &object;
    }
    return nullptr;
}

SceneObjectState* findSceneObject(SceneState& scene, const std::string& id)
{
    for (auto& object : scene.objects)
    {
        if (object.id == id) return &object;
    }
    return nullptr;
}
