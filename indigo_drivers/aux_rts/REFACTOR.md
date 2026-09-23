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

## Validation Evidence of the Simulator-Only Work

- `indigo_generator indigo_aux_rts.driver` run twice leaves the generated `.c`, `.h` and `_main.c` unchanged.
- Strict build: `make -B build/integration/test_aux_rts_shutter CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` passed.
- `build/integration/test_aux_rts_shutter` passed 9 of 9 cases. Against version 9, built separately from `git show HEAD:...` without touching the working tree, `exposure_countdown_is_published_in_whole_seconds` and `a_stalled_queue_does_not_extend_the_exposure` fail and the other seven pass.
- `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/integration/test_aux_rts_shutter_asan` passed all 9 cases with the production driver source instrumented and no sanitizer report. LeakSanitizer is unavailable on this macOS runtime and is not claimed.
- `build/integration/test_aux_geoptikflat_transport` and `build/integration/test_guider_cgusbst4_transport` pass after the shared source lost its `TEST_KIND == 3` branches.

## Hardware-Test Decision

No RTS-on-COM shutter release and no camera are available, so the device itself remains untested
and `README.md` still records it as such. The driver's entire device interaction is, however,
`indigo_uni_open_serial()`, `indigo_uni_set_rts()` and `indigo_uni_close()`, and a second serial
port wired to the first one stands in for that interaction exactly. A physical serial loopback run
was therefore decided on and performed on 2026-09-23.

- Host: `indigosky`, Raspberry Pi 5, Linux arm64, booted from `/dev/sda2`.
- Driver side: FTDI FT232R, `usb-FTDI_FT232R_USB_UART_A91S25CS-if00-port0`, USB `1-2` on `xhci-hcd.0`.
- Monitor side: FTDI US232R, `usb-FTDI_US232R_FTDFZ2FW-if00-port0`, USB `3-2` on `xhci-hcd.1`.
- Cable: null modem. Measured mapping: `RTS -> DCD`, `DTR -> CTS + DSR`, symmetric in both
  directions. It is not the full-handshake `RTS -> CTS` wiring, which is why
  `hardware/test_aux_rts_hw.c` works the mapping out before the driver owns the port rather than
  assuming one, and why the monitoring side reads the line with a raw `TIOCMGET`: `indigo_uni_io`
  exposes `indigo_uni_get_cts()` only and cannot see a line the cable delivers as DCD. That is test
  code on the observing side of the cable; the driver under test uses the portable API throughout.
- Unplug coverage was driven by disabling the USB port the driver's adapter sits on. The port is
  derived from the adapter the driver actually drives and refuses to run if it resolves to the port
  carrying the root filesystem.

**What this run does and does not establish.** It is a real electrical measurement of the line the
shutter contact hangs on, taken on a clock that is not the driver's own, which the hardware-free
suite structurally cannot do: `integration/test_aux_rts_shutter.c` replaces the three transport
calls at compile time and therefore sees only the calls the driver makes. It is **not** a test of a
shutter release or of a camera. Nothing is wired to the adapter, so shutter release behaviour,
contact current, opto-isolator timing and camera response remain uncovered.

## Defect Found and Fixed by the Loopback Run (driver version 10 -> 11)

**Connecting opened the shutter and left it open.** `open()` on a tty leaves `TIOCM_RTS` asserted
and `indigo_uni_io.c` never clears it, while the driver lowered RTS only on disconnect, on abort
and at the end of an exposure. Connecting therefore closed the shutter contact immediately and held
it closed until the first exposure ended, so the first frame after connecting started early and ran
long, and a session that connected without exposing left the camera exposing indefinitely.

The first loopback run reported it directly: `the line is HIGH - the shutter is open after
connecting`, followed by `0.300 s requested: line HIGH before the request, no rise/fall pair seen`.

Fix: `rts_open()` lowers the line after opening the port. A port whose line cannot be lowered
cannot serve a shutter, so that failure fails the open and releases the handle, which keeps the
transactional open contract of `indigo_drivers/AGENTS.override.md`.

**The hardware-free suite had encoded the defect as correct behaviour.** Its fake
`indigo_uni_open_serial()` left the line alone, and
`identity_inventory_and_property_contract` asserted `0` transitions after connecting under the
comment "Connecting arms nothing and leaves the line alone". The fake now asserts the line on open,
the way a tty does, and the case asserts the rising and falling edge the connection is expected to
produce. `connecting_closes_the_shutter` is the regression test for the defect itself.

Against driver version 10, built from `git show HEAD:...` without touching the working tree, the
corrected suite fails 8 of its 10 cases: with the line already high at connect, almost every
exposure loses its rising edge. Against version 11 all 10 pass.

## Observations Recorded, Not Fixed

