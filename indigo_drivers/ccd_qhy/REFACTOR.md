# Refactoring plan for INDIGO 3.0 QHY CCD drivers

Date: 2026-09-12.

Status: generated C++ implementation and dual-SDK software validation complete; physical acceptance is partial because of recorded SDK failures. Remaining hardware scenarios require another physical USB reset while the user is available. This is the shared plan for `ccd_qhy` and `ccd_qhy2`.

## Goal and scope

Migrate the shared QHY implementation to a single authoritative `.driver` source, retain the legacy/new SDK branches, and compile shared generated C++ directly (`cpp = true`, authorized later in this session). Preserve CCD, optional ST4 guider and camera-connected CFW capabilities. Replace handwritten property/lifecycle scaffolding with generator output and move acquisition and motion completion to bounded handler/finalizer operations.

Use the current generator defaults, without overriding `MAX_DEVICES`. Do not upgrade or edit vendor SDKs. Do not assume that the two SDKs have identical behavior because the driver source is shared. User-approved physical acceptance targets are QHY 5L-II with `ccd_qhy`, and QHY 5III 178 with `ccd_qhy2`; execute each SDK in a separate process. The drivers remain mutually exclusive.

The user approved the concrete C++ prerequisite on 2026-09-12: generated allocation casts, a `libusb_hotplug_event` cast and scoped INIT/SHUTDOWN case bodies. Preserve extraction of both old and new registration syntax. This approval does not cover unrelated generator lifecycle or DSL extensions; report another concrete limitation before changing those.

## References and baseline

Baseline commit: `fdacefa743d24669d7f33f9b6225115cbf7db730`. The workspace was clean before this work. Source line references below refer to that commit.

- Read root `AGENTS.md`, `README.md`, `TESTING.md`, `indigo_docs/DEVELOPMENT.md`, `DRIVER_DEVELOPMENT_BASICS.md`, `DRIVER_GENERATOR_MIGRATION.md`, `MAKEFILES.md`, and both driver READMEs.
- Follow `indigo_test/AGENTS.md` and the common, CCD, guider and wheel matrices in `indigo_test/DRIVER_TESTING_RULES.md`.
- Use `ccd_asi/REFACTOR.md`, generated ASI/Player One drivers and their fake SDK/hardware harnesses as structural references, not as QHY protocol specifications.
- Audit both bundled `qhyccd.h`, `qhyccdcamdef.h`, `qhyccderr.h`, SDK configuration and binary architectures. No PDF/XLSX/DOC protocol document was found in the two SDK directories during the initial inventory. Header comments and measured behavior must distinguish documented contracts from workarounds.
- `ccd_qhy/indigo_ccd_qhy.cpp` has 1,876 lines, version `0x0300001A`; `ccd_qhy2/indigo_ccd_qhy2.cpp` is a symlink to it. QHY2's `_main.c` also links to the legacy wrapper. The driver folders have different headers and `Makefile.inc` files; QHY2 supplies `-DQHY2` and its own SDK.
- Legacy code is excluded on macOS ARM; QHY2 is allowed there. Preserve that restriction until SDK architecture/runtime evidence supports a change. Build/test legacy hardware with the supported x86_64 process and dependencies, or record the platform blocker explicitly.
- Both READMEs say that devices must be present at INIT and hot-plug is unsupported. The source's `HOTPLUG` define is commented out. Historical failures in `TESTING.md` are context, not current acceptance results.
- No dedicated CCD QHY fake SDK or hardware suite was found. Existing QHY focuser/wheel simulator tests cover different drivers and do not count toward this migration.

## Source inventory and SDK split

| Baseline lines | Responsibility / constraint |
| --- | --- |
| 28–149 | Version, SDK/header/architecture branches, custom properties, shared private data and old timers/refcount. |
| 151–425 | Bayer mapping, SDK open/setup/start/read/abort/cooling/close; SDK handle differs between header generations. |
| 427–541 | Exposure and streaming readout, temperature suppression before readout, periodic cooling. |
| 543–601, 1206–1298 | Optional ST4 guider, blocking duration API, independent RA/DEC requests and shared camera ownership. |
| 602–861 | Custom property allocation, camera capability discovery, dynamic controls/read modes and connection teardown. |
| 863–1204 | Bus-side SDK operations, frame/bin/mode coupling, acquisition/abort, CONFIG and detach. |
| 1300–1434 | CFW position/status encoding, initialization, slot changes and polling. |
| 1437–1806 | Manual device arrays, discovery/probe opens, optional hot-plug threads and removal. |
| 1808–1876 | Driver conflict checks, resource initialization/release and unsupported-architecture entry point. |

Audit every `QHY2` branch through both preprocessing configurations:

1. Public entry point, label, conflicting-driver name and selected header/SDK.
2. Legacy macOS ARM exclusion versus QHY2 availability.
3. QHY2-only read-mode storage, property allocation/definition/deletion/save and SDK calls.
4. `SetQHYCCDAutoDetectCamera(false)` before new SDK resource initialization.
5. Platform logging/firmware initialization supplied by current headers/entry code.

Legacy compilation must not acquire references to QHY2-only symbols. QHY2 must retain read-mode functionality. Use generated property attributes and runtime capability visibility; do not introduce a DSL preprocessor merely to remove an unused hidden read-mode allocation from the legacy branch.

## Build and source layout

- Keep the authoritative definition in `ccd_qhy/indigo_ccd_qhy.driver` and its generated `.cpp`, `.h`, `_main.c` synchronized. Preserve licensing and update the copyright range to 2026.
- Generate `.cpp` directly with `cpp = true`. Keep QHY2 entry/header/label adaptation in the shared definition and retain the source symlink, without maintaining a second hand-edited driver implementation.
- Audit `Makefile.drv` discovery: `.c` and `.cpp` with the same basename currently both contribute object names. Add driver-local build rules/dependencies so the generated implementation is compiled once as C++, including incremental and parallel builds. Avoid a broad makefile redesign.
- Preserve standalone executables, shared libraries, SDK linking/fixups and the conflicting-driver check. Verify symbols with the linker/object tools, including C linkage for public entry points.
- Add every persistent new source, definition, test and document to the appropriate Xcode groups. Keep the existing `.cpp` source build entries; do not retain the intermediate generated `.c`. Audit the QHY2 Windows project/filter entries and shared-header references.
- Increment both driver versions above `0x0300001A` when migrating behavior. Do not regenerate unrelated checked-in drivers solely for the C++ generator change.

