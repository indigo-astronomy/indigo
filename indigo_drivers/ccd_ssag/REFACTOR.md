# SSAG/QHY5 generator migration

Date: 2026-09-13.

## Scope and baseline

`indigo_ccd_ssag` was migrated from a handwritten lifecycle and USB hot-plug implementation to the unchanged `indigo_generator`. The `.driver` file is now the source of truth and the checked-in C source, public header and standalone entry point are generated from it.

- Baseline commit: `0e7ef87ab2e2825f2c84f49f9be94ab063e03ca0`; the worktree was clean before the migration.
- Host: macOS Darwin 25.6.0 on arm64; the normal driver build creates arm64+x86_64 universal binaries.
- Driver version: increased from `0x0300000C` to `0x0300000D` (13).
- Devices: `SSAG` and `SSAG (guider)`, sharing one USB handle.
- Initialized USB ID: `1856:0012`. Loader IDs: `1856:0011`, `1618:0901`, `16c0:296d`, plus the `SSAG_VID`/`SSAG_PID` environment pair.
- CCD: fixed 1280x1024 MONO8 image and standard gain 1..15. The guider supports standard RA and DEC pulses.
- Before this work `MIGRATION_STATUS.md` listed no automated tests for this driver.

The existing CloudMakers copyright and the OpenSSAG/Eric J. Holmes attribution are preserved and the copyright range is extended through 2026.

## Defects fixed

- The handwritten gain switch fell through and encoded most gain values incorrectly.
- Failed USB initialization leaked the opened handle and close did not safely clear ownership.
- Exposure-start errors and short bulk transfers could still be reported as successful images.
- Sub-100 ms exposures blocked a property handler.
- Exposure start and abort had a queue race. The generated driver now uses an atomic IDLE/PENDING/STARTED/CANCELLED state machine, rejects overlaps, cancels a still-pending handler and drains a physically started cancelled frame before accepting another exposure.
- The legacy one-byte endpoint-IN transfer used as a hardware abort is not supported by the tested QHY5 (`LIBUSB_ERROR_NOT_FOUND`). Cancellation is therefore logical and publication-safe: the outstanding frame is drained at its sensor deadline and is never published.
- The image row-compaction loop used overlapping `memcpy`; an instrumented run found this and it is now `memmove`.
- Guider pulses did not expose BUSY-to-completion state, clear public pulse values, report command failure or support safe same-axis replacement.
- Cancelling a replacement pulse's start handler after it had published BUSY could deadlock against its synchronous property update. A BUSY replacement now cancels only the finalizer; a start handler is cancelled only while genuinely pending.
- Firmware parsing performed an unaligned four-byte read for a two-byte address and did not balance every libusb reference on failure.
- Shared open/close, buffer lifetime and hot-plug teardown were split between handwritten counters and callbacks.

## Implemented design

- Generator `libusb { hotplug = true; }` owns discovery, attach/detach arrays, capacity, callback registration and shutdown cleanup. There is no `MAX_DEVICES` override.
- `ssag_match()` filters initialized devices and recognizes loader devices for firmware upload.
- Exact `ssag_open(indigo_device *device)` and `ssag_close(indigo_device *device)` helpers implement transactional acquisition and rollback. Generator-owned `PRIVATE_DATA->count` controls shared CCD/guider ownership.
- A reusable private USB response/image buffer replaces per-operation stack buffers.
- Ordinary exposure countdown uses `indigo_ccd_exposure_setup()`. A named `ccd_exposure_finalizer` performs bounded delayed readout and completion.
- Independent RA and DEC finalizers publish completion and handle replacement, zero pulses, simultaneous axes, failures and disconnect/removal cancellation.
- Generated sources, Xcode groups, Windows project/filter inputs, property documentation and migration status are synchronized.

## Automated scenario mapping

The integration test compiles the production driver separately against a fake libusb boundary while using the real INDIGO bus, CCD/guider base classes, image processing, queues and timers.

| # | Test case | Covered scenarios |
|---|---|---|
| 1 | Identity, properties and image | Driver/device metadata, version, property inventory, fixed geometry, gain bounds and deterministic cropped RAW image. |
| 2 | Shared lifecycle | CCD-only, guider-only, both connection orders, one shared open, sibling operation and one final close. |
| 3 | Initialization failure and recovery | Open, claim, configuration and initialization failures, complete rollback and retry. |
| 4 | Gain mapping | All gain encodings and initialization-packet contents. |
| 5 | Guider directions and completion | All four directions, duration encoding, BUSY, public value reset and OK completion. |
| 6 | Exposure failures | Start-control failure, bulk error and short transfer become ALERT without a published image. |
| 7 | Abort, drain and reacquisition | Pending and started cancellation, no cancelled image, overlap rejection, drain guard and successful restart. |
| 8 | Guider replacement and failure | Zero pulse, same-axis replacement, simultaneous axes, explicit cancellation and USB errors. |
| 9 | Guider completion timing | 20/100/500 ms pulses on four directions, two measured repetitions, idle and during an eight-second CCD exposure. |
| 10 | Discovery and capacity | Initialized, loader and unrelated IDs, duplicate/burst events, generated capacity and recovery after detach. |
| 11 | Firmware and reference balance | Loader variants, custom environment ID, upload failures, address decoding and libusb reference balance. |
| 12 | Attach failure and retry | Failed logical attach cleanup followed by successful enumeration. |
| 13 | Queue and registration rollback | INIT/SHUTDOWN cycles, registration failure, connected shutdown rejection, queued removal and no USB access after close. |

