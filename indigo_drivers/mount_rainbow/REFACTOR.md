# mount_rainbow refactoring record

## Scope and baseline

This record covers characterization and migration of the hand-written `indigo_mount_rainbow` driver to `indigo_generator`, a manufacturer-protocol audit of the host-side simulator, complete applicable simulator-backed mount coverage, and preservation of observable protocol ordering and behavior.

Baseline date and source: 2026-09-16, current working tree. The repository already contains unrelated in-progress `focuser_dsd` work and edits in `indigo_test/Makefile` and `indigo.xcodeproj/project.pbxproj`; this migration must preserve those edits and add only isolated RainbowAstro entries.

Baseline commands and results:

```sh
cd indigo_drivers/mount_rainbow
make -B -f ../../Makefile.drv

make -C indigo_test build/integration/test_mount_rainbow_simulator
cd indigo_test
build/integration/test_mount_rainbow_simulator
```

The universal x86_64/arm64 driver build passed without warnings. The simulator and test built successfully. An initial invocation from the repository root failed because the test's simulator executable path is relative to `indigo_test`; this was an invocation error, not a driver failure. Re-running from `indigo_test` passed the one registered case, 1 / 1. Teardown emitted `Failed to wait for (Bad file descriptor)`, which is retained as baseline evidence of the current close-before-reader-cancel ordering and requires a dedicated regression.

## Current-state audit

### Architecture and lifecycle

- The driver exposes one mount logical device, supports additional runtime instances, and connects either to a 115200-baud serial port or a `rainbow://` TCP endpoint using the portable unified-I/O layer.
- A dedicated reader timer continuously consumes `#`-terminated asynchronous responses. A separate one-second position timer writes compound polling sequences for RA/DEC/slew state, time/location, tracking and tracking rate.
- Property changes use zero-delay timers and a private port mutex. `rainbow_sync_command()` writes a request and polls a public property state for up to one second while the reader changes that state.
- Connection probes with `:AV#`, starts the reader, then performs an ordered initialization sequence. Disconnect closes the transport before synchronously cancelling the reader and position timers; teardown ordering and transport-loss behavior require characterization.
- Long-running slew and park completion arrive asynchronously as `:MM0#` and `:CHO#`. Manual motion uses firmware-dependent axis stop commands. The current implementation is not based on INDIGO 3.0 handler queues.

### Published capabilities and behavior

- The inherited mount property set is used; no driver-specific custom property is defined. The driver unhides serial ports, host-time and UTC-time properties; restricts coordinate-set modes to TRACK/SYNC, guide rate to RA only, and park to PARKED only.
- Supported operations are identity/firmware, coordinate polling, GOTO, SYNC, park/home command, manual RA/DEC motion at four rates, abort, tracking on/off, sidereal/solar/lunar/custom tracking rates, guide-rate read/write, geographic coordinates, UTC/time-zone read/write and host-time synchronization.
- Firmware `200625` gates date/time-zone queries and independent RA/DEC manual-motion stop commands. Older firmware uses local host date context, time-only polling and global `:Q#` stops.
- Coordinate commands convert between J2000 and device epoch. Longitude translation follows the mount protocol's EAST-negative/WEST-positive convention.
- Driver version is `0x0200000F`; any behavior fix or completed generated migration must increment it.

### Manufacturer protocol and simulator audit

- Manufacturer PDFs `RainbowAstro_191217.pdf` and `RainbowAstro_200629.pdf` specify case-sensitive 115200-baud commands, response formats, motion/stop semantics, guide-rate range, tracking states/rates, home outcomes, location signs and the firmware-200625 additions for date, time-zone and independent axis stops.
- The current host simulator implements the happy-path command vocabulary used by the driver, but immediately jumps to slew and park destinations, does not use `serial_motion.h`, provides no deterministic elapsed-time movement, exposes no firmware selector or fault injection, and cannot model partial/malformed/delayed replies, command rejection, transport loss, failed slew or failed homing.
- The simulator parser accepts concatenated `:`-prefixed commands and ACK byte 0x06, but protocol correctness, command casing, command/reply ordering and all failure branches need explicit tests against the manufacturer tables.

