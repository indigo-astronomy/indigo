# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `test_agent_scheduler_asan`: commands_queued_at_shutdown_are_released | 200 requests, shutdown, restart | no leak, task kept | before and after the fix |
| T-002 | AC-002 | full `test_agent_scheduler`, ASan build, `run_driver_test.py agent_scheduler` | as recorded | no failure | final |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
T-001 fails before the fix with 6 leaked allocations from `agent_change_property`.

## Negative cases and permission boundaries
A request sent while the agent is gone is ignored.

## Isolation and cleanup
Private HOME per case.

## Deferred tests and coverage limitations
The leak check runs only in this case; other cases are not leak-checked (FND-20261113001457-32cc-scheduler-shutdown-command-leak-001).
