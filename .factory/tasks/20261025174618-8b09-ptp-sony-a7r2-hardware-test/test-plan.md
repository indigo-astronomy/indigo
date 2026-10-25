# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | fake `sony_liveview_after_capture` | old-API profile, live view denied 7 s after a download | 3 frames, streaming OK | negative control with the old driver |
| T-002 | AC-002 | fake `sony_old_api_aborted_shot` | aborted shot's object arrives during the next exposure | one image | negative control |
| T-003 | AC-002 | fake `sony_old_api_aborted_shot_idle` | aborted shot's object arrives while idle | no image, then one | after review |
| T-004 | AC-002 | fake `sony_old_api_lost_shot_expires` | ignored press, next exposure after expiry | own image kept | negative control without expiry |
| T-005 | AC-003, AC-004 | hardware `test_ccd_ptp_hw` via script | ILCE-7RM2 | 15/15 | final candidate |
| T-006 | AC-005 | recorded fake run, ASan/UBSan | macOS arm64 | all pass | final candidate |

## Baseline evidence
See plan.md and verification.md.

## Bug reproduction
Each hardware observation traced on the Pi (temporary transport trace) and modelled in the fake old-API profile.

## Negative cases and permission boundaries
Aborted shot during next exposure and while idle; lost shot expiry; live view denial longer than the old limit.

## Isolation and cleanup
Hardware suite private image directory and settings restore; Pi diagnostics removed.

## Deferred tests and coverage limitations
Polling-path stale discard (ObjectInMemory 0x8001 on old API) has no dedicated fake case; one unexplained Decoded JPEG timeout in an intermediate run not reproduced; hot-plug not run.
