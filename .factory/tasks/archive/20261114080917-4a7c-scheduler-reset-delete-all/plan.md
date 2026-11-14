# Development plan

Status: complete

## Approach and affected components
Two switch items appended to AGENT_SCHEDULER_TASK_COMMAND, handled before the name lookup; RESET shares a reset_task() helper with RESET_ALL, which does not renumber the urgent order; DELETE_ALL uses remove_task() for every non-hook slot when the scheduler is stopped.

## Baseline
- Branch `scheduler` at 8e4cffd54; recorded run 61/61 at 3.0.0.7.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Names, items, hints, handlers; version | AC-001..AC-005 | None | build, no new warnings | done |
| 2 | Two integration cases | AC-001..AC-005 | 1 | cases pass; three mutations fail them | done |
| 3 | Docs, sandbox verbs, MIGRATION_STATUS | AC-006 | 1 | review | done |
| 4 | ASan and recorded run | all | 2, 3 | verification.md | see verification.md |

## Test strategy
One case per command, each with a restart.

## Security risks and mitigations
Accidental mass deletion: client hint plus the stopped-scheduler rule; hook tasks kept.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
No configuration format change.

## Deviations and owner decisions
RESET_ALL hint added on owner request during implementation.
