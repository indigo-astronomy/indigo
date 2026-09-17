# ccd_svb refactoring record

## Scope and baseline

This record covers migration of `indigo_ccd_svb` from its hand-written INDIGO 2.x/early-3.x implementation to `indigo_generator`, the corresponding production fixes, complete applicable hardware-free fake-SDK coverage, and physical validation with an SVBONY SV305Pro.

Baseline date and source: 2026-09-16, commit `9bbd6a8e33ce1034e040d7be5a22bcf55de12201` plus a pre-existing user edit to `indigo.xcodeproj/project.pbxproj` which this work must preserve. Host: macOS 26.6 (`Darwin 25.6.0`), Apple Silicon arm64.

Baseline build command:

```sh
cd indigo_drivers/ccd_svb
make -B -f ../../Makefile.drv
```

Result: passed. The driver, archive, dynamic library and executable built for x86_64 and arm64. The linker reported only pre-existing deployment-target warnings because the bundled `libSVBCameraSDK.dylib` slices require macOS 10.13/14.0 while the driver build requests 10.10/11.0. The bundled SDK is `libSVBCameraSDK v.1.13.4` and contains both x86_64 and arm64 slices.

Baseline automated tests: no driver-specific `test_ccd_svb_sdk` or hardware test exists; `MIGRATION_STATUS.md` records `0 / 0`. The similarly named `ccd_svb2` is a different ToupTek-family driver and is not evidence for this driver.

## Current-state audit

### Architecture and lifecycle

- One physical SDK camera can expose a CCD logical device and, when `SVBCanPulseGuide()` reports support, a guider logical device sharing one `svb_private_data` allocation and SDK handle count.
- The driver uses libusb VID `0xf266`, product `0x9a0a`, a hand-written hot-plug callback, fixed arrays for 12 logical devices and SDK ids, delayed global timers for arrival/removal, and SDK enumeration to correlate events with devices.
- Discovery opens the camera temporarily, disables SDK auto-save, reads camera properties and guider capability, then closes it. Connection uses a global INDIGO lock, `count_open`, and a per-camera mutex so either logical device can own the shared SDK session.
- Exposure and streaming use SVB video capture plus software trigger. The implementation uses timers, but the completion callbacks contain blocking polling loops. Temperature polling is periodic. Guider pulses are duration-based SDK calls with software timers used only for public-property completion.
- The current driver performs its own attach/detach, connection, property allocation, dispatch and hot-plug ownership. Generator migration will replace that boilerplate with an `sdk { hotplug = true; ... }` definition, driver-wide queue serialization, conditional guider attachment and generated shared-open reference ownership.

### Published properties and behavior

- CCD base properties are inherited from `indigo_ccd_driver`; the driver exposes streaming and streaming settings and dynamically exposes SDK-dependent exposure, gain, offset, gamma, cooler, temperature and cooler-power controls.
- Custom `PIXEL_FORMAT` is a one-of-many switch containing up to four SDK formats selected from RAW8, RGB24, RAW16, Y8 and Y16. The current name is not compliant with the repository rule requiring driver custom properties to start with `X_`.
- Custom `SVB_ADVANCED` is a dynamic numeric property populated from remaining writable SDK controls. Its current name is also non-compliant. The migrated names will be documented and regression-tested.
- `CCD_MODE`, `CCD_BIN`, `CCD_FRAME` and the pixel-format property are synchronized. SVB cameras require equal horizontal and vertical binning. Subframes are aligned to the SDK requirements implemented by the driver (width multiple of 8, height multiple of 2, minimum delivered width/height of 64 binned pixels).
- Camera identity, serial, SDK version, sensor geometry, bit depth, Bayer pattern, supported formats/bins and model-derived pixel sizes are published. Exposure values are translated between seconds and SDK microseconds. Target and current temperature use tenths of a degree at the SDK boundary.
- Optional guider exposes all four duration-based pulse directions and shares the camera SDK lifetime.

