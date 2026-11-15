# Specification

Status: approved by owner request 2026-11-13, design `indigo_docs/SCHEDULER.md`

## Problem and outcome
See brief.md.

## Current-state audit
Phase 3a agent: selection calls `indigo_scheduler_next_eligible()` per task, which checks the whole duration at every 5-minute candidate (FND-20261112184305-e0a6-scheduler-agent-core-003). `AGENT_SCHEDULER_PLAN` is designed but not implemented. Hardware decision: no hardware testing.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Interval checks, the next-start search and the time to set computed from per-task tables give the same results and reasons as the direct evaluation | unit `points_match_reference` (720 random checks) |
| AC-002 | The next-start search for 128 tasks of 24 hours and the plan of 128 tasks each take well under one second | unit `next_start_search_cost`, `performance` |
| AC-003 | The plan simulates the selection rules: urgent tasks first in submission order, then score with ties to the earlier task, the running task first, repeat intervals, windows, unschedulable tasks left out, the end of the night, the run limit | unit `test_agent_scheduler_plan` |
| AC-004 | `AGENT_SCHEDULER_PLAN` lists the planned runs with UTC times, run numbers and priorities, Busy for the running task, followed by Alert items with the reason for tasks without a run; it is shown while stopped and recomputed after an upload, a command, an urgent submission, a start and an end | integration `plan_lists_the_night`, `plan_follows_the_run` |
| AC-005 | No regression in the scheduler suites and the server acceptance run | suite runs, recorded run |

## Failure behavior and edge cases
Unknown site: tasks with sky constraints are listed with "site coordinates unknown". No sunrise or sunset within the grid: the night ends 24 hours from now. More runs than items: the list is cut at the item limit.

## Permissions, data, and security requirements
The plan is read-only; labels are built from validated task names and cut without splitting UTF-8.

## Compatibility and nonfunctional requirements
No configuration change; the plan is not persisted.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
None beyond the design.
