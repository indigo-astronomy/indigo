# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | indigo_test/integration/test_agent_scheduler.c reset_all_runs_finished_work_again | repeat 2, retry 1, syntax error, disabled task | counters cleared, script rechecked, disabled kept, rerun | after change |
| T-002 | AC-002 | same file, delete_all_waits_for_the_shutdown_hook | shutdown hook named in lower case | rejected while stopping and during the hook, then 1 deleted, 1 kept | after change |
| T-003 | regression | full integration suite, ASan, server acceptance via run_driver_test.py | | all pass | after change |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
DELETE_ALL during stopping and during the shutdown hook.

## Isolation and cleanup
Per-case private HOME.

## Deferred tests and coverage limitations
Network transport of the hints: blocked by the JSON finding (FND-20261114090521-b043-scheduler-bulk-command-tests-001).
