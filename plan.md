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

## Current Focus

Current focus is `Phase 3B`.

Success criteria:

- expand the authored JSON5 UI format beyond the current first menu/panel slice
- add richer `ui.*`, `model.*`, `view.*`, and `data.*` inspection paths where needed
- keep the tested command/query and live playback seams stable while the authored UI grows

Next focus after current slice:

- add more authored widget properties and panel structure to `ui/layout.json5`
- introduce more application-bound inputs and outputs through the wrapper layer
- decide whether widget registry metadata should become a dedicated UI subsystem record rather than staying under `UiState`
