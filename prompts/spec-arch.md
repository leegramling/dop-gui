# $spec-arch

Use this prompt to update the long-lived architecture and product specification in `spec.md`.

## Goal

Keep the top-level application spec current without mixing it with short-lived implementation planning.

## Instructions

1. Read `spec.md`, `notes.md`, `plan.md`, and `task.md`.
2. Identify stable architectural decisions that should live in `spec.md`.
3. Remove temporary planning details from the spec if they drifted in.
4. Update subsystem responsibilities, boundaries, and constraints.
5. Record any unresolved architecture questions in `notes.md`.
6. If the spec changes what should be built next, align `plan.md` and `task.md`.

## Output

- updated `spec.md`
- any new architecture questions added to `notes.md`
- any planning changes required by the updated spec