### SDK, platforms and integration

- Vendor API: bundled `SVBCameraSDK.h`/`libSVBCameraSDK` 1.13.4. Relevant APIs cover enumeration/identity, open/close, control capabilities and values, ROI/output format, start/stop/video readout, soft trigger, firmware-upgrade query, auto-save, camera mode and pulse guiding.
- The user-facing README declares Linux x86/x64/ARM and macOS Intel support, while the current bundled macOS library also has arm64. README changes are outside this task unless explicitly approved.
- The driver is registered in `Makefile.drv` auto-discovery, platform projects and `indigo.xcodeproj`. The migration must add `.driver`, `REFACTOR.md`, fake-SDK test and hardware-test sources to the Xcode project without overwriting the user's existing project-file edit.
- No existing `.driver`, fake SDK, protocol simulator, driver-specific automated suite or opt-in hardware harness exists.

### Defects and risks found by source audit

- The exposure completion callback calls `pthread_mutex_unlock()` twice around `SVBStopVideoCapture()` without acquiring the mutex. Observable impact is undefined behavior during every completed exposure; this is source-audit evidence pending the fake-SDK regression.
- Exposure and streaming completion poll in unbounded loops on a callback thread. A camera that returns timeout forever can leave the operation and teardown stuck indefinitely.
- Several SDK failures are logged but do not set the public property to ALERT or roll back accepted values: soft trigger, advanced-control writes/readbacks, control-capability reads, cooler writes and some initialization reads.
- `SVBGetControlCaps()` reuses the previous `res` variable without assigning its return value in the connection loop, so capability failures can consume stale/uninitialized data.
- Temperature read failure still uses `temp_x10`; cooler-enable/target/power partial failures are inconsistently reported. Target comparison mixes degrees with tenths of a degree, causing redundant writes or incorrect equality.
- Streaming uses wall-clock `time(NULL)` and integer-second elapsed time for countdown, losing subsecond accuracy. It must use monotonic time and retain the exact requested duration.
- Start/setup/trigger failures can leave acquisition properties BUSY and do not consistently call failure cleanup. Abort races, stale callbacks, readout teardown and immediate reacquisition are not bounded or tested.
- Guider SDK errors are logged but the property is still driven BUSY and later OK. Replacement completion timers can clear a newer pulse unless stale finalizers are prevented.
- Discovery assumes SDK ids are safe indices into `connected_ids[SVBCAMERA_ID_MAX]`, marks ids connected before attachment succeeds, leaks/strands state on several master/guider attach and capacity failures, and can misassociate libusb events with SDK enumeration order.
- An optional guider can exhaust the last logical-device slot after the CCD was attached, leaving a partial physical device.
- Manual unplug and shutdown paths have overlapping close/free ownership and do not consistently destroy the per-camera mutex. Delayed global timers are not explicitly drained before shutdown.
- Custom property names violate the mandatory `X_` prefix rule and `indigo_docs/PROPERTIES.md` documents those legacy names.
- The driver's behavior version is `0x03000014`; it must be incremented by the generated definition.

These findings remain audit-only until a production fix and named regression test are recorded in the dedicated found-defects section below.

## Hardware-test decision and result

The opt-in `ccd_svb` hardware harness is excluded from normal `test`/`test-integration` targets and is retained for an SDK-supported SVBONY camera. It validates identity/capabilities, lifecycle, formats, ROI/bin, controls/configuration, exposure, abort/reuse, finite/long streaming, reload and optional guider behavior while restoring captured settings and using a temporary configuration directory.

The initially connected SV205 was correctly rejected because it is a UVC `0x0bda:0x3038` device rather than an SVBCameraSDK camera. The final test used an SVBONY SV305Pro (`0xf266:0x9a0a`, firmware `v2.0.0.3`) with bundled SDK 1.13.4. The supplied INDIGO 2.0 reference log established that this camera and a three-second RAW8 exposure worked before refactoring.

