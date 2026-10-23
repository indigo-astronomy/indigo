# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-003 | simulator `sestosenso3_rotator_needs_calibration` | `--profile sestosenso3-arco-uncalibrated` | ALERT with the reason; ARCO calibration; then the move ends OK | before (fails) and after |
| T-002 | AC-002, AC-004 | simulator `needs_calibration` | `--profile needs-calibration`, `ignore` fault | moves that run end OK; a move that does not run names the calibration | before (fails) and after |
| T-003 | AC-004 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |
| T-004 | AC-001 | supervised hardware | requester's SESTO SENSO 3 with a flagged ARCO | ALERT with "The rotator needs calibration" | after |

## Baseline evidence
Recorded 64/64 at 3.0.0.31. The hardware log shows the ALERT without a message.

## Bug reproduction
T-001 and T-002 fail with the previous messages.

## Negative cases and permission boundaries
`move_not_started` keeps the generic reason without the flag.

## Isolation and cleanup
One simulator per case. The server is stopped after the hardware check.

## Deferred tests and coverage limitations
Only Linux is run. The focuser case on hardware was not tried, because the focuser was not flagged.
