# Specification

Status: accepted

## Problem and outcome
`indigo_ccd_svb` 3.0.0.34 (INDIGO 3.0-8 and 3.0-9) opens the camera with `SVBOpenCamera()`, disables the SDK's
parameter auto-save, stops the video capture, selects the soft-trigger mode and enumerates the controls, but never
writes a control before the first exposure. The SVB SDK (v1.13.4, bundled unchanged since 3.0-7) refuses a
`SVB_GAIN` write after `SVBOpenCamera()` until the exposure has been written once: the 3.0-7 driver wrote
`SVB_EXPOSURE` = 1 s in its connect handler with the comment "fix for SDK gain error issue", the INDI driver does
the same, and an independent binding of the same SDK documents the refusal as verified on hardware. The migration
of the driver to the code generator (3.0-8) dropped that write. A `CCD_GAIN` request before the first exposure is
therefore refused (ALERT, value unchanged), including the gain restored from the saved configuration at connect,
which puts `CONFIG` into ALERT and makes an agent's device selection fail. `svb_set_number()` does not log the SDK
error codes, so the refusal leaves no trace at any log level.

Outcome: the driver writes the exposure once right after a successful connect so the SDK accepts gain writes before
the first exposure; a refused control write is logged with the SDK error codes; the fake SDK models the refusal so
the suite reproduces the defect and pins the fix; the hardware suite writes the gain before its first exposure.

## In scope
- `on_connect` of the CCD device in `indigo_drivers/ccd_svb/indigo_ccd_svb.driver`: after `initialize_camera()`
  succeeds, write `SVB_EXPOSURE` = 1 s (`SVBSetControlValue(id, SVB_EXPOSURE, 1000000, SVB_FALSE)`) under the
  master lock, with a comment naming the SDK behaviour; log a failed write at error level and continue (the next
  exposure writes the requested exposure anyway and a refused gain is then logged by `svb_set_number()`).
- `svb_set_number()`: log a refused write or a failed/out-of-range readback at error level with the property
  name, device id, control, requested value, both SDK results and the value read back; log a successful write at
  debug level.
- Driver version 34 -> 35; regenerated `.c`, `.h` and `_main.c`.
- Fake SDK (`indigo_test/integration/test_ccd_svb_sdk.c`): `SVBOpenCamera()` clears a per-camera
  "exposure written" state, `SVBSetControlValue(SVB_EXPOSURE)` sets it, and `SVBSetControlValue(SVB_GAIN)` returns
  `SVB_ERROR_GENERAL_ERROR` while it is clear, unless the fixture opts the camera out of the behaviour
  (`accepts_gain_before_exposure`, default off = the SDK v1.13.4 behaviour observed on the SV705C, SV905C2 and
  SV605CC).
- New integration case: connect, write the gain without any exposure, expect OK and the value in the SDK; expect
  the SDK's exposure to have been written by the connect.
- Hardware suite `indigo_test/hardware/test_ccd_svb_hw.c`: a gain write right after the first connect, before the
  first exposure of the run (run on the attached SV305PRO at the owner's request of 2026-11-07; the reported
  SV705C/SV905C2 are not available).
- `MIGRATION_STATUS.md`: automated-test count of the `ccd_svb` row.

## Out of scope
The empty `indigo.log` during a session on the user's Pi (depends on how the server is started there), the 3.0-7
short-exposure hang over USB 3, the agent's device-selection policy on a configuration restore in ALERT, the
generator, `ccd_svb2` (ToupTek-based SDK), hardware validation.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A `CCD_GAIN` request right after connecting, before any exposure, ends in OK with the requested value in the SDK and in the property | new case `gain_is_accepted_before_the_first_exposure` |
| AC-002 | The connect writes the SDK exposure once (the fake SDK's "exposure written" state is set after connect, before any exposure request) | same case |
| AC-003 | Against the unchanged driver the same case fails with `CCD_GAIN` in ALERT at the previous value (reproduction of the report) | the case run against the master driver before the fix |
| AC-004 | A refused control write is logged at error level with both SDK results; a successful one at debug level | code inspection of `svb_set_number()` (log output is not asserted by the suite) |
| AC-005 | Every other case of the suite passes unchanged | final recorded run `python3 tools/run_driver_test.py ccd_svb` |
| AC-006 | The driver version is 35 (`3.0.0.35`) | `.driver` source, regenerated `.c`, recorded test line |
| AC-007 | The hardware suite writes the gain before its first exposure, compiles and passes on the SV305PRO | `python3 tools/run_driver_test.py ccd_svb --hw` |

## Failure behavior and edge cases
- A failed exposure write at connect is logged and does not fail the connect; the gain request that follows is
  then refused by the SDK and logged by `svb_set_number()` with the SDK error code.
- The exposure written at connect (1 s) is the SDK's working value only until the first exposure request, which
  writes the requested duration as before (`svb_setup_exposure()` compares and writes).
- The write is made after the control enumeration, so the `CCD_EXPOSURE` property shows the SDK's pre-write value
  at connect (3.0-7 wrote before the enumeration and showed 1 s); the difference does not matter because
  `svb_setup_exposure()` compares the SDK value with the requested duration and writes it for every acquisition.
- The guider logical device shares the open session; the write happens once per CCD connect.

## Permissions, data, and security requirements
No new input; one SDK write of a constant and additional log lines without secrets.

## Compatibility and nonfunctional requirements
No property change. A saved configuration holding `CCD_GAIN` restores again; the renamed properties of 3.0-7
configurations (`PIXEL_FORMAT`, `SVB_ADVANCED`) are skipped by the restore as before. Linux, macOS and Windows
use the same portable C.

## Assumptions and open questions
Assumption: an exposure write of 1 s unlocks the gain on the SV705C and SV905C2 as it did in 3.0-7 on the user's
cameras and as INDI relies on; whether a write of the value the SDK already holds would do the same is unknown, so
the proven constant is used.

## Owner decisions and requirement changes
2026-11-07: the owner approved the proposed fix ("Áno, oprav") after the assessment of the user's report (chat), and
asked to test with the attached SV305PRO ("skus to otestovat aspon s nou").