## Discovery and lifecycle design

Use generated `sdk { hotplug = false; ... }` startup-only attachment with SDK
identity, optional logical interfaces and a generated driver queue, as explicitly
requested after physical SDK failures. The `.driver` transport block determines
this policy; physical hot-plug is excluded from acceptance.

Match checked SID/model enumeration against already attached devices, reject
duplicates and reserve enough logical slots for the whole camera/guider/wheel
group. Filter the existing vendor IDs (0x16c0, 0x1618, 0x1856, 0x04b4, 0x0547).
Never reinterpret the SDK's opaque handle as `libusb_device_handle` to derive a
USB path. Startup performs one USB inventory pass; newly plugged devices require
a driver restart. No runtime SDK identity-removal scan or retry is retained.

Keep firmware initialization before discovery and release resources only after
queued work and logical devices are torn down. Failed resource/queue creation
must reset initialization state and release acquired SDK resources.

`qhy_open(device)` must either acquire all resources or roll back SDK handle, image buffer and global lock. The generator owns shared `count`; remove `count_open` and `gp_bits` ownership. Only the last successfully opened session closes. Test CCD/guider/wheel alone, both sibling connection orders, surviving siblings, failed later logical initialization, and master/slave detach order.

## Properties and acquisition design

Rename custom names at cutover and update `indigo_docs/PROPERTIES.md` in the same change:

| Existing | Planned | Semantics |
| --- | --- | --- |
| `PIXEL_FORMAT` | `X_PIXEL_FORMAT` | RAW 8 / RAW 16, capability-dependent, coupled with frame BPP and CCD modes; persistent. |
| `QHY_ADVANCED` | `X_ADVANCED` | Dynamic USBTRAFFIC, USBSPEED, SHUTTERMOTORHEATING controls with checked ranges/readback; persistent. |
| `READ_MODE` | `X_READ_MODE` | QHY2-only SDK operations, dynamic mode names/count, geometry refresh; persistent. |

Preserve standard names. Inventory every property/item initialization, count, hidden flag, permission, range and default. Check seven image formats, streaming settings/max exposure 4 seconds, square supported bins 1–4 (including holes), minimum 64 binned pixels, effective-area origin/size, gain/offset/gamma, temperature-only versus cooled cameras, and optional wheel/guider properties. Verify the old 900-second minimum exposure maximum and USB traffic floor 50 against SDK/device evidence; document any retained workaround precisely.

Use `indigo_ccd_exposure_setup()` for the shared single-exposure countdown. Preserve requested fractional/subsecond duration in operation state independently of the published countdown. Streaming uses monotonic deadlines and `ceil()` only for remaining-time display. Finalizers do one bounded status/read step, reschedule if pending, and stop on a checked deadline. Remove driver polling/sleep loops that wait for exposure, stream or wheel completion.

Abort is urgent: cancel pending acquisition starts and finalizers, stop the actual SDK acquisition mode, settle exposure/stream/image properties exactly once, then allow reacquisition. Apply cross-property operational conflicts before copying accepted geometry/mode/control settings; rely on framework guards for repeated requests to the same BUSY property.

SDK calls can themselves block. Establish real behavior of `GetQHYCCDSingleFrame`, `GetQHYCCDLiveFrame` and `ControlQHYCCDGuide`; the legacy code explicitly calls guiding blocking. A delayed finalizer cannot make an already blocking SDK call interruptible. Audit available timeout/guide APIs, preserve pulse units and direction mapping, and document measured queue latency and any SDK-imposed bound. Do not introduce unowned threads or falsely claim independent axes without evidence.

## Risk-to-test matrix

The source observations below are regression targets, not claims of reproduced hardware failures. Planned test families will run the production driver through public bus APIs with a fake SDK/USB boundary and real handler queues.

| Family | Baseline risk | Required assertions |
| --- | --- | --- |
| Q01 | 185–255: partial open failures leak handles; ignored Init result; rescan workaround. | Every open/init/geometry failure balances resources; retry succeeds; SDK workaround call order is explicit. |
| Q02 | 1472–1806: unchecked IDs/models, duplicate/partial attach, SDK handle cast, manual shared cleanup. | Checked bounded identities, optional interfaces, duplicates/capacity, attach rollback, enumeration failure, removal/retry and INIT/SHUTDOWN. |
| Q03 | 694–750, 1002–1170: advertised formats/bins and coupled geometry may disagree. | SDK calls use accepted format/bin/ROI, supported bin holes and effective offsets; failed changes preserve coherent properties. |
| Q04 | 340–363: unbounded remaining-time loop, unchecked frame dimensions/depth/channels. | Single-frame readiness, deadline, malformed SDK output, buffer-size safety and no invalid BLOB. |
| Q05 | 427–512, 879–912: fractional timing, blocking stream loop and start-failure cleanup. | 0.1/1.5/2.5-second exposures, shared countdown progress, finite/indefinite streams, start/read/stop failures and exact completion. |
| Q06 | 915–925, 841–860: abort/disconnect omit active stream ownership. | Queued start overtaken by abort, active abort/disconnect, no post-close SDK calls or late image, immediate restart. |
| Q07 | 635–651, 753–835: ignored control/range/readback failures and false OK. | Each advanced control, gain/offset/gamma, partial failure and recovery; temporary CONFIG restore sends expected commands. |
| Q08 | 784–800, 1040–1099: unchecked mode count/labels/geometry and missing conflict guard. | QHY2-only API coverage, zero/large/invalid count, malformed label/index, changed geometry, failed mode/metadata read and active-acquisition rejection. |
| Q09 | 375–401, 514–541: cooler calls ignore errors, sensor-only path and readout suppression. | Cooler ON/OFF, target/readback, power scaling, invalid measurement, error recovery and timer cancellation; temperature-only profile. |
| Q10 | 543–601: pulse errors become OK; blocking duration API and shared lifetime. | Direction and millisecond arguments, zero pulse, independent axes as SDK permits, errors, disconnect and sibling survival; bounded measured latency. |
| Q11 | 1300–1423: tight CFW loop, target/status offset, stale value and false OK on timeout. | Documented ASCII slot encoding, delayed progress, current versus target, invalid status, send/poll failure, timeout/reconnect and slot-name/offset counts. |
| Q12 | 1808–1857: resource init/conflict state and pending discovery cleanup. | Both SDK variants, mutually exclusive entry points, failed INIT retry, unload/reload, no work after shutdown and balanced USB references. |

