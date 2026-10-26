# Development plan

Status: final

## Approach and affected components
Mirror the Imager Agent's `AGENT_WHEEL_FILTER` into a new `AGENT_PLATESOLVER_FILTER` property of the shared
plate-solver library; select the chosen filter in `start_exposure()` (imager branch, process running) and return to the
previous one when `CCD_IMAGE` arrives, in `process_failed()` and at the end of a successful process.

## Baseline
- Commands and environment: `make -C indigo_test test-agent-astrometry` on Linux x86_64 at `24319f3e6`.
- Existing failures: none (77/77 recorded 2026-10-25).
- Repository revision / working-tree state: master `24319f3e6`, task branch created from it.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | names, header, library | AC-001..AC-005 | None | builds | done |
| 2 | agent versions, PROPERTIES.md, REFACTOR.md | AC-006 | 1 | review | done |
| 3 | tests in `test_agent_astrometry.c` | AC-001..AC-005 | 1 | cases pass, mutation fails them | done |
| 4 | recorded run, imager suite, end-to-end, leak check | AC-006 | 3 | verification.md | done |

## Test strategy
Fake Imager Agent peer with `AGENT_WHEEL_FILTER` (BUSY → OK, fail and hold knobs) and an ordered request log; CCD
simulator end to end with the real Imager Agent.

## Security risks and mitigations
No new input surface beyond property names coming from the related agent (copied with bounded copies).

## Dependencies and architecture decisions
Items mirror the imager slots one to one. State: the imager's slot and state (snooped), the slot to return to and a "return when the wheel stops" flag, guarded by a small `filter_mutex` (the existing `mutex` is held by `save_config` across a bus call, so the snoop cannot use it).

## Migration, rollout, and rollback
Not applicable; the default `CURRENT` keeps the old behaviour.

## Deviations and owner decisions
`agent_solver` has no automated suite (`run_driver_test.py` finds no test), so only `agent_astrometry` gets a recorded
run; the shared code is the same.
