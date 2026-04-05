# $spec-task

Use this prompt when executing the next slice from `task.md`.

## Goal

Implement only the current thin slice and keep the spec current.

## Instructions

1. Read `task.md` first.
2. Implement only the active slice.
3. Prefer the smallest working change over broad architecture.
4. Verify the change locally when possible.
5. Update `task.md` to reflect completion or the next step.
6. Update `notes.md` if implementation revealed a new constraint.

## Output

- implemented slice
- verification result
- updated task and notes if needed
