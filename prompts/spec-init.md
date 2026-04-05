# $spec-init

Use this prompt when starting a new feature or subsystem.

## Goal

Turn a rough request into a constrained spec and a small first slice.

## Instructions

1. Ensure the repo is on a dedicated feature branch for the requested work.
2. If no suitable branch exists yet, create one before planning or implementation.
3. Read `README.md`, `notes.md`, `plan.md`, and `task.md`.
4. Summarize the requested feature in project terms.
5. Identify constraints, assumptions, and unknowns.
6. Update `notes.md` with any new facts or open questions.
7. Update `plan.md` with the milestone that owns the work.
8. Reduce the work into one small executable slice in `task.md`.
9. Avoid implementation until the slice is explicit.

## Output

- branch name used or created
- concise feature summary
- updated notes
- updated plan
- updated task
