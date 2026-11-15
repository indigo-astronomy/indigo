# Development plan

Status: complete

## Approach and affected components
Full read of the guide; update the stale sections; full example with task files; check on a private server.

## Baseline
- Branch `scheduler` at 0b7d345dc.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Read the guide, list stale sections | AC-001 | None | list in specification.md | done |
| 2 | Edit the guide | AC-001..AC-003 | 1 | review | done; removed a claim about clouds (no weather input yet) |
| 3 | Private server check of the example; link check | AC-002, AC-004 | 2 | verification.md | done |

## Test strategy
Documentation only: example commands on a private server, link check.

## Security risks and mitigations
None.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
None.
