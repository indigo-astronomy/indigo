# Test and regression plan
Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002 | simulator `sestosenso3_calibration_lost_position` | `--profile sestosenso3`, `lostposition` fault on StoreAsMaxPos | ALERT with the rerun message; position 1000000 published; new START runs | before (fails) and after |
| T-002 | AC-003 | `python3 tools/run_driver_test.py focuser_primaluce` | full suite | all pass | final |

## Baseline evidence
Recorded 63/63 at 3.0.0.30. The session logs contain the hardware reading.

## Bug reproduction
T-001 fails without the check: X_CALIBRATE never turns ALERT.

## Negative cases and permission boundaries
Covered by T-001.

## Isolation and cleanup
One simulator per case.

## Deferred tests and coverage limitations
The reading cannot be provoked on hardware on demand, so there is no hardware check. Only Linux was
run.
