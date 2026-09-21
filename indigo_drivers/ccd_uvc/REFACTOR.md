# ccd_uvc generator migration

## Scope and decisions

- Refactor the hand-written `indigo_ccd_uvc` driver to an `indigo_generator` `.driver` source while preserving the checked-in generated `.c`, `.h`, and `_main.c` outputs.
- Build characterization-first hardware-free coverage around a fake libuvc/libusb boundary before changing production code. The normalized public-property and SDK-call trace from the original driver is the compatibility contract for the migration.
- Windows is explicitly out of scope because the bundled libuvc implementation is Unix-only. Linux and macOS remain the supported targets; available local validation is macOS arm64.
- Hardware validation will be performed with an SVBONY SV205 after fake-SDK migration coverage passes. Planned scenarios are discovery/identity, connect/disconnect/reconnect, advertised modes and controls, short and longer exposures, mode changes, finite and indefinite streaming with stop, disconnect/reload, and unplug/replug recovery. Initial settings will be restored. Physical interruption during active acquisition will be attempted only when safe for the device and host.
- The macOS camera-ownership issue documented in `README.md` will be investigated without editing that README. Any temporary workaround must identify the exact service/process and use a reversible stop/start procedure before it is used for hardware testing.

## Current-state audit

### Architecture and implementation

- Version before migration: `DRIVER_VERSION 0x0200000F` (2.0.15), hand-written C implementation in `indigo_ccd_uvc.c` with a hand-written header and executable wrapper.
- The repository bundles libuvc sources directly in the driver directory and links JPEG plus libusb. Discovery is driven by a libusb hot-plug callback. Arrival asynchronously enumerates libuvc devices and matches bus/address; removal detaches the first matching logical device.
- One CCD logical device is attached per matched physical camera. The private data owns the retained `uvc_device_t *`, an open `uvc_device_handle_t *`, selected format/stream control, stream handle, and image buffer.
- Connection opens libuvc, enumerates uncompressed/frame-based formats and resolutions into `CCD_MODE`, sets the first stream control, forces manual exposure mode, discovers exposure limits and optional gain/gamma controls, and allocates the image buffer. Disconnect closes the handle and frees the buffer.
- Exposure and streaming configure manual exposure, exposure duration, optional gain/gamma, then open and start a polling stream. Timer callbacks synchronously wait for frames in one-second libuvc polling calls. Supported native data are mono/raw 8-bit and mono 16-bit; RGB/YUYV/UYVY are converted to RGB24 through libuvc. Images are handed to the common CCD image pipeline.
- Exposure cannot be aborted by the current driver. Streaming abort is cooperative and completes after the current blocking frame wait.

### Public property contract

- Common device properties are inherited from the framework.
- The device exposes the CCD interface. Before connection only the normal common visible properties are expected.
- After connection the driver-specific contract uses inherited `CCD_INFO`, `CCD_MODE`, `CCD_FRAME`, `CCD_EXPOSURE`, `CCD_STREAMING`, `CCD_STREAMING_SETTINGS`, `CCD_ABORT_EXPOSURE`, `CCD_GAIN`, `CCD_GAMMA`, `CCD_IMAGE_FORMAT`, upload/image properties, and the remaining base CCD properties.
- `CCD_BIN` is hidden. `CCD_FRAME` is read-only. `CCD_INFO` is reduced to width and height. `CCD_STREAMING` and `CCD_STREAMING_SETTINGS` are visible. Exposure minima default to 1 ms and are replaced with device-reported limits when available. `CCD_IMAGE_FORMAT` exposes seven base items. Gain and gamma are shown only when the UVC GET_INFO response advertises GET_CUR, and become read/write only when SET_CUR is advertised.
- There are no custom driver properties, so no custom `X_` prefix migration is required. `indigo_docs/PROPERTIES.md` already lists the inherited properties used by this driver.

### Documentation, build, and platform integration

- Repository documentation consists of `README.md`, the bundled USB Video Class 1.1 PDF, an AVFoundation PDF, bundled libuvc headers/sources, and the repository driver-development and generator-migration guides.
- The driver is in the stable driver list and has Xcode references for its current C/header/main sources and bundled libuvc sources. There is no `.driver`, refactoring record, fake SDK, automated integration test, or hardware test target yet.
- `MIGRATION_STATUS.md` records API level 2, no Windows support, no generator/async migration/retest, and `0 / 0` automated sim/hardware cases. The Comment column is `⛔ libuvc is Unix only` and must remain unchanged.
- `README.md` says macOS Monterey and later are blocked by `com.apple.UVCService` and documents running the application once as root as the only known workaround. The exact modern launchd/process ownership and a reversible temporary stop procedure still require verification.

