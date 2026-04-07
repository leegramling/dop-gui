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
- preserve a fallback hand-coded panel path for cases where a panel still needs a custom `initialize` builder flow and explicit `render` logic before the JSON5 UI schema is expressive enough

## Phase 3F: Window Management And Tear-Out Preparation

- introduce `WindowManager` as the lifecycle boundary for the primary window and future tear-out windows
- route ImGui docking/platform callback observation through `WindowManager`
- prepare `VsgVisualizer` ownership boundaries for per-window render resources and command graphs
- defer actual tear-out window creation until the callback path and VSG window policy are explicit

## Phase 3F1: Tear-Out Event Detection

- confirm whether the current `vsgImGui` / ImGui docking path exposes enough drag-out or viewport callbacks to detect panel tear-out intent
- record callback/status information through `WindowManager`
- keep this slice observational first: do not add new VSG windows until callback viability is proven

## Phase 3F2: Secondary Window Creation

- define the first `vsg::WindowTraits` policy for tear-out windows
- capture managed-window records keyed by viewport id before real VSG secondary-window creation starts
- expose managed-window snapshots and derived trait data through queries so secondary-window policy can be tested headlessly
- let `WindowManager` create and destroy a managed secondary VSG window when tear-out is requested
- prepare `VsgVisualizer` to create per-window render resources and command graphs for that window

## Phase 3F3: Panel Command-Graph Migration

- move detached panel UI presentation into the tear-out window scene graph
- support reattaching a torn-out panel back into the main dockspace and primary UI command graph
- keep window and render ownership boundaries explicit between `WindowManager` and `VsgVisualizer`

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

## Phase 4C: Hand-Coded Panel Dev Example

- add a hand-coded `New Shape` panel as the explicit fallback-panel example
- define panel-local Yoga layout in `init()` instead of JSON5 for this example
- define widget callbacks in `render()` using the wrapped widget path
- keep the panel queryable and controllable through the existing test command surface
- add scene-state and renderer support for `sphere`, `torus`, and `pyramid`
- let the panel create new scene objects with transform and color data

## Current Focus

Current focus is tear-out preparation after the `New Shape` panel slice: verify whether the current docking path can tell us when panels are dragged out, then add the first managed secondary-window policy only after that callback/event path is real.

Success criteria:

- keep the tested command/query and live playback seams stable while layout becomes more declarative
- prove the main panels can load their flex-layout structures from JSON5 and still drive Yoga layout rects
- preserve `ui.layout.slot.*` inspection while changing the authored layout source
- preserve stable slot ids and legacy flat widget queries while panel-local widget ids are introduced incrementally
- shift UI test command usage toward panel-scoped widget ids so shorter local names no longer depend on one global flat action namespace
- move panel-local layout ownership out of specific panel classes and into a reusable built JSON5 panel-tree path
- move panel-local widget rendering dispatch into the reusable built panel-tree path so panel classes stop hand-walking widget specs
- replace remaining per-panel bind `if` chains with table-driven or generic bind resolution as panel-tree rendering takes over
- extend the built panel-tree render path from `Properties` to `Scene Info` so authored panels share one default render architecture
- move common widget binding/render patterns into reusable panel-tree helpers so panel classes keep only exceptional UI behavior
- migrate both current panels onto those generic panel-tree helpers so the remaining custom code is isolated to genuinely exceptional widgets
- finish the current feature by moving selected-object controls onto the same generic panel-tree binder path, leaving popup and table behavior as the last intentional custom renderers to defer
- keep deeper window/tear-out work documented but deferred until the callback path is viable
- keep the fallback hand-coded panel path documented as an escape hatch, not the default authored UI path
- confirm whether the current docking path exposes enough tear-out events or callbacks to support managed windows at all
- keep the first tear-out slice observational before adding any secondary VSG window implementation
- add the first `WindowTraits` policy only after callback viability is demonstrated
- keep a managed-window snapshot/query slice ahead of real secondary-window creation so viewport lifecycle and trait derivation can be verified independently
- prepare `VsgVisualizer` for future per-window command-graph ownership without collapsing window lifecycle into render code
- move detached panel UI command graphs into a secondary window only after both callback detection and secondary-window creation exist
- support reattaching detached panels back into the main dockspace after the detach path is working

Next focus after current slice:

- add real secondary-window creation and destruction if tear-out callback observation is viable
- move `.gltf` / `.glb` scene growth into the next feature after the tear-out direction is clarified
- revisit popup/table generalization in a later feature if we still want zero custom renderers in the declarative path
