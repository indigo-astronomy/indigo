# Development plan

Status: complete

## Approach and affected components
Dependency table in SCHEDULER.md "Architecture" and in the guide "Before you start", from the code; then review of the owner's commit against a running server and correction of wrong statements.

## Baseline
- Branch `scheduler` at 4a6d32bf1; first version committed by the owner as e33784568.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Dependency tables in both documents | AC-001, AC-002 | None | review | done (e33784568) |
| 2 | Review on a private server | AC-003, AC-004 | 1 | verification.md | done: two wrong statements |
| 3 | Fix site source and driver loading text | AC-002, AC-004 | 2 | rerun | done |

## Test strategy
Private server on port 7700, agents loaded one by one while the scheduler runs; REASON and DONE read with `indigo_prop_tool`.

## Security risks and mitigations
None; documentation only.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
The owner committed step 1 before review; the fixes are a separate commit.
