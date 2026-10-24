# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002 | simulator `rotator_lifecycle` | normal profile, port initially off | one `"ARCO":1`, no `"ARCO":0`, reconnect without another switch-on | before (fails) and after |
| T-002 | AC-003 | simulator `sestosenso3_arco_stays_calibrated` | sestosenso3 profile, loss model | a move after reconnect works with no calibration notice | before (fails) and after |
| T-003 | AC-002 | simulator `rotator_metadata` | normal | first connect switches the port on | after |
| T-004 | AC-004 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |
| T-005 | AC-001 to AC-003 | hardware, `indigo_prop_tool` cycles | requester's SESTO SENSO 3 with ARCO, after one calibration | flag stays 0, no switch-off, no further switch-on | after |

## Baseline evidence
- Recorded run: 66/66 at 3.0.0.34.
- Hardware logs: `"ARCO":0` followed by `"ARCO":1` sets the calibration flag.

## Bug reproduction
- T-001 and T-002 fail with the old disconnect.
- On hardware, the first connect after the old server shows the loss.

## Negative cases and permission boundaries
A full state without the `ARCO` key keeps the old switch-on (code review).

## Isolation and cleanup
- One simulator per case.
- On hardware, the devices were disconnected and the server stopped afterwards.

## Deferred tests and coverage limitations
- Power-cycle retention was not tested.
- Only Linux was run.
