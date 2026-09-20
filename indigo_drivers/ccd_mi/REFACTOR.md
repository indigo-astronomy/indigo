# INDIGO 3.0 refactoring record for `ccd_mi`

## Goal and scope

Migrate the hand-written Moravian Instruments camera driver to `indigo_generator`, make the generated `.driver` definition the source of truth, move device work to INDIGO handler queues, and add comprehensive hardware-free integration coverage through a deterministic fake `gxccd` SDK. Preserve the existing CCD, ST-4 guider and internal filter-wheel behavior unless a documented defect requires a tested correction.

The supported implementation and validation scope is macOS and Linux. Windows is explicitly out of scope because no compatible Moravian Instruments Windows SDK is available; Windows must remain recorded as unverified rather than inferred from the Unix SDK API.

## References read

- Root `AGENTS.md` and `indigo_drivers/AGENTS.override.md`.
- `README.md` build requirements.
- `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` and the relevant lifecycle, property, queue and CCD sections of `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`.
- `indigo_test/AGENTS.md` and the CCD, guider and wheel acceptance standards in `indigo_test/DRIVER_TESTING_RULES.md`.
- Driver source and public header, bundled `gxccd.h`, bundled SDK README, driver README, build integration, Xcode project entries, `MIGRATION_STATUS.md` and the `ccd_mi` section of `indigo_docs/PROPERTIES.md`.
- Existing generated SDK drivers and fake-SDK suites will be used as implementation references, especially ASI, Player One and ToupTek camera tests and ASI wheel tests.

## Baseline audit

### Repository and environment

- Baseline revision: `e3bcc374bcc5db58dc7a269657fc469fbe2d0998` with a working tree that contained pre-existing build products in the driver directory; no pre-existing tracked `ccd_mi` refactor or test changes were found.
- Host: macOS 26.6.2 (Darwin 25.6.0), Apple Silicon arm64; Apple clang 21.0.0.
- Current driver version: `0x0200001D`.
- Driver is in the stable Unix driver list. The bundled SDK contains macOS universal and Linux x86, x64, ARM and ARM64 static libraries. The driver README declares Linux and macOS support. Windows is unavailable because its SDK is not present and is intentionally excluded from this work.

### Architecture and lifecycle

- The current implementation is a 920-line hand-written C driver with hand-written public header and executable main; no `.driver` input exists.
- USB hot-plug is registered for vendor id `0x1347`. A libusb arrival triggers asynchronous SDK enumeration, probing and attachment. One physical camera may expose a CCD master plus optional guider and internal-wheel logical devices sharing one `mi_private_data` and one `camera_t *` handle.
- Shared open ownership is tracked by `device_count`; the first logical connection acquires the INDIGO global lock and calls `gxccd_initialize_usb()`, and the last disconnection calls `gxccd_release()` and releases the lock.
- Physical identity currently combines the libusb bus/address saved from the arrival with a newly discovered SDK id selected through global `new_eid` state. The logical-device registry has ten total slots, so a fully featured physical camera consumes three slots.
- CCD temperature and cooler-power polling use recurring timers. Exposure completion uses a timer that enters a synchronous `gxccd_image_ready()` polling loop. Guider completion uses one timer shared by both axes. Wheel movement is reported complete immediately after `gxccd_set_filter()`.

### Public properties and capabilities

- No driver-specific custom properties are declared. The public surface consists of inherited CCD, guider and wheel properties, so the `X_` custom-property prefix rule is currently not applicable.
- CCD always provides 16-bit RAW acquisition and dynamically configures sensor geometry, pixel size, exposure limits, bin limits and equal-bin `CCD_MODE` items from SDK capabilities.
- Optional CCD branches include multiple read modes, cooler enable/target, cooler power, writable gain and electronic gain readback. Temperature is exposed even for uncooled cameras as read-only.
- Frame types map light/flat to shutter use and bias/dark/dark-flat to closed-shutter acquisition. Bias uses the framework shortest-exposure helper.
- Optional guider maps north/south to signed DEC milliseconds and east/west to signed RA milliseconds through `gxccd_move_telescope()`.
- Optional internal wheel exposes the SDK filter count and maps the public one-based slot to the SDK zero-based index.
- The properties reference already has a `ccd_mi` section sourced from `indigo_ccd_mi.c`; it lists no custom properties. It will need only a source note update if ownership moves to `.driver`, unless the property inventory changes.

### SDK contract and error handling

