# Specification

Status: final

## Problem and outcome
After a reconnect the ILCE-7RM4 sometimes (a) acknowledges the full shutter press, refocuses and never shoots, so the exposure waits for the 60 s capture timeout, or (b) answers DeviceBusy to every control for the whole session and sends no events, so the exposure ends ALERT. Outcome: both are prevented or recovered, and a capture after a reconnect completes.

## In scope
`indigo_ptp_sony.c/.h` (shot confirmation and retry), `indigo_ccd_ptp.c` and `indigo_ptp.h` (Sony session reopen delay, version 3.0.0.59), fake USB cases.

## Out of scope
Other vendors' capture paths; physical hot-plug; the macOS ICA transport (no OpenSession there).

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A Sony NEW_API exposure whose full press was ignored (ObjectInMemory still 0 at shutter time + 3 s) presses the shutter again, at most twice, and completes with one image; if the camera never shoots it ends ALERT after the third press instead of waiting for the capture timeout. | fake `sony_ignored_press`; hardware reconnect loop (retry observed delivering the image) |
| AC-002 | A Sony session is not reopened within 1 s of its CloseSession. | fake `sony_fast_reopen`; hardware reconnect loop without DeviceBusy sessions |
| AC-003 | The complete hardware suite passes on the ILCE-7RM4, including `Reconnect`. | `python3 tools/run_driver_test.py ccd_ptp --hw` |
| AC-004 | Existing fake USB and ICA cases still pass. | `python3 tools/run_driver_test.py ccd_ptp` |

## Failure behavior and edge cases
Abort during a retry ends the exposure as before. BULB exposures and old-API bodies are unchanged. A camera that reports an object (ObjectInMemory not 0) or an ObjectAdded event is never pressed again. The reopen wait only applies to a reconnect of the same attached device and is at most 1 s.

## Permissions, data, and security requirements
No new inputs; camera replies remain untrusted and bounded as before.

## Compatibility and nonfunctional requirements
Linux libusb and macOS builds. Connection handler blocks at most 1 s longer on a fast Sony reconnect (Sony init already waits 1 s).

## Assumptions and open questions
The reopen delay is applied to every Sony body, measured only on the ILCE-7RM4; the cost is at most 1 s on a reconnect within 1 s.

## Owner decisions and requirement changes
2026-10-25: owner asked to fix the finding on the camera attached to the Pi.
