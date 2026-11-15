# Development plan

Status: complete

## Approach and affected components
Two integration cases in indigo_test/integration/test_agent_scheduler.c; documentation of the coverage.

## Baseline
- Branch `scheduler` at 2ed5675dd; recorded run 63/63 at 3.0.0.8.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Two cases | AC-001, AC-002 | None | cases pass; two mutations fail the reset case | done |
| 2 | Finding for JSON item hints | AC-003 | None | findings check | done |
| 3 | Docs, MIGRATION_STATUS | AC-004 | 1 | review | done |
| 4 | ASan and recorded run | all | 1-3 | verification.md | see verification.md |

## Test strategy
Integration with the real Scripting Agent.

## Security risks and mitigations
None.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
None.

## Deviations and owner decisions
None.