- The bundled `gxccd.h` documents `0` success and `-1` failure for parameter, acquisition, cooler, gain, guiding and wheel calls. Camera initialization returns a handle or `NULL`, and release invalidates the handle.
- The current code checks only some acquisition and polling results. Most connection-time parameter reads, cooler writes, gain writes, guide commands and wheel commands ignore return values, allowing invalid outputs or SDK failures to be published as success.
- The SDK supports capability queries for subframe, shutter, cooler, wheel, guider, asymmetric binning, power utilization, gain, Bayer layouts and other features. The current driver consumes only the subset listed above.

### Existing test coverage

- There is no `test_ccd_mi_sdk` source, target, fake SDK or opt-in hardware test.
- Baseline relevant automated cases: 0 hardware-free, 0 hardware.
- Baseline build command on 2026-09-14: `make -B -C indigo_drivers/ccd_mi -f ../../Makefile.drv all`.
- Baseline result: PASS for the archive, dylib and executable, each built as x86_64/arm64. There were no C compiler diagnostics. The linker emitted pre-existing warnings that x86_64 objects in bundled `libgxccd.a` target macOS 10.12 while the driver link target is macOS 10.10.
- No baseline driver-specific tests were available to run.

### Initial risks and candidate defects from source audit

- Exposure completion can remain indefinitely inside a tight 200-microsecond readiness loop when the SDK never reports ready; this blocks its callback context and provides no watchdog or bounded recovery.
- One guider timer is shared by RA and DEC, so starting a pulse on one axis cancels completion tracking for the other. The callback also completes both axes together, which can shorten or extend public pulse state incorrectly.
- Connection initialization uses outputs from unchecked SDK reads, risking undefined geometry, limits, read modes or capability state and incomplete rollback.
- Cooler, gain, guiding and wheel setters commonly publish OK without checking SDK failure.
- The wheel publishes completion immediately and never confirms device position because the SDK header exposes no filter-position getter; this limitation must be represented honestly and set-command failures must still surface.
- Hot-plug association relies on a global last-unseen SDK id and can associate the wrong SDK camera with a libusb event when enumeration order differs from event order or multiple arrivals overlap.
- Registry capacity is counted in logical devices; partial attachment or capacity exhaustion can leave inconsistent physical-device topology or allocations.
- Disconnect/removal races with exposure, polling and guider timers may access a released shared SDK handle or detached device unless cancellation and queue ownership are made explicit.

These are audit findings, not yet reproduced defects. They will move into the found-defects table only after the exact root cause, fix and regression test are established.

## Hardware-test result

The opt-in acceptance run used a physically connected Moravian Instruments G0-0300 (USB `1347:0412`). It exposed a 656 x 494, 16-bit, 7.4 um sensor, one 1x1 bin mode, read modes 0 and 1, a read-only temperature, and an ST-4 guider. Cooler control, gain, bins above 1x1 and an internal wheel were absent and are therefore not applicable to this camera; those branches remain covered by fake profiles. The camera was returned to read mode 1, full-frame, light-frame settings and disconnected before shutdown.

Fourteen hardware scenarios passed: identity/capability inventory; guider-first shared connection; full-frame light acquisition and a 653760-byte RAW16 FITS payload; ROI/read-mode-0 dark acquisition; minimum-duration bias acquisition; long-exposure abort and reacquisition; restoration of read mode 1/full frame/light; all four guider directions; overlapping RA/DEC pulses and zero requests; guiding during acquisition; both shared disconnect/reconnect orders; clean driver unload/reload; idle unplug/replug followed by acquisition and guiding; and unplug during a 600-second exposure followed by replug, a 50 ms exposure, a 50 ms DEC pulse and clean shutdown. The captured full-frame FITS reported `NAXIS1=656`, `NAXIS2=494`, `BITPIX=16` and `EXPTIME=.2`.

## Atomic plan

