# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `test_agent_scheduler_sky`: points_match_reference, night_end | fixed dates, deterministic random tasks | identical results and reasons | after implementation |
| T-002 | AC-002 | `test_agent_scheduler_plan`: performance, next_start_search_cost | 128 tasks | < 1 s | after implementation |
| T-003 | AC-003 | `test_agent_scheduler_plan`: score, order_and_times, ties_to_earlier_task, urgent_first, running_task_first, running_task_overrun, initial_delay, urgent_repeat_queues_behind, repeats, unschedulable_tasks, window_and_night_end, max_runs | fixed date and site | exact order and times | after implementation |
| T-004 | AC-004 | `test_agent_scheduler`: plan_lists_the_night, plan_follows_the_run, plan_gives_the_reasons | Scripting Agent, site at local evening | labels, states, recompute | after implementation |
| T-005 | AC-005 | full scheduler suites, ASan, `run_driver_test.py agent_scheduler` | as recorded | no new failure | final |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Defects found are listed in verification.md with their regression tests.

## Negative cases and permission boundaries
Disabled, done, never-rising and too-long tasks; unknown site via the reference tests.

## Isolation and cleanup
Private HOME per integration case.

## Deferred tests and coverage limitations
The Raspberry Pi 4 timing is not measured; the timing cases run on the development host (Linux arm64).
