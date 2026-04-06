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

Documentation requirement:

- public classes, structs, enums, methods, and free functions should carry Doxygen-style doc comments as they are introduced or refactored
- the repo should include a contributor-facing `HowToAddTestCommands` guide for extending the command/query and UI test surfaces safely
- contributor-facing workflow docs should explain how to add, serialize, and verify new command/query/UI-test actions

## Core Architectural Goals

- keep domain state separate from VSG node ownership
- keep rendering logic separate from input/event orchestration
- make top-level application flow easy to test and reason about
- create explicit seams for automation, recording, and playback
- allow GUI actions, CLI actions, and tests to converge on the same command surface
- prefer explicit command/query data over ad hoc callback-driven test hooks
- keep `data.*` queries backed by plain application state rather than renderer-owned metadata
- make declarative JSON5 UI the primary authored path while preserving a fallback hand-coded panel path when the JSON5 schema is not yet sufficient

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
- long-lived ownership of ImGui platform-window callback state

### `WindowManager`

Responsibilities:

- own window lifecycle metadata for the primary window and future tear-out windows
- observe and eventually own ImGui platform/renderer window callbacks
- provide a stable boundary where ImGui tear-out requests can be translated into VSG window creation and destruction
- keep window creation policy separate from `UiLayer` and from scene/render synchronization

Non-responsibilities:

- direct scene graph construction
- per-frame UI content rendering
- replacing `InputManager` as the source of raw input events

### `VsgVisualizer`

Responsibilities:

- own VSG scene setup
- own rendering-facing resources and updates
- translate application state into VSG structures
- expose a narrow API to update the visualization
- own optional scene helpers such as a display grid when requested by state
- prepare to own per-window render resources, command graphs, and scene/UI presentation for future managed windows

Non-responsibilities:

- window/input orchestration
- direct CLI parsing
- long-lived application flow control
- ownership of application truth for inspectable scene and view state

### `Theme`

Responsibilities:

- define default ImGui look-and-feel choices for the application
- provide a stable styling boundary for wrapped UI widgets
- allow later extension or overload without coupling callers to raw ImGui styling setup
- own named theme colors, widget variants, and panel/window flag defaults

Non-responsibilities:

- application state ownership
- menu or panel content decisions

### `Panel`

Responsibilities:

- represent a testable authored UI panel controller
- own panel-local widget binding and layout interpretation
- expose panel callbacks, docking policy, and future tear-out behavior through explicit state and options
- support a fallback hand-coded panel path for early prototypes or exceptional panels that still need an `initialize` builder phase and a custom `render` method

Non-responsibilities:

- low-level ImGui window begin/end calls
- application-global theme policy

### `PanelWindow`

Responsibilities:

- provide a narrow wrapper over raw ImGui panel/window calls
- apply authored window flags and minimum size constraints
- register stable panel labels for test/query inspection

Non-responsibilities:

- panel-local business logic
- authored layout interpretation

### `UI Widget Wrappers`

Responsibilities:

- provide wrapped `Text`, `Input`, `Button`, `Checkbox`, `RadioButton`, `ComboBox`, `Popup`, `Table`, and additional input variants over raw ImGui widgets
- default to basic styling while allowing overloads or richer options later
- use unique labels so widgets can be registered, queried, and tested consistently
- support headless or test-mode evaluation against explicit state where practical
- accept explicit test input so headless tests and GUI-driven simulation can control widget behavior deliberately
- return enough widget result/state so tests can verify button presses, checkbox changes, input edits, and displayed text
- support explicit callbacks or callback adapters so panel/widget behavior can remain testable rather than being buried in ad hoc ImGui code

Non-responsibilities:

- direct scene rendering
- owning long-lived application truth outside explicit state

### `UI Test Command`

Responsibilities:

- provide a stable way to simulate clicking, toggling, and editing UI elements by unique label
- work in both headless mode and live GUI mode
- drive the same wrapped widget paths rather than bypassing UI behavior with ad hoc mutations

Non-responsibilities:

- replacing ordinary application commands for non-UI workflows
- owning unrelated application data

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

## UI Direction

Initial ImGui surface should include:

- menubar
- menu items
- panels
- wrapped text widgets
- wrapped input widgets

Declarative UI direction:

