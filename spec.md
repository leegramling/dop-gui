# Spec

## Purpose

This file captures the stable top-level specification for `dop-gui`.

Use it for:

- application goals
- architectural boundaries
- subsystem responsibilities
- testing direction
- long-lived design constraints

Do not use it for:

- temporary implementation notes
- branch-specific progress tracking
- short-lived task breakdowns

Those belong in:

- `notes.md`
- `plan.md`
- `task.md`

## Product Intent

`dop-gui` is a VulkanSceneGraph application built with:

- data oriented programming
- functional-style C++20
- spec-driven development

The application should remain small, testable, and modular while growing toward a richer interactive tool.

## Core Architectural Goals

- keep domain state separate from VSG node ownership
- keep rendering logic separate from input/event orchestration
- make top-level application flow easy to test and reason about
- create explicit seams for automation, recording, and playback
- allow GUI actions, CLI actions, and tests to converge on the same command surface
- prefer explicit command/query data over ad hoc callback-driven test hooks
- keep `data.*` queries backed by plain application state rather than renderer-owned metadata

## Top-Level Components

### `AppState`

Responsibilities:

- hold plain application state for scene, view, and future model data
- act as the source of truth for `data.*` inspection
- stay serializable and test-friendly

Non-responsibilities:

- VSG node ownership
- window lifecycle
- direct CLI parsing

### `App`

Responsibilities:

- initialize the application
- own top-level subsystem lifetime
- own top-level application state
- coordinate startup and shutdown
- expose `run()`
- host the main loop

Non-responsibilities:

- raw rendering details
- raw window/input implementation details
- embedding feature logic directly in `main.cpp`

### `InputManager`

Responsibilities:

- own window creation and window-facing event wiring
- normalize input and application events
- host docking-related input concerns
- provide seams for event recording and playback
- translate relevant input flows into command execution

Non-responsibilities:

- VSG scene construction
- rendering policy
- domain visualization updates

### `VsgVisualizer`

Responsibilities:

- own VSG scene setup
- own rendering-facing resources and updates
- translate application state into VSG structures
- expose a narrow API to update the visualization

Non-responsibilities:

- window/input orchestration
- direct CLI parsing
- long-lived application flow control
- ownership of application truth for inspectable scene and view state

### `Command`

Responsibilities:

- represent actions that can be triggered by CLI, tests, GUI widgets, or playback
- provide a stable automation surface
- help unify human-driven and test-driven interaction flows
- support serialization-friendly request/response handling

Preferred implementation shape:

- command requests should be tagged data, preferably `std::variant`-based in C++20
- command execution should be explicit functions over application state and subsystem seams
- string parsing should stay at the boundary and convert early into typed command data
- prefer path-based commands plus explicit argument parsing over many bespoke command structs

### `Query`

Responsibilities:

- represent structured read-only inspection of application state
- expose test-visible state such as window metrics, scene object data, panel geometry, and model values
- return serializable results suitable for CLI-driven automation

Preferred implementation shape:

- query requests should be tagged data, preferably `std::variant`-based in C++20
- prefer a small number of top-level request kinds such as path-based queries over one C++ type per query
- query results should remain explicit tagged data rather than ad hoc strings
- execution should be explicit functions over inspectable state, with JSON serialization at the edge
- prefer namespace or prefix registries such as `view.*`, `data.*`, and `runtime.*` so query growth stays disciplined

## Main Entry Point

`main.cpp` should stay thin.

Target shape:

```cpp
int main(int argc, char** argv)
{
    App app{argc, argv};
    return app.run();
}
```

## Rendering Baseline

Current baseline:

- build-time GLSL to SPIR-V compilation
- explicit graphics pipeline creation
- a simple generated scene used for bootstrap validation

Constraint:

- avoid depending on runtime GLSL compilation because the local VSG build does not provide GLSLang support

## Input And Testing Direction

The application should support:

- direct CLI-driven commands
- integration-test-triggered commands
- unit-testable command handling where practical
- future event recording and playback

Preferred direction:

- normalize inputs
- map them to commands
- execute commands against application state first, then adapt rendering and UI from that state
- answer state inspection through queries
- record semantic commands rather than raw GUI callbacks by default

Chosen first implementation:

- batch CLI command/query mode
- JSON5 for authored command and UI files
- JSON-compatible structured responses for tests and tooling

Explicitly not chosen yet:

- embedded Lua as a first-class architecture requirement

## Immediate Refactor Target

The next structural milestone is:

- move the current bootstrap app into `App`, `InputManager`, and `VsgVisualizer`
- preserve the current triangle rendering behavior
- establish the first `Command` seam without overbuilding the system

## First Testing Interface

The first testing interface should be CLI-driven.

Required direction:

- support commands and queries from command line arguments and/or JSON5 script files
- make non-window commands and queries usable in headless environments when possible
- expose structured output suitable for frameworks such as Robot Framework

Early target examples:

- `get_window_size`
- `get_scene_objects`
- `get_object_transform`
- `get_panel_rect`
- `get_model_value`
- `help`

Preferred architecture:

- a shared command/query runtime used by CLI first
- GUI widgets should later call the same command/query layer
- future playback should target semantic commands and queries where practical
- prefer tagged request/result data over inheritance-heavy command/query hierarchies
- prefer path-based query resolution plus functional readers over `QueryFooBarBaz` type proliferation

Preferred query naming direction:

- `view.*` for view-facing state such as window and camera information
- `data.*` for inspectable application or scene data
- compatibility aliases may exist temporarily, but long-term growth should prefer namespaced queries
