# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002, AC-005 | indigo_test/integration/test_agent_scheduler.c reset_all_keeps_running_task_and_urgent_order | real Scripting Agent; urgent tasks placed so the first submitted has the higher slot | hints, order kept, running task untouched, counters 0, saved | after change |
| T-002 | AC-003..AC-005 | same file, delete_all_keeps_hooks_and_needs_a_stopped_scheduler | three hook tasks, one stored, one pending urgent | rejected while running; 3 deleted, 3 kept; hooks unchanged after restart | after change |
| T-003 | regression | full integration suite, ASan, server acceptance via run_driver_test.py | | all pass | after change |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Not a defect; new feature.

## Negative cases and permission boundaries
DELETE_ALL while running (Alert); running task under RESET_ALL.

## Isolation and cleanup
Per-case private HOME.

## Deferred tests and coverage limitations
Client confirmation dialogs are client behaviour; only the hint text is tested.