The final physical case passed. It exercised RAW8, RGB24, RAW16 and Y8 full-frame images at bins 1 and 2, 256 x 256 ROI at bins 1 and 2, configuration roundtrip, long exposure with concurrent guider commands, abort and immediate reuse, finite and sustained streaming (76 full-frame RAW8 images in approximately ten seconds), CCD/guider reconnect, driver shutdown/reinitialization and a fresh exposure. An initial direct SDK probe showed that `SVBSetOutputImageType()` succeeds while an immediate `SVBGetOutputImageType()` can still report the preceding RAW8 format. This was initially misclassified as refusal of RGB24 and Y8. A later INDIGO 2.0 reference run on the same SV305Pro demonstrated successful RGB24 and Y8 acquisition at bins 1 and 2, proving that the getter is delayed/stale rather than authoritative immediately after the setter. The corrected hardware harness validates delivered images rather than skipping those formats based on immediate readback. Its RAW8 validation also rejects the characteristic RGB32 alpha-byte pattern that was found in the supplied bad refactored FITS image.

## Atomic plan

1. **Done — instructions, sources and baseline.** Read repository/driver/test rules, generator and driver-development references, the driver, bundled SDK header, build files and reference generated CCD/fake-SDK tests. Built the original driver successfully as recorded above; confirmed zero existing `ccd_svb` tests.
2. **Done — migration design and property/SDK inventory.** Mapped the generated SDK hot-plug blocks, optional guider, SDK identity/property structures, control types, formats/bins, shared ownership and applicable CCD/guider scenarios. Chose `X_PIXEL_FORMAT` and `X_ADVANCED`; ASI-only presets/suffix behavior was intentionally not introduced. Chose bounded `SVBGetVideoData(..., 20)` finalizer polling with a monotonic deadline because this SDK has no separate snapshot-status/readout API.
3. **Done — create the `.driver` source of truth.** Added `indigo_ccd_svb.driver` with generator-owned lifecycle/hot-plug/connection/property dispatch, SDK-id-based unplug matching, six bounded discovery retries, validated SDK metadata, transactional open, optional guider, shared count ownership, synchronized individual SDK calls, bounded exposure/streaming completion, monotonic countdown, temperature polling, duration-based guider completion and `X_` custom properties. Version increased from 20 (`0x03000014`) to 21. Generated outputs build successfully.
4. **Done — generate and inspect outputs.** Ran `../../build/bin/indigo_generator indigo_ccd_svb.driver` and `make -B -f ../../Makefile.drv`; build passed for x86_64+arm64 with only the baseline SDK deployment-target linker warnings. Inspected generated exposure/streaming/urgent-abort and CCD/guider connection handlers: the `_finalizer` blocks own their start/completion updates, shared `count` increments only after open and rolls back on initialization failure, and final close occurs at zero. Repeated generation is deterministic.
5. **Done — fake-SDK infrastructure.** Added `indigo_test/integration/test_ccd_svb_sdk.c` with deterministic RAW8/RAW16/RGB24/Y8/Y16 data, color/Bayer validation, mock capability profiles, USB/SDK identity control, failure injection, blocking gates, resource counters and lifecycle invariants.
6. **Done — lifecycle/discovery/property tests.** The suite covers repeated INIT/SHUTDOWN, CCD/guider connection order, optional guider and temperature/cooler profiles, duplicate/reordered/burst hot-plug, retries, capacity and attach rollback, failed discovery open, malformed metadata, full property inventory and reconnect rebuilding.
7. **Done — controls/cooling/image tests.** Tests cover gain/offset/gamma/advanced controls and rollback, tenths-degree cooling, sensor-only/absent profiles, every advertised format, Bayer/channel metadata, ROI/bin/modes/frame types, microsecond exposure units and deterministic image payloads.
8. **Done — acquisition/streaming/race tests.** Tests cover fractional and long exposures, mutual-operation conflicts, bounded readout watchdog, setup/start/trigger/read/stop failures, abort at queued/readout/final-frame boundaries, immediate reuse, finite/indefinite streaming, disconnect/removal/shutdown races and SDK-after-close/detach counters.
9. **Done — guider tests and timing evidence.** Tests cover all directions, SDK failure, same-axis replacement, simultaneous axes, acquisition/streaming interaction, disconnect/removal and reconnect. The timing benchmark completed all 120 requests (four directions, 20/100/500 ms, idle and streaming, five trials with one warmup); retained-sample maximum absolute completion error was 13.533 ms and every row completed.
10. **Done — opt-in hardware harness.** Added `indigo_test/hardware/test_ccd_svb_hw.c` and `test-ccd-svb-hw`. It is excluded from normal tests, accepts an exact `INDIGO_TEST_DEVICE`, validates images/formats/ROI/bin/controls/config/exposure/abort/stream/reload and conditionally guider behavior, restores captured properties, uses a temporary configuration directory, waits up to 15 seconds for slow SDK enumeration, and offers explicit hot-plug/acceptance modes. The harness builds and its automated SV305Pro workflow passes.
11. **Done — repository integration and documentation.** Updated the test Makefile, Xcode groups, `indigo_docs/PROPERTIES.md` for the `X_` names and `.driver` source, and the `ccd_svb` row in `MIGRATION_STATUS.md`. The pre-existing Xcode reorder/edit was preserved.
12. **Done — final verification.** Universal normal and strict-warning driver builds pass (only the baseline SDK deployment-target linker warnings; strict mode excludes the generator-owned incompatible callback cast diagnostic). Strict fake-SDK compilation passes. The final ordinary fake-SDK run passes 46 / 46. The complete arm64 ASan+UBSan run passes 46 / 46 with leak detection disabled because this macOS runtime does not support it. The timing benchmark passes 1 / 1. Regeneration is deterministic with final SHA-1 values `577772d62539af286f42765f49fde01c6d5656c9`, `31802bf3964357cafbbad6b01416aeca13439b37` and `d4e39ae7cdc1bb9f081895f8fbc3b45b7d1b51e2` for `.c`, `.h` and `_main.c` respectively.
13. **Done — physical-device validation.** The SV305Pro automated hardware workflow passes with SDK 1.13.4. The initial SV205 attempt remains recorded only as an inapplicable UVC-device check.
14. **Done — restore RGB24/Y8 compatibility with delayed SDK readback.** The supplied INDIGO 2.0 reference log proves successful RGB24 and Y8 images at bins 1 and 2. The fake SDK now models the observed delayed `SVBGetOutputImageType()` and reproduced the failure before the production change. The driver no longer adds the non-original second format getter or treats its stale value as a setup failure; setter errors and final ROI validation remain enforced. Version increased from 21 to 22 and generated outputs were refreshed. Ordinary and sanitizer fake suites pass 46 / 46, and the SV305Pro hardware matrix passes RAW8, RGB24, RAW16 and Y8 at both 1x1 and 2x2 with validated payload sizes.

