# Agents

This file defines how humans and coding agents should work in this repository.

## Project Intent

Build a VulkanSceneGraph application using:

- data oriented programming
- functional-style C++20
- spec-driven workflow

## Agent Rules

- make small changes with a clear before/after behavior
- start new work on a dedicated feature branch when the repository is under git
- update `plan.md`, `task.md`, and `notes.md` when implementation changes the plan or reveals new facts
- prefer simple structs, spans, arrays, and value types
- avoid introducing inheritance-heavy architecture without a written reason in `notes.md`
- keep rendering integration separate from domain data transformations
- treat `../vsg_deps` and `../vsg_deps/install` as the local dependency baseline
- verify assumptions against checked-in code or local dependencies before encoding them into the spec

## Coding Preferences

- use C++20
- prefer free functions over stateful service objects
- prefer explicit inputs and outputs
- minimize hidden ownership and global mutable state
- favor composition over framework-like abstraction
- write modules so they can be tested without a live Vulkan device when practical

## Workflow Contract

Before implementation:

- capture decisions and unknowns in `notes.md`
- keep the active milestone in `plan.md`
- keep only the next thin slice in `task.md`

During implementation:

- finish a small vertical slice
- verify buildability or document why verification was blocked
- record new constraints immediately

After implementation:

- mark completed tasks
- add follow-up items
- prune stale notes so the docs remain operational rather than archival
