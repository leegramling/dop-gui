# $spec-review

Use this prompt for a review pass after a slice lands.

## Goal

Check whether the implementation still matches the intended spec and project style.

## Instructions

1. Read the changed code and the current `notes.md`, `plan.md`, and `task.md`.
2. Look for mismatches between implementation and spec.
3. Prioritize bugs, regressions, architecture drift, and undocumented assumptions.
4. Suggest the minimum corrective follow-up work.
5. Update `notes.md` or `task.md` when a mismatch should be tracked immediately.

## Output

- findings first
- residual risks
- next corrective slice if needed