## Scenario-to-test mapping

Identity, hot-plug and ownership are covered by the `Matrix discovery_*`, `Remaining discovery retries`, `Remaining unplug SDK read`, `Edges shutdown_pending_discovery_and_capacity`, `Open Init rollback` and `Guider shared session` cases. Property/capability reconstruction is covered by `Complete property inventory`, `Sensor only and absent`, `Edges capability_rebuild_and_cooling`, `Matrix limited_formats_and_bins` and `Matrix malformed_identity_recovery`.

Controls and environmental behavior are covered by `Matrix control_failures`, `Unchanged advanced controls`, `Advanced failure ALERT recovery`, `Edges malformed_numeric_readback`, `Initialization read rollback`, `Temperature read failure` and both cooling edge cases. Image/acquisition behavior is covered by `Formats and ROI`, `Matrix frame_types_and_fractional_countdown`, `Setup read failure recovery`, `Snapshot watchdog recovery`, both video-error cases, queued/readout/final-frame abort cases and the baseline exposure/configuration cases. Finite/indefinite streaming, overlap and teardown are covered by the finite/long-stream, urgent-abort, sibling-disconnect, active-unplug and acquisition-conflict cases. Guider function is covered by the four direction/shared-session/opposed/overlap/error cases and the separate timing benchmark.