The timing endpoint is the fake USB duration-command entry to public zero/OK completion, so it validates scheduling and property semantics rather than electrical relay timing. Each workload/duration result contains eight measured samples after per-direction warm-up. The final normal run's worst absolute completion error was 10.056 ms. Instrumented timing stayed within 10.140 ms in its final run.

Simulated result: **13 tests run, 13 passed**.

## Physical hardware evidence

The user supplied a physical Orion SSAG/QHY5. It enumerated first as loader `1618:0901`; firmware upload produced initialized device `1856:0012`.

The base hardware case passed:

- exact CCD/guider discovery and driver version 13;
- guider-first and CCD-first shared connection ownership;
- seven non-uniform 1280x1024 MONO8 RAW frames of the expected size;
- 0.05 s and 1.5 s exposures;
- abort 400 ms after a one-second exposure started, rejection while the physical frame was draining, no cancelled frame, then successful reacquisition;
- all four 100 ms guide requests;
- both sibling disconnect orders;
- driver shutdown, reinitialization, reconnection and a fresh image.

Physical idle hot-unplug detached both logical devices and closed the USB handle. Replug exercised loader firmware upload, transient `NO_DEVICE` recovery, initialized-device reattachment and reconnection. A separate active hot-plug acceptance passed with a real 120-second USB exposure in progress: physical unplug detached both devices and closed the handle immediately; replug, loader-to-initialized transition, reconnect, fresh RAW image and EAST 100 ms guide request all succeeded.

The hardware target contains **2 registered cases**: base acceptance and physical hot-plug acceptance. The idle and active hot-plug phases were accepted in separate interactive runs because their combined exploratory run reached the active-unplug prompt after the operator timeout; each final accepted phase completed successfully. The production driver uses the shared macOS polling hot-plug implementation. The test adapter additionally performs delayed real-device enumeration only when polling has not exposed the replugged device, covering macOS libusb pointer reuse; all open, control and bulk traffic remains physical.

Electrical ST4 relay timing was not measured because no external relay instrumentation was available. Multiple-camera behavior was validated with fake USB because only one physical camera was available. Dynamic dylib unload was not exercised, while equivalent driver SHUTDOWN/INIT and fresh-operation behavior was exercised. Windows project integration was updated but Windows runtime testing was unavailable.

## Validation record

- Baseline driver build passed before modification.
- The six-case seed suite against the handwritten driver reproduced four failures: initialization handle leak, incorrect gain encoding, missing guider completion reset, and ignored exposure start/short-read errors.
- `make -C indigo_test build/integration/test_ccd_ssag_usb` and `indigo_test/build/integration/test_ccd_ssag_usb`: 13/13 passed.
- The replacement/simultaneous-axis/failure concurrency case passed 20/20 isolated repetitions after the deadlock correction.
- The started-abort/drain/reacquisition case passed 10/10 isolated repetitions after its test waited for the USB start command rather than only the framework's queued BUSY update.
- `make -C indigo_test build/integration/test_ccd_ssag_usb_sanitize` with AddressSanitizer and UndefinedBehaviorSanitizer: 13/13 passed after correcting the overlapping copy. LeakSanitizer is unsupported by the macOS runtime used here, so `detect_leaks=0` was required.
- `indigo_test/build/hardware/test_ccd_ssag_hw --run`: base hardware acceptance passed.
- `indigo_test/build/hardware/test_ccd_ssag_hw --active-hotplug-only`: active physical unplug/replug acceptance passed; the idle phase passed in the preceding interactive hot-plug run.
- No manufacturer protocol document is bundled for this driver. The protocol implementation and firmware format were audited against the pre-migration source and its preserved OpenSSAG attribution.

## Non-applicable capabilities

Streaming, ROI, binning, cooler, temperature, offset, mode selection and FITS-header-specific controls are neither implemented nor exposed by this fixed-format camera driver. Generic framework numeric validation is intentionally not duplicated. These items are therefore outside the applicable CCD class test matrix.

## Overlapping guide pulses (2026-09-20)

`GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare `accept_while_busy = true` and `on_change_request`
is reduced to zeroing both axis items, matching every other INDIGO driver that exposes a guider.

The driver used to detect a replacement by reading the property state in `on_change_request` and
passing the answer to the handler through the `ra_replacement` / `dec_replacement` atomics. With the
property state now owned by the dispatch macro that no longer works, so the handler decides from the
`ra_guiding` / `dec_guiding` flag it already maintains and the two atomics were removed. The
`ssag_cancel_guide()` call that stops an in-flight pulse on the device is therefore still made
exactly when a pulse is actually running.
