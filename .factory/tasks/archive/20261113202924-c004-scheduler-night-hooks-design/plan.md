# Development plan

Status: complete, awaiting owner review of the design

## Approach and affected components
Planned section "Night hooks (planned)" after "Hooks" in SCHEDULER.md; survey row and adopted list; phase 5; test bullets. Guide: "Several nights" subsection with current behaviour and a link to the plan.

## Baseline
- Branch `scheduler` at 8983cf9b7.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Design section, survey, phases, tests in SCHEDULER.md | AC-001..AC-003 | None | review | done |
| 2 | Guide subsection | AC-004 | 1 | review against the code | done |
| 3 | Link, anchor and whitespace check; independent review | AC-005, all | 2 | verification.md | done |

## Test strategy
Documentation only: link/anchor script, `git diff --check`, review against the code, independent reviewer.

## Security risks and mitigations
None; documentation only.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
None.
