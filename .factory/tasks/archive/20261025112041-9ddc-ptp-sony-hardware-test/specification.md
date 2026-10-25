# Specification

Status: final

## Problem and outcome
The driver passes a complete physical acceptance run on the Sony ILCE-7RM4, and every defect that run exposes is fixed and reproduced hardware-free.

## In scope
`indigo_drivers/ccd_ptp/indigo_ccd_ptp.c`, `indigo_ptp.h`, `indigo_ptp_sony.[ch]`; `indigo_test/hardware/test_ccd_ptp_hw.c`, `indigo_test/Makefile` (`test-ccd-ptp-hw`), `indigo_test/integration/test_ccd_ptp_sdk.c`; version 3.0.0.57.

## Out of scope
Physical hot-plug; other bodies and vendors on hardware.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | INFO shows the camera model, firmware and serial number after connecting. | hardware `Discovery and identity`; fake `device_identity_published` |
| AC-002 | An exposure right after a (re)connection succeeds although the camera answers DeviceBusy for a few seconds; a camera that stays busy still ends ALERT. | hardware `Reconnect`; fake `sony_busy_after_connect` |
| AC-003 | A camera setting goes OK only once the camera reports the requested value; a dropped write is rewritten, one never applied ends ALERT showing the camera value. | hardware `Image sizes`, `RAW+JPEG with preview`, `Bulb exposures`, `Lens focuser`; fake `sony_dropped_setting_write` |
| AC-004 | The complete hardware suite passes and restores the camera settings. | `python3 tools/run_driver_test.py ccd_ptp --hw` 15/15 |
| AC-005 | Existing fake USB and ICA cases still pass. | recorded fake run, sanitizer run |

## Failure behavior and edge cases
Verification applies to Sony NEW_API switch properties whose items are named by their hexadecimal camera value; iterate items (`+`/`-`) and string properties complete as before.

## Permissions, data, and security requirements
Camera replies remain untrusted; images stay in a private test directory; no card deletion.

## Compatibility and nonfunctional requirements
Other vendors and Sony old API keep their previous property path. Linux and macOS builds.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: non-interactive hardware run without hot-plug; verify dropped writes in the driver (after the libgphoto2 check).