- **`close()` does not lower the line.** `configure_tty_options()` leaves `HUPCL` clear, so closing
  the port keeps RTS wherever it was. The driver lowers it on every path it controls, so this is
  reachable only if the process dies mid-exposure, and it would latch the shutter open until the
  port is opened again. Changing the shared termios setup affects every serial driver - some
  devices are powered from DTR or RTS - so it is recorded here rather than changed.
- **An unplugged adapter is discovered only at the exposure deadline.** A shutter release has no
  readback of any kind, so the driver has nothing to poll and cannot learn that the port is gone
  until it next drives the line. It then reports `ALERT` instead of publishing a completed
  exposure, which is the correct outcome for a device that cannot be interrogated.
  `rts_survives_the_adapter_unplug` pins this down.
- **The connection leaves a pulse of about 1 ms on the line**, between the kernel asserting RTS on
  open and the driver lowering it. Measured repeatedly as 1.0 ms. The suite bounds it at 50 ms.
  Whether a given shutter release reacts to a pulse that short is a property of the device, which
  this run cannot establish.

## Test-Side Defect Found During the Run

The first passing hardware run intermittently reported `line HIGH before the request` for exposures
started immediately after a connect, while still measuring the correct duration. It was a race in
the suite, not in the driver: the falling edge of the 1 ms connection pulse landed in the exposure's
own edge log, about a millisecond after the level was sampled. `monitor_settle()` now waits for the
line to stop changing before a request is timed. It never asserts, so a line that is steadily high -
the defect above - is still reported as high rather than being waited out. Three consecutive full
runs are clean since.

## Hardware Scenario-to-Test Mapping

| Required behavior | Automated scenario |
| --- | --- |
| Driver metadata, API generation, interface bit | `rts_reports_identity_and_capabilities` |
| Published property contract, both properties and items, port properties unhidden | `rts_publishes_the_property_contract` |
| Connecting leaves the shutter closed, the connection pulse is bounded, and the first exposure after connecting is not lengthened | `rts_leaves_the_shutter_closed_on_connect` |
| The line is electrically held for the requested time at 0.3 s, 1 s and 2.5 s | `rts_holds_the_line_for_the_requested_time` |
| The published countdown is whole seconds after the echoed request, never increases, ends at zero | `rts_publishes_a_whole_second_countdown` |
| Abort lowers the line at once, reports the exposure as failed, clears the remaining time, and the device stays usable | `rts_aborts_and_closes_the_shutter` |
| Disconnecting mid-exposure releases the line and withdraws the properties; reconnect republishes them | `rts_releases_the_line_on_disconnect` |
| Repeated disconnect, reconnect, fresh exposure | `rts_reconnects` |
| The adapter is pulled mid-exposure: the loss is reported at the deadline, teardown does not hang, the driver works again after replug | `rts_survives_the_adapter_unplug` |

### Not Covered by the Loopback

- Real shutter release behaviour, contact current, opto-isolator response and camera reaction. No
  shutter release or camera is available.
- Windows runtime. `indigo_uni_set_rts()` has a separate Windows implementation that this run does
  not reach.
- Whether a real shutter release reacts to the ~1 ms connection pulse.

## Loopback Run Validation Evidence

- `indigo_generator indigo_aux_rts.driver` run twice leaves the generated `.c`, `.h` and `_main.c`
  unchanged; `DRIVER_VERSION` is `0x0300000B`.
- Strict build: `make -B build/hardware/test_aux_rts_hw CC='gcc -Wall -Wextra -Werror
  -Wno-unused-function -Wno-unused-parameter'` passed, as did the same build of
  `build/integration/test_aux_rts_shutter`.
- `build/hardware/test_aux_rts_hw --run --hotplug` passed 9 of 9 cases on five consecutive full
  runs since the settle fix below, the last of them at 2026-09-23 19:22 against the sources as
  committed. Measured hold error was at most 1 ms on every exposure, the connection pulse measured
  1.0 ms every time, and the abort closed the shutter 1 ms after it was requested.
- `build/integration/test_aux_rts_shutter` passed 10 of 10, last at 2026-09-23 19:19.
- `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1
  build/integration/test_aux_rts_shutter_asan` passed all 10 cases with no AddressSanitizer or
  UndefinedBehaviorSanitizer report. LeakSanitizer reports 36 allocations leaked inside
  `indigo_start()`; the same build of the HEAD driver and HEAD suite leaks 32, and the difference is
  exactly the four allocations one additional `indigo_start()` leaks. The leaks are framework-owned
  and predate this change.
- Against driver version 10: the corrected hardware-free suite fails 8 of 10 cases, including
  `connecting_closes_the_shutter`.

## Final Test Summary

- Simulated tests: **10 run, 10 passed**, plus the same 10 under ASan/UBSan.
- Hardware tests: **9 run, 9 passed**, against a physical serial loopback of two FTDI adapters. No
  RTS-on-COM shutter release or camera was involved; see the hardware-test decision above for what
  that does and does not establish.
