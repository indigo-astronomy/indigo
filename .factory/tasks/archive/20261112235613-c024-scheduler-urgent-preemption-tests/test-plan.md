# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `test_agent_scheduler`: urgent_insertion_retries_the_preempted_task | Scripting Agent, site at local evening | order, plan, history, retry kept | after implementation |
| T-002 | AC-002 | `test_agent_scheduler`: urgent_insertion_fails_the_preempted_task | same | FAILED, not planned, not rerun | after implementation |
| T-003 | AC-003 | `run_scheduler_simulators.py` "urgent task fails a preempted capture" | private indigo_server, CCD and mount simulators | aborted capture, urgent frames, FAILED | after implementation |
| T-004 | AC-004 | `run_driver_test.py agent_scheduler` | as recorded | no failure | final |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Not applicable; no defect found.

## Negative cases and permission boundaries
The failed task is never started again; its capture does not continue.

## Isolation and cleanup
Private HOME per integration case and per server run.

## Deferred tests and coverage limitations
None.