1. **Baseline, audit and mandatory record — complete.** Read the governing documents and SDK contract; inventory architecture, properties, capabilities, lifecycle and coverage; build the untouched driver; record platform, warnings, hardware decision and this plan. Evidence is recorded above.
2. **Study generator and reference-driver patterns — complete.** Inspected the generator migration contract, generated Player One/ASI/DSI/QHY CCD patterns, optional guider attachment, generated shared `count` ownership, SDK discovery retries and `unplug_match`, independent guider finalizers, bounded acquisition finalizers, and existing fake-SDK build patterns. The generator already supplies a serialized per-driver queue for SDK discovery and all logical-device connection changes, so no generator change is needed. Chosen mapping: SDK hot-plug with enumeration-based id ownership; mandatory CCD plus conditional guider/wheel; transactional `mi_open()`/`mi_close()`; handler/finalizer acquisition; periodic CCD poll handler; independent guider finalizers. A temporary unmodified-source extraction confirmed metadata, version, multi-device detection and inherited-property discovery. Because the hand-written lifecycle intermixes callbacks, timers and hot-plug state and declares no custom properties, completing the extracted skeleton directly is safer and clearer than retaining old boilerplate through extensive annotations; this is an intentional deviation from the initial annotation wording, not a generator change.
3. **Extract and complete the `.driver` definition — complete.** Ran `../../build/bin/indigo_generator -c indigo_ccd_mi.driver` with the required output-path form, completed the source-of-truth definition, and regenerated all three checked-in outputs without modifying the generator.
4. **Implement robust shared lifecycle and discovery — complete.** The generated SDK hot-plug path now owns serialized arrival/removal and connection work. Enumeration selects an unattached SDK id, probes identity and optional interfaces transactionally, checks total logical-device capacity before attachment, retries failed arrival probes, and matches removal by SDK identity. Shared open count increments only after a successful global-lock/SDK-open transaction. Fake cases cover connection order, rollback, attach retry, duplicate arrival, capacity, removal/replug and resource balance.
5. **Migrate CCD acquisition and polling — complete.** Acquisition uses the generated handler queue, `indigo_ccd_exposure_setup()`, exact requested seconds and a reusable private buffer. A monotonic finalizer polls readiness without blocking the queue and enforces a 30-second readout watchdog. Abort, disconnect and unplug cancel/clean up acquisition deterministically. The image test verifies RAW16 geometry and every deterministic pixel.
6. **Migrate controls and optional capabilities — complete.** Connection-time core geometry and binning results are mandatory and validated. Exposure limits, read-mode metadata and capability probes are optional: failed probes retain safe framework defaults, expose conservative capabilities or use generic read-mode labels. Read mode, gain/e-gain, cooler target, temperature and cooler-power results propagate operational errors and recover. Optional controls are hidden or read-only according to SDK capability flags, and recurring polling stops before handle release.
7. **Migrate guider and wheel behavior — complete.** RA and DEC have independent completion finalizers; overlapping axes preserve the other axis's remaining signed duration. SDK failures become ALERT, zero is handled, and the public pulse range is capped at the SDK's `int16_t` limit. Wheel commands use one-based/public to zero-based/SDK mapping, suppress an already-selected write and propagate set failures. The SDK has no wheel-position getter, so successful set-command completion remains acknowledgement-based rather than claimed readback.
8. **Regenerate and integrate repository outputs — complete.** Driver version is now `0x0300001E`, higher than baseline `0x0200001D`. Regenerated `.c`, `.h` and `_main.c`, added the `.driver`, `REFACTOR.md` and fake test to the Xcode groups, changed the properties-reference source note to `.driver`, and validated the project with `plutil -lint`. Two consecutive generations produced identical SHA-256 hashes for all outputs.
9. **Create the fake `gxccd` SDK integration suite — complete.** Added `integration/test_ccd_mi_sdk.c` and narrow normal/sanitizer Makefile targets. The production generated driver is compiled as a separate object against replacement hooks, and the executable supplies the fake `gxccd` ABI without linking the vendor library. Each fixture uses a fresh in-process bus and deterministic RAW16 noise.
10. **Complete capability and lifecycle coverage — complete.** Seventeen registered cases cover CCD-only and combined CCD/guider/wheel profiles, supported and unsupported capability sets, metadata/property schemas, connection order and reconnect, lock/open/parameter/probe/attach/queue/registration failures, repeated INIT/SHUTDOWN, connected-shutdown rejection, duplicate/multiple arrivals, capacity, removal and replug. Resource counters assert no duplicate release, leaked open handle, lock imbalance, invalid USB unref or SDK call after close.
11. **Complete CCD/control/failure/race coverage — complete.** Tests cover full frame and ROI, equal/asymmetric bins, dynamic read modes, light/dark/flat/dark-flat/bias shutter mapping, exposure units, deterministic image payload, setup/start/ready/read/watchdog/abort failures and restart, busy overlap, read/disconnect and hot-unplug races, cooler/gain/e-gain/temperature/power failures and recovery, and polling cancellation.
12. **Complete guider/wheel and timing coverage — complete.** Tests cover all four signed directions, representative 20/100/500 ms durations, zero, cross-axis overlap, SDK failure, connection ownership, pulses during acquisition, wheel boundary slots, already-selected no-op, invalid wheel capability and set failure. Same-axis replacement is deliberately rejected by the standard BUSY guard rather than supported. Timing endpoints are fake SDK function entry and the public guider property transition to OK; they measure host-side software completion, not electrical relay accuracy. The final normal run discarded one warm-up per direction and collected 32 samples (two repeats for each idle direction/duration plus two active-acquisition repeats per direction at 100 ms): signed error min `+0.182 ms`, mean `+3.774 ms`, median `+4.722 ms`, p95 `+5.051 ms`, p99/max `+5.057 ms`, standard deviation `1.593 ms`, maximum absolute error `5.057 ms`; percentage error min `+0.332%`, mean `+7.336%`, median `+3.635%`, p95 `+25.226%`, p99/max `+25.254%`, standard deviation `9.358%`, maximum absolute `25.254%`. Actual duration is requested duration plus the signed error.
13. **Strict and sanitizer verification — complete for the available host.** The universal macOS driver build and normal fake suite pass. The arm64 ASan+UBSan target passes with leak detection disabled because this run validates driver/test memory and undefined behavior while INDIGO process-global allocations are outside the fixture. A `-Wall -Wextra -Werror` syntax build passes after suppressing only unused callback parameters/static helpers from generated or shared harness code, the generator-owned incompatible callback cast and intentional fake-hook macro redefinitions; driver/test-specific sign warnings found by the first strict run were fixed. `git diff --check`, generated-version/source checks and Xcode plist validation pass. Linux was not available in this environment. Windows was intentionally not attempted because the compatible SDK is absent.
14. **Reconcile migration and coverage records — complete.** `MIGRATION_STATUS.md` records API 3, generated/queued implementation, hardware retesting and `17 / 0` automated tests while preserving the Comment column exactly. The hardware column remains zero because the 14 physical scenarios were manual rather than persistent opt-in automated cases. `PROPERTIES.md` points at the `.driver` source, and all new persistent files are registered in Xcode.
15. **Run opt-in G0-0300 hardware acceptance — complete.** All 14 capability-driven scenarios described above passed. Settings were restored, both logical devices were disconnected, and the final server shutdown deregistered hot-plug, detached both devices and unloaded the driver cleanly.
16. **Add `reject_change` guards and a persistent hardware regression test — complete.** Driver version is now `0x0300001F`. `CCD_READ_MODE`, `CCD_GAIN` and `CCD_BIN` declare `reject_change { condition = CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE; message = "Acquisition in progress"; }`, so a client change to an acquisition parameter is refused with the driver-side values forced back instead of being applied to a camera that has already been programmed for the running exposure. `CCD_FRAME` and `CCD_MODE` are framework owned in this driver and have no generated change branch, so the generator cannot attach a guard to them; adding one would mean re-implementing the base-class geometry logic in the driver and is deliberately not done. `indigo_test/hardware/test_ccd_mi_hw.c` and the `test-ccd-mi-hw` target turn the previously manual physical scenarios into one opt-in automated case, excluding unplug/replug so the test can run unattended.

