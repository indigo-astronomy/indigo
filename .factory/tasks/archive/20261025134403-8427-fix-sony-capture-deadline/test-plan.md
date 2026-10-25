# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | fake USB `sony_ignored_press` (`indigo_test/integration/test_ccd_ptp_sdk.c`) | camera ignores 1, then 10 presses | one retry then one image; ALERT after 3 presses; next exposure OK | after fix; negative control without fix |
| T-002 | AC-001 | fake USB `sony_long_exposure_noise_reduction` | 1 s camera shutter, object 1.5 s after the press | no second press, one image | after review; negative control with undoubled window |
| T-003 | AC-002 | fake USB `sony_fast_reopen` | camera stuck busy when reopened within 0.5 s | two reconnects, exposure OK, never stuck | after fix; negative control with delay 0 |
| T-004 | AC-003 | hardware `test_ccd_ptp_hw` via `tools/run_driver_test.py ccd_ptp --hw` | ILCE-7RM4 on the Pi | 15/15 | final candidate |
| T-005 | AC-001, AC-002 | Pi-only diagnostic loop (discovery + N reconnect/expose), not committed | ILCE-7RM4 | all iterations pass | diagnosis and final candidate |
| T-006 | AC-004 | `tools/run_driver_test.py ccd_ptp`, ASan/UBSan targets | macOS arm64 | all pass | final candidate |

## Baseline evidence
Diagnostic loop on 3.0.0.58: failed in 6/6 runs (see verification.md).

## Bug reproduction
Both modes reproduced on hardware with PTP traces; both modelled in the fake as selectable quirks.

## Negative cases and permission boundaries
Ignored press exhausted (ALERT), LENR delay (no retry), busy session (fake), abort/timeout/reconnect cases unchanged.

## Isolation and cleanup
Hardware suite uses a private image directory and restores settings; diagnostic binary, logs and tracing lived only on the Pi under /tmp and were removed.

## Deferred tests and coverage limitations
Physical hot-plug not run; LENR not exercised on hardware (no long timed exposure in the suite); other Sony bodies not tested.
