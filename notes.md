# Notes

## Merge Request Notes

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
