# Development plan

Status: complete

## Approach and affected components
New guide `indigo_docs/INDIGO_SCHEDULER_GUIDE.md` written from the implementation; every code block extracted and run against a private server; HOME.md link, Xcode reference, SCHEDULER.md pointer and history example fix.

## Baseline
- Branch `scheduler` at c52655486.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Collect facts from code, docs and tools | AC-001..AC-004 | None | — | done |
| 2 | Write the guide | AC-001..AC-004 | 1 | — | done |
| 3 | Run every example | AC-005 | 2 | verification.md | done: found and fixed four guide errors |
| 4 | Links, Xcode, SCHEDULER.md | AC-006 | 2 | diff, link check | done |
| 5 | Independent fact check | all | 2 | quality-review.md | see quality-review.md |

## Test strategy
Code blocks extracted from the Markdown file so the tested text is exactly the published text.

## Security risks and mitigations
None; documentation only.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
None.
