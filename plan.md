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

## Phase 4: Content And Iteration

- add scene loading/saving
- add task-oriented examples
- add tests for pure data transforms and serialization paths

## Current Focus

Current focus is `Phase 2C`.

Success criteria:

- define the first command/query runtime shape
- support CLI-driven commands and queries
- use JSON5 for authored script files
- expose structured responses for test automation
