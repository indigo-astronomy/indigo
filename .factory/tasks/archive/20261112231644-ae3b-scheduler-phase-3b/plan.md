# Development plan

Status: complete

## Approach and affected components
Sky module: per-task tables of the first failing grid point; plan module (score and simulation) shared with the agent; agent property and recompute triggers; tests, docs, project files.

## Baseline
- Revision 788433fc2 on branch `scheduler`; scheduler integration 30/30, unit suites passing, recorded run 36/36.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Points tables, night end in `indigo_agent_scheduler_sky.c/.h` | AC-001, AC-002 | None | unit sky | done |
| 2 | `indigo_agent_scheduler_plan.c/.h`, shared score | AC-003 | 1 | unit plan | done |
| 3 | Agent: tables in selection, `AGENT_SCHEDULER_PLAN`, names, version 0x03000004 | AC-004 | 2 | integration | done: found and fixed state-only plan updates not being sent |
| 4 | Project files, Makefile, docs | AC-005 | 3 | builds | done |
| 5 | Independent review, fixes, recorded run | all | 4 | verification.md | see verification.md |

## Test strategy
Fast-versus-reference equivalence on random tasks; deterministic simulation cases; timing cases; integration cases against the real Scripting Agent with the site moved to local evening so that the cases do not depend on the time of day.

## Security risks and mitigations
No new input; labels bounded and UTF-8 safe; arrays bounded by MAX_TASKS, MAX_PLAN_RUNS and MAX_PLAN_ITEMS.

## Dependencies and architecture decisions
None new. The reference functions stay as the specification of the tables and are used by the tests.

## Migration, rollout, and rollback
Not applicable; no persisted data changes.

## Deviations and owner decisions
- Night end during the day is the sunrise after the next sunset, so a plan made by day shows the coming night (SCHEDULER.md updated).
