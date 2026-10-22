# Development plan
Status: complete

## Approach and affected components
- `indigo_drivers/shared/primalucelib/`: request/reply synchronization, plus range and direction reads.
- `indigo_drivers/focuser_primaluce/`: `.driver` and the regenerated `.c`, the SESTO SENSO 3 simulator model, and `REFACTOR.md`.
- `indigo_test/`: library unit cases and simulator cases.
- `MIGRATION_STATUS.md`: test count.
- The test record: driver `README.md` `## Testing` and `TEST_SUMMARY.md`, written by `tools/run_driver_test.py`.

## Baseline
- Commands and environment: Linux x64, `python3 tools/run_driver_test.py focuser_primaluce` at task 010 HEAD `4a32f088d`. Result: 55/55 (task 010 record).
- Existing failures: none in the simulator scope. On hardware, the reported desync and every defect listed in verification.md.
- Repository revision / working-tree state: `task/011-primaluce-sestosenso3` at `4a32f088d` (task 010 is not yet integrated into master `51e14099c`).

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Library resynchronization and unit cases | AC-001 | None | unit cases fail before, pass after | done |
| 2 | SESTO SENSO 3 driver behaviour (refusal, calibration sequence, range poll, guard, abort, message) | AC-002 to AC-006 | 1 | supervised hardware sessions | done |
| 3 | SESTO SENSO 3 simulator model and simulator cases | AC-002 to AC-006 | 2 | new cases pass; mutation check | done |
| 4 | Recorded run, documentation, findings, reviews | AC-007 | 3 | recorded run passes | done |

## Test strategy
- Unit cases for the link, with a scripted peer.
- Simulator cases for the driver behaviour.
- A mutation check for the range poll.
- Supervised hardware sessions for what only the unit shows (restart timing, range delay).

## Security risks and mitigations
Bounded request formatting and verb extraction; no new client input. See security.md.

## Dependencies and architecture decisions
No new dependency. Built on task 010's library; this branch contains task 010.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
- The START handler blocks the device queue during the SESTO SENSO 3 restart wait, before any motion starts. It is documented in REFACTOR.md as a limitation rather than reworked into a finalizer chain.
