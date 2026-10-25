# Specification

Status: final

## Problem and outcome
On the ILCE-7RM2 the hardware suite could not select values on stepping controls, live view right after a capture ended ALERT, and the image of an aborted timed shot was published as the next exposure's image. Outcome: all fixed or handled, recorded run passes.

## In scope
Sony old-API live view tolerance, stale-object handling, ignored-press retry, write confirmation and leftover drain in `indigo_ptp_sony.c/.h`, `indigo_ccd_ptp.c`, `indigo_ptp.h`, version 3.0.0.60; stepping support in the hardware suite; fake old-API profile and regressions.

## Out of scope
NEW_API behaviour (unchanged); other vendors; physical hot-plug.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Sony live view tolerates AccessDenied/DeviceBusy or frameless replies for up to 30 s without a frame, the window restarting after each frame. | fake `sony_liveview_after_capture`; hardware live view cases |
| AC-002 | On old-API Sony bodies the object of a shot whose exposure ended before its image arrived is downloaded and discarded, never published as another exposure's or a stream's image; a shot never reported expires 30 s after its expected end. | fake `sony_old_api_aborted_shot`, `sony_old_api_aborted_shot_idle`, `sony_old_api_lost_shot_expires`; hardware `Abort and reacquire`, `Finite live view` |
| AC-003 | The hardware suite selects, reads back and restores stepping controls. | hardware `Control readback`, image cases, restore at exit |
| AC-004 | The complete hardware suite passes on the ILCE-7RM2. | `python3 tools/run_driver_test.py ccd_ptp --hw` |
| AC-006 | A Sony full press the camera ignores is repeated (at most twice) on every Sony body, old API included. | fake `sony_old_api_ignored_press`; hardware (two retries in a traced loop) |
| AC-007 | A Sony SetControlDeviceA write keeps its property BUSY until the camera reports the value (old API: 15 s window, 5 s rewrite period); a stepping write is not confirmed. | fake `sony_old_api_late_setting_write`, `sony_old_api_dropped_setting_write`, `sony_old_api_step_not_rewritten` |
| AC-008 | An old-API exposure drains ready objects left in the camera before the press, one at a time. | fake `sony_old_api_leftover_drained` |
| AC-005 | Existing fake cases pass. | `python3 tools/run_driver_test.py ccd_ptp`, sanitizer targets |

## Failure behavior and edge cases
A camera-button shot while idle is still published unless one of the driver's own shots is pending. Live view on a camera that never delivers frames now ends ALERT after 30 s instead of about 6 s.

## Permissions, data, and security requirements
Camera replies untrusted; discarded objects are downloaded only to free the camera buffer and are not stored.

## Compatibility and nonfunctional requirements
Linux libusb and macOS; NEW_API paths unchanged apart from the live view window.

## Assumptions and open questions
None blocking.

## Owner decisions and requirement changes
2026-10-25: owner asked to test with the A7R II attached to the Pi.