### Existing tests and coverage gaps

- No driver-specific simulator, fake SDK, automated integration test, reference trace, or opt-in hardware test exists.
- Required fake coverage includes metadata and property inventory; successful and failed INIT/hot-plug/discovery; duplicate, burst, capacity, and multi-camera events; attach rollback; connect/open/format/control discovery and rollback; disconnect/reconnect/shutdown ownership; mode and control changes; all supported image paths and deterministic payloads; exposure and streaming success/errors/timeouts; abort/disconnect/removal races; serialization/no calls after close or detach; and normalized call/property traces.
- Cooling, guider, wheel/focuser/rotator, ROI, binning, frame-type-specific SDK behavior, and device-side configuration persistence are not implemented and are non-applicable. Framework-owned encoding/upload/config semantics need only integration checks where the driver supplies data.

### Risks and source-audit findings

- Frame waits are blocking loops on a device timer callback and may delay abort/disconnect/removal. Migration must use a handler/finalizer pattern with bounded checks while preserving libuvc call ordering where hardware requires it.
- The return from `uvc_get_device_descriptor()` is not assigned to `res`; descriptor failure can therefore be interpreted using the preceding list result and an invalid descriptor may be dereferenced.
- Repeated arrival events can create duplicate logical devices because there is no duplicate check.
- Arrival at capacity allocates a private/device pair but neither attaches nor releases it.
- Device-reference ownership is implicit and appears unbalanced on removal/shutdown; fake tests must establish the actual list/unref contract before changing it.
- Streaming sets `CCD_EXPOSURE_PROPERTY` rather than `CCD_STREAMING_PROPERTY` to OK after processing frames, and the RGB conversion failure branch does not free its allocated conversion frame.
- Exposure/streaming setup overwrites earlier UVC errors with later control writes, so a failed AE/exposure/gain operation may still start streaming if a later call succeeds.
- Stream open/start failure paths need explicit ownership checks to ensure a partially opened stream is closed exactly once.
- The image buffer size uses six bytes per maximum pixel, which is conservative but its geometry and overflow assumptions need fake coverage.

## Found defects

The source-audit findings were first reproduced against the original driver, then fixed in the generated implementation. UVC-007 was added when the expanded generated-driver suite exposed an additional short-frame validation gap.

