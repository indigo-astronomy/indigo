# Development plan

Status: complete

## Approach and affected components
Two counters in the private data, counted where one-off urgent tasks leave the list, saved as a `#urgent` line at the top of the saved history text, cleared with the history.

## Baseline
- Branch `scheduler` at b285376f8; recorded run 60/60 at 3.0.0.6.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Names, items, counting, persistence, clear; version | AC-001..AC-004 | None | build, no warnings | done |
| 2 | Integration case | AC-001..AC-003 | 1 | case passes; mutation fails it | done |
| 3 | Docs, sandbox status, MIGRATION_STATUS | AC-005 | 1 | review | done |
| 4 | ASan and recorded run | all | 2, 3 | verification.md | done: first run found a positional TOTAL read in a test, fixed; 61/61 |

## Test strategy
One integration case covering success, failure, expiry, deletion, stored urgent task, restart, clear and saved clear.

## Security risks and mitigations
Restored values clamped.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Older configurations: zero counters. Older agents reading a new configuration skip the `#urgent` line (3 fields < 5).

## Deviations and owner decisions
None.
