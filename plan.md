# Plan

## Phase 0: Project Bootstrap

- establish the spec-driven workflow docs
- document local VSG dependencies
- confirm ImGui docking status before UI architecture depends on it
- define prompt-pack conventions for repeatable agent workflows
- vendor a docking-capable `vsgImGui` under `external/`
- prove the vendored library builds against `../vsg_deps/install`

## Phase 1: Minimal VSG Application

- create `CMakeLists.txt` wired to `../vsg_deps/install`
- add the vendored `external/vsgImGui` as a project dependency
- create a minimal `src/main.cpp`
- open a VSG window and render a simple scene
- prove local build and run instructions

## Phase 2: Data-Oriented Scene Core

- define plain data structs for scene entities and transforms
- add system functions that update scene data
- separate scene state from VSG node creation
- create a translation layer from scene data to renderable VSG structures

## Phase 2A: App Structure Refactor

- introduce `App` as the top-level application owner with `run()`
- move window and event ownership behind `InputManager`
- move scene/render ownership behind `VsgVisualizer`
- define the first `Command` abstraction for non-GUI-triggered actions
- preserve the current executable behavior while splitting responsibilities

## Phase 2B: Input And Command Flow

- route CLI-triggered actions through `Command`
- establish an input-to-command path suitable for future GUI widgets
- add event recording and playback seams in `InputManager`
- make the first testing-oriented command executions possible without GUI interaction

## Phase 2C: CLI Query And Script Interface

- define command and query request/response shapes
- add direct CLI query support for simple inspection commands
- add JSON5 script loading for batch command/query execution
- return structured machine-readable results for automation
- expose the first test-facing state queries without requiring GUI widgets

## Phase 3: Tooling UI

- integrate `vsgImGui` if the dependency surface is sufficient
- add scene inspection/debug panels
- validate whether docking is needed enough to justify an ImGui dependency change

## Phase 3A: Testable UI Core

- add the first `Theme` boundary for default ImGui styling
- add the first `Panel` wrapper boundary for labeled panels
- add wrapped `Text`, `Input`, `Button`, and `Checkbox` functions with sane defaults
- ensure wrapped widgets can run in a headless or test-mode path against explicit UI state
- add unique-label registration so UI elements can be queried predictably

## Phase 3A1: UI Simulation And State Cleanup

- separate UI-local state from unrelated application/model values such as FPS and object count
- make wrapped widgets consume explicit test input rather than inferring all behavior from a coarse test flag
- add a `test_command` path for UI interaction by unique label
- support both headless widget simulation and live GUI widget simulation through the same wrapper layer
- ensure input widgets propagate edited values back into application data instead of only storing UI-local copies
- use those seams on a real panel input such as background color so render-facing state can be edited and queried consistently

## Phase 3B: JSON5 UI Specification

- define a JSON5 format for menu, panel, and widget layout/properties
- load the first authored UI layout from a repo scene or UI file
- map authored UI descriptions into wrapped ImGui elements
- expose enough UI state for test queries such as labels, checkbox state, and panel visibility/geometry

## Phase 3D: Expanded Widget And Panel System

- add more wrapped widget types including radio buttons, combo boxes, popups, tables, and richer input variants
- add explicit panel callbacks and panel/window flag support
- expand `Theme` to include named colors and reusable widget/window variants
- introduce docking and tear-out aware panel options where the local ImGui/vsgImGui path supports them

## Phase 3E: Yoga Layout Integration

- evaluate the local `../vsgLayt` examples and notes as the first implementation reference
- add a Yoga-backed layout adapter for panel-local widget placement
- support `setPos`-style widget placement driven by Yoga-computed rects in panel init code
- keep the layout computation testable and separate from widget rendering where possible

## Phase 3E1: Declarative JSON5 Flex Layout

- define a minimal JSON5 layout schema that is closer to flexbox than to ad hoc widget coordinates
- keep Yoga as the layout engine, but generate Yoga trees from authored JSON5 layout data
- start with a small vocabulary:
  - `column`
  - `row`
  - `gap`
  - fixed `width` / `height`
  - `flex`
  - leaf `slot`
- convert one real panel first, with `Properties` as the first target
- preserve existing `ui.layout.slot.*` and `ui.widget.*` query behavior while the layout source changes
- avoid exposing the entire Yoga API in JSON5 until the smaller layout vocabulary stabilizes

## Phase 3F: Window Management And Tear-Out Preparation

- introduce `WindowManager` as the lifecycle boundary for the primary window and future tear-out windows
- route ImGui docking/platform callback observation through `WindowManager`
- prepare `VsgVisualizer` ownership boundaries for per-window render resources and command graphs
- defer actual tear-out window creation until the callback path and VSG window policy are explicit

## Phase 3C: First Tool UI

- add a menubar with `File -> Exit`
- add a menubar with `Scene -> cubes`
- add a menubar with `Scene -> Shapes`
- add a panel showing FPS
- add a panel showing object count
- add a panel checkbox for `display grid`
- add a grid helper to `VsgVisualizer` controlled by application/UI state

## Phase 4: Content And Iteration

- add scene loading/saving
- add task-oriented examples
- add tests for pure data transforms and serialization paths

## Phase 4A: Documentation Quality

- add Doxygen-style doc comments to public classes, structs, enums, methods, and free functions
- add a `HowToAddTestCommands` guide for extending the command/query and UI test surfaces
- expand the repository docs so command/query/UI-test additions have a documented workflow and verification path

## Phase 4B: Scene Asset Growth

- add scene support for `.gltf` and `.glb` assets
- grow the scene model so larger composed scenes can be constructed and queried cleanly
- keep primitive-generated objects available for tests and lightweight bootstrap scenes

## Current Focus

Current focus is broader authored-layout adoption after the first `Phase 3E1` conversions: keep the current Yoga/query path stable while reducing the remaining builder-only layout assumptions and moving from long flat widget ids toward panel-local declarative ids.

Success criteria:

- keep the tested command/query and live playback seams stable while layout becomes more declarative
- prove the main panels can load their flex-layout structures from JSON5 and still drive Yoga layout rects
- preserve `ui.layout.slot.*` inspection while changing the authored layout source
- preserve stable slot ids and legacy flat widget queries while panel-local widget ids are introduced incrementally
- keep deeper window/tear-out work documented but deferred until the callback path is viable

Next focus after current slice:

- expand authored layout support only after the first minimal vocabulary is stable
- finish shortening authored widget ids panel by panel once the panel-scoped widget query path is established enough to avoid relying on one flat widget namespace
- add richer panel/window options and later `.glb` scene growth without breaking the layout/query seams
