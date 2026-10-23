# Development plan
Status: complete

## Approach and affected components
- `indigo_focuser_primaluce.driver` (rotator finalizer, version) and the regenerated `.c`.
- `test_focuser_primaluce_simulator.c` (helper and extended move case).
- REFACTOR.md, the test record.

## Baseline
- Commands and environment: Linux x64 at master `218d3bf17`. Recorded 65/65 at 3.0.0.32.
- Existing failures: none.
- Repository revision / working-tree state: the task branch from master.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish the angle on every poll; extend the move case | AC-001 | None | the case fails without the fix | done |
| 2 | `preserve_values` on FOCUSER_POSITION and ROTATOR_POSITION; value log in the test client | AC-003 | 1 | the checks fail without it | done |
| 3 | Hardware checks, recorded run, docs, reviews | AC-002 | 1, 2 | recorded run passes | done |

## Test strategy
- The move case is extended with a longer move and an in-motion check.
- The extended case is run once without the fix to confirm it fails.
- A supervised hardware check.

## Security risks and mitigations
See security.md.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
None.
