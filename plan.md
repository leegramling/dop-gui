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

## Phase 3: Tooling UI

- integrate `vsgImGui` if the dependency surface is sufficient
- add scene inspection/debug panels
- validate whether docking is needed enough to justify an ImGui dependency change

## Phase 4: Content And Iteration

- add scene loading/saving
- add task-oriented examples
- add tests for pure data transforms and serialization paths

## Current Focus

Current focus is `Phase 2`.

Success criteria:

- define the first plain-data scene structures
- separate scene state from VSG node creation
- keep the current app runnable while refactoring toward DOP boundaries
