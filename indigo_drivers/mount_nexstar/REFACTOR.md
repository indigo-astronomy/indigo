# Mount NexStar Refactor

## Current Code Findings

- The driver was still hand-written INDIGO 2.x style code in `indigo_mount_nexstar.c`; the source of truth is now `indigo_mount_nexstar.driver` and generated `indigo_mount_nexstar.c`, `indigo_mount_nexstar.h` and `indigo_mount_nexstar_main.c` must stay synchronized.
- The old connection lifecycle kept a private `count_open` and unlocked `serial_mutex` from inside the failed `mount_open()` path even though the caller owned that lock. The generated lifecycle replaces this with the generator-owned shared connection counter and a transactional `nexstar_open()` / `nexstar_close()` pair.
- The mount and guider shared one serial `libnexstar` connection. The migration keeps that topology and lets the generator serialize connection open/close ownership across both logical devices.
- The old guider RA/DEC handlers slept for the whole guide duration in the property handler. The migration changes guide pulses to a start handler plus `guider_guide_ra_finalizer()` and `guider_guide_dec_finalizer()` so the device queue is not blocked.
- The old `MOUNT_SLEW_RATE` change path marked and updated `MOUNT_GUIDE_RATE_PROPERTY` instead of `MOUNT_SLEW_RATE_PROPERTY`. The generated property handler now updates the handled slew-rate property.
- The old GPS detach path returned `indigo_guider_detach(device)` from a GPS device. The migrated custom GPS device returns `indigo_gps_detach(device)`.
- GPS remains dynamically attached only for the Celestron dialect, matching the old driver behavior. The generator owns mount and guider boilerplate; custom GPS code remains in the `.driver` code block because the old driver exposes GPS conditionally after mount identification.
- Baseline validation before migration: `make -C indigo_test build/integration/test_mount_nexstar_simulator` succeeded, but running `indigo_test/build/integration/test_mount_nexstar_simulator` exited with signal 11 before any migration changes.
- Coverage audit baseline (2026-09-13, commit `6b6b9bd69`, macOS Darwin 25.6.0 arm64): `make -C indigo_test build/integration/test_mount_nexstar_simulator` built the universal arm64/x86_64 simulator and test executable; a standalone `cd indigo_test && build/integration/test_mount_nexstar_simulator` run passed all 10 existing cases. `make -C indigo_test test-clean` removed the build artifacts afterward.
- The existing simulator has deterministic position, motion, tracking, location/time, guide-rate and GPS responses, but no command observation channel or failure injection. Consequently the existing tests cannot prove exact wire commands, malformed/short reply handling, command/read/poll/stop failures, transport loss or recovery.
- The mount test covers the Celestron happy path broadly, while the Sky-Watcher branch is limited to connection and identity. It omits parts of the visible property contract, coordinate boundary/sign cases, already-at-target and overlapping GOTO, full manual direction/reversal/simultaneous-axis behavior, failure recovery and reconnect.
- The guider test does not observe fresh property revisions. Three pulse requests wait only for a non-BUSY state and can accept the previous OK state before the queued handler starts. It also lacks simultaneous axes, disconnect/reconnect with a pending pulse, error recovery and the required timing measurements.
- The dynamic GPS test covers only a Celestron 3D-fix happy path; it does not assert the complete property/item contract, exact values, fix loss/reacquisition or shared-connection lifetime.
- Shared mount/guider ownership is not covered in both connection orders, and sibling survival/final-close behavior is not verified.

## Documentation Findings

- `README.md` describes serial/network NexStar protocol support, single startup instance plus runtime additional instances, `libnexstar` dependency, and two non-standard controls: `TRACKING_MODE` and `COMMAND_GUIDE_RATE`.
- `indigo_docs/PROPERTIES.md` already lists `TRACKING_MODE` and `COMMAND_GUIDE_RATE` as custom NexStar properties; after migration its source note needs to point to the `.driver` source and generated `.c` output.
- `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` listed the Arduino `.ino` simulator candidate even though a host-side C simulator exists. The migration moves the C simulator into the implemented simulator table and removes the stale candidate entry.
- The mount testing rules require simulator-backed coverage for connect/identity, GOTO and SYNC, manual motion and abort, tracking/rates, park/unpark, time/location/options, transport failure/recovery, and guider coverage for all guide directions.
- The bundled `celestron.pdf` and `skywatcher.pdf` protocol references document the command framing and replies used by the simulator, including precise/legacy RA/DEC GOTO and SYNC, tracking, fixed/variable slew, time/location, model/version/alignment and pass-through commands. The simulator audit and new command assertions will follow these documented encodings.