| ID | Status | Observable impact | Root cause | Planned regression |
| --- | --- | --- | --- | --- |
| UVC-001 | Fixed and covered | Descriptor-query failure may crash or attach corrupt identity data. | `uvc_get_device_descriptor()` return value was ignored and stale `res` was checked. | The generated SDK plug block checks the descriptor result and frees it only when acquired; `Descriptor failure does not attach` passes with balanced references. |
| UVC-002 | Fixed and covered | Duplicate arrival or capacity overflow can duplicate devices or leak allocations/references. | Arrival lacked duplicate/capacity rollback ownership. | Generated discovery rejects an already attached bus/address and rolls back capacity failures; `Hotplug multiple duplicate capacity and recovery` and `Duplicate arrival is ignored` pass with balanced counters. |
| UVC-003 | Fixed and covered | A failed acquisition control write can be masked and acquisition can start with stale settings. | One `res` variable was overwritten unconditionally by later writes. | Acquisition setup now short-circuits every required control write; `Acquisition control failures and values` plus `Control failure blocks stream start` pass. |
| UVC-004 | Fixed and covered | RGB conversion failure leaks the conversion frame during streaming. | `uvc_free_frame()` was inside only the streaming success branch. | Conversion-frame cleanup now runs on success and failure; `Conversion failure frees frame` and the resource-balance case pass. |
| UVC-005 | Fixed and covered | Streaming frame processing updates the wrong property internally. | Successful streaming branches assigned `CCD_EXPOSURE_PROPERTY->state`. | The finalizer owns and updates the active property; finite/indefinite streaming cases verify streaming completion without exposure-state mutation. |
| UVC-006 | Fixed and covered | Abort, disconnect, and unplug may be delayed or race a blocking frame loop. | A long blocking timer callback owned polling and stream teardown. | A queued start handler plus bounded one-millisecond finalizer checks replaced the blocking loop; deterministic abort/disconnect/removal cases pass and assert no SDK call after close/detach. |
| UVC-007 | Fixed and covered | A short RGB/YUYV/UYVY frame can reach the converter and be treated as a valid image. | The original driver did not validate `frame->data_bytes` before native copies or conversion. | Native and converted paths validate the required byte count; `Short frame and watchdog recovery` passes and verifies a following acquisition succeeds. |
| UVC-008 | Current upstream fix applied; hardware rerun required | Root-run macOS discovery and interface claims succeed, but CONNECT intermittently rejects the first advertised YUY2 mode as `Invalid mode`, including immediately after a physical reset. | Bundled libuvc 0.0.6 requires format, frame and maximum payload size to match. SV205 does not return a stable payload value across negotiations. Upstream first relaxed the payload comparison, then removed it entirely; current upstream validates only format and frame indices. | The bundled source now matches current upstream and validates format/frame without comparing the transport-dependent payload value. Earlier intermediate `required >= actual` behavior connected on some runs but still failed after a physical reset, proving both payload directions must be accepted. |
| UVC-009 | Fixed and covered; behavior confirmed on SV205 | A libuvc wake-up without a completed frame is reported as an immediate generic readout failure instead of continuing until the watchdog deadline. | Both bundled libuvc and current upstream can return `UVC_SUCCESS` with `*frame == NULL` after their condition variable is signalled without advancing the completed-frame sequence, including transfer-error wake-ups. The verbose SV205 run showed exactly this sequence after a macOS bulk-transfer error. | Exposure and streaming finalizers now retry `UVC_SUCCESS` with no frame until their existing deadline. `Empty frame wakeup retries` injects this exact result and passes; the SV205 trace confirms the driver continued polling instead of failing immediately. |
| UVC-010 | Fixed and covered; confirmed on SV205 | A macOS bulk transfer reports `device not responding`; bundled libuvc permanently removes that transfer. With enough failures every transfer disappears and acquisition times out. Direct post-reset 320x180 runs prove the failure is intrinsic to that profile, not a 640x480-to-320x180 transition or exposure duration. | Bundled libuvc submits 100 concurrent transfer buffers on every platform and permanently removes a buffer on every generic transfer error. The verbose five-buffer retry trace proves the first request returns `0xe00002ed`; its immediate resubmit then fails synchronously because the Darwin pipe is stalled (`0xe000404f`), while the other four requests remain pending. A one-request recovery prototype fixed low resolutions but could not sustain the callback rate required above 1280x720. | macOS and upstream isochronous streams use five requests; Linux retains 100. macOS bulk streams now use five requests and recover transactionally: after an error they cancel all outstanding requests, wait until none is active, clear the stalled endpoint, and resubmit the set. Stop/abort tracks submitted requests under the stream mutex and frees dormant recovery requests only after callbacks quiesce. The complete SV205 run delivered every advertised YUY2 mode through 3264x2448, then completed exposure, streaming, abort, reconnect and driver reload. |
| UVC-011 | Fixed; confirmed on SV205 and Live! Cam Sync HD | On Linux a bulk-transport camera delivers frames only for the first acquisition after connect. Every later exposure times out, with or without a mode change; the hardware test failed at its second exposure, which happens to be the 320x180 mode switch. | A bulk streaming endpoint keeps the halt condition and data toggle it was left with when the previous stream was torn down, so the transfers of the next `uvc_stream_start()` are submitted and complete while the device sends nothing. Bundled libuvc cleared the endpoint only in its macOS transfer-error recovery path, never on a normal start. | `uvc_stream_start()` issues `libusb_clear_halt()` on the bulk endpoint before queueing the first transfer. A standalone three-cycle libuvc probe reproduced the silence on cycles two and three, stayed silent with a two-second settle and passed with the endpoint cleared. The SV205 hardware run now delivers all twelve advertised YUY2 modes through 3264x2448 plus streaming, abort, reconnect and driver reload; the isochronous Live! Cam Sync HD is unaffected. |
| UVC-011 | Fixed and covered; confirmed on SV205 | Aborting indefinite streaming successfully stops frame delivery but reports `CCD_ABORT_EXPOSURE` as ALERT. | The abort handler manually changed `CCD_STREAMING` from BUSY to OK before calling `indigo_ccd_abort_exposure_cleanup()`. The common helper therefore saw no active exposure or stream and classified the abort request as invalid. | The driver now finalizes video output while leaving the BUSY state for the common helper to resolve. `Finite and indefinite streaming` asserts that both the stream and abort property finish OK; the targeted SV205 acceptance run completed successfully through the same abort path. |
| UVC-012 | Fixed and covered; confirmed on SV205 | A transient short frame after macOS bulk recovery immediately fails the exposure even though later frames can be complete. | The driver validated frame length only inside the terminal processing step and treated every validation failure as fatal. The verbose 1600x1200 run received an EOF-marked 35828-byte frame where 3840000 bytes were required; high-resolution mode changes produced further incomplete frames before a valid frame arrived. | Exposure and streaming finalizers now discard frames with stale geometry or insufficient native source data and continue polling for up to the bounded 15-second readout allowance. Conversion/allocation/SDK failures remain fatal. `Short frame and watchdog recovery` covers short-then-valid success, persistent-short timeout and subsequent recovery; the complete SV205 run passed all modes. |

