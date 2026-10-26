# Specification

Status: final

## Problem and outcome
Probe result on the ILCE-7RM4: a 10 s shot interrupted by a disconnect is delivered after the reconnect as the image of the next exposure (EXIF ExposureTime 10 for a 1/100 s exposure); ObjectInMemory is still 0 at the reconnect, so a drain before the press alone cannot catch it. PriorityMode is 0 at connect; the camera accepts 1. Outcome: a shot pending across a reconnect is discarded, ready leftovers are drained on every Sony body, PriorityMode is written.

## In scope
`indigo_ptp_sony.c/.h`, `indigo_ptp.h` (pending_shot_until, version 3.0.0.61), hardware case "Reconnect during exposure", fake cases.

## Out of scope
PriorityMode effect measurement; old-API behaviour beyond sharing the same accounting.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The image of a shot pressed before a disconnect is not published as an image of the next session; the next exposure delivers its own shot. | hardware `Reconnect during exposure` (EXIF < 1 s); fake `sony_shot_across_reconnect`, `_7rm4` |
| AC-002 | Every Sony exposure drains ready objects (ObjectInMemory > 0x8000) one at a time before the press. | fake `sony_old_api_leftover_drained` (now generic code path) |
| AC-003 | PriorityMode 0xD25A = 1 is written after SDIOConnect(3) where the body reports it. | fake `sony_priority_mode_written`; hardware trace (accepted, read back 1) |
| AC-004 | A pending shot expires, also when the capture timed out with an empty camera memory. | fake `sony_old_api_lost_shot_expires` |
| AC-005 | The complete hardware suite passes on the ILCE-7RM4; existing fake cases pass. | recorded runs |

## Failure behavior and edge cases
A failed PriorityMode write is logged at debug level and ignored. The hardware case is skipped where no writable 10 s shutter exists (stepping bodies, Canon/Nikon labels).

## Permissions, data, and security requirements
Camera replies untrusted; EXIF parsing in the test is bounds-checked.

## Compatibility and nonfunctional requirements
One GetAllDevicePropData per Sony exposure before the press (after the A7R4's post-connect wait).

## Assumptions and open questions
PriorityMode effect on ignored presses unmeasured (owner decision).

## Owner decisions and requirement changes
2026-10-25: owner chose "write it, no measuring" for PriorityMode.