### Existing tests and gaps

- `indigo_test/integration/test_mount_rainbow_simulator.c` registers one smoke/compliance case. It checks a subset of property items and exercises guide rate, tracking, solar rate and idle abort only.
- Missing coverage includes complete property visibility, both firmware branches, initial value parsing, exact initialization/poll trace, GOTO BUSY/progress/completion/failure, SYNC without slew, all manual directions/rates/stops/reversal/simultaneous axes, abort during slew/manual motion, park success/failure and parked guards, all tracking rates, location/time conversions, malformed/partial replies, write/read failures, disconnect/reconnect, transport loss, concurrent requests and queue/teardown races.
- There is no retained normalized original-driver reference trace and no sanitizer/strict test result.

## Hardware-test decision

No compatible RainbowAstro mount is available. No hardware test will be performed, registered or claimed. Simulator/fake-transport evidence will be reported separately and must not be described as physical mount validation.

## Atomic plan

1. **Done - instructions, documentation, audit and baseline.** Read repository, driver and test rules; inspected the driver, simulator, existing test, build/project integration, generated-driver guide, serial-simulator contract and both manufacturer protocol PDFs. The driver build passed and the original one-case simulator test passed from its required working directory; the teardown bad-file-descriptor diagnostic is recorded above.
2. **Done - protocol-complete simulator foundation.** Audited commands against both protocol revisions. The simulator now has firmware selection, deterministic event traces, one-shot/sticky fault injection, split/delayed/malformed/drop/close actions, elapsed-time GOTO/park/manual motion through `serial_motion.h`, unsolicited completion replies and a retained PTY endpoint for reconnect tests.
3. **Done - original-driver property and lifecycle coverage.** Expanded the suite through public INDIGO APIs for property inventory, identity, firmware branches, initialization order, location/time, tracking/rates, guide rate, disconnect and reconnect.
4. **Done - original-driver motion coverage.** Covered GOTO, SYNC, BUSY/completion, J2000 command values, replacement while BUSY, every manual axis/rate combination, independent stops, abort, park success/failure and the parked-motion guard.
5. **Done - original-driver transport and concurrency coverage.** Added split, malformed and dropped polling replies, invalid device identity, write failure after active transport loss, reconnect and overlapping GOTO coverage with bounded waits.
6. **Done - original reference trace.** Retained `indigo_test/fixtures/mount_rainbow/original_reference_trace.txt`; the suite compares the ordered initialization protocol against it, including compound-command ordering.
7. **Done - generated source design.** `indigo_mount_rainbow.driver` is the source of truth. No generator implementation change or driver-local capacity override was made.
8. **Done - generate, build and compare.** Regenerated `.c`, `.h` and `_main.c`, incremented the driver to version 16, built the universal driver and ran the complete trace-backed suite after behavior changes.
9. **Done - generated lifecycle and async audit.** Writes run on the generated device handler queue; abort is urgent; GOTO and park use bounded finalizers; the asynchronous reader has explicit lifetime state and is stopped before transport close; polling and pending handlers are cancelled on disconnect.
10. **Done - repository integration.** Registered the `.driver`, refactor record and retained trace in project files, added the `.driver` to the Windows project, updated the simulator dependency and changed only the `mount_rainbow` status columns in `MIGRATION_STATUS.md`. The property surface did not add or remove names, so `PROPERTIES.md` required no change.
11. **Done - final verification.** Deterministic regeneration, normal and strict builds, the complete simulator suite, an ASan+UBSan suite, Xcode plist and Visual Studio XML validation, `git diff --check` and process cleanup all pass.

## Scenario-to-test mapping