Fake profiles: uncooled mono CCD, color CCD with ST4, cooled shutter camera with CFW, sparse formats/bins, temperature-only camera, and QHY2 multiple read modes. Exercise all exposed interfaces, not only camera smoke tests. Do not test generic numeric validation or image encoders through this suite.

## Physical acceptance and SDK failure reporting

Run opt-in hardware tests outside default integration, separately for each SDK/model. Record exact executable architecture, SDK/firmware identity where exposed, camera SID, driver version and logs. Begin with discovery/connect/disconnect/reconnect; then RAW8/RAW16, ROI/binning, short and fractional exposures, >15-second QHY 5L-II acquisition with the USB-traffic workaround, finite/indefinite streams, abort/restart, simultaneous guiding, settings restoration, shutdown/reinitialization and actual driver unload/reload. Test read modes only if QHY2 reports them. Physical unplug/replug phases require user cable actions when reached.

Cooling, mechanical shutter, camera-connected CFW and multiple-camera survival are fake-covered but hardware-unavailable unless additional equipment is supplied. Lack of those capabilities on the two named cameras is not a failed test. Linux/Windows runtime remains unverified until exercised there.

For every failure retain the scenario, last SDK call/return, timing, property transitions, process exit and reconnect outcome. Classify as reproduced driver regression, baseline SDK/device limitation, hardware unavailable, or unresolved. A failed fake test is not excused by poor vendor SDK quality. Compare a minimal SDK call sequence or the baseline driver when needed before attributing a hardware failure to the SDK. After a crash/hang, recover in a fresh bounded process; do not leave test processes or modified camera settings behind.

## Execution checkpoints

- [x] Inventory shared source, symlinks, QHY2 branches and initial risk matrix.
- [x] Obtain explicit approval for the concrete generator C++ prerequisite.
- [x] Finish C/C++ generator regression validation and document results.
- [x] Add dual-SDK fake boundary and executable Q01–Q12 scenario mapping in `indigo_test/CHANGES.md`.
- [x] At the final hardware checkpoint, compare SDK/baseline behavior where a physical failure requires diagnosis (hardware tests deferred to the end by user instruction).
- [x] Implement shared `.driver`, direct C++ output and local build/project integration.
- [x] Replace lifecycle/discovery, acquisition, guiding and CFW work with checked queue-owned operations.
- [x] Rename/document custom properties and synchronize generated outputs; verify both versions increased.
- [x] Pass applicable fake suites, sanitizer checks and builds against both real SDK headers/libraries.
- [x] Run hardware acceptance and record failures/limits here and in `TESTING.md`.
- [x] Update only status columns of both `MIGRATION_STATUS.md` rows; preserve Comment cells exactly.
- [x] Inspect the final diff, confirm reproducible generation and remove test artifacts/processes.

Do not advance incremental `REVIEW.md` baselines for this migration. This file tracks implementation and acceptance; `indigo_test/CHANGES.md` tracks executable coverage. Migration remains incomplete until outstanding checkpoints and explicit hardware/platform gaps have been accounted for.

## Validation log — generator prerequisite

2026-09-12, macOS host: rebuilt `build/bin/indigo_generator` and ran
`make -C indigo_test test-generator-architecture`. All ten test cases passed,
including 86 existing DSL attribute/block checks, 12 generated-code syntax
compilations (C11/C++11 across serial, virtual, libusb, SDK, SDK retries with
identity removal, and single-device HID), and reverse extraction of old and
casted USB registration preserving VID/PID. The new SDK-retry fixture also
exposed the callback's `void *` conversion, now explicitly cast without changing
ownership. These checks do not constitute camera driver or hardware validation.

The new plan is registered in the existing `ccd_qhy` Xcode group. Project plist
validation and `git diff --check` passed. The existing handwritten QHY drivers
have not been replaced and their versions remain unchanged at this checkpoint.

## Implementation and hardware-free validation — 2026-09-12

The shared `.driver` now owns CCD, optional guider and optional CFW lifecycle, startup-only SDK discovery and property handlers. Generated C++ includes a small `.driver`-owned adapter selecting the public entry point for each SDK and rolling back SDK resources if generated queue initialization fails. Versions are `0x0300001C` for both variants (previously `0x0300001A`). No generator capacity override is used; the default five logical-device slots apply, and discovery reserves the whole camera/guider/wheel group before attachment.

Single exposures use the shared countdown and exact stored durations; streaming and wheel completion use monotonic deadlines and queued finalizers. Streaming count zero completes without starting the SDK. Checked control writes retain the accepted value after failure, including a subsequent SDK reopen. RAW 8/16, sparse bins, effective-area geometry, QHY2 read modes, sensor-only and cooled profiles remain capability-driven. All custom properties now use `X_`; the property reference and both READMEs were updated.

The legacy and modern SDK headers compile separately against one fake SDK implementation. The fake supplies coordinate-independent deterministic byte noise and verifies RAW headers, complete payload and Bayer metadata; it does not use simulator photographs. Real INDIGO bus, handler queues, configuration restore and image processing remain in the test process. No vendor library or physical USB discovery is linked into these tests. Configuration files and the image directory are isolated under a temporary folder.

