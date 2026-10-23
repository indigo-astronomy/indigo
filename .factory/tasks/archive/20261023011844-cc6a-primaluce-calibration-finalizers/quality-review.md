# Task quality review
Verdict: pass
Reviewer and independence: quality reviewer pass, same agent as the developer (Claude Code, claude-opus-5-5). Not independent; a separate pass over the final diff.
Reviewed revision and working-tree state: `task/20261023011844-cc6a-primaluce-calibration-finalizers` at `d37d4392d` plus the uncommitted diff: `.driver` and regenerated `.c`, the simulator test, REFACTOR.md, MIGRATION_STATUS.md, findings, task records.

## Scope and method
- Read the full diff of the `.driver` and the test file.
- Read the regenerated X_CALIBRATE and abort handlers to check the generator's `_finalizer` semantics.
- Compared step order, intervals, limits, states and messages with 3.0.0.27.
- Checked the formatting rules.

## Quality findings
No quality finding remains in the reviewed diff. Notes:
- The step table replaces two nearly identical sleep sequences, so the SESTO SENSO 2 and 3 orders
  are visible side by side.
- `focuser_calibration_failed()` takes over the failure epilogue that the generator no longer
  supplies.
- Both handlers that reference the finalizers own their completion, as AGENTS.md requires. No
  callback was renamed to work around the rule.
- The tests observe queue availability through a real request (a LED change), not internal flags.
  They fail against the previous driver.

## Fix reinspection
Hardware testing found two defects in the first version, both fixed and covered by tests that
fail without the fix:
- Requests waited for the 5 s read timeout of the silent controller during the restart. Fixed by the
  restarting gate.
- The late probe replies failed Init. Fixed by the drain.

The gate lives in the library, so every handler is covered without per-handler checks. Only the
idle timer checks the gate itself, to avoid publishing ALERT.

Two defects were found during development and fixed before the recorded run:
- The starting-abort branch returned before the abort completion was published.
- Because the abort handler references the finalizers, the generator dropped that handler's
  epilogue, so it now publishes OK itself.

`focuser_calibration`, `sestosenso3_abort_during_restart` and the abort cases caught both. The
retest is in verification.md.

## Quality verdict
Pass. Remaining limitations: Linux only, and no dedicated case for a restart without an answer
(the path is shared with the earlier failure handling).
