# Notes

## Merge Request Notes

- [x] Create feature branch `feature/app-input-visualizer-command`.
- [x] Create feature branch `feature/vsgimgui-docking-external`.
- [x] Vendor `vsgImGui` under `/home/lgramling/dev/dop-gui/external/vsgImGui`.
- [x] Switch vendored `imgui` to `v1.91.6-docking`.
- [x] Verify the vendored library builds against `/home/lgramling/dev/vsg_deps/install`.
- [x] Add the first application target in `main.cpp`.
- [ ] Decide when to enable actual ImGui rendering in the app loop.
- [x] Add top-level CMake wiring for the app target and vendored dependency.
- [x] Verify the top-level project builds successfully.
- [x] Move shader compilation from runtime to build time.
- [ ] Validate successful window creation in an environment with XCB access.
- [x] Refactor the bootstrap app into `App`, `InputManager`, `VsgVisualizer`, and `Command`.
- [x] Verify the refactored project still builds.
- [x] Verify a non-window CLI command path works with `--command help`.

## Architecture Notes

- [x] Start from a working windowed VSG baseline, then move toward a data-first scene model.
- [x] Keep domain state out of VSG node types.
- [x] Aim for a pipeline where scene data is transformed into render data, rather than making VSG the primary model.
- [x] Keep the first app self-contained by generating its scene in code.

## Local Dependency Inventory

Dependencies found in `/home/lgramling/dev/vsg_deps`:

- [x] `VulkanSceneGraph`
- [x] `vsgXchange`
- [x] `vsgImGui`
- [x] `vsgExamples`
- [x] `install`

Likely integration strategy:

- [x] Prefer the `install` tree for includes and libraries during app bootstrap.
- [x] Read source trees for examples and API reference when wiring the first build.

## ImGui Docking Check

Checked path:

- `/home/lgramling/dev/vsg_deps/vsgImGui/src/imgui`

Findings:

- [x] `/home/lgramling/dev/vsg_deps/vsgImGui` is on git branch `master`.
- [x] `imgui.h` identifies the vendored library as Dear ImGui `v1.91.6`.
- [x] Headers and sources mention docking in comments and branch-sync notes.
- [x] The upstream local checkout did not expose the usual public docking API markers like `ImGuiConfigFlags_DockingEnable`, `DockSpace`, or `DockBuilder`.

Conclusion:

- [x] This is not a checkout we should treat as a confirmed docking build.
- [x] The source appears to be standard upstream `master` with some shared comments and internal compatibility points.

## Vendored Docking Dependency

`$spec-init` branch created:

- `feature/vsgimgui-docking-external`

Vendored dependency path:

- `/home/lgramling/dev/dop-gui/external/vsgImGui`

Local sourcing strategy:

- [x] Parent `vsgImGui` repo cloned from `/home/lgramling/dev/vsg_deps/vsgImGui`.
- [x] Nested `imgui` repo cloned from `/home/lgramling/dev/vsg_deps/vsgImGui/src/imgui`.
- [x] Nested `implot` repo cloned from `/home/lgramling/dev/vsg_deps/vsgImGui/src/implot`.

Docking checkout used:

- [x] `external/vsgImGui/src/imgui` checked out at tag `v1.91.6-docking`.
- [x] Verified public docking API markers exist, including `ImGuiConfigFlags_DockingEnable` and `DockSpace()`.

Build verification:

- [x] Configure command:
  `cmake -S external/vsgImGui -B build/vsgImGui -DCMAKE_PREFIX_PATH=/home/lgramling/dev/vsg_deps/install -DCMAKE_BUILD_TYPE=Release`
- [x] Build command:
  `cmake --build build/vsgImGui -j 8`
- [x] Result:
  `build/vsgImGui/lib/libvsgImGui.a` built successfully.

## First App Slice