| Family | Named executable cases / evidence |
| --- | --- |
| Q01 | `open rollback`, `initialization failures`, `setup reopen`: failed Open/Init/geometry, balanced locks/handles, retry and failed mode-reopen recovery. Baseline fake reproduced a leaked handle; migrated code passes. |
| Q02/Q12 | `lifecycle`, `startup only discovery`, `init enumeration attachment`, `discovery failure reload`, `discovery capacity identity`, `sibling orders guide overlap`: one initial inventory, no callback/runtime discovery, list/SDK-id failures, failed resource/master attachment, distinct names and logical capacity, restart recovery, independent interfaces/shared last-close and balanced references. |
| Q03 | `ROI formats bins`, `optional sparse profiles`, `property contract frame types`: custom/inherited inventory, RAW formats, sparse bin support, accepted frame parameters, frame/BPP/mode coupling and all five frame types with shutter commands. |
| Q04 | `start and read failures`, `readout buffer contract`, `RAW color payload`: SDK remaining/read failure, invalid frame width, oversized memory requirement, no bad image, RAW8/16 dimensions/bytes/Bayer content and recovery. |
| Q05 | `acquisition`, `fractional exposures`, `streams abort`, `stream errors`, `zero stream failed setting`: 0.1/1.5/2.5-second exact SDK exposure, finite/indefinite/zero streams, timeout and start/read/stop errors. |
| Q06 | `abort restart`, `queued abort`, `disconnect sibling`, `acquisition conflicts`: urgent abort overtakes a gated queued start, restart, active disconnect, cross-property conflicts and no SDK use after close. |
| Q07 | `advanced failure`, `controls no bus IO`, `metadata readback`, `configuration restore`, `zero stream failed setting`: ranges/readback/write failures, custom CONFIG restoration and accepted controls restored after SDK reset. Baseline fake reproduced false OK for failed advanced writes. |
| Q08 | `read modes`, `optional sparse profiles`: QHY2-only read modes, changed geometry, invalid mode count, failed change and restored selection; legacy hidden/no new-SDK symbol requirement. |
| Q09 | `cooling sensor`, `cooler failure recovery`: cooler/temperature-only visibility, target and power scaling, invalid temperature/power, cooler ON/OFF write failure/recovery, cancellation on disconnect. |
| Q10 | `guide directions errors`, `guide blocking zero`, `sibling orders guide overlap`: all four directions and milliseconds, zero pulse, SDK failure, immediate-return and blocking SDK behavior, two axes with exposure and sibling survival. |
| Q11 | `wheel position errors`, `wheel timeout status`, `property contract frame types`: slot/name/offset inventory, ASCII zero-based commands/readback, send/poll failure, timeout, malformed status and recovery. |

Hardware-free limitations: no allocator/queue-creation fault injection, no exhaustive interleaving or all malformed SDK buffer permutations. A failed optional slave attachment leaves the successfully attached camera operational according to the unchanged generator; retry of that individual missing interface requires driver restart. Fake SDK completion cannot prove actual SDK interruptibility, physical shutter/guide timing, firmware startup reliability or optical image quality. The SDK CFW interface and historical driver use eight ASCII positions; a physical CFW is needed to verify actual wheel capacity/status conventions. No serial protocol simulator or manufacturer motion document is applicable; bundled SDK headers were audited, and no camera PDF/XLSX protocol source was supplied.

The opt-in `indigo_test/hardware/test_ccd_qhy_hw.c` uses `dlopen`/`dlclose`, a required camera-name selector and explicit `--run`; it is excluded from default tests. It covers fractional/16.5-second exposures, formats/modes/ROI/binning, settings restoration, finite/indefinite streaming, abort/reacquire, guiding, reconnect and actual driver-library reload. `QHY_HW_CASE` selects a bounded scenario for SDK failure diagnosis. `--hotplug` waits for explicit cable actions. Actual vendor libraries may stay mapped after driver unload; a successful driver reload is not proof of vendor-runtime unload.

Per the user's latest instruction, all physical tests run only after software/build checks. Targets now include QHY5 and QHY5L-II with the legacy SDK, plus QHY5III178 with QHY2. The user confirmed all three are connected. Linux/Windows runtime and macOS legacy ARM remain unverified/unsupported respectively. Mac builds succeeded for both SDK variants, standalone executables and libraries; the legacy SDK supplies x86_64 code only, and the modern SDK emits deployment-version linker warnings. These SDK warnings are not camera-test results.


Software verification result: 35/35 fake cases pass for each SDK (70 total).
AddressSanitizer + UndefinedBehaviorSanitizer passed the complete 34-case suite
before the final capacity case, then both capacity and overlapping-guide cases
for each SDK; corrected RAW8/RAW16/Bayer selection was rerun under both sanitizers.
No sanitizer diagnostics were emitted. Generator reproduction matches all three
checked-in outputs byte-for-byte; both real-SDK builds and public C-linkage symbols
were checked, Xcode plist and Windows project XML parse, and diff whitespace checks
pass (retaining existing Windows CRLF). No hardware result is included in these counts.

Physical checkpoint has begun. Initial sandbox discovery returned no cameras;
I/O Registry outside the sandbox confirmed three QHY devices. All three require
firmware (user confirmation). The modern SDK requires the directory containing
firmware files, contrary to the old legacy README's parent-directory wording.
Using the bundled firmware directory, QHY5III178M delivered full 3056 x 2048 RAW16
frames for 0.1, 1.5, 2.5 and 16.5 seconds. Elapsed times were 2.969 (first setup),
1.754, 2.750 and 16.745 seconds. Disconnect then aborted in SDK CloseQHYCCD ->
StopQHYCCDLive -> libusb_cancel_transfer (invalid mutex assertion). The unchanged
baseline driver built from the recorded commit reproduced the same assertion
at disconnect after the same four exposures (exit SIGABRT). This failure predates
migration; reconnect/unload cannot be claimed passed. Further scenarios run in
separate bounded processes. Physical hot-plug remains unverified.


## Direct C++ output follow-up

The user additionally authorized `cpp = true;` (default false) in the generator.
The option and reverse-extraction support are implemented and all 11 generator
architecture cases pass. QHY opts in: `indigo_ccd_qhy.cpp` is now generated directly,
QHY2 retains its symlink to that source, and the short SDK-dependent public entry
adapter lives in the shared `code` block. The intermediate generated `.c` was
removed, as was its Xcode reference. Existing CPP source build entries remain;
Windows navigation entries point at the CPP source. The public adapter preserves
C linkage and SDK rollback on failed generated queue/callback initialization.
The initial wrapper plan above is superseded by this layout.

