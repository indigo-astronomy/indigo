# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | hardware "Reconnect during exposure" | ILCE-7RM4, 10 s shot interrupted | next image EXIF < 1 s | recorded run |
| T-002 | AC-001 | fake `sony_shot_across_reconnect`, `_7rm4` | object held across reconnect | one image | negative control: record reset on reconnect |
| T-003 | AC-003 | fake `sony_priority_mode_written` | body reports 0xD25A | value 1 | negative control: write removed |
| T-004 | AC-004 | fake `sony_old_api_lost_shot_expires` | aborted ignored press, wait past expiry | own image, 2 presses | negative control: no expiry |

## Baseline evidence
Probe on 3.0.0.60: stale 10 s image returned (verification.md).

## Bug reproduction
Probe with EXIF readback on the Pi (not committed).

## Negative cases and permission boundaries
Expiry, skip conditions of the hardware case.

## Isolation and cleanup
Probe binary, tracing and images removed from the Pi.

## Deferred tests and coverage limitations
PriorityMode effect on ignored presses not measured; fake object model uses events, the NEW_API polling path is covered by the shared handler and the hardware case.
