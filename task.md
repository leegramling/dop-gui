# Task

## Active Slice

- [x] Integrate the vendored `external/vsgImGui` build into the first runnable application skeleton.

## Next Tasks

- [x] Create `CMakeLists.txt` with C++20 enabled.
- [x] Point CMake at `../vsg_deps/install`.
- [x] Add `external/vsgImGui` with `add_subdirectory()` or an equivalent local dependency path.
- [x] Create `src/main.cpp`.
- [x] Compile GLSL shaders to SPIR-V during the CMake build.
- [x] Prepare the app to open a window and render a trivial generated scene.
- [x] Wire the build so later ImGui usage is straightforward.
- [x] Write short build instructions into `README.md`.

## Follow-Up Validation

- [ ] Run `./build/dop-gui/dop-gui` in a desktop session with XCB/X11 access.
- [ ] Confirm the window opens and the generated scene renders outside the sandbox.

## Constraints

- [x] Use the vendored `external/vsgImGui/src/imgui` checkout at `v1.91.6-docking`.
- [x] Keep the initial app free of complex scene architecture.
- [x] Keep platform assumptions minimal.
- [x] Prefer straightforward VSG setup over custom abstraction in the first slice.
- [x] Avoid runtime GLSL compilation because the local VSG build lacks GLSLang support.

## Done Definition

- [x] Project config exists in repo.
- [x] Project builds locally.
- [x] Vendored `vsgImGui` dependency is wired from repo-local sources.
- [x] Runtime startup reaches window creation and reports environment-specific failures clearly.
- [x] Docs reflect the actual build and runtime path.