- [x] Add `/home/lgramling/dev/dop-gui/CMakeLists.txt`.
- [x] Add `/home/lgramling/dev/dop-gui/src/main.cpp`.
- [x] Replace the original `vsg::Builder` approach with an explicit graphics pipeline and compiled SPIR-V shaders.
- [x] Create a viewer, window, camera, command graph, and render loop.
- [x] Catch and print `vsg::Exception` failures for clearer runtime diagnostics.
- [x] Add `/home/lgramling/dev/dop-gui/src/shaders/triangle.vert`.
- [x] Add `/home/lgramling/dev/dop-gui/src/shaders/triangle.frag`.
- [x] Compile shaders with `glslangValidator` during the CMake build.
- [x] Configure the top-level project with:
  `cmake -S . -B build/dop-gui -DCMAKE_BUILD_TYPE=Release`
- [x] Build the top-level project with:
  `cmake --build build/dop-gui -j 8`
- [x] Confirm the previous runtime failure
  `VulkanSceneGraph not compiled with GLSLang, unable to compile shaders`
  no longer occurs.
- [ ] Runtime probe with `./build/dop-gui/dop-gui -f 1` opens a window successfully.
- [x] Current runtime probe result is documented:
  `Failed to create Window, unable to establish xcb connection`

## Prompt Workflow Notes

Repo-local prompt specs live in `prompts/`.

Intent:

- [x] Keep prompt workflows versioned with the codebase.
- [x] Make agent behavior reproducible.
- [x] Give `$`-prefixed names to common planning flows even if the runtime does not automatically register them as native commands.
- [x] Make `$spec-init` the point where git feature-branch workflow begins.

Follow-up option:

- [ ] If we want true command registration later, add a repo-local Codex plugin or skill wrapper.

## Refactor Request: App, InputManager, VsgVisualizer, Command

Requested architecture:

- [x] `App` should own initialization and expose `run()`.
- [x] `InputManager` should own window, event handling, docking integration, and future event record/playback.
- [x] `VsgVisualizer` should own VSG scene creation, render updates, and visualization-facing state changes.
- [x] `Command` should become the testable action surface for CLI, integration tests, unit tests, and future GUI widgets.

Working interpretation:

- [x] `main.cpp` should become thin and delegate to `App`.
- [x] `App` should coordinate `InputManager` and `VsgVisualizer`, not absorb their responsibilities.
- [x] `InputManager` should become the boundary between raw input/events and higher-level commands.
- [x] `Command` should be designed so both tests and UI actions can invoke the same behavior.
- [x] Recording/playback should target commands and/or normalized input events rather than ad hoc GUI callbacks.

Open design notes:

- [x] Decide whether `Command` is an abstract base type, a value type wrapping callable behavior, or a tagged command data model.
- [ ] Decide whether event recording should capture raw events, normalized semantic events, commands, or both.
- [ ] Decide how docking state persistence should be owned by `InputManager`.
- [ ] Decide how `VsgVisualizer` exposes testable update seams without requiring a live window.

Recommended first slice:

- [x] Introduce `App`, `InputManager`, and `VsgVisualizer` as thin wrappers around the current behavior.
- [x] Keep rendering behavior unchanged while moving code out of `main.cpp`.
- [x] Add only the minimum `Command` abstraction needed to establish the direction.

Implementation outcome:

- [x] `main.cpp` now only constructs `App` and calls `run()`.
- [x] `InputManager` owns current window creation and default event-handler wiring.
- [x] `VsgVisualizer` owns the current triangle scene and VSG render setup.
- [x] `Command` supports an initial CLI/test seam with `help` and `noop`.
- [x] `help` executes without requiring window creation, which keeps the command seam usable in headless environments.

## Spec Workflow Notes

- [x] Add `spec.md` as the stable top-level specification file.
- [x] Add `$spec-arch` to maintain long-lived architecture and product decisions.
- [x] Keep `spec.md` focused on stable boundaries and responsibilities rather than branch-level progress.

## Command And Query Direction

- [x] Choose Option 1 as the testing direction: batch CLI command/query mode.
- [x] Use JSON5 for authored command files and UI-related authored files.
- [x] Prefer JSON-compatible structured output for machine-readable responses.
- [x] Do not require Lua as part of the current architecture.
- [x] Prefer semantic commands and queries over raw GUI callback testing.