Fake profiles explicitly cover color and mono, with/without guider, cooled, temperature-sensor-only and no-temperature devices. Generic framework numeric validation, generic encoders and optical/electrical image quality are not driver-owned and are not duplicated. The opt-in hardware case passed on SV305Pro; the corrected matrix replaces the earlier stale-readback skips with delivered-image validation for RAW8, RGB24, RAW16 and Y8 at both supported bin factors.

## Found defects

- **Completion mutex misuse and blocking loops:** the old exposure callback double-unlocked its mutex and both exposure/streaming could poll forever. The generated implementation uses one balanced mutex around each SDK transaction and a rescheduled bounded finalizer with a monotonic deadline. Covered by `Edges video_final_frame_abort_orders`, `Snapshot watchdog recovery`, `Matrix finite_video_abort_and_timeout` and the resource invariants run after every case.
- **Acquisition failures left BUSY or stale work alive:** setup, start, trigger, read and stop failures did not consistently finalize public state. All starts now use one acquisition state machine with explicit finish/abort cleanup. Covered by `Start failure immediate recovery`, `Setup read failure recovery`, `Matrix video_error_paths`, `Readout error and subsequent acquisition`, `Matrix readout_abort_and_restart` and `Urgent abort overtakes pending start`.
- **Control/capability error propagation:** a stale capability return code and partial read/write failures could publish success or bad values. Every capability/read/write is checked, values are read back, and accepted values roll back on failure. Covered by `Edges control_metadata_rollback`, `Initialization read rollback`, `Matrix control_failures`, `Advanced failure ALERT recovery` and `Edges malformed_numeric_readback`.
- **Cooling conversion and partial-read defects:** target comparison mixed degrees with SDK tenths and failed reads could consume stale values. SDK values now cross the boundary in tenths with fully checked reads. Covered by `Edges cooling_reads_writes_and_settling`, `Edges capability_rebuild_and_cooling` and `Temperature read failure`.
- **Fractional timing and countdown:** wall-clock/integer countdowns lost subsecond accuracy. Exact deadlines now use `indigo_monotonic_time()` and only the published countdown is rounded. Covered by `Matrix frame_types_and_fractional_countdown`, `Finite streaming baseline` and `Long indefinite stream abort restart`.
- **Guider error/replacement behavior:** SDK failures previously continued through BUSY to OK and stale completion could clear a newer pulse. Per-axis finalizers are canceled/replaced, failures publish ALERT and disconnect clears both properties. Covered by `Matrix guide_failures_and_overlap`, `Remaining opposed guides`, `Guide axes and disconnect`, `Guider shared session` and `Timing guide ON-OFF`.
- **Discovery/ownership failures:** SDK ids were used as array indices and state could be stranded by reordered discovery, capacity or partial attachment. Generated driver-wide serialization plus SDK-id matching and transactional generated ownership replace that scaffolding. Covered by the discovery matrix, capacity/shutdown and active-unplug cases.
- **Close after failed discovery open:** the first generated draft unconditionally called `SVBCloseCamera()` after a failed probe open. Close is now conditional on successful open. Covered by `Matrix discovery_open_failure_rollback` and the SDK-after-close invariant.
- **Idle stop incorrectly rejected valid sessions:** the first generated draft treated an idle `SVBStopVideoCapture()` error as a connection/start failure even though stop is best-effort cleanup. Connection and pre-start drain now ignore that stop result while still requiring subsequent setup/start/trigger success. Covered by `Open Init rollback` and `Matrix video_error_paths`.
- **SV305Pro short-exposure timeout:** the first generated draft stopped video before draining pending frames, reversing the working INDIGO 2.0 sequence from the supplied reference. On SV305Pro this made a 0.1-second exposure time out. Acquisition now drains before stop, and the fake SDK asserts that no post-stop read occurs.
- **First-frame format initialization lost during migration:** the original driver forced `SVBSetOutputImageType()` before the first exposure even when `SVBGetOutputImageType()` already reported the requested format. The migrated driver trusted that readback and omitted the apparently redundant setter. On SV305Pro the readback reported RAW8 while the SDK still delivered RGB32 (`R, G, B, 255` groups), causing both an SDK write past the original RGB24-sized buffer and a nonsensical RAW8 FITS image. The generated driver now restores the original `first_frame` contract and the original color/mono buffer sizing. The fake SDK starts with a deliberately false RAW8 readback and RGB32 delivery state, requires exactly one first-exposure setter and validates the resulting pixels. The hardware harness rejects the RGB32 alpha-byte pattern in a RAW8 frame; the complete physical workflow passes after the fix.
- **Immediate output-format readback incorrectly treated as authoritative:** the migrated setup validation added a second `SVBGetOutputImageType()` and required `c_pixel_format == requested_format`, neither of which existed in the original driver. SV305Pro accepts `SVBSetOutputImageType()` but its immediate getter can retain the preceding value. This made valid RGB24 and Y8 acquisitions fail before readout. The INDIGO 2.0 reference log proves successful RGB24 and Y8 frames at both 1x1 and 2x2. The delayed-getter fake regression failed before the fix. The generated driver now follows the original call order and manner: it checks the setter result, performs no second format getter, and retains the final ROI readback validation. The complete fake suites and physical 1x1/2x2 format matrix pass.
- **Uninitialized diagnostic fields:** failed ROI/output queries could log indeterminate readback locals. The readback structure is initialized before SDK calls. Covered by `Setup read failure recovery`; strict and sanitizer builds also pass.
- **Non-compliant custom properties:** legacy `PIXEL_FORMAT` and `SVB_ADVANCED` are now `X_PIXEL_FORMAT` and `X_ADVANCED`, documented and asserted by `Complete property inventory`.