## Atomic plan and progress

1. **Complete — Audit and original build baseline.** Read governing instructions, driver/libuvc sources and documentation, generator examples, property inventory, status/build/project integration. The unchanged driver build succeeded on macOS arm64; evidence is below.
2. **Complete — Fake libuvc/libusb boundary.** Added deterministic fake devices, four representative formats, optional controls, frames, injected errors, resource counters, call trace, and bounded frame-hold behavior without changing production code. The fake is implemented in the conventionally named `test_ccd_uvc_sdk.c` test source and linked against the separately compiled production driver.
3. **Complete — Original-driver characterization suite.** Added and registered 12 passing preservation cases covering the original property contract, lifecycle/reconnect/rejected shutdown, modes and controls, mono/raw/RGB images, finite/indefinite streaming and abort, open/mode/stream/frame failures and recovery, active-stream disconnect, multi-camera/capacity/recovery, removal/replug, initialization/registration failures, and trace order. Four separately invoked known-defect reproducers fail on the original driver as intended and are not counted as passing baseline behavior.
4. **Complete — Original reference trace.** Captured the normalized ordered public-property/libuvc/libusb trace in `indigo_test/fixtures/ccd_uvc/original_reference_trace.txt`; the test compares it byte-for-byte before teardown. It includes connect discovery, arguments, control ordering, timeout retry, conversion allocation/free, stream close, and property BUSY/OK transitions.
5. **Unavailable — Original-driver hardware baseline.** `system_profiler` and IORegistry did not show an SVBONY/SV205 camera attached on 2026-09-16, so the hardware baseline cannot run in the current state. Per the requested workflow, hardware execution is deferred until the end when the SV205 is attached; this absence does not waive the completed fake-SDK contract.
6. **Complete — Generator source migration.** Added `indigo_ccd_uvc.driver` as source of truth, moved the driver to API 3 and incremented its revision from 15 to 25 (`0x03000019`), generated `.c`, `.h`, and `_main.c`, and retained the public property contract. A small local libuvc adapter avoids the required generated `uvc_open(indigo_device *)` / `uvc_close(indigo_device *)` helper names colliding with libuvc's ABI names; the generator was not changed. The bundled libuvc stream negotiation and macOS bulk-transfer handling were updated for UVC-008 and UVC-010.
7. **Complete — Async/lifecycle hardening.** Acquisition now uses a queued start handler and a bounded finalizer with explicit watchdog, abort, disconnect, and removal ownership. All hardware-free defects have passing regressions, resource balances, and no-call-after-close/detach assertions. The generated reference trace records the intentional queue/property ordering changes while retaining device command order.
8. **Complete — Repository integration.** Registered the generator source, adapter, fake test, physical-camera harness, and traces in Xcode and the test targets in the Makefile; changed only the `ccd_uvc` status fields/count in `MIGRATION_STATUS.md`, preserving its Comment; and changed the `PROPERTIES.md` source note to the authoritative `.driver` without changing the property inventory. The one-case hardware harness discovers the physical camera's actual modes and optional controls at runtime and remains opt-in, excluded from normal integration runs.
9. **Complete — Final automated verification.** Explicit regeneration produced identical hashes for `.c`, `.h`, and `_main.c`; `make -f ../../Makefile.drv` passed for the universal macOS arm64/x86_64 build; all 20 fake cases passed normally with the generated source compiled using Clang's unreachable-code warning as an error; all 20 passed with the generated driver object instrumented by ASan/UBSan; and the four original defect reproducers passed separately. The opt-in physical-camera harness compiled successfully. `plutil -lint indigo.xcodeproj/project.pbxproj` and `git diff --check` passed. `make -C indigo_test test-clean` removed test binaries afterward.
10. **Complete — Physical SV205 acceptance.** Root traces prove libusb opened the device and claimed UVC control interface 0 and streaming interface 1; no `UVCAssistant`/exclusive-access failure occurred. Diagnostic and targeted runs isolated unstable payload negotiation, transient Darwin bulk failure, a subsequently stalled pipe, abort-property ordering, incomplete frames, insufficient single-request throughput and a stop/recovery race. The final unfiltered run delivered all 12 advertised YUY2 modes from 320x180 through 3264x2448, restored 640x480, exercised controls, 0.1- and 1-second exposures, exposure abort/reacquire, five-frame and indefinite streaming with abort, reconnect, driver shutdown/reinitialization and a fresh exposure. Physical unplug/replug remains optional because disconnect/reconnect, active-removal recovery and hot-plug capacity are deterministically covered by the fake SDK.