## Hardware-Test Decision

- No hardware testing will be performed in this work: no specific NexStar/SynScan mount and hand controller has been made available. Simulator results are software/protocol validation only and will not be presented as physical mount motion, tracking or electrical guide-pulse accuracy.

## Migration Plan

1. Convert the driver to `indigo_generator` with `indigo_mount_nexstar.driver` as the source of truth.
2. Preserve serial protocol behavior through `libnexstar`, including Celestron and Sky-Watcher dialect detection, ST4 guide-rate handling, tracking-mode selection, mount time/location updates, side-of-pier probing and conditional Celestron GPS exposure.
3. Move long-running operations to generator queues and finalizers: mount park polling and guider pulse completion.
4. Expand the host-side serial simulator to cover elapsed goto/park motion, manual slew stop state, sync, tracking, time/location, guide-rate readback, Celestron GPS pass-through and failure injection needed by automated tests.
5. Replace the smoke-style NexStar test with full mount/guider simulator coverage following `indigo_test/DRIVER_TESTING_RULES.md`; keep Windows status unchanged because this migration explicitly does not solve `libnexstar` Windows support.
6. Run generator, build the driver/test target, run the full NexStar simulator test, clean test artifacts and update `MIGRATION_STATUS.md`, `indigo_test/CHANGES.md`, `indigo_docs/PROPERTIES.md` and `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` with the verified results.

## Coverage Completion Plan

1. **Complete** — Extended the NexStar host simulator with a monotonic `<ready-file>.events` command log and one-shot `<ready-file>.control` actions for dropped, short and malformed replies and transport closure. The existing documented binary command framing is unchanged. `make -C indigo_test build/integration/mount_nexstar_simulator` passed.
2. **Complete** — The shared harness now records both fresh property revisions and the latest revision for every property state, preventing transient BUSY/ALERT updates from being hidden by a subsequent poll. Mount and GPS property/item/range contracts now include LST, horizontal coordinates, epoch, numeric bounds, all GPS fix states, exact location and UTC offset.
3. **Complete** — Added exact command and state coverage for Celestron and Sky-Watcher identity/firmware, coordinate readback, signed/boundary coordinates, GOTO/SYNC completion and overlap rejection, all manual directions and simultaneous axes, tracking/modes/rates, park/unpark/abort, time/location translation, malformed/short/dropped replies and transport closure/recovery.
4. **Complete** — Added guider-only, mount-first and guider-first shared connections, sibling survival after either disconnect, final-close/reopen behavior, transport reconnect and disconnect/reconnect during a pending pulse. GPS connection and fix loss/reacquisition are exercised while sharing the mount transport.
5. **Complete** — Guider coverage now requires fresh BUSY-to-OK or BUSY-to-ALERT revisions for east, west, north and south; covers zero duration, same-axis BUSY rejection, concurrent independent axes, failed start, failed stop, recovery and pending-pulse disconnect/reconnect.
6. **Complete** — Added opt-in `bench_mount_nexstar_guider_timing`, measuring monotonic simulator command start-to-stop endpoints for 20, 100 and 500 ms pulses in all four directions, with one warm-up and four measured samples per matrix cell under idle polling and simultaneous mount motion. It reports min/mean/median/p95/p99/max/sd, maximum absolute error and mean percentage error without a host-dependent threshold.
7. **Complete** — Protocol failures exposed production bugs where polling replaced the last valid position/location/time and GPS fix with invalid data, failed location writes published unaccepted targets, and failed tracking changes left the requested rather than actual mode selected. These are fixed in `indigo_mount_nexstar.driver`; the driver version is 32 and the generated C/header/main outputs are synchronized. The firmware formatter also uses bounded `snprintf()`.
8. **Complete** — Driver, strict-warning test, normal integration test, combined ASan/UBSan test and timing benchmark all passed on macOS arm64. The unchanged generator produced byte-identical output on consecutive runs, `git diff --check` passed, no new persistent file needs Xcode registration, and the test cleanup removed its build and simulator artifacts.

