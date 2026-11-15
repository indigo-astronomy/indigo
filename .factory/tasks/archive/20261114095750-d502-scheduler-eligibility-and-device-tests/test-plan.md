# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | indigo_test/integration/test_agent_scheduler.c, nine eligibility cases | equator site, targets placed by hour angle; site under the moon | reasons and starts as specified | after change |
| T-002 | AC-002..AC-004, AC-006 | same file, five cases | | as specified | after change |
| T-003 | AC-005, AC-006 | indigo_test/integration/run_scheduler_simulators.py, three cases | CCD, wheel, mount simulators; Mount Agent related to the Imager Agent | frames and headers as specified | after change |
| T-004 | regression | full integration, ASan, run_driver_test.py | | all pass | after change |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Defect 1: device case targets_in_priority_order failed twice before the fix ("the normal task ran before the high one"), passes after it. The in-process guard does not reproduce it, because the estimate runs on the scheduler's own queue there.

## Negative cases and permission boundaries
Every constraint has a waiting case; missing filter and camera; over-limit text and count.

## Isolation and cleanup
Per-case private HOME; private indigo_server with its own HOME.

## Deferred tests and coverage limitations
Hardware not run. "beyond the forecast" not exercised end to end.