## Found defects

- **Unbounded exposure-ready loop — reproduced in the fake SDK and fixed.** Impact: a camera that never became ready could retain BUSY forever and block useful callback work. Root cause: the old completion callback spun on `gxccd_image_ready()` without a watchdog. Fix: monotonic, one-poll-per-finalizer readiness checks with a 30-second deadline, abort and failure cleanup. Regression: `exposure errors, pending readout, watchdog and recovery`.
- **Shared guider completion timer — source-audit defect fixed and behavior covered.** Impact: a pulse on one axis cancelled or shared completion with the other, publishing incorrect per-axis completion. Root cause: one timer stored in shared private data completed both axes. Fix: independent RA/DEC end times and finalizers which preserve the other axis in each SDK command. Regression: `guider directions, overlap, failure and timing`.
- **Unchecked SDK results and optional-query contract — reproduced and fixed.** Impact: the hand-written driver could consume stale outputs and report failed operations as successful; conversely, initially treating every query as mandatory rejected a real G0-0300 because it returns `Not implemented for this camera` for `GIP_MAXIMAL_EXPOSURE`. Root cause: after `gxccd_initialize_usb()` the old driver checked none of its parameter-query return values, so camera open was its only de facto mandatory connection operation. Fix: require the core geometry inputs `GIP_CHIP_W`, `GIP_CHIP_D`, `GIP_PIXEL_W`, `GIP_PIXEL_D`, `GIP_MAX_BINNING_X` and `GIP_MAX_BINNING_Y`; treat exposure limits, read-mode metadata and capability probes as optional with safe defaults; and validate supported operational commands/readbacks. Regressions: `connection failures roll back and recover`, `unsupported capability profile`, `optional probe defaults and master attach retry`, `exposure errors, pending readout, watchdog and recovery`, `cooler, gain and polling errors and recovery`, `guider directions, overlap, failure and timing`, and `wheel slots, failures and shared ownership`.
- **Guider duration narrowing — source-audit defect fixed.** Impact: accepted values above 32767 ms overflowed the SDK's signed 16-bit axis argument and could change duration or direction. Root cause: inherited guider maxima exceeded the `int16_t` transport type. Fix: publish `INT16_MAX` maxima for all four direction items. Regression: `metadata and CCD/guider/wheel property profiles` verifies both axis limits.
- **Partial hot-plug topology and ambiguous shared ownership — reproduced and fixed.** Impact: capacity or attach failures could leave only part of a physical camera's logical topology, and lifecycle rollback was difficult to reason about. Root cause: hand-written attachment and global discovery state mixed SDK identity, libusb events and logical-device slots. Fix: generated per-driver queue, per-physical private data, preflight slot count, conditional child attachment and generator-owned shared reference count. Regressions: `optional probe defaults and master attach retry`, `hotplug duplicate, removal, replug and capacity`, `shared connection orders and reconnect`, and `repeated INIT/SHUTDOWN and resource balance`.
- **Disconnect/removal races — reproduced and fixed.** Impact: pending exposure/poll/guide work could race handle release. Root cause: timers and SDK lifetime were not uniformly serialized or cancelled. Fix: generator queues, master-device locking, named finalizers and deterministic disconnect/unplug cleanup. Regressions: `abort, overlap rejection and disconnect/read race`, `hot unplug during exposure and recovery`, and all resource-balance assertions.

