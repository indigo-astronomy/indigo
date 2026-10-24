# Development plan
Status: complete

## Approach and affected components
`indigo_focuser_primaluce.driver` (two messages, version) and the regenerated `.c`;
`test_focuser_primaluce_simulator.c`; REFACTOR.md; the test record.

## Baseline
- Commands and environment: Linux x64 at master `6ed7dbde4`; the merged tree passed 67/67.
- Existing failures: none.
- Repository revision / working-tree state: the task branch from master.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Messages; tests | AC-001, AC-002 | None | the tests fail without the messages | done |
| 2 | Recorded run, docs, reviews | AC-003 | 1 | the recorded run passes | done |

## Test strategy
Three existing calibration cases are extended and checked to fail without the messages.

## Security risks and mitigations
See security.md.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
None.
