# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001..AC-003 | indigo_test/integration/test_agent_scheduler.c urgent_tasks_are_counted | real Scripting Agent | counters as specified | after change |
| T-002 | regression | full integration suite, ASan, server acceptance via run_driver_test.py | | all pass | after change |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Before the change the state had no urgent items (T-001 asserts five items).

## Negative cases and permission boundaries
Deleted and aborted one-off urgent tasks not counted.

## Isolation and cleanup
Per-case private HOME.

## Deferred tests and coverage limitations
Older-version compatibility of the saved line checked by review only.