## Coverage map

| Registered fake-SDK case | Covered scenarios |
| --- | --- |
| `metadata and CCD/guider/wheel property profiles` | Driver metadata/version/multi-device flag, all three logical interfaces, required property/item presence and SDK-safe guider maxima. |
| `shared connection orders and reconnect` | Guider-first/CCD/wheel shared open, disconnect order, one SDK handle, last-close release and reconnect. |
| `connection failures roll back and recover` | Global-lock, SDK-open, essential integer parameter and read-mode enumeration failures; clean retry and balanced ownership. |
| `exposure geometry, frame types and deterministic pixels` | RAW16 full frame/ROI/bin geometry, SDK units, light/flat versus dark/dark-flat/bias shutter mapping, deterministic complete pixels. |
| `exposure errors, pending readout, watchdog and recovery` | Binning/start/readiness/read failures, delayed readiness, watchdog abort and successful acquisition after every failure. |
| `abort, overlap rejection and disconnect/read race` | Standard BUSY rejection, abort success/failure, restart, disconnect blocked on an in-flight read, and no call after close. |
| `read mode, binning and control failures` | Read-mode mapping/rollback, symmetric-bin rejection and asymmetric capability acceptance. |
| `cooler, gain and polling errors and recovery` | Temperature/cooler/gain/e-gain writes/readback, target convergence, poll errors/recovery and timer cancellation. |
| `guider directions, overlap, failure and timing` | East/west/north/south signed SDK values, zero, independent cross-axis overlap, command failure and required software timing report. |
| `guider during acquisition and shared teardown` | Guide pulse while CCD acquisition is active, logical-device disconnect order and SDK serialization. |
| `wheel slots, failures and shared ownership` | One-based/zero-based mapping, boundaries, selected-slot no-op, set failure/value preservation, zero-filter connection failure/recovery and shared lifetime. |
| `hotplug duplicate, removal, replug and capacity` | Duplicate arrival suppression, SDK-identity removal/replug, multiple non-contiguous ids and all-or-nothing logical-slot capacity. |
| `unsupported capability profile` | CCD-only camera whose optional maximum-exposure query and capability probes fail; connection succeeds with framework defaults, read-only temperature and hidden optional properties. |
| `optional probe defaults and master attach retry` | Failed optional guider probe defaults to absent while the CCD/wheel topology attaches; removal/replug plus CCD attach failure verifies generated retry and clean eventual topology. |
| `hot unplug during exposure and recovery` | Removal during active exposure, cancellation/release/no late SDK calls, replug and subsequent exposure. |
| `queue/registration failure and connected shutdown rejection` | Driver-queue and hot-plug registration rollback, retry, shutdown refusal while connected and continued operation. |
| `repeated INIT/SHUTDOWN and resource balance` | Three fresh bus/driver lifecycles and all handle/lock/USB/no-call-after-close counters balanced. |