## Scenario-to-Test Mapping

| Required behavior | Automated scenario(s) |
| --- | --- |
| Connect, identity, firmware and both dialects | `nexstar_mount_connects_both_protocol_dialects` |
| Sky-Watcher coordinates, tracking and manual motion | `nexstar_skywatcher_executes_coordinates_tracking_and_motion` |
| Celestron property/item/range contract and basic options | `nexstar_celestron_mount_passes_serial_compliance_checks` |
| NexStar HC time and location translation/readback | `nexstar_nexstar_hc_sets_time_and_location` |
| Signed/boundary SYNC, GOTO completion/overlap, abort and recovery | `nexstar_mount_tracks_syncs_and_aborts_coordinate_changes` |
| Unaligned coordinate rejection | `nexstar_mount_rejects_coordinates_when_unaligned` |
| Malformed, short and dropped replies; failed writes; transport close/reconnect | `nexstar_mount_recovers_from_protocol_and_transport_failures` |
| All manual directions, simultaneous axes, abort, park/unpark and parked guard | `nexstar_mount_moves_manually_and_parks` |
| Abort while park work is queued | `nexstar_mount_aborts_queued_park` |
| Alt/Az tracking modes AA, EQ, AUTO and off | `nexstar_tracking_mode_is_exposed_for_altaz_models` |
| GPS property contract, no/2D/3D fix, exact data, loss and reacquisition | `nexstar_celestron_gps_device_reports_fix` |
| Guide-rate commands, all directions, fresh transitions, overlap, concurrent axes and failures | `nexstar_celestron_guider_passes_serial_compliance_checks` |
| Shared ownership in both orders, sibling survival and pending-pulse reconnect | `nexstar_shared_devices_survive_both_connection_orders` |
| Guide timing for all directions/durations under idle and motion workloads | `bench_mount_nexstar_guider_timing` |

## Validation Results

- Consecutive `../../build/bin/indigo_generator indigo_mount_nexstar.driver` runs produced identical SHA-1 hashes for `indigo_mount_nexstar.c`, `indigo_mount_nexstar.h` and `indigo_mount_nexstar_main.c`.
- `make -C indigo_drivers/mount_nexstar -f ../../Makefile.drv` passed after regeneration and rebuilt the archive, dynamic library and executable.
- `make -B -C indigo_test CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter' build/integration/test_mount_nexstar_simulator` passed for both the simulator and integration test.
- `cd indigo_test && build/integration/test_mount_nexstar_simulator` passed all 13 scenarios listed above.
- `make -B -C indigo_test build/integration/test_mount_nexstar_simulator_asan` passed. Running it with `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1` passed the same 13 scenarios with no AddressSanitizer or UndefinedBehaviorSanitizer report. LeakSanitizer is unavailable on this macOS runtime and was not claimed.
- `make -B -C indigo_test build/benchmark/bench_mount_nexstar_guider_timing` and the benchmark run passed. The 24 matrix cells produced 96 measured samples plus 24 warm-ups. Mean signed error ranged from +1.699 to +4.442 ms; maximum absolute error over all cells was 5.973 ms. These are host-side serial/scheduler measurements, not physical relay or mount accuracy.
- `git diff --check` passed. `make -C indigo_test test-clean` removed test build artifacts, and no `/tmp/indigo-serial-sim.*` directory remained after the final runs.
- Windows runtime remains unverified and unchanged; `MIGRATION_STATUS.md` keeps the existing `libnexstar` Windows TODO in the comment column.

## Final Test Summary

- Simulated functional test executions: **26 run, 26 passed** (13 normal plus the same 13 under ASan/UBSan).
- Timing samples: **96 measured successfully**, plus 24 warm-ups; timing data is reported as measurement rather than pass/fail behavior.
- Hardware tests: **0 run, 0 passed**. No compatible physical NexStar/SynScan mount was available, so hardware motion, electrical ST4 behavior and Windows runtime remain explicitly unverified.