## Test evidence

- Baseline simulated/fake-SDK tests run/passed: 0 / 0 (no suite existed).
- Final ordinary fake-SDK run: 46 / 46.
- Arm64 ASan+UBSan fake-SDK run: 46 / 46, no sanitizer report (`detect_leaks=0`; LeakSanitizer unsupported on this macOS runtime).
- Guider timing benchmark: 1 / 1 aggregate case, 120 / 120 pulse requests completed; worst retained absolute endpoint error 13.533 ms.
- Hardware harness build and SV305Pro automated workflow: 1 / 1 passed. RAW8, RGB24, RAW16 and Y8 passed at both 1x1 and 2x2; all delivered payloads passed size/signature/content validation, and the sustained stream delivered 76 frames in approximately ten seconds.

## Final test summary

- Simulated/fake-SDK registered: 47 cases (46 ordinary plus one opt-in timing benchmark). Final ordinary run/passed: 46 / 46; sanitizer run/passed: 46 / 46; timing run/passed: 1 / 1.
- Hardware registered/passed: 1 / 1 on SV305Pro with SDK 1.13.4. The earlier SV205 attempt was not applicable because that camera uses UVC.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard are now declared with the generator's `reject_change` block. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values instead of an `INDIGO_OK_STATE` update carrying no items, which left the refused value visible in the client.

Covered by `Edges rejected_change_alerts_and_keeps_values` in `indigo_test/integration/test_ccd_svb_sdk.c`: during an exposure it checks that `CCD_GAIN`, `CCD_FRAME`, `CCD_BIN` and `X_PIXEL_FORMAT` end in ALERT with unchanged values and targets, and that `CCD_GAIN` is accepted again once the exposure finishes.

