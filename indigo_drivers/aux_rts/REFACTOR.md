# RTS-on-COM Shutter Coverage Completion

## Current-State Audit

- Audit baseline: branch `refactoring` at commit `9c3dca168`, macOS Darwin 25.6.0, universal arm64/x86_64 artifacts, arm64 execution.
- `indigo_aux_rts.driver` is the authoritative generator input; `indigo_aux_rts.c`, `.h` and `_main.c` are checked-in generated outputs. The driver is API 3 and exposes `INDIGO_INTERFACE_AUX_SHUTTER` with runtime additional instances, `CCD_EXPOSURE` and `CCD_ABORT_EXPOSURE`.
- There is no protocol. The device is a camera shutter release wired to the RTS line of a serial port, so the whole device interaction is `indigo_uni_open_serial()`, `indigo_uni_set_rts()` and `indigo_uni_close()`, and the exposure is timed by the driver's own callback.
- Before this work the driver had **no test file of its own**. Its only coverage was `integration/test_serial_outputs.c`, a source shared with `guider_cgusbst4` and `aux_geoptikflat` and parameterized by `TEST_KIND`, which could assert that the line went up and down but never how long it stayed up.

## Defect Found and Fixed (driver version 9 -> 10)

**The exposure was counted in callbacks instead of being timed.** The timer subtracted one from
`CCD_EXPOSURE_ITEM->number.value` on every tick and rescheduled itself from the value it had just
written, so the published property was both the countdown display and the timing state. Two
consequences, both reproduced against version 9 by the new suite:

- The published countdown was the raw fractional remainder. A 2.5 s exposure published
  `2.5, 1.5, 0.5, 0` instead of whole seconds. `AGENTS.md` requires the published countdown to be
  rounded up with `ceil()` and clamped at zero.
- Any callback that ran late added its lateness to the exposure, because a late tick still
  subtracted only one second. With the device queue occupied for 1.5 s, a 3 s exposure held the
  shutter for **3.518 s**; after the fix the same case holds it for **3.005 s**.

The fix keeps the deadline in monotonic time in the driver's private data and publishes
`ceil(time_left)`, which is exactly how `aux_dsusb` - the other AUX shutter in the tree - times its
exposure. The shared countdown of `indigo_ccd_driver.c` is not an option here: it lives in
`CCD_CONTEXT`, which is `(indigo_ccd_context *)device->device_context`, and `indigo_aux_attach()`
allocates only `sizeof(indigo_device_context)`; `indigo_ccd_exposure_setup()` also touches
`CCD_IMAGE`, `CCD_IMAGE_FILE` and `CCD_UPLOAD_MODE`, none of which an AUX device has. An AUX
shutter therefore has to own its countdown.

An abort now also clears the deadline and zeroes the published remaining time, so a cancelled
exposure does not leave "twelve seconds left" behind it.

## Test Work

- Added `indigo_test/integration/test_aux_rts_shutter.c`, the driver's own test. It replaces
  `indigo_uni_open_serial`, `indigo_uni_close` and `indigo_uni_set_rts` at compile time and records
  every RTS transition with a monotonic timestamp, which is the only place the length of an exposure
  can be observed. A PTY simulator cannot serve this driver at all: a pseudo terminal has no modem
  control lines.
- Removed `TEST_KIND == 3` from `integration/test_serial_outputs.c` together with the
  `test_aux_rts_transport` rules. Everything it covered for this driver is now in the dedicated
  file, and the shared source is left to the two drivers that exchange lines of text. `TEST_KIND 1`
  (`guider_cgusbst4`) and `TEST_KIND 2` (`aux_geoptikflat`) were rerun and pass unchanged.

## Scenario-to-Test Mapping

| Required behavior | Automated scenario |
| --- | --- |
| Driver metadata, API generation, interface bit, base inventory before connect, hidden properties, exact property counts, both property shapes; connecting leaves the line alone | `identity_inventory_and_property_contract` |
| The line is held for the requested time, whole seconds or not, at 0.3 s, 1 s and 2.5 s | `exposure_holds_the_shutter_for_the_requested_time` |
| The published countdown is whole seconds, never increases and ends at zero | `exposure_countdown_is_published_in_whole_seconds` |
| A callback that runs late does not lengthen the exposure | `a_stalled_queue_does_not_extend_the_exposure` |
| Abort lowers the line at once, reports the exposure as failed, clears the remaining time and does not reopen the shutter afterwards | `abort_closes_the_shutter` |
| Abort accepted while the exposure request is still queued, and an abort carrying a false value | `queued_abort_and_false_abort` |
| Failure to raise the line, to lower it at the end and to lower it on abort, each reported on the property that asked for it, with the driver usable afterwards | `line_failures_are_reported` |
| Disconnecting mid-exposure releases the line instead of latching the shutter open | `disconnect_during_exposure_releases_the_line` |
| Failed open with rollback, serial handle open/close balance, repeated disconnect, `INDIGO_DRIVER_SHUTDOWN` refused while connected | `open_rollback_reconnect_and_refused_shutdown` |

### Non-applicable and Deferred Coverage

- There is nothing to read back, no identity, no protocol and no persistent setting, so handshake, reply parsing and configuration roundtrip coverage do not apply.
- **Deferred, affects `aux_dsusb` too.** Both AUX shutters duplicate the same countdown. If a generic countdown for non-CCD devices is ever added to the framework, both should move onto it.
- Real shutter release behavior, real RTS electrical levels and Windows runtime require environments not available for this work.

## Hardware-Test Decision

No physical RTS-on-COM shutter release is available, and `README.md` records the driver as untested against hardware. No hardware run was performed.

## Validation Evidence

- `indigo_generator indigo_aux_rts.driver` run twice leaves the generated `.c`, `.h` and `_main.c` unchanged.
- Strict build: `make -B build/integration/test_aux_rts_shutter CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` passed.
- `build/integration/test_aux_rts_shutter` passed 9 of 9 cases. Against version 9, built separately from `git show HEAD:...` without touching the working tree, `exposure_countdown_is_published_in_whole_seconds` and `a_stalled_queue_does_not_extend_the_exposure` fail and the other seven pass.
- `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/integration/test_aux_rts_shutter_asan` passed all 9 cases with the production driver source instrumented and no sanitizer report. LeakSanitizer is unavailable on this macOS runtime and is not claimed.
- `build/integration/test_aux_geoptikflat_transport` and `build/integration/test_guider_cgusbst4_transport` pass after the shared source lost its `TEST_KIND == 3` branches.

## Final Test Summary

- Simulated tests: **9 run, 9 passed**, plus the same 9 under ASan/UBSan.
- Hardware tests: **0 run, 0 passed**; no compatible physical device is available.