| Standard area | Test coverage |
| --- | --- |
| Identity and coordinates | `rainbow_driver_info_and_property_inventory`, `rainbow_preserves_modern_initialization_order`, `rainbow_uses_legacy_firmware_protocol`, `rainbow_recovers_from_faulted_poll_replies`, `rainbow_rejects_non_rainbow_transport` |
| GOTO and SYNC | `rainbow_goto_sync_and_abort_have_distinct_protocols` covers exact J2000 RA/DEC commands, BUSY, completion, SYNC without `MS`, BUSY replacement rejection and abort/recovery. |
| Manual motion and abort | `rainbow_manual_motion_covers_all_rates_axes_and_stops` covers four rates, four directions and firmware-specific axis stops; GOTO test covers abort during slew. |
| Tracking and rates | `rainbow_tracks_and_selects_every_rate` covers on/off, immediate sidereal/solar/lunar rate commands, preservation of the rate-before-GOTO sequence and guide-rate units. Custom rate is not applicable because the driver publishes three rate items and no custom-rate property. |
| Park and home | `rainbow_park_reports_success_and_failure` covers BUSY/success, failure, parked state and parked-motion rejection. RainbowAstro `Ch` is exposed as INDIGO park; no separate home/unpark/set-position capability is published. |
| Time/location and options | `rainbow_writes_location_and_time` covers signed latitude/longitude, exact requested UTC offset conversion, invalid UTC rejection and host-time write. The firmware-191217 test covers the legacy time-only branch. |
| Polling and transport loss | `rainbow_recovers_from_faulted_poll_replies`, `rainbow_reconnects_after_clean_disconnect`, and `rainbow_handles_active_transport_loss` cover split/malformed/drop recovery, stale-input discard, reconnect, write failure and cancellation without closed-handle access. |

Guider pulse timing is not applicable: this driver exposes only the mount guide-rate setting and no guider logical device or pulse properties. Hardware acceptance is deferred because no mount is available.

## Found defects

- The original disconnect closed the transport before stopping the reader, producing a reproducible bad-file-descriptor diagnostic and making reconnect unsafe. Fixed with explicit reader lifetime state, cancellation before close and stale-input discard on open; `rainbow_reconnects_after_clean_disconnect` passes without the diagnostic.
- The original park completion did not set `MOUNT_PARK.PARKED`, so framework parked guards could not work. Fixed and covered by `rainbow_park_reports_success_and_failure`.
- Changing `MOUNT_TRACK_RATE` did not send a mount command until the next GOTO. By explicit user request, the change now sends `CtR`, `CtS` or `CtM` immediately while GOTO retains the defensive rate-before-slew ordering.
- UTC writes ignored the requested `UTC_TIME.OFFSET` and used the process time zone. Firmware 200625+ now converts UTC to mount-local time using the requested offset and sends the protocol's opposite-sign `SG` value; old firmware rejects unsupported writes.
- Epoch behavior is now explicit: `MOUNT_EPOCH` is read-only J2000 and exact J2000 GOTO/SYNC command values are tested.
- A stale `CL0` poll could mark a newly started GOTO complete before `MM0`. The driver now tracks an active GOTO and accepts `MM0` as its completion event; replacement requests remain blocked while BUSY.
- The old unreachable custom-rate branch was removed from behavior: the published property contains only sidereal, solar and lunar items, matching the tested public surface.

## Test evidence

- Original-driver build: passed, universal x86_64/arm64.
- Original simulator suite baseline: 1 / 1 passed; teardown emitted a bad-file-descriptor diagnostic.
- Characterization suite and normalized trace were established before changing the production driver. Confirmed original defects were kept as failing reproducers until the generated implementation fixed them.
- Generated driver build: passed for the local universal x86_64/arm64 target. Windows project integration is updated but Windows compilation is unverified in this environment.
- Generated simulator suite: 12 / 12 passed.
- Strict compile: generated driver and simulator pass `-Wall -Wextra -Werror`; the integration test passes with the common harness's known unused static helpers/parameters excluded from warning-as-error treatment.
- Sanitizers: 12 / 12 passed with the generated driver and test compiled under AddressSanitizer and UndefinedBehaviorSanitizer. The expected injected transport-loss case logs one failed write and completes cleanly.
- Regeneration: SHA-1 checksums of generated `.c`, `.h` and `_main.c` are unchanged after a fresh generator run.
- Project/whitespace validation: Xcode project plist, both Visual Studio XML files and `git diff --check` pass.
- Hardware: 0 / 0, explicitly unavailable.

## Final test summary

- Simulated tests run/passed: 12 / 12.
- Hardware tests run/passed: 0 / 0; no compatible hardware is available.
