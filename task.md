# Task

## Active Slice

- [x] Refine the UI test model so widget simulation works cleanly in both headless and GUI modes.

## Next Tasks

- [x] Remove unrelated data such as FPS from `UiState` when it should come from application/model state instead.
- [x] Define explicit widget test input/result data for `Text`, `Input`, `Button`, and `Checkbox`.
- [x] Add a `test_command` path for UI interaction by unique label.
- [x] Support headless simulation of button clicks, checkbox changes, and input edits through wrapped widgets.
- [x] Support live GUI simulation of the same labeled UI interactions without bypassing the wrapper layer.
- [x] Support timed live script playback for the same labeled UI interactions in one desktop session.
- [x] Make input widgets propagate edited values into application data rather than only UI-local storage.
- [x] Add a panel input for background color and validate that it updates live render clear color in a desktop session.

## Next Slice Candidates

- [ ] Add richer model/view/data query namespaces as those layers are introduced.
- [x] Add `ui.*` query namespaces for panels, menu items, widgets, and theme-facing state.
- [ ] Add richer command argument and script-object formats beyond simple numeric path commands.
- [x] Add direct commands for `app.exit`, `scene.load.*`, and `sleep.ms`.
- [x] Add an authored combined CLI regression script that exercises scene load, UI state changes, sleep, and exit.
- [x] Add an object-based ordered `actions` script format with `command`, `query`, and `sleepMs`.
- [x] Add JSON5 UI layout loading for menu and panel definitions.
- [x] Route menu actions through existing command/state seams instead of raw ImGui callbacks.
- [x] Add `ui.test_command.*` or equivalent command namespaces for simulated UI interaction.
- [x] Add `ui.*` queries for widget values/results after simulated interaction.

## Follow-Up Validation

- [x] Confirm the menubar renders and menu actions behave in a desktop session.
- [x] Confirm the panel shows FPS, object count, `display grid`, and background color input.
- [x] Confirm the headless/test flag can evaluate wrapped widgets without opening a window.
- [x] Confirm the grid toggle changes live visualization state.
- [x] Confirm a simulated UI click or input edit works in both headless and live GUI mode.
- [x] Confirm a simulated UI input edit updates application state in headless mode.
- [x] Confirm a simulated UI menu click can switch scenes in headless mode.
- [x] Confirm a simulated UI checkbox change updates wrapped widget state in headless mode.
- [x] Add repeatable desktop-run helper scripts for live UI action validation.

## Constraints

- [x] Use the local external `external/vsgImGui/src/imgui` checkout at `v1.91.6-docking`.
- [x] Preserve the current scene-loading and multi-object render behavior while adding UI.
- [x] Keep platform assumptions minimal.
- [x] Avoid runtime GLSL compilation because the local VSG build lacks GLSLang support.
- [x] Use JSON5 for authored command and UI files.
- [x] Do not add Lua-specific architecture unless a later requirement forces it.
- [x] Do not implement full recording/playback yet unless required to establish the command/query seam.

## Done Definition

- [x] The app has a wrapped menubar and panel path instead of direct raw ImGui calls everywhere.
- [x] Wrapped widgets are uniquely labeled and queryable.
- [x] Wrapped widgets accept explicit test input and return stable result/state objects.
- [x] A `test_command` path exists for simulated UI interaction by label.
- [x] Headless and GUI modes both support simulated widget interaction through the same wrapper logic.
- [x] The first panel can show FPS, object count, `display grid`, and background color input.
- [x] The project still builds locally.