The driver does not implement streaming, asymmetric-mode enumeration, Bayer/color conversion, external triggering or wheel position readback because the previous MI driver did not expose them and the bundled SDK contract does not provide enough portable behavior to add them in this scoped refactor. Framework-owned generic number validation and persistence are not duplicated in this driver suite. Electrical ST-4 pulse accuracy was not instrumented; the hardware run verified SDK acceptance and public state completion. The G0-0300 has no cooler, writable gain, multi-bin mode or wheel, so physical coverage of those capabilities is not applicable and their behavior remains fake-SDK-covered. Linux compilation/runtime was unavailable; Windows is not applicable because a compatible SDK is absent.

## Persistent hardware regression test

`make -C indigo_test test-ccd-mi-hw` runs `indigo_test/hardware/test_ccd_mi_hw.c` against a physically connected camera. It is opt-in, is excluded from `make -C indigo_test test`, and needs no operator interaction: hot-plug recovery stays covered by the fake gxccd suite, so no unplug/replug phase exists. `INDIGO_TEST_DEVICE` selects one camera when several are attached and `MI_HW_DEBUG` raises the log level.

One registered case drives, in order: discovery and a capability inventory; guider-first shared connection; full-frame RAW16 light acquisition; one exposure per reported read mode; a 128 x 128 dark frame at 16,24; a minimum-duration bias frame; restoration of full-frame light settings; a 60-second exposure aborted and re-acquired without a stray image; the `reject_change` guards; all four guide directions, overlapping RA/DEC pulses, a zero-duration pulse and pulses during an active exposure; every filter slot when a wheel is present; camera-first and guider-first disconnect/reconnect with the guider still usable while the camera is disconnected; and driver `INDIGO_DRIVER_SHUTDOWN`/`INDIGO_DRIVER_INIT` followed by a fresh exposure. Every acquisition is checked for frame count, geometry and a valid RAW16 payload, and the run asserts no invalid frame was received.

The guard check starts a 10-second exposure and, for each guarded property, requests a value the property does not hold. It asserts the refusal state, that the driver-side values are unchanged, and that the `Acquisition in progress` message is delivered for that property; it then asserts the exposure still completes with its image and that the same properties are accepted again once the camera is idle. Capability-dependent guards report themselves as skipped rather than silently passing. The generated per-item `do_update` marking is not observable through the in-process client, only through a protocol adapter, so it stays covered by the generator architecture test.

Hardware result on 2026-09-18 with the G0-0300 (USB `1347:0412`): all scenarios passed. The camera reported 656 x 494, 16-bit, 7.4 um pixels, maximum binning 1x1, two read modes, a read-only temperature, no cooler, no writable gain, an ST-4 guider and no wheel. `CCD_READ_MODE` exercised the guard; `CCD_GAIN` is absent on this camera and `CCD_BIN` has a single legal value, so both were reported as skipped and remain covered by the fake gxccd suite.

## Final test summary

- Simulated/fake-SDK tests run: 17; passed: 17.
- Manual hardware scenarios run: 14; passed: 14.
- Persistent automated hardware tests run: 1; passed: 1.

## Overlapping guide pulses (2026-09-20)

A guide pulse requested while another pulse on the same axis was still running was silently
discarded: the generated change branch dispatched both guide properties through the BUSY-guarded
`INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE` macro, so the second request never reached the handler.
Both properties now declare `accept_while_busy = true` and zero both axis items in
`on_change_request`, and each handler drops the finaliser of the pulse it replaces.

Without that cancellation the superseded finaliser would clear `guider_ra_end` / `guider_ra_duration`
and publish OK in the middle of the new pulse. `remaining_pulse()` only ever covered the *other*
axis, which `gxccd_move_telescope()` requires in every call; it never covered a replacement on the
same axis.

`guider_directions_overlap_failure_and_timing` gained two duration-measuring cases: a 2000 ms pulse
replaced after 500 ms by a 600 ms pulse in the same direction (1130 ms measured) and by a 300 ms
pulse in the opposite direction (812 ms), the second also asserting the signed duration handed to
`gxccd_move_telescope()` turns negative.