Planned capabilities:

- [x] Query window size and related window state.
- [x] Query scene object metadata and transforms.
- [x] Query scene object data by object id.
- [x] Query focused scene object transform and property slices by object id.
- [x] Query bootstrap camera pose.
- [x] Add initial `view.*` and `data.*` namespace-style query aliases.
- [ ] Query panel rectangles and visibility when UI state exists.
- [ ] Query model/view/data values as those data layers are introduced.
- [x] Execute commands from CLI and JSON5 scripts.
- [x] Produce structured output suitable for Robot Framework-driven automation.

Architecture notes:

- [x] Commands should mutate state through an explicit surface.
- [x] Queries should read state through an explicit surface.
- [x] CLI, tests, and future GUI widgets should converge on the same command/query runtime.
- [x] Recording/playback should prefer semantic commands by default.
- [x] Command and query parsing should convert early into typed tagged data.
- [x] `std::variant` request/result models are a better fit than virtual command/query hierarchies for the current DOP/functional direction.
- [x] Query growth should prefer path-based lookup plus functional prefix readers instead of adding `Query*` types for each path.

Current implementation status:

- [x] Add `Query` as a first-class abstraction.
- [x] Replace the earlier polymorphic query implementation with `std::variant` request/result types and explicit execution functions.
- [x] Replace the earlier polymorphic command implementation with `std::variant` request/result types and explicit execution functions.
- [x] Collapse the concrete query surface into a single path-based request shape with canonical path aliases.
- [x] Add a small functional query registry for `view.window`, `view.camera`, `data.scene`, and `runtime`.
- [x] Return query values through a recursive generic value tree rather than bespoke per-query result structs.
- [x] Add plain `AppState`, `SceneState`, and `ViewState` bootstrap data owned by `App`.
- [x] Move bootstrap camera and scene metadata out of `VsgVisualizer` and into `AppState`.
- [x] Make `data.scene.*` and `view.camera.*` queries read from application state instead of renderer-owned literals.
- [x] Replace the earlier small command enum set with a path-based command request and explicit mutation handlers.
- [x] Add `state.reset.bootstrap`.
- [x] Add `data.scene.object.<id>.translate=<dx>,<dy>,<dz>`.
- [x] Add `view.camera.set_pose=<eyeX>,<eyeY>,<eyeZ>,<centerX>,<centerY>,<centerZ>,<upX>,<upY>,<upZ>`.
- [x] Add `tests/mutate_cli.json5` to verify mutation commands followed by queries in one process.
- [x] Add `--query window.size`.
- [x] Add `--query scene.objects`.
- [x] Add `--query scene.object.<id>`.
- [x] Add `--query scene.object.transform.<id>`.
- [x] Add `--query scene.object.properties.<id>`.
- [x] Add `--query view.window.size`.
- [x] Add `--query view.camera.pose`.
- [x] Add `--query data.scene.objects`.
- [x] Add `--query data.scene.object.<id>`.
- [x] Add `--query data.scene.object.transform.<id>`.
- [x] Add `--query data.scene.object.properties.<id>`.
- [x] Add `--query camera.pose`.
- [x] Add `--query runtime.capabilities`.
- [x] Add `--query help`.
- [x] Add `--script <file>` batch execution.
- [x] Add a constrained JSON5-style parser for `commands` and `queries` string arrays.
- [x] Add `tests/smoke_cli.json5` as the first batch example.
- [x] Add JSON output for direct command execution.
- [x] Keep these query/script flows usable without XCB window creation.
- [x] Replace the earlier hard-coded command/query factory branches with explicit parsing and tagged dispatch.
- [x] Return structured JSON errors for unknown commands, unknown queries, and missing script files.

Remaining DOP gap:

- [ ] Make live rendering and future UI observe state mutations while the app is running.
- [ ] Introduce richer model data beyond bootstrap scene metadata and camera pose.
- [ ] Replace the constrained string-array script format with a more expressive JSON5 command/query object format when richer arguments are needed.