```sh
make -C indigo_test test-ccd-svb-sdk
```

## Per-camera `sdk_mutex` removal (2026-09-18)

The migrated driver kept `svb_private_data.sdk_mutex`, a rename of the original hand-written `usb_mutex`, and wrapped every individual SDK transaction in it. That lock has been removed, together with the now-unused `#include <pthread.h>` in the `.driver` include block. Version increased from 23 to 24.

The mutex protected nothing that the generated queue split does not already protect. `CONNECTION`, hot-plug arrival and removal run on the per-driver queue; acquisition, guiding, temperature polling and every `_finalizer` run on the master-device queue. Both disconnect branches begin with `indigo_cancel_pending_handlers()`, and `indigo_queue_remove()` blocks until the currently running matching task finishes, so no device-queue handler can touch the SDK once close proceeds. `indigo_lock_master_device()` remains where `ccd_asi` and `ccd_playerone` use it: around `svb_open()`/`svb_close()` and the CCD `on_connect`/`on_disconnect` and guider `on_disconnect` blocks. The driver now matches those two drivers, which never carried an equivalent per-camera lock.

One window is not closed by either driver and is unchanged by this edit: `ccd_connection_handler` runs `initialize_camera()` on the driver queue while an already-connected sibling guider can be executing a pulse on the master-device queue. `ccd_asi` and `ccd_playerone` hold the master-device lock on the connect side only, and their device-queue handlers do not take it, so the pairing is one-sided there as well. Closing it belongs in the generator rather than in one driver's private lock.

Verification on macOS 26.6, Apple Silicon arm64:

```sh
cd indigo_drivers/ccd_svb && make -B -f ../../Makefile.drv
make -B -C indigo_test test-ccd-svb-sdk
make -B -C indigo_test test-ccd-svb-sdk-sanitize
```

- Universal x86_64+arm64 driver build: passed, no compiler warnings; only the pre-existing `libSVBCameraSDK.dylib` deployment-target linker warnings (10.13 and 14.0 versus the 10.10/11.0 build targets).
- Strict driver build `make -B -f ../../Makefile.drv CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -Wno-cast-function-type-mismatch'`: passed. The excluded diagnostic is the generator-owned `(indigo_timer_callback)process_sdk_retry_handler` cast at `indigo_ccd_svb.c:1897`, which is pre-existing and not driver-authored.
- Strict fake-SDK build with the same flags: passed, no warnings.
- Ordinary fake-SDK run: 47 / 47.
- Arm64 ASan+UBSan run: 47 / 47, no sanitizer report (`detect_leaks=0`; LeakSanitizer unsupported on this runtime).
- Arm64 ThreadSanitizer run, built by overriding `SVB_CCD_SANITIZE_CFLAGS`/`SVB_CCD_SANITIZE_LDFLAGS` with `-fsanitize=thread`: 47 / 47 cases pass, 6 data races reported. The identical run against the pre-change driver reports the same 6 races in the same functions, so the removal introduces none. They are property and private-data races between the driver queue and the master-device queue that the removed mutex never covered: `ccd_connection_handler` versus `ccd_timer_callback`, `ccd_temperature_callback` versus the connection path, one in the test harness itself, and one on the guider RA path. They are recorded here as a pre-existing finding, not fixed by this change, and `libindigo.a` is not instrumented in that build so the happens-before edges inside `indigo_timer.c` are invisible to the detector.
- Regeneration is deterministic. Final SHA-1 values are `8bbc37c7c3645d5d317d05865a00b3550e67fd2e`, `31802bf3964357cafbbad6b01416aeca13439b37` and `d4e39ae7cdc1bb9f081895f8fbc3b45b7d1b51e2` for `.c`, `.h` and `_main.c`; the `.h` and `_main.c` outputs are unchanged.
- No hardware was available for this change; the SV305Pro workflow was not repeated.