Native SDK depth is selected at connection rather than always choosing the
largest advertised format. A single-format camera does not call the unsupported
bits-mode setter; a fake fixed-RAW8 profile verifies acquisition in this case.
QHY5 initially rejected SetQHYCCDBitsMode; after this correction it delivers
1280 x 1024 RAW8. The historical setup also checked that unsupported setter,
so this is an additional compatibility fix, not an established new regression.

Legacy firmware-path diagnosis: unlike the modern SDK, svn r6536 appends
`/firmware/...` internally (confirmed in bundled binary strings). Legacy tests
therefore use the parent `bin_externals/qhyccd` directory. Incorrect-path runs
and their detection timeouts are setup failures. Starting framework USB first
removed a context warning but was not necessary once the firmware path was
correct; no such production workaround was added. The hardware harness now
waits up to 60 seconds for discovery because a legacy SDK scan can take six seconds.

With correct firmware, QHY5LII-M delivered 1280 x 960 RAW16 for 0.1/1.5/2.5/16.5 s
(elapsed 2.568/3.203/4.187/18.180 s). A snapshot after disconnect/reconnect hung
inside QHY5LIIBASE::GetSingleFrame; sampled stack confirms the blocking vendor
call prevents queued abort. The unchanged baseline reproduced the reconnect
hang after the same exposures at its native RAW8 depth; both processes required
the external time limit. The driver's finalizer watchdog cannot interrupt a
blocked SDK call, and legacy SDK has no single-frame readout timeout API.
This remains a documented SDK limitation rather than a passing lifecycle test.


## Final acceptance state and remaining work

Both direct-C++ driver builds pass. All 35 fake cases per SDK pass, and the
complete direct-C++ suite passes ASan/UBSan (70 cases). A final guider follow-up
clears stale direction values after failed/opposed pulses; opposite-direction
recovery passes under both sanitizers. All 11 generator tests pass, including
`cpp` true/false/default, C ABI and reverse extraction. Physical tests were not
repeated for this small fake-verified guider change because cameras had become
unavailable after SDK crashes.

QHY5 delivered four 1280 x 1024 RAW8 images after the fixed-depth correction
(0.1/1.5/2.5/16.5-second requests; elapsed 0.767/3.436/5.506/21.915 seconds).
It then crashed in SDK CloseQHYCCD -> QHYCAM::closeCamera during acquisition
setup for reuse. Another process crashed in the SDK discovery probe close for
QHY5L-II; subsequent settings and abort attempts found no selectable camera.
These distinct crashes were not reproduced against a patched baseline and remain
unresolved, not automatically classified as vendor-only. The remaining batch
was stopped after repeated pre-selection failure; all owned HW processes ended.

No complete physical camera suite passed. QHY5III178's stop/close assertion and
QHY5L-II's reconnect readout hang were independently reproduced with unchanged
baseline drivers. RAW/ROI/bin transitions, complete settings/guide/stream phases,
driver reload, physical removal/replug and multiple-camera survival remain
partially tested or blocked as detailed in TESTING.md. A camera image alone is
not full hardware acceptance. Full persistent CONFIG restore, cooling, CFW and
mechanical shutter hardware are also unverified. Physical settings restoration
could not finish after SDK crashes/hangs; no persistent configuration was saved.

The user disconnected QHY5III178 and replugged QHY5/QHY5L-II for legacy tests,
then became unavailable. These cable actions were preparation, not hot-plug
acceptance. Before resuming, reset the attached legacy cameras over USB and
reconnect QHY5III178 only for its separate modern-SDK process. The generator's
five logical slots allow two CCD+guider cameras together; never infer that a
third unlisted camera is unsupported by an SDK without considering capacity.

Firmware paths differ by SDK and are documented in the READMEs. Legacy uses
`.../bin_externals/qhyccd`; modern uses `.../bin_externals/qhyccd/firmware`.
The initial wrong-path/no-camera runs have been classified as setup failures.

- [x] Validate final static startup on both SDK variants and record partial HW acceptance plus SDK-blocked phases; hot-plug is excluded by user request.

Logs retained outside the repository: `/tmp/qhy2-hw-exposure-firmware.log`,
`/tmp/qhy2-hw-baseline.log`, `/tmp/qhy2-hw-{geometry,settings,abort,stream,guide}.log`,
`/tmp/qhy-hw-5lii-correct-firmware.log`, `/tmp/qhy-hw-5lii-baseline-correct.log`,
`/tmp/qhy-legacy-hang.sample`, `/tmp/qhy-hw-qhy5-final-*.log`,
`/tmp/qhy-direct-cpp-tests.log`, `/tmp/qhy-direct-cpp-sanitize.log` and
`/tmp/qhy-generator-cpp-test.log`. macOS crash reports provide the cited stacks.


Final cleanup: `make -C indigo_test test-clean` removed test binaries and dSYMs;
temporary baseline build files were removed while diagnostic logs were retained.
Process inspection found no remaining QHY hardware test process. Final builds,
CPP/header/main byte-for-byte regeneration, Xcode plist, Windows XML and diff
whitespace checks pass. Both migration-table Comment cells were preserved exactly.


## Resumed physical validation

After the user reset USB with only QHY5L-II attached, the isolated guide run
connected the CCD but skipped pulses because the harness selected logical
siblings before hot-plug attachment had completed. The process then crashed
on disconnect in CloseQHYCCD -> QHY5IIBASE::DisConnectCamera ->
QHYCAM::closeCamera (report `test_ccd_qhy_hw-2026-09-12-104206.000.ips`,
log `/tmp/qhy-hw-5lii-resume-guide.log`). This is not a passing guider test.
The harness now resolves siblings after the serialized CCD connection and
fails an explicitly selected guide scenario if no guider is exposed. Both
architecture builds of the harness pass; the physical guide run awaits a
fresh USB reset. No production driver behavior changed in this follow-up.

The next reset exposed only QHY5-M (16c0:296d), while the selected scenario
required QHY5L-II. That run timed out in selection, then detached both logical
QHY5 devices and shut down cleanly (exit 1); no guide pulses ran. Log:
`/tmp/qhy-hw-5lii-resume-guide-fixed.log`. Confirm the physical model before
the next test; this selection mismatch is not a QHY5L-II driver failure.