- widget existence, binding, callbacks, and layout should converge into a JSON5-authored UI description
- layout should move toward a small flexbox-like schema rather than remaining permanently hard-coded in C++ builder code
- Yoga should remain the layout engine underneath that schema
- C++ panel controllers should interpret authored layout data rather than duplicating widget ids and slot structure long-term
- the intended authored vocabulary should stay intentionally small at first:
  - `column`
  - `row`
  - `gap`
  - fixed `width` / `height`
  - `flex`
  - leaf `slot`

Transition rule:

- builder-style Yoga layout in panel init code is acceptable as a temporary adapter
- new work should bias toward making layout declarative rather than expanding hand-written builder trees indefinitely
- wrapped button widgets
- wrapped checkbox widgets
- radio buttons
- combo boxes
- popups
- tables
- theme colors and window flags
- docking and tear-out aware panel behavior

Required direction:

- all UI elements should use unique labels
- widget labels should use stable path-like test IDs, favoring flat names such as `panel-fps`, `panel-bgcolor`, or `menuitem-scene-cubes`
- labels should be registrable so tests and queries can target UI elements consistently
- a headless test flag or equivalent mode should allow wrapped widgets to evaluate against explicit test state
- wrapped widgets should return or expose enough value for tests to verify text, input, checkbox, and button state without a live desktop session
- wrapped widgets should be able to consume explicit test input such as "clicked", "checked", or "input text"
- in live GUI mode, tests should still be able to simulate those UI interactions against the same wrapped widgets
- UI layout and properties should be authorable in JSON5 rather than hard-coded everywhere
- panel layout should be able to use a Yoga-based rect computation path, informed by the local `../vsgLayt` examples and `setPos`-style placement model

State ownership guidance:

- do not treat `UiState` as a dumping ground for unrelated application data
- values such as FPS, object count, or scene selection should come from application/model state and be presented by UI wrappers, not owned redundantly by the UI layer unless there is genuine UI-local interaction state
- `UiState` should focus on UI-local concerns such as test mode, widget registry, pending simulated interactions, and panel/widget interaction state
- application-bound inputs such as background color should read from and write back to application/view/model state rather than living only inside UI-local storage

Initial UI target:

- a menubar with `File -> Exit`
- a menubar with `Scene -> cubes`
- a menubar with `Scene -> Shapes`

Next UI expansion target:

- richer widget coverage including radio groups, combos, popups, and tables
- panel callbacks that trigger explicit commands or state updates
- docking and tear-out capable panels where supported by the local ImGui integration
- Yoga-backed layout definitions in panel init code, using the local `../vsgLayt` examples as the first reference point

## Scene Growth Direction

- bootstrap scenes should grow from simple generated primitives toward authored mixed scenes
- scene data should support `.gltf` and `.glb` assets in addition to current primitive records
- scene structures should be able to represent larger composed scenes rather than only a flat bootstrap object list
- a panel showing FPS
- a panel showing object count
- a panel input for background color expressed as a hex string such as `#0000FF`
- a panel checkbox for `display grid`

Related rendering requirement:

- `VsgVisualizer` should support a grid helper object that can be enabled or disabled from UI state

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
- UI wrappers should use the same explicit-state and queryable-label discipline as the command/query runtime
- UI test simulation should use a dedicated `test_command` or equivalent structured UI action path by unique label

Preferred query naming direction:

- `view.*` for view-facing state such as window and camera information
- `data.*` for inspectable application or scene data
- `ui.*` for inspectable panel, menu, widget, and theme-facing state as those layers are introduced
- compatibility aliases may exist temporarily, but long-term growth should prefer namespaced queries

## UI Testing Direction

Headless mode:

- wrapped widgets should not need raw `ImGui::` calls in order to evaluate test behavior
- tests should be able to provide explicit widget test input and receive the resulting widget state or return value
- button wrappers should be able to simulate pressed/not-pressed
- checkbox wrappers should be able to simulate checked state changes
- input wrappers should be able to simulate text edits and propagate the edited value into application data
- text wrappers should expose the text they would display for inspection

GUI mode with simulation:

- the real GUI should still render in the application
- tests should be able to simulate clicking or changing labeled widgets while the GUI is visible
- simulated UI interactions should use the same wrapped widget path as ordinary GUI rendering

Desired testing surface:

- add a `test_command` or equivalent structured UI action path
- target UI elements by unique label
- allow actions such as `click`, `set_checked`, and `set_input`