## Baseline evidence

- Host: macOS 26.6.2 (25G83), Darwin 25.6.0, arm64 Apple Silicon. The normal build is a universal arm64/x86_64 build as configured by the repository.
- Unchanged production build: `make -C indigo_drivers/ccd_uvc -f ../../Makefile.drv` — passed, exit 0. Existing build products were already current, and the target reported the original C/libuvc sources, archive, dylib, executable, and rules file.
- Original-driver fake build/run: `make -C indigo_test test-ccd-uvc-sdk` — 12 cases run, 12 passed.
- Expected-failure reproducers on the original driver:
  - `indigo_test/build/integration/test_ccd_uvc_sdk --known-defects Descriptor` — failed as expected (`attached` 1, expected 0).
  - `... --known-defects Control` — failed as expected because exposure reached success instead of ALERT.
  - `... --known-defects Conversion` — failed as expected (`allocated_frames` 1, `freed_frames` 0).
  - `... --known-defects Duplicate` — failed as expected (`attached` 2, expected 1).

## Scenario-to-test mapping

- `Metadata and property contract`: INFO/interface, connected property inventory, visibility, counts, permissions, mode count, geometry, and exposure limits.
- `Lifecycle reconnect and rejected shutdown`: single open/close ownership, connected shutdown rejection, disconnect and reconnect.
- `Mode controls and call order`: mode parsing/geometry/depth, gain/gamma accepted values, and stream-control selection.
- `Mono8 mono16 raw8 and RGB images`: all representative native/conversion branches, BLOB validity, and conversion allocation balance on success.
- `Finite and indefinite streaming`: exact finite count, indefinite start, cooperative abort, and stream close balance.
- `Open mode and stream failures recover`: open, stream-control, stream-open, stream-start failures and subsequent successful recovery.
- `Frame failure and reacquire`: frame read error, ALERT cleanup, and fresh successful acquisition.
- `Empty frame wakeup retries`: a libuvc success wake-up with no frame remains pending and a later complete frame succeeds.
- `Short frame and watchdog recovery`: one incomplete frame is discarded before a complete frame succeeds; persistent incomplete frames reach ALERT at the existing deadline; a following acquisition recovers.
- `Active stream abort and disconnect`: held-frame abort, disconnect, and no UVC calls after close.
- `Hotplug multiple duplicate capacity and recovery`: multiple cameras, capacity limit, removal, and capacity reuse. Duplicate-arrival correctness remains an expected-failure reproducer until fixed.
- `Active removal and replug`: disconnected physical removal and fresh arrival.
- `Removal during exposure and recovery`: deterministic active-exposure removal, cancellation, detach, replug, reconnect, and fresh acquisition.
- `Initialization and registration failures`: hot-plug registration rollback and a later successful INIT/SHUTDOWN cycle.
- `Initialization queue discovery and attach recovery`: queue creation, discovery, and attach failures each roll back and permit a later successful initialization.
- `Resource ownership balances`: final aggregate libuvc/libusb device, descriptor, stream, frame, handle, and attach/detach ownership checks.
- `Reference trace contract`: exact preserved public-state and SDK-call order.
- Dedicated expected-failure reproducers cover descriptor failure, control-write failure, conversion allocation leakage, and duplicate arrival. They will join the passing regression evidence after production fixes.
- Non-applicable capabilities remain cooling, guider, additional logical devices, ROI/binning, and device-side persistence. The driver advertises no such capability.