With QHY5L-II isolated after USB reset, the corrected guide scenario passed:
all four 100 ms directions, a pulse during a 1.5 s exposure, guider operation
after CCD disconnect, reconnection and clean shutdown. This verifies SDK/property
completion, not electrical ST4 timing. The separate settings scenario also
passed gain change/restoration and advanced-settings restoration. Logs:
`/tmp/qhy-hw-5lii-guide-isolated.log` and
`/tmp/qhy-hw-5lii-isolated-settings.log`.
The next abort process crashed before selection while closing the discovery
probe (CloseQHYCCD -> QHY5IIBASE::DisConnectCamera -> QHYCAM::closeCamera,
`test_ccd_qhy_hw-2026-09-12-104725.000.ips`). Abort did not execute;
streaming/geometry were not started. Physical hot-plug remains unverified.

A minimal x86_64 program linked only the bundled legacy libqhy.a and project
libusb (with a no-op indigo_debug symbol, no INDIGO bus/driver/queues) reproduced
the QHY5L-II discovery-close crash. Sequence: InitQHYCCDResource, firmware init,
3 s wait, ScanQHYCCD, GetQHYCCDId/Model, OpenQHYCCD, query ST4/CFW, CloseQHYCCD,
ReleaseQHYCCDResource. The first process after a five-second USB reset passed;
the second identical process without USB reset crashed at CloseQHYCCD ->
QHY5IIBASE::DisConnectCamera -> QHYCAM::closeCamera (report
`qhy_probe-2026-09-12-105120.000.ips`). Logs are
`/tmp/qhy-sdk-only-probe-reset.log` and `/tmp/qhy-sdk-only-probe-repeat.log`.
This reproduces the specific discovery-close failure independently of migration;
it does not classify every QHY crash or prove physical hot-plug behavior.
A prior driver abort attempt after a short reset also crashed at discovery
(`/tmp/qhy-hw-5lii-abort-reset.log`, report 104824.000); no abort ran.

The abort scenario run first after a five-second USB reset passed: start a 5 s
exposure, request abort, then receive a fresh 1280 x 960 RAW8 image at 0.1 s,
restore settings and shut down cleanly (`/tmp/qhy-hw-5lii-abort-cold.log`).
Historical commit 074bd49a8c612d97ad5e2bdcad4bedc929e22fc6 (2020-10-03,
"ccd_qhy/ccd_qhy2: hot-plug support disabled") confirms hot-plug was deliberately
disabled. Its message does not identify the exact crash as the reason. The old
source separately documents repeated QHY5L-II open/close crashes and rescans as
a leaking workaround. Do not infer that serialized generated hot-plug fixes them.


## Approved removal of hot-plug (current target)

The user requested disabling hot-plug for both SDK variants and explicitly
approved implementing the generator's previously empty `sdk { hotplug = false; }`
branch. Both drivers advance to version 28 (0x0300001C). The shared definition
uses static startup discovery instead of registration and periodic discovery
retries. INIT queues one USB inventory pass, invoking the existing filtered
SDK plug block for each initial USB device. The generated per-driver queue
continues to serialize attachment and logical connection handlers. SHUTDOWN
verifies disconnected devices, drains startup work, detaches and frees resources;
there are no arrival/removal callbacks. Existing hot-plug transports retain their
behavior. No fallback promises to recover the vendor close failures.

Acceptance plan: compile static SDK output as C/C++, assert no callback/retry
emission, test one-time enumeration/capacity/identity, list and SDK discovery
failure, attachment rollback and release balance for both QHY SDK profiles;
run full driver fake tests, sanitizers and real SDK builds. Update README and
remove the physical hot-plug harness option. Physical results above remain
historical evidence from version 27; validate version 28 only after software
checks, with USB resets as needed. No new persistent files are required.

The final version-27 QHY5L-II streaming attempt crashed during the mode-change
close in acquisition_start, before delivering a stream frame (report
`test_ccd_qhy_hw-2026-09-12-105321.000.ips`, log
`/tmp/qhy-hw-5lii-stream-cold.log`). This exact path was not separately reproduced
in the SDK-only discovery test. Hot-plug is now deliberately out of scope,
rather than a pending claimed capability.

The entry adapter was simplified after user feedback: one internal static
`qhy_generated_entry`, one conditional public `QHY_ENTRY`, and `DRIVER_NAME`
for metadata. The premature entry-name define/undef pair and duplicate name
macros were removed. QHY2 still overrides the generator's literal DRIVER_NAME
and DRIVER_LABEL defaults; eliminating those two overrides would require a
separate generator metadata customization, not another local macro layer.

Software validation of final version 28: 35/35 cases per SDK (70 total), and
70/70 again with ASan/UBSan. The generator suite passes all 12 cases, including
14 C11/C++11 transport compilations and startup-only discovery assertions.
Both vendor-SDK universal builds pass, public entry symbols retain C linkage,
and the generated implementation has no USB callback or discovery-retry code.
Final logs: `/tmp/qhy-final-tests.log`, `/tmp/qhy-final-sanitize.log`,
`/tmp/qhy-static-generator-final.log`, `/tmp/qhy-final-build.log`,
`/tmp/qhy2-final-build.log`. Hardware startup verification and the concurrently
changed Xcode source references are the remaining final checks.

Xcode follow-up: the user added QHY2 source copies while tests ran and approved
redirecting those references to the existing `indigo_ccd_qhy2.cpp` and
`indigo_ccd_qhy2_main.c` symlinks. The standalone main remains a group reference
but was removed from the indigo/indigo_m1 server Sources phases to avoid duplicate
main symbols. Other concurrent Xcode/SDK edits were preserved. Plist validation,
source-link checks, new-file group references and byte-for-byte regeneration
pass. Migration-table Comment cells remain unchanged. The user's unreferenced
source copies were not modified or removed.

First version-28 physical startup had both legacy cameras attached (confirmed by
the user and IOUSBHostDevice inventory). QHY5-M was attached once, without a
hot-plug callback, then detached cleanly at shutdown. QHY5L-II remained at its
cold 1618:0920 USB identity while QHY5 was 16c0:296d. SDK enumeration exposed
only QHY5-M, so the QHY5L-II guide selection timed out without connecting or
sending pulses. This is not a guide pass or a logical-capacity failure.
Log: `/tmp/qhy-v28-hw-5lii-guide.log`. Isolated QHY5L-II firmware/startup
validation follows a fresh user USB reset.

