# Development plan
Status: complete

## Approach and affected components
- `indigo_focuser_primaluce.driver` (flags, messages, version) and the regenerated `.c`.
- Simulator: the `sestosenso3-arco-uncalibrated` profile.
- `test_focuser_primaluce_simulator.c`.
- REFACTOR.md, MIGRATION_STATUS.md, and the test record.

## Baseline
- Commands and environment: Linux x64 at master `f019b0047` (equal to origin/master). Recorded 64/64 at 3.0.0.31.
- Existing failures: none in the simulator; on hardware, the missing reason.
- Repository revision / working-tree state: the task branch from master.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Flags and messages; simulator profile; tests | AC-001 to AC-003 | None | tests fail without the messages | done |
| 2 | Hardware check, recorded run, docs, reviews | AC-004 | 1 | recorded run passes | done |

## Test strategy
- A new SESTO SENSO 3 rotator case.
- The focuser case extended with the `ignore` fault.
- A check that both cases fail without the change.
- A supervised hardware check.

## Security risks and mitigations
See security.md.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
None.