## Reference trace

Stored at `indigo_test/fixtures/ccd_uvc/original_reference_trace.txt` and verified by `Reference trace contract`. Nondeterministic addresses, timestamps, and generated unique suffixes are excluded; deterministic camera index, sizes, enum values, request codes, and property states remain.

## Hardware evidence

Decision: yes, SVBONY SV205. The one registered hardware case was run 22 times as root on 2026-09-16, with four complete targeted passes and one complete unfiltered pass. The staged runs exposed and verified negotiation, macOS bulk recovery, abort ordering, incomplete-frame handling, high-resolution throughput, stop/recovery synchronization and the bounded readout allowance. Targeted complete passes covered 320x180, 1280x720, 1600x1200 and 3264x2448. The final unfiltered pass produced 26 valid images: one in each of 12 advertised YUY2 modes through 3264x2448, followed by controls, short and long exposure, abort/reacquire, finite and indefinite streaming, reconnect and driver reload at restored 640x480. The old-driver log confirms only a 640x480 acquisition and contains no mode-change request.

The opt-in `test_ccd_uvc_hw` case is registered but excluded from normal tests. It selects one discovered UVC CCD (or `INDIGO_TEST_DEVICE`), validates every advertised mode with a RAW image and restores the initial mode, exercises writable gain/gamma and restores their initial values, acquires short and up-to-1.5-second frames within the device range, aborts and reacquires, checks exact finite and indefinite streaming, reconnects, performs driver shutdown/reinitialization, and optionally prompts for a physical unplug/replug when invoked with `HW_HOTPLUG=1`. It passed completely on the SV205.

Local macOS service inspection identifies `system/com.apple.cmio.uvcassistantextension`, PID 762 at inspection time, executing `.../UVCAssistant.systemextension/Contents/MacOS/UVCAssistant`. Its launchd plist is `/System/Library/LaunchDaemons/com.apple.cmio.uvcassistantextension.plist`. Apple documents that macOS 12.3 and later install the default UVC CMIO extension as `com.apple.UVCService`; the supported long-term override is a device-specific DriverKit extension. The attempted `bootout` failed with error 5 and made no service-state change. The subsequent root trace shows that the linked libusb 1.0.29 whole-device capture path nevertheless opened the SV205 and both required UVC interfaces, so no launchd disable/kill workaround is needed for this camera/test. The older claim that detach is categorically unavailable on macOS is incomplete for this build.

## Final test summary

- Simulated/fake-SDK tests: 44 run, 44 passed on the final generated driver (20 normal, the same 20 with the generated driver object instrumented by ASan/UBSan, and 4 dedicated defect-regression invocations). The registered suite contains 20 distinct cases. The original-driver characterization baseline was separately 12/12, with four dedicated reproducers failing as expected before the fixes.
- Hardware tests: 22 executions of the one registered case, 4 complete targeted passes and 1 complete unfiltered pass. The final SV205 run delivered 26 images, covered every advertised YUY2 mode through 3264x2448, controls, both exposure durations, abort/reacquire, finite and indefinite streaming, reconnect and driver reload. macOS device ownership did not block any root run.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard in `indigo_ccd_uvc.driver` are now declared with the generator's `reject_change` block for `CCD_MODE`. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values.

The guard returned `INDIGO_OK` without any update, so a mode change requested during acquisition was dropped silently and the client kept showing a mode the camera never switched to.

Covered by `Rejected mode change during exposure` in `indigo_test/integration/test_ccd_uvc_sdk.c`.

```sh
cd indigo_test && ./build/integration/test_ccd_uvc_sdk
```
