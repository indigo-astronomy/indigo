# Task quality review
Verdict: pass
Reviewer and independence: quality reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5). Not independent; this was a separate pass over the final diff.
Reviewed revision and working-tree state: `task/011-primaluce-sestosenso3` at `4a32f088d` plus the uncommitted task diff: library, `.driver` with the regenerated `.c`, simulator, unit and simulator tests, REFACTOR.md, MIGRATION_STATUS.md, findings, task records.

## Scope and method
- Read the full `git diff HEAD` of the library, the `.driver`, the simulator and both tests.
- Compared the generated handlers with the `.driver` blocks: the `X_CALIBRATE` `on_change_request` dispatch and the abort epilogue.
- Checked the formatting rules: tabs, single-line calls, braces, comma spacing.
- Checked the generator conventions: no `return` in `on_connect` or `on_disconnect`, no duplicated BUSY guard, finalizer naming unchanged.

## Quality findings
No quality finding remains open in the reviewed diff. Notes:

- The two blocking waits (SESTO SENSO 3 restart before START motion, and the range poll after END) are documented in REFACTOR.md as limitations. Reworking them into finalizer chains would require a new multi-step state machine for a one-off operation in which no motion runs.
- `focuser_motion_failed` was split into `focuser_motion_refused` (any message) and the controller-reason wrapper. This is needed because the uncalibrated refusal has its own message; there is no duplication.
- The range handling at connect and at END is shared through `focuser_set_range()`. It replaces the inline code at connect and the per-preset maximum assignment.
- The simulator SESTO SENSO 3 branches are gated by `sestosenso3()`, which leaves the SESTO SENSO 2 replies unchanged. The single SESTO SENSO 2 change is that `StoreAsMaxPos` stores `CAL_MAXPOS`, as a controller does.
- The tests check behaviour (requests, states, messages, published ranges) rather than internal flags. `sestosenso3_calibration` failed under the mutation of the range poll.

## Fix reinspection
One defect came up during this review and was fixed before the recorded run: the first version of the SESTO SENSO 3 abort branch returned before the generated `FOCUSER_ABORT_MOTION` update, so that property stayed BUSY. `sestosenso3_calibration_restart` caught it. The branch now falls through to the generated update. Retest is in verification.md.

## Quality verdict
Pass. Remaining limitations are listed in REFACTOR.md: the two blocking waits, simulator-only coverage of the SESTO SENSO 3 abort and HIZ cases, and Linux-only builds.
