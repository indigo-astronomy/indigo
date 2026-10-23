# Development plan
Status: complete

## Approach and affected components
- `indigo_focuser_primaluce.driver` (END finalizer check, version) and the regenerated `.c`.
- Simulator `lostposition` fault.
- `test_focuser_primaluce_simulator.c`.
- REFACTOR.md, MIGRATION_STATUS.md, and the test record.

## Baseline
- Commands and environment: Linux x64 at master `8989126b5` (equal to origin/master). Recorded
  63/63 at 3.0.0.30.
- Existing failures: none.
- Repository revision / working-tree state: the task branch from master.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Position check in the END finalizer; simulator fault; test | AC-001, AC-002 | None | case fails without the check | done |
| 2 | Recorded run, documentation, reviews | AC-003 | 1 | recorded run passes | done |

## Test strategy
One simulator case reproduces the lost coordinate at StoreAsMaxPos. The recorded run covers the rest.

## Security risks and mitigations
See security.md.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
None.
