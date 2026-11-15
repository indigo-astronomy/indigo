# Development plan

Status: complete

## Approach and affected components
Integration cases place targets with the agent's own sky module and the stand-in site; server acceptance cases read the saved FITS headers. One agent change in select_task().

## Baseline
- Branch `scheduler` at 7bd0c2e5e; recorded run 65/65 at 3.0.0.8.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Eligibility cases | AC-001 | None | pass; five sky mutations fail them | done |
| 2 | Score, limits, delays, estimate guard | AC-002..AC-004, AC-006 | None | pass; three mutations fail them | done |
| 3 | Device cases | AC-005 | None | pass after test fixes; found defect 1 | done |
| 4 | Report defects; owner decisions | AC-006, AC-007 | 3 | answers recorded | done |
| 5 | Fix defect 1, finding for defect 2, docs | AC-006, AC-007 | 4 | device suite 11/11 | done |
| 6 | Full suites and recorded run | all | 5 | verification.md | see verification.md |

## Test strategy
Each constraint negative and positive; mutations of the sky module, score, delays.

## Security risks and mitigations
None.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Agent version 0x03000009; no format change.

## Deviations and owner decisions
See specification.md.
