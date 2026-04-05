# Task

## Active Slice

- [x] Add the first CLI command/query interface for testing and automation.

## Next Tasks

- [x] Define a first query abstraction alongside the current command abstraction.
- [x] Add direct CLI support for simple queries.
- [x] Add a query for window size.
- [x] Add a query for current scene object information.
- [x] Add a query for camera pose.
- [x] Add a runtime capabilities query.
- [x] Define JSON5 as the authored script/config format for command/query batches.
- [x] Add a first batch script loading path, even if only partially implemented.
- [x] Return structured machine-readable output for command/query execution.
- [x] Return structured machine-readable output for direct command execution.

## Next Slice Candidates

- [x] Add object-specific queries by id instead of only aggregate scene metadata.
- [x] Add a command/query registry instead of hard-coded factory branches.
- [x] Add structured error JSON for unknown commands, unknown queries, and script parse failures.
- [x] Add initial `view.*` and `data.*` query namespaces while keeping compatibility aliases.
- [x] Move queries from polymorphic classes to a tagged `std::variant` request/result model.
- [x] Move commands from polymorphic classes to a tagged `std::variant` request/result model.
- [x] Collapse the query type explosion into a path-based query request plus a prefix registry and generic value tree.
- [ ] Add richer model/view/data query namespaces as those layers are introduced.
- [x] Add object transform and property namespaces beyond the current bootstrap metadata.
- [x] Introduce plain `AppState` and `SceneState` data so `data.*` queries are backed by stable model data instead of bootstrap visualizer-owned literals.
- [x] Add the first mutation commands that update `AppState` in headless CLI/script mode.
- [x] Drive `VsgVisualizer` updates from mutated state during a live running app.
- [x] Add `ctest` smoke coverage for the headless CLI query and script paths.
- [x] Add a repo-local desktop runner script for `--stay-open` startup command/script testing.
- [x] Load bootstrap scene data from JSON5 and include multiple shape records for scene inspection tests.
- [ ] Add richer command argument and script-object formats beyond simple numeric path commands.

## Follow-Up Validation

- [ ] Run `./build/dop-gui/dop-gui` in a desktop session with XCB/X11 access.
- [ ] Confirm the window opens and the generated scene renders outside the sandbox.
- [x] Confirm CLI queries can run in a useful way for tests.
- [x] Confirm JSON5-authored scripts can drive at least one command/query batch.
- [x] Confirm `ctest` can execute headless smoke coverage in the build tree.

## Constraints

- [x] Use the local external `external/vsgImGui/src/imgui` checkout at `v1.91.6-docking`.
- [x] Preserve the current triangle app behavior while adding test-facing interfaces.
- [x] Keep platform assumptions minimal.
- [x] Avoid runtime GLSL compilation because the local VSG build lacks GLSLang support.
- [x] Use JSON5 for authored command and UI files.
- [x] Do not add Lua-specific architecture unless a later requirement forces it.
- [x] Do not implement full recording/playback yet unless required to establish the command/query seam.

## Done Definition

- [x] The first query seam exists next to the command seam.
- [x] The app supports at least one useful CLI query for tests.
- [x] JSON5 is the documented authored format for scripts/config.
- [x] The project still builds locally.
- [x] Docs reflect the chosen CLI testing direction.
- [x] Commands and queries both use tagged request/result data instead of inheritance-heavy runtime polymorphism.
- [x] Query growth no longer requires a new C++ request/result type for each inspectable path.
- [x] At least one command can mutate `AppState` and a later query in the same process can observe the change.
