# Development plan
Status: complete

## Approach and affected components
- `indigo_focuser_primaluce.driver` (label, message, version) and the regenerated `.c`.
- `test_focuser_primaluce_simulator.c`.
- REFACTOR.md and the test record.

## Baseline
- Commands and environment: Linux x64 at master `29abdaf6a`.
- Recorded result: 67/67 at 3.0.0.35.
- Existing failures: none.
- Repository revision / working-tree state: task branch from master. The merge with origin/master is planned after the commit.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Label and message; test | AC-001, AC-002 | None | case fails without each | done |
| 2 | Recorded run, docs, reviews | AC-003 | 1 | recorded run passes | done |

## Test strategy
`focuser_calibration` is extended and checked once with each hint removed.

## Security risks and mitigations
See security.md.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
None.
