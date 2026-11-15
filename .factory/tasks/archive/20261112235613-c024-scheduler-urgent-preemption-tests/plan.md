# Development plan

Status: complete

## Approach and affected components
Tests only: `indigo_test/integration/test_agent_scheduler.c`, `indigo_test/integration/run_scheduler_simulators.py`; SCHEDULER.md test list; counts and records.

## Baseline
- Revision 996df2eb9 on branch `scheduler`; recorded run 39/39.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Integration cases for retry and fail | AC-001, AC-002 | None | pass, mutation detected | done |
| 2 | Server case for fail | AC-003 | None | 7/7 | done |
| 3 | Docs, counts, recorded run, review | AC-004 | 1, 2 | verification.md | done |

## Test strategy
Deterministic message order and plan items; mutation check (fail and retry decision swapped) fails both integration cases.

## Security risks and mitigations
None; tests only.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
None.
