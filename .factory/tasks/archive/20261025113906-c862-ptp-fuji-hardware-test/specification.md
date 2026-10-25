# Specification

Status: final

## Problem and outcome
The driver passes the physical acceptance suite on the Fujifilm X-T1, and every defect that run exposes is fixed and reproduced hardware-free.

## In scope
`indigo_drivers/ccd_ptp/indigo_ccd_ptp.c`, `indigo_ptp.h`, `indigo_ptp_fuji.c`; `indigo_test/hardware/test_ccd_ptp_hw.c`, `indigo_test/integration/test_ccd_ptp_sdk.c`; version 3.0.0.58.

## Out of scope
Physical hot-plug; other bodies on hardware; USB BULB on bodies that do not declare it.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | DSLR_SHUTTER offers Bulb on a Fujifilm body only when the body declares USB BULB start/stop, or while the body itself reports Bulb. | hardware `Bulb exposures` (skipped as not offered); fake `fuji_bulb_not_offered` |
| AC-002 | After a rejected property write the property is ALERT and shows the value the camera holds; the driver uses that value. | fake `rejected_setting_readback` |
| AC-003 | A USB device without a still image interface from a camera vendor is not attached as a camera. | hardware discovery (charging ILCE-6000 not listed); fake `sony_charger_not_attached` |
| AC-004 | The complete hardware suite passes on the X-T1 and restores the camera settings. | `python3 tools/run_driver_test.py ccd_ptp --hw` 15/15 |
| AC-005 | Existing fake USB and ICA cases still pass. | recorded fake run, sanitizer run |

## Failure behavior and edge cases
A body reporting Bulb (shutter dial on B, camera priority) keeps the Bulb item; a BULB exposure there is still refused with the existing message. Readback after a rejected write is skipped when disconnected or detaching; a failed readback leaves the previous behaviour.

## Permissions, data, and security requirements
Camera replies remain untrusted; images stay in a private test directory; no card deletion.

## Compatibility and nonfunctional requirements
Other vendors keep their property path; the readback uses the vendor refresh callback or GetDevicePropDesc when the camera supports it. Linux and macOS builds.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-25: non-interactive hardware run without hot-plug; the shutter dial is on B (owner, mid-run); the owner switched the lens to MF after intermittent AF-S failures.