Version 28 with only QHY5L-II attached passed the isolated guide scenario:
startup enumeration, CCD/guider connection, four 100 ms guide directions,
guide during a 1.5 s exposure, one valid 1280 x 960 RAW8 image, guider survival
after logical CCD disconnect, CCD reconnect, settings restoration and clean
shutdown. Exit 0, log `/tmp/qhy-v28-hw-5lii-guide-isolated.log`.
This confirms the static-discovery path on this camera; it is not a full camera
suite or an electrical ST4 measurement. Modern-SDK static startup is next.

Version 28, isolated QHY5III178M / modern SDK: static startup and both logical
connections succeeded, all four 100 ms guide directions completed, a pulse ran
during a 1.5 s exposure, and a valid 3056 x 2048 RAW16 frame arrived. Guiding
also completed after the logical CCD disconnected. Final guider disconnect
(last shared close) aborted in CloseQHYCCD -> StopQHYCCDLive ->
QHY5IIIBASE::StopLiveExposure -> libusb_cancel_transfer -> usbi_mutex_lock.
Report `test_ccd_qhy_hw-2026-09-12-111241.ips`, log
`/tmp/qhy-v28-hw-178-guide.log`, exit -6. This matches the stop/close assertion
already reproduced with the unchanged baseline; the complete guide scenario
is not a pass. Static startup now has physical evidence for both SDK variants.

The final physical checkpoint is complete with partial acceptance, not a full
camera-suite pass. Existing SDK-blocked streaming/geometry/reconnect/unload
limitations remain as documented. Hot-plug is intentionally disabled and is
excluded by user request. No persistent configuration was saved; final settings
restoration could not complete after the modern SDK abort. All owned HW
processes have ended.

Final verification: 70/70 fake tests, 70/70 ASan/UBSan, 12 generator cases,
legacy/modern vendor-SDK builds, C entry symbols, generated-output reproducibility,
Xcode references/plist and whitespace checks passed. Final physical results are
recorded above. Test binaries were cleaned after the final hardware checkpoint.

### Legacy SDK source investigation and experimental fix (2026-09-12)

Inspected the separate `libqhy` checkout at commit `a826bac` (the user's
Development checkout). The user authorized fixes in that SDK checkout. These
changes have not been installed into INDIGO's bundled SDK archives.

Confirmed source defects:

- `libusbIo.cpp` freed terminal/cancelled transfers but cleared only the callback's
  local pointer, leaving `cydev[].img_transfer[]` dangling. `StopAsyQCamLive()`
  could subsequently pass a freed transfer to `libusb_cancel_transfer()`.
- Stop terminated its event thread before cancelling transfers and did not wait
  for cancellation callbacks before returning. QHY5-II's non-Windows close did
  not stop asynchronous transfers at all.
- `QHY5IIBASE::DisConnectCamera()` did not reset `connected`; reopening the same
  object could report success with its previous, already closed handle.
- Repeated `ScanQHYCCD()` clears all open flags and recreates SDK objects globally.
  This remains an independent multi-camera risk; it is not repaired by the
  transfer-lifetime patch.

The experimental SDK patch serializes transfer ownership, clears owning slots
in terminal callbacks, processes received data before resubmission, drains
cancellation callbacks, joins the event thread, and cleans up partial startup.
Low-level close now stops asynchronous acquisition before closing USB, and
QHY5-II close clears its connection/live flags. USB reset behavior is unchanged.
A regression test is in the SDK checkout's `tests/test_libusb_transfers.cpp`.
It passed with AddressSanitizer and UndefinedBehaviorSanitizer for terminal
statuses, resubmit failure, successful completion/timeout resubmission,
cancellation drain, repeated stop, and invalid handles. All modified SDK units
compiled, and a complete arm64 SDK archive was built in a temporary directory.
Thread creation failure, full public open/close cycles, and physical behavior
remain unverified. The test uses mocked USB events, not physical transfers.

Legacy `SetQHYCCDBitsMode()` dispatches directly to model setters without closing
the handle, including QHY5L-II and QHY5III178. This offers a candidate alternative
to the driver's current close/reopen sequence, but it has not been implemented
or tested on hardware. Single-frame reads may leave SDK acquisition active, so
safe parameter transitions still need verification. Modern SDK source is not
available here: resemblance to the observed modern cancellation crash is an
inference, not proof that its implementation has these exact defects.

Next physical comparison: isolated QHY5III178 with the patched legacy SDK, then
with the user's newly installed modern SDK. Hardware testing is pending SDK
installation and a clean camera state; no pass is claimed yet.

### Modern SDK package update (2026-09-12)

Installed the user-supplied Downloads SDK 26.06.04 into ccd_qhy2: Linux x64
and ARM64 26.06.04.16, ARM32 26.06.04.15, Windows x64 26.06.04.16,
and common API headers 26.06.04.16. Windows Win32 and Linux x86 retain their
previous binaries because no replacement packages were supplied. RELEASE
records these exceptions. Existing auxiliary Windows libraries remain in place.

The macOS vendor dylibs were combined with lipo into the existing SDK path.
Extracted x86_64 and arm64 slices match their input binaries byte for byte.
The resulting driver builds for both architectures; the vendor minimum OS
versions are 10.14 on Intel and 14.0 on Apple Silicon. An arm64 load/version
query returned 26.6.4.16 without initializing SDK resources or accessing cameras.
Linux and Windows binaries were checked for architecture and exact copy hashes;
those platforms have not been runtime-tested.

Shared firmware was updated from the Linux package (identical across all three
Linux packages). Four added firmware files have Xcode group references; firmware
for QHY5, QHY5L-II and QHY5III178 is unchanged. The patched legacy SDK remains
separate from this vendor update. Physical comparison is still pending.

After the package update, all 70 hardware-free driver/SDK scenarios passed
(35 per SDK variant). These use the fake SDK and verify driver compatibility
with the updated headers, not vendor acquisition or lifecycle behavior.

### QHY5III178 retest with modern SDK 26.06.04.16 — 2026-09-12

