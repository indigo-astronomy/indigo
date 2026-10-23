# Development plan
Status: complete

## Approach and affected components
- `indigo_focuser_primaluce.driver`: the relative move handler, `rotator_relative_move_ended()`, the abort handling and the version; the `.c` is regenerated.
- `test_focuser_primaluce_simulator.c`: two cases.
- `PROPERTIES.md`, `REFACTOR.md`, `MIGRATION_STATUS.md` and the test record.

## Baseline
- Commands and environment: Linux x64 at master `17681f4f8`, recorded 67/67.
- Existing failures: none.
- Repository revision / working-tree state: the task branch from master.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Handler, end state, abort | AC-001 to AC-003 | None | build | done |
| 2 | Tests; origin from the reached target | AC-004 | 1 | the SS3 case failed before the change and passes after it | done |
| 3 | Recorded run, docs, reviews | AC-005 | 2 | recorded run passes | done |

## Test strategy
One case is run on both profiles: two moves, one refusal and one abort.

## Security risks and mitigations
See security.md.

## Dependencies and architecture decisions
None. This follows the ZWO rotator.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
None.
