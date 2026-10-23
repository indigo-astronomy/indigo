# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | simulator `sestosenso3_calibration_restart` | `--profile sestosenso3` | LED change completes while X_CALIBRATE is BUSY during the restart and steps | before (fails) and after |
| T-002 | AC-002 | simulator `sestosenso3_calibration` | `--profile sestosenso3` | LED change completes while X_CALIBRATE is BUSY during the range poll | before (fails) and after |
| T-003 | AC-003 | simulator `sestosenso3_abort_during_restart` | `--profile sestosenso3` | ALERT "Calibration aborted"; no CAL_FOCUSER; range kept; the next START works without a restart | before (fails) and after |
| T-004 | AC-004, AC-005 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |
| T-005 | AC-004, AC-006, AC-007 | supervised hardware calibration | requester's SESTO SENSO 3 | restart, steps and END work with the real timing; LED refused at once; abort at once | after |
| T-006 | AC-006 | `test_primalucelib` `a_restarting_link_refuses_requests_until_probed` | scripted peer | refused without a request; probe under 1 s; reopened after the answer | after |
| T-007 | AC-007 | `sestosenso3_calibration_restart` with the 5 s restart | simulator replays queued requests | calibration starts after the restart | before (fails without the drain) and after |

## Baseline evidence
The task 011 record is 60/60 at 3.0.0.27.

## Bug reproduction
T-001 to T-003 run against the 3.0.0.27 `.driver`: the two BUSY checks fail, and the abort waits
behind the restart and fails.

## Negative cases and permission boundaries
The abort during the start, a restart without an answer (covered by the same failure path as before),
and a failed step (`focuser_calibration`).

## Isolation and cleanup
One simulator per case. The server is stopped after the hardware check.

## Deferred tests and coverage limitations
- Only Linux is run.
- The restart-without-answer path is covered by code review, not by a dedicated case.