Working-tree generated driver v28, macOS arm64, isolated QHY5III178M after
user USB reset. Static discovery, CCD/guider connection, four 100 ms guide
directions, guiding during a 1.5 s exposure, and a 3056 x 2048 RAW16 image
completed. A guide pulse after logical CCD disconnect also completed. Final
guider disconnect aborted (SIGABRT/exit -6). The crash stack is CloseQHYCCD ->
StopQHYCCDLive -> QHY5IIIBASE::StopLiveExposure ->
QHYBASE::StopAsyQCamLiveClearLibUsb -> StopAsyQCamLive ->
libusb_cancel_transfer -> usbi_mutex_lock. Thus the supplied modern update
does not resolve the previously observed last-close failure. The full guide
scenario remains failed, not passed. No persistent configuration was saved.

Log: `/tmp/qhy-modern-260604-178-guide.log`; crash report:
`test_ccd_qhy_hw-2026-09-12-150436.ips`. The process has ended.

Legacy comparison preparation exposed two test-build issues before camera
access: the first temporary build omitted INDIGO_MACOS (so skipped firmware
initialization); the corrected arm64 build hit the driver's existing Intel-only
macOS architecture guard. Neither is a camera/SDK acceptance result. Both
processes ended. A full patched x86_64 SDK and driver have now been built for
Rosetta. The legacy source intentionally lacks SetQHYCCDLogLevel; only the
temporary test link supplies a no-op for it, leaving existing logging intact.

## Mode and bit depth workaround (2026-09-12, version 29)

The user narrowed the approved change to mode/bit depth switching only. Camera open, logical disconnect, physical close and driver unload retain their previous lifecycle. The known QHY5III178 close crash remains unresolved.

Plan: reconfigure the current handle with `SetQHYCCDStreamMode`, `InitQHYCCD` and `SetQHYCCDBitsMode`, restoring controls after initialization. After successful streaming stop, return to single-frame mode and reinitialize, invalidating cached settings so the next acquisition restores the requested bit depth and controls. Invalidate settings before any reconfiguration attempt so failure cannot leave a stale success cache. Keep discovery, generator and both vendor SDKs unchanged. Extend fake SDK coverage for repeated 8/16-bit and single/live transitions with `OpenQHYCCD` deliberately unavailable, initialization failure/retry and normal close/unload. Build both variants before physical retesting.

SDK follow-up: user additionally requested INDI's newer SDK, pinned to commit `387a31e4ff863ca29a0d0dbb8b652efc02d6b0f0` (26.07.21). Imported Linux ARM32/ARM64/x64 libraries, four public headers and macOS Intel/Apple Silicon libraries merged with lipo. Each downloaded blob matched its Git SHA; both slices extracted from the universal library match the input bytes. Runtime version-only queries on both macOS architectures return `26.7.21.5`, without camera enumeration or initialization. Ancillary headers are retained because INDI does not supply replacements. Existing Windows and Linux x86 SDKs remain unchanged. Four shared firmware files changed and three were added with Xcode references; udev rules were updated from the same commit. Firmware for the three test cameras and all original legacy SDK headers/libraries remain unchanged.

The user is unavailable and explicitly deferred all physical tests. Required later checks: repeated RAW8/RAW16 exposures, single/live transitions, abort/restart, settings restoration and CCD/guider coexistence on QHY5L-II (legacy) and QHY5III178 (modern). A known crash on actual close is still an accepted limitation, not a passing scenario.

Final software validation: 74/74 fake SDK cases pass (37 per SDK variant), including all new switching/error-recovery cases. Both macOS driver builds succeed; legacy retains its unsupported ARM stub. Generated C++/header/main exactly match regeneration, Xcode project parses successfully, imported SDK/header bytes and universal-library slices match INDI inputs. No new property names or visibility changes. Linux/Windows runtime validation and all physical camera retests remain unverified. Test artifacts were cleaned after validation. Logs: `/tmp/qhy-mode-sdk-final-tests.log`, `/tmp/qhy-mode-build-final.log`, `/tmp/qhy2-mode-build-final.log`.

QHY5III178 physical retest resumed with user confirmation. Added hardware-only `QHY_HW_CASE=switching`: two rounds across all exposed pixel formats, each with a 0.1 s single exposure, three 0.1 s streaming frames and another single exposure, checking frame counts and RAW validity before final disconnect. This isolates the mode/bit-depth workaround from the known close failure; no configuration is saved.

## QHY5III178M hardware retest — SDK 26.7.21.5, driver v29 (2026-09-12)

User confirmed QHY5III178 connected and authorized resuming physical tests. macOS arm64, modern `ccd_qhy2`, SDK 26.7.21.5 and libusb 1.0.29.11990. Seven sequential isolated scenarios all exited 0 without an intervening USB reset:

- `switching`: two rounds of RAW8 and RAW16; each format runs single/live/single, 20 valid full-frame images in total (3056 x 2048).
- `exposure`: 0.1, 1.5, 2.5 and 16.5 s requested exposures; logical disconnect/reconnect, driver shutdown/dlclose/dlopen/init and a new exposure all succeed. First exposure includes mode-initialization overhead; this is not optical shutter timing validation.
- `abort`: interrupt a 5 s exposure and acquire a new 0.1 s exposure.
- `guide`: four 100 ms directional commands, guiding during a 1.5 s exposure, guider survival after CCD disconnect and reconnect after final guider disconnect. Command completion is verified, not physical mount motion.
- `geometry`: RAW8/RAW16, 128 x 128 ROI and all exposed modes (1x1 and 2x2).
- `settings`: gain/advanced settings restoration and available read modes with exposure.
- `stream`: three-frame acquisition, continuous stream, abort and return to single exposure.

Every scenario also completed final physical camera close and driver shutdown without a crash. This supersedes the earlier close-failure result for this camera under this exact driver/SDK combination; it does not establish which part of the combined update fixed it or guarantee other SDK/platform/camera combinations. Hot-plug remains disabled and was not tested. Legacy QHY5/QHY5L-II, cooling, camera-connected CFW, other platforms and optical image quality remain outside this retest. No configuration was saved, and test processes ended. Hardware test binaries were cleaned afterward.

Logs: `/tmp/qhy178-sdk26721-{switching,exposure,abort,guide,geometry,settings,stream}.log`.
