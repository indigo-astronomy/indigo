# Refactoring plan for INDIGO 3.0 ZWO ASI CCD driver

Date: 2026-09-11.

Status: complete for available hardware. Generated driver version 58 / 0x0300003A; 50 fake SDK cases and ASan/UBSan pass; physical ASI120MC-S and ASI294MC Pro acceptance passes. Platform and hardware applicability limits are recorded below.

## Goal and scope

Migrate `indigo_ccd_asi.c` to `indigo_generator`, following the staged approach used for `ccd_playerone`: establish executable coverage first, preserve supported capabilities, replace handwritten lifecycle/property/hot-plug scaffolding, and use handler queues with bounded acquisition finalizers. Include complete applicable CCD and optional ST4 guider coverage with a fake ASI SDK throughout implementation, followed by real camera acceptance.

Keep independently verifiable changes scoped to this driver, its tests, project registration and related documentation. Do not upgrade or edit the bundled vendor SDK, copy Player One protocol assumptions, or introduce platform-dependent driver code. Generator changes require explicit approval of a concrete proposed diff and its impact; this plan does not authorize them. Use existing generator defaults, including capacity, without a `MAX_DEVICES` override.

This is a migration inventory, not an incremental code review. Do not advance `REVIEW.md` baselines. If independent findings need tracking, follow root `REVIEW.md` and the indexed folder review file; keep test coverage in `indigo_test/CHANGES.md` and physical evidence in `TESTING.md`.

## References

- Root `AGENTS.md`, `.editorconfig`, `uncrustify.cfg`, `README.md`, `TESTING.md` and `MIGRATION_STATUS.md`.
- `indigo_docs/DEVELOPMENT.md`, `DRIVER_DEVELOPMENT_BASICS.md`, `DRIVER_GENERATOR_MIGRATION.md`, `MAKEFILES.md`, and `PROPERTIES.md`, section `ccd_asi`.
- `indigo_drivers/ccd_playerone/REFACTOR.md` and `.driver`: completed migration structure, optional guider, SDK discovery retries, shared ownership, finalizers and abort handling. Its SDK, suffix length and readout policies are different.
- `indigo_test/AGENTS.md` and `DRIVER_TESTING_RULES.md`: common, CCD and guider acceptance standards, scope and cleanup.
- `indigo_test/integration/test_ccd_playerone_sdk.c`, `test_guider_asi_sdk.c`, `test_wheel_asi_sdk.c`, `simulator_test_common.h`, `ccd_test_noise.h`, and `indigo_test/Makefile`: test architecture references, not ASI camera coverage.
- `indigo_test/hardware/test_ccd_playerone_hw.c`: capability-based physical tests, temporary configuration and separate static/dynamic lifecycle tests.
- This driver's `README.md`, `bin_externals/libasicamera/VERSION`, `include/ASICamera2.h` under that SDK directory, and bundled `doc/ASICamera2 Software Development Kit.pdf`. Header contracts below were inspected; reconcile the bundled manual during fake SDK implementation before treating its model as authoritative.

All paths are repository-relative unless explicitly described otherwise.

## Recorded baseline

Source commit: `d373999270f5070ced1a7d5fcd3958adf357a696`. The workspace was clean before adding this file. Source line references below refer to `indigo_drivers/ccd_asi/indigo_ccd_asi.c` at that commit, not future generated output.

- Handwritten C: 2,166 lines; no `.driver`. Entry `indigo_ccd_asi`, label `ZWO ASI Camera`, version `0x0300002D`, multi-device support advertised.
- README claims all ASI cameras except non-S USB2 ASI120 models. Historical tested models include ASI224MC, ASI120MM, ASI120MC-S, ASI1600MC-Cool, ASI071MC-Cool/Pro, ASI2600MM Pro, ASI174 Mini and ASI290 Mini. The ASI120 documentation is not fully consistent; do not expand supported hardware based on this list.
- Bundled VERSION says `libasicamera2 v.1.41`; libraries exist for macOS, Linux x86/x64/ARM/ARM64 and Windows Win32/x64. Record actual `ASIGetSDKVersion()` at runtime. README lists Linux/macOS; the migration table already marks Windows supported, but neither establishes a current Windows build/run.
- `make -n -C indigo_drivers/ccd_asi -f ../../Makefile.drv all` resolved the existing archive, dylib, standalone executable and macOS SDK. This was a dry run only, not a forced compilation or runtime test.
- No dedicated `test_ccd_asi_sdk` or `test_ccd_asi_hw` target is present. Existing ASI guider tests cover a different driver.
- `TESTING.md` records older ASI120MC-S/ASI224MC tests, including an ASI224MC unplug/readout timeout over 60 seconds. They are historical evidence, not validation of the proposed migration. Available physical cameras have not yet been established.

### Source and execution inventory

| Baseline lines | Responsibility and migration constraint |
| --- | --- |
| 85–117, 241–292, 501–522 | Shared private data, `count_open`, `gp_bits` connection alias, global lock, SDK Open/Init/Close, image buffer and enumeration/private mutexes. |
| 119–228 | Unity gain, preset matching, Bayer pattern and SDK pixel-format mapping. ASI120 unity gain special case is 29. |
| 294–423 | Exposure setup, ROI/start-position/control readback, single exposure start, status wait, download and stop. |
| 425–499, 654–681 | Cooler/temperature/power commands and polling, target versus measured values, temperature in tenths of a degree, 0.5-degree settling tolerance. |
| 525–652 | Single-frame and video timer workers, image publication, abort/failure cleanup and video finalization. |
| 684–730, 1654–1793 | Optional ST4 device, independent RA/DEC timers, explicit relay ON/OFF and shared session. |
| 732–854, 896–1071 | Format/bin mode inventory, metadata, four custom properties, SDK-dependent standard properties and advanced controls. |
| 857–894, 1074–1312 | Dynamic controls, connection initialization/teardown, gain/gamma/offset, presets and flash suffix. |
| 1315–1629 | Bus dispatch, acquisition conflicts, frame/bin/mode coupling, CONFIG saves and synchronous advanced SDK work. |
| 1631–1651, 1794–2166 | Detach, manual device arrays, SDK identity scans, probe opens, USB callbacks and INIT/SHUTDOWN. |

### Discovery, identity and ownership

VID is `0x03c3`. Initialization obtains supported PIDs with `ASIGetProductIDs`; arrival filters against this list and starts an unowned two-second timer. Removal schedules an unowned half-second timer and reconciles all SDK cameras. Legacy capacity is 24 logical slots; a CCD+guider consumes two. `connected_ids[]`, temporary presence arrays and shutdown private-data arrays index directly by SDK camera id with `ASICAMERA_ID_MAX = 256`.

Discovery removes the SDK-added `(CAM...)` suffix from model names, opens the camera to read its optional eight-byte flash id and eight-byte serial, then closes it. Serial is published as 16 hexadecimal characters. Default names are `model` and `model (guider)`; a model collision adds `#id`, and a stored suffix produces `model #suffix` / `model (guider) #suffix`. Preserve usable names and suffix placement, but explicitly test duplicate suffixes, bounded nonterminated SDK strings and literal `%` characters. Do not assume SDK enumeration order identifies the libusb event.

CCD is the master; `ST4Port` gates the guider, which shares the same private data, SDK handle and image buffer. Preserve CCD-only and guider-only operation and both connection orders. Generated ownership must replace `count_open`, the connection alias, manual arrays and timer lifetime management, not coexist with duplicate accounting.

### Property compatibility inventory

Use the same public naming convention as the migrated Player One driver. These are intentional compatibility changes, to be documented with configuration/script migration instructions. Keep item names, groups, labels and rules unless a separately tested correction requires otherwise.

| Baseline name | Planned name | Contract |
| --- | --- | --- |
| `PIXEL_FORMAT` | `X_PIXEL_FORMAT` | RW one-of-many; supported `RAW 8`, `RGB 24`, `RAW 16`, `Y 8`; linked to CCD mode, frame BPP and binning; CONFIG saved. |
| `ASI_ADVANCED` | `X_ADVANCED` | Dynamic RW number vector from SDK controls, SDK names/ranges, integer values and readback; CONFIG saved; bandwidth default 45. Audit read-only controls before exposing writable items. |
| `ASI_PRESETS` | `X_PRESETS` | RW at-most-one; `ASI_HIGHEST_DR`, `ASI_UNITY_GAIN`, `ASI_LOWEST_RN`; gain/offset and electron-per-ADU synchronization. |
| `ASI_CUSTOM_SUFFIX` | `X_CUSTOM_SUFFIX` | RW text, item `SUFFIX`; zero through eight bytes, empty clears, flash-backed name changes on replug. SDK documents this feature for USB3 cameras only. |

Update `indigo_docs/PROPERTIES.md` and README at the rename checkpoint, and change the documented source mapping to `.driver` at cutover. Do not add legacy aliases implicitly. No property names change in this planning checkpoint.

Capture baseline property snapshots per fake capability profile, including every `indigo_init_*_property()`, `indigo_init_*_item()`, `count`, `hidden`, permission, range and default assignment. Specifically:

- CCD INFO count 6, or 8 with serial; guider INFO count 5; model/SDK/serial, sensor dimensions, pixel size and bit depth.
- SDK format/bin-derived CCD modes, square bins, frame BPP and ROI synchronization. Current frame edits align unbinned width to 8 and height to 2 except full sensor dimensions, with minimum 64 binned pixels per dimension; compare actual SDK constraints before preserving this normalization.
- Streaming and streaming settings visible, seven image-format items, stream exposure maximum five seconds.
- SDK-dependent exposure permissions/range and microsecond conversion; gain, gamma, offset and egain; optional read-only temperature versus writable target, cooler and power.
- Advanced enumeration explicitly excludes `ASI_GPS_SUPPORT`, `ASI_GPS_START_LINE`, `ASI_GPS_END_LINE` and `ASI_ROLLING_INTERVAL`. Preserve exclusion; this migration does not add GPS/trigger/sensor-mode interfaces just because SDK APIs exist.
- CONFIG explicitly saves pixel format and advanced controls. Suffix is device flash state, not a CONFIG save item. Verify standard settings only through the driver's integration with framework persistence.

## Migration risks and required regressions

The following are source observations or test hypotheses, not claims of reproduced failures. Convert each into a failing regression before its fix where feasible.

| ID | Baseline evidence | Required outcome / test family |
| --- | --- | --- |
| ASI-M01 | 294–353: ROI, origin and exposure reads log errors and continue using output variables; final failed ROI read falls back to requested geometry. | Reject invalid/failed SDK outputs without publishing corrupt metadata or accessing an undersized buffer; F03/F04. |
| ASI-M02 | 376–412: 30,000 status polls with 2 ms sleeps; `ASIGetExpStatus` result ignored; ASI120 download adds 150 ms sleep. | Bounded status finalizer, explicit errors/deadline, delayed ASI120 cooldown without monopolizing the queue; F05/F07. |
| ASI-M03 | 1170–1181: exposure start result ignored and readout scheduled anyway. | Failed setup/start terminates acquisition/image BUSY, schedules no successful readout and allows fresh acquisition; F05. |
| ASI-M04 | 575, 590–606: video timeout is `1000 * (seconds * 2 + 500)` ms; header recommends `2 * exposure_ms + 500` ms. Countdown subtracts seconds and sets negative count to zero. | Separate bounded video reads from an overall deadline, preserve indefinite streams at durations >= 2 seconds and exact fractional timing; F06/F07. |
| ASI-M05 | 607–642: a failed video read can be overwritten by the subsequent successful stop result. | Preserve primary failure, report stop failure separately and finalize exactly once; F06/F07. |
| ASI-M06 | 857–894, 896–1167, 1242–1290: unchecked caps/control/metadata outputs; presets assign requested values after failures; advanced dispatch overwrites state with OK. | Validate SDK discovery/readback, avoid false success and handle partial preset/control writes explicitly; F02/F08. |
| ASI-M07 | 684–730, 1666–1761: guide ON errors still arm completion, OFF errors ignored; guider disconnect cancels timers without explicit relay OFF. | Correct ALERT and relay state, independent axes, relay OFF before sibling-preserving disconnect; F10/F11. |
| ASI-M08 | 1811–2120: id-indexed storage, reservation before discovery finishes, ignored attach results, master-first removal and extra close after detach. | Identity-based reconciliation, capacity/attach rollback, slave-first detach, balanced sessions and retryable discovery; F01/F02/F11. |
| ASI-M09 | 2123–2166: last action becomes INIT before PID enumeration/callback registration can fail. | Failed INIT can be retried; shutdown/reinitialization and pending discovery remain coherent; F01/F11. |
| ASI-M10 | 1292–1312: suffix cache updated before SDK write succeeds; discovery/properties have capability-dependent string assumptions. | Preserve accepted suffix on failed writes, handle unsupported flash IDs and bounded strings; F09. |
| ASI-M11 | 1315–1629: guards differ across pixel format, mode, bin, frame and other controls; advanced calls run synchronously on the bus. | Cross-property conflicts reject before accepted acquisition configuration changes; no SDK calls on the bus; F03/F07/F08. |
| ASI-M12 | 425–499: temperature output used after failed read; temperature/target/power read failures do not consistently affect the returned success. | Preserve valid measurements and propagate individual/partial cooling failures; F10. |

### SDK constraints requiring explicit decisions

- `ASISetROIFormat` takes dimensions after binning, requires width divisible by 8 and height by 2, and requires stopped capture. USB2 ASI120 has an additional area constraint in the header, but README excludes that model; do not silently broaden hardware support. Preserve/test the existing ASI120-family cooldown and unity-gain branches on supported profiles.
- `ASIGetVideoData` has an explicit blocking timeout; `ASIGetDataAfterExp` has no caller timeout. Queue priority cannot interrupt an active vendor call. Measure actual readout/abort latency on hardware and document any vendor bound that cannot be controlled; never claim a hard end-to-end deadline solely from fake results.
- Keep single exposure on `ASIStartExposure`/`ASIGetExpStatus`/`ASIGetDataAfterExp` and streaming on Start/Get/StopVideoCapture unless protocol evidence justifies a separately reviewed change. Player One's continuous-exposure workaround does not apply automatically.
- The cooler helper deliberately defers target writes until a later poll after switching the cooler on (baseline 466–487). Preserve this ordering unless SDK/hardware evidence supports changing it; test ON, subsequent target write, measured temperature and power separately.
- `ASIGetProductIDs` documents a size-query/allocation contract and deprecation in favor of `ASICameraCheck`. Preserve camera filtering while choosing the supported bundled API deliberately; do not carry an unchecked fixed-size PID write forward. Validate init rollback, same-VID wheels/focusers and delayed SDK visibility.
- Build the fake against the bundled header and audit it against the bundled manual. Model status/readout, control access restrictions and error codes independently of driver implementation. The serial motion helper is not applicable to this USB SDK camera; no serial simulator is needed.

## Target execution and ownership

Use `driver asi`, `asi_open(indigo_device *device)` and `asi_close(indigo_device *device)`. Open must either acquire all resources or roll back every resource acquired by that attempt. Increment generated `PRIVATE_DATA->count` only after successful open; later initialization failure must release only its own reference. No compensating persistent `opened` flag or manual duplicate refcount.

Use generated `sdk { hotplug = true; vid = ASI_VENDOR_ID; ... }`, existing `attach_if`/`name_value` for optional ST4 and exact names, SDK presence-based `unplug_match` where needed, and bounded generated discovery retries. Prove these against a temporary complete prototype before cutover. Failed SDK enumeration is inconclusive, not proof of removal. Reserve enough logical slots before accepting a CCD+guider; use generator default capacity and document the resulting limit.

| Work | Intended context |
| --- | --- |
| SDK discovery, attachment/removal, CONNECTION and Open/Init/Close | Generated per-driver queue. |
| Acquisition setup/readout/stop, control I/O, temperature, guide ON/OFF | Physical camera master queue; guider callback retains the guider `device` argument. |
| Exposure/video progress and pulse completion | Bounded delayed `*_finalizer` handlers, with TIME priority for pulse completion and generated URGENT abort. |
| Ordinary single-exposure countdown | Framework CCD countdown through `indigo_ccd_exposure_setup()`. |
| Bus change callbacks | Existing framework validation/copy/dispatch and short driver-specific conflict guards; no SDK calls or waits. |

Driver and master queues are different threads. Inspect generated cancellation/draining and framework locking, and prove serialization before removing the existing mutexes. Disconnect must stop producers, cancel pending work and wait for relevant active work before freeing the buffer or closing the SDK. CCD disconnect must preserve a connected guider; guider disconnect must stop both axes while preserving a connected CCD. Shared state is freed once after slave-before-master removal.

Acquisition start publishes BUSY and schedules completion; each finalizer performs one bounded progress/readout step and yields between stream frames. Preserve the exact requested exposure separately from display countdown. Streaming countdown derives from monotonic deadlines, rounds only published remaining time upward with `ceil()` and clamps to zero. Test 0.1, 1.5 and 2.5 seconds as distinct timing branches.

Urgent abort must handle overtaking a queued start: cancel that pending start plus completion work, settle properties even if SDK start has not run, and publish a terminal idle-abort result too. Do not add defensive cancellation to every new request already protected by the BUSY macro. Record the intended same-axis guide overlap/zero policy and test it against generated guards rather than accidentally preserving legacy timer replacement.

Use generator attributes and epilogues, `connection_result` and no early return from connection/disconnection blocks. Inspect every generated handler containing `_finalizer`: it must explicitly publish start, immediate failure/no-op and completion because the generator suppresses its usual OK/update epilogue. Other handlers normally need only an ALERT assignment on failure. Do not duplicate framework numeric validation or BUSY checks.

## Atomic implementation sequence

Each checkpoint is a separately reviewable change, split further where noted. Checkboxes below are the authoritative progress tracker; update them after every completed action and keep partial/unverified work unchecked. Detailed execution records supplement these checkboxes. Record commands/results and update coverage as it lands. Never cut over to a partial generated driver that has lost existing acquisition, controls or guiding. Every behavior fix increments the driver's current version; `.driver` version is authoritative after cutover. Final version must exceed baseline `0x0300002D`.

1. **Executable baseline and fake boundary.**
   - [x] Record source/worktree, compiler/OS/architecture, SDK runtime version and force compilation of both driver translation units.
   - [x] Add `integration/test_ccd_asi_sdk.c` with separately compiled production driver, fake ASI SDK and libusb boundary; keep real bus, CCD/guider bases, queues and timers. Split fake helpers into `ccd_asi_fake_sdk.[ch]` if useful.
   - [x] Add a narrow build target and register it in normal hardware-free integration tests. Capture capability/property snapshots, success paths, session/USB accounting and deterministic image fixtures.
   - [x] Add test coverage/planned gaps to `CHANGES.md`; mark known failing regressions explicitly instead of blessing defects as expected behavior.
   - [x] Run an untouched d373999 physical baseline on ASI120MC-S: 1.5 s/2.5 s snapshots and long finite streaming pass; abort then 0.1 s snapshot reproduces ASI_EXP_FAILED (status 3). Detailed checkpoint 8 comparison below.
   - Acceptance: production rebuild and baseline cases pass; source risks have reproducible tests or a specific pending test assignment.

2. **Isolated handwritten formatting.**
   - [x] Format only the ASI driver source/header/main where needed: tabs, K&R/braced control flow, spaces, one blank line between functions, none within, and single-line calls/macros.
   - [x] Preserve license/history, extend copyright through 2026 and follow the Player One refactoring notice convention. Keep expressions, commands and SDK order unchanged.
   - Acceptance: `git diff --check`, manual semantic diff inspection, rebuild and unchanged baseline suite. Never hand-format generated output later.

3. **Custom-property compatibility change.**
   - [x] Apply the four-name mapping above, update internal handles, README migration notes, `PROPERTIES.md` and test snapshots together.
   - [x] Verify new names, absence of old advertised names, item compatibility and save/change/load with restored SDK settings.
   - Acceptance: only intended public-name differences; acquisition and guider baseline remains green.

4. **SDK correctness fixes with regressions.**
   - [x] Implement setup/start/status failure handling, control/preset readback, suffix transaction handling and Open/Init/post-open rollback in separate fixes; 12-case checkpoint suite passed (details in execution record).
   - [x] Add and pass adversarial regressions for ASI-M01/M03/M06/M08/M09/M10: setup/start/status errors, invalid geometry, control read/write/partial presets, discovery/attach/INIT failure and suffix boundaries (named Matrix/Edges cases).
   - [x] Add identity/dimension/allocation bounds, control count/range checks and failure-output rejection; no fallback to uninitialized SDK outputs.
   - [x] Complete malformed geometry, sparse-bin, limited-format and permission/readback edge cases: invalid SDK bin and unsupported requested bin reproduced before fixes and passed afterward; version 0x03000033.
   - Acceptance: each fix has a regression failing before and passing afterward, production rebuild and affected narrow cases; retain coverage for successful workflows.

5. **Queue-compatible acquisition and peripherals.**
   - [x] Split single-exposure finalizers, video finalizers and peripheral/guider queue migration into verifiable changes; preserve functioning lifecycle between them. Handwritten version 0x03000031 passed 16/16 cases.
   - [x] Implement ASI-M02/M04/M05/M07/M11 changes: bounded waits, monotonic countdown, ASI120 delayed cooldown, primary-error retention, exact finite counts, explicit relay-off and acquisition conflict guards.
   - [x] Pass stop/OFF failure, opposed-axis, SDK-read/poll removal and SDK-entry timing checks; full 47-case normal and ASan/UBSan suites pass.
   - [x] Add deterministic abort-before-start, long indefinite video abort/restart, idle abort, guide directions and simultaneous axes/disconnect checks.
   - [x] Add both final-frame/abort orders and gated readout/disconnect with surviving guider; Matrix readout_abort_and_restart/disconnect_readout_with_sibling and Edges video_final_frame_abort_orders pass.
   - [x] Guide timing measured during normal periodic temperature polling and streaming; all SDK-overlap assertions pass. Gated removal during temperature SDK reads also passes.
   - Acceptance: no sleeping exposure/video loop occupies the queue, no bus SDK work, no stale completion, bounded functional liveness; measure SDK-entry guide timing and record vendor readout limitations.

6. **Complete generator cutover.**
   - [x] Verify SDK discovery/retries, PID filtering, optional guider, names, shared rollback and removal against the complete generated driver and fake SDK. This supersedes the planned temporary prototype; the existing generator expresses the migration without changes. Repeated generation of C/header/main is byte-identical.
   - [x] Write the full `.driver` from the tested implementation in generator-owned blocks; remove superseded lifecycle/property/hot-plug scaffolding and duplicate ownership. Generated version 0x03000032 passed the same 16 cases and production build.
   - [x] Generate `.c`, `.h`, `_main.c`; add `.driver` to the Xcode group and applicable project listings; update `PROPERTIES.md` source mapping.
   - Acceptance: complete production build and full ASI fake suite pass; explain behavioral differences in generated diff, inspect finalizer epilogues and BUSY guards; second generation is byte-identical. No `MAX_DEVICES` override.

7. **Close the fake SDK acceptance matrix.**
   - [x] Complete applicable F01–F12 coverage with 50 named normal cases plus opt-in timing. Common/CCD/guider property inventories, capability profiles, zero-count streaming, retry exhaustion and gated snapshot/video/poll removal pass; all failure paths use fixture cleanup. Physical electrical/optical validation remains explicitly separate.
   - [x] Pass the 43-case normal suite at version 0x03000035; add and pass sensor-only/no-temperature inventory and limited-format regressions (44 normal cases now registered). The fake filters cooler-only controls and applies missing/reordered controls consistently.
   - [x] Complete opt-in SDK-entry guide timing: 20/100/500 ms in four directions, idle and streaming, one warm-up plus four samples per group (96 measured samples). Results report signed error and distribution; electrical timing remains hardware-only.
   - [x] Run the final 50-case normal and ASan/UBSan suites at version 0x0300003A: all pass; driver, fake, bus, driver base, CCD, guider, timer, I/O and RAW helpers instrumented. Vendor codecs and remaining framework archive objects are uninstrumented. Existing complete timer unit suite also passes; no electrical or real SDK latency claim.
   - [x] Build available macOS universal arm64/x86_64 driver/archive/standalone targets. Runtime and sanitizers verified on arm64; Linux and Windows builds/runtime and Intel runtime are unavailable in this workspace and remain unverified.
   - Acceptance: each applicable scenario maps to named cases and results in this file and `CHANGES.md`; complete coverage is not defined by an arbitrary case count or a smoke test.

8. **Physical acceptance and final documentation.**
   - [x] Add opt-in `hardware/test_ccd_asi_hw.c`, `test-ccd-asi-hw` and a real dynamic-driver reload mode/target, following Player One's harness structure with ASI capabilities and eight-byte suffix limits.
   - [x] Execute applicable H01–H08 on ASI120MC-S and ASI294MC Pro: complete suites, user-performed USB cycles, suffix restoration and final-version dynamic reload pass. See the final acceptance table below for applicability; precise physical removal inside SDK readout and unprovided hardware/platforms are explicitly separate. Hardware tests remain opt-in.
   - [x] ASI120MC-S final full physical suite at version 58 passes (`hw-120-final.log`): all four formats, all five frame types, ROI/bin/config/controls, fractional/long/short exposures, abort/restart, exact finite and sustained streaming (102 frames/~10 s), all guide directions, simultaneous axes, BUSY reversal rejection, zero requests, sibling-preserving disconnect, both connection orders and shutdown/reinitialization. Long-stream abort 2.502 s; following short snapshot recovered after one retry in 2.645 s.
   - [x] ASI294MC Pro final complete suite at version 58 passes (`hw-294-final.log`): all formats and five frame types, geometry/config/controls, target 21 °C settled at 21.4 °C with 5% power, original cooler/target restored, long-stream abort 0.345 s and fresh short snapshot, 108 frames/~10 s, reconnect and shutdown/reinitialization. ST4 is not present.
   - [x] ASI294MC Pro earlier default physical suite passes: all four formats and ROI/bin checks, controls/presets/configuration, cooling from ~20 °C to 19.3 °C for a 19 °C target with original target/cooler restored, fractional exposures, long/short streams, abort/restart, reconnect and driver shutdown/reinitialization. No ST4 on this model; subsequent successful cable-cycle and dynamic-reload results are recorded below.
   - [x] Compare original advanced-property dispatch and exclusions against baseline d373999: partial requests write only supplied items; SDK failures were logged then masked by OK; GPS/rolling controls alone are excluded.
   - [x] Isolate ASI120 acquisition failure from advanced-control changes: fresh connection passes 1.5 s/2.5 s snapshots and three 2.5 s streaming frames, then the first 0.1 s snapshot fails after abort (SDK/general status error 16). Terminal stream abort took 2.486 s.
   - [x] Compare with untouched d373999 on ASI120MC-S: aborting a 100-frame, 2.5 s stream after three received frames also causes the next 0.1 s snapshot to fail (ASI_EXP_FAILED, status 3); original abort takes 4.999 s and delivers a fourth frame. This failure predates migration.
   - [x] Diagnose and verify ASI120 recovery after long-stream abort: version 58 real hardware passed the 0.1 s snapshot after two retries (3.883 s), three further short snapshots, abort of a 5 s snapshot, exact five-frame stream and a sustained stream (105 frames/~10 s). Fake coverage includes three failed snapshots before recovery, retry exhaustion, cancellation/errors, delayed successive video frames and watchdog recovery. Final complete-suite replay follows the corrected hardware BUSY-rejection assertion.
   - [x] Direct SDK 1.41 ASI120 tests reproduce ASI_EXP_FAILED after duration transitions, with or without preceding video; immediately retrying the same snapshot succeeds without StopExposure/reset. This matches the ASIGetExpStatus header contract.
   - [x] Add a failing fake regression and implement one queued ASI120-only retry for ASI_EXP_FAILED in version 54 / 0x03000036. Preserve requested duration/dark flag; cancel retry on abort/disconnect, retain ALERT on repeated failure. Physical verification is in progress.
   - [x] ASI294MC Pro eight-byte suffix write/replug/name/image, clear/replug/name/image and original empty suffix restoration all pass. ASI120 stays operational during both removals (`hw-294-suffix.log`).
   - [x] ASI120MC-S actual dynamic driver dlclose/dlopen and fresh image after INIT pass (`hw-120-reload.log`); final version 58 replay also passes (`hw-120-reload-final.log`).
   - [x] ASI294MC Pro dynamic reload: a 0.1 s frame passes before shutdown, dlclose/dlopen succeeds, and a fresh 0.1 s frame passes after INIT; both sessions disconnect cleanly. Final version 58 replay also passes (`hw-294-reload-final.log`).
   - [x] ASI294MC Pro physical USB cycles with user: idle, active 0.1 s streaming and active 120 s exposure all detach and recover with a new 0.1 s image after replug. Removal produces expected SDK removed-camera/stop errors; cleanup and rediscovery complete.
   - [x] ASI120MC-S physical removal during simultaneous 120 s exposure and 60 s ST4 pulse detaches both logical devices and releases the session.
   - [x] ASI120MC-S idle USB removal/replug: both logical devices reconnect, a fresh 0.1 s image and ST4 pulse pass; ASI294MC Pro stays connected and acquires an image while ASI120 is absent. Evidence: resumed `hw-120-physical.log`.
   - [x] ASI120MC-S USB removal during a 60 s ST4 pulse: both logical devices detach, then reconnect and pass a fresh image and guide pulse; ASI294 acquires while ASI120 is absent.
   - [x] ASI120MC-S eight-byte suffix: original empty; write INDIGOT1, physical replug, verify CCD/guider names and image/pulse; clear, physical replug, verify original names and image/pulse. Original empty suffix restored, both interfaces disconnected; all suffix-cycle assertions pass.
   - [x] ASI120MC-S removal during an active 120 s exposure: expected removed-camera stop failure, both logical devices detached, ASI294 acquired while ASI120 was absent, replug followed by fresh image and ST4 pulse passed (`hw-120-active-exposure.log`).
   - [x] ASI120MC-S active streaming USB removal/replug: terminal read failure and detach, ASI294 image while ASI120 absent, then restored image and ST4 pulse pass (`hw-120-active-stream.log`). The earlier expired 120 s exposure cycle is counted only as idle removal; the dedicated active-exposure run above supplies active-removal evidence.
   - [x] Final cleanup: both cameras' original empty suffixes and modified settings restored, all logical devices disconnected and all test/diagnostic processes ended. `make -C indigo_test test-clean` removed generated test outputs after final verification.
   - [x] Final records synchronized: physical evidence in `TESTING.md`, fake coverage in `CHANGES.md`, ASI status columns in `MIGRATION_STATUS.md` with Comment preserved. Version 58 / 0x0300003A exceeds baseline 0x0300002D; independent generation of C/header/main is byte-identical. Xcode registration and project syntax, universal production build and `git diff --check` pass.
   - Acceptance: each row is passed, failed, unavailable or not applicable with evidence. Missing cooled/ST4/second-camera/platform coverage stays explicit; do not label full hardware acceptance from one camera profile.

## Fake SDK coverage matrix

The requirements below are implemented by the named-case mapping. All 50 normal cases and ASan/UBSan pass; hardware and platform applicability are recorded separately. Use public property requests and entry points, fresh property revisions, exact SDK argument/order assertions and deterministic pixel fixtures. Mock transport/SDK boundaries, not the driver. No real USB enumeration, vendor library, server or network sockets in normal integration tests.

Use representative mono/no-ST4, color/ST4, cooled, temperature-only, ASI120-family and limited-format/bin profiles, plus malformed-output/failure fixtures. Cross distinct branches rather than every possible capability combination. Generic property validation, image encoders and configuration storage are framework responsibilities.

| Family | Required scenarios and assertions | Checkpoints |
| --- | --- | --- |
| F01 Discovery/identity | Startup enumeration, supported/foreign VID/PID and same-VID non-camera, delayed visibility/retry exhaustion, duplicate/reordered events, two cameras, equal model/suffix, sparse/boundary/invalid SDK ids, count/metadata/descriptor/PID/registration failure, logical capacity, failed attach, retry after failed INIT. Exact associations, no leaked USB refs/reservations or orphan guider. | 1, 4, 6, 7 |
| F02 Lifecycle/capabilities | Common/CCD/guider property completeness; dynamic counts/permissions/defaults; open/init/global-lock and post-open initialization failures; controls absent/reordered/read-only; reconnect rebuild; CCD alone, guider alone, both connection orders and disconnect orders, sibling survival and last close. | 1, 4, 6, 7 |
| F03 Geometry/formats | All advertised RAW8/RGB24/RAW16/Y8 branches with deterministic payload/length/dimensions/channel/Bayer assertions; full frame and nonzero ROI, post-bin alignment, minimum size, supported sparse bins, frame/BPP/mode/bin coupling, SDK-adjusted and malformed geometry, buffer bounds. No stale accepted settings after acquisition conflicts. | 1, 4, 5, 7 |
| F04 Frame types/units | Light, dark, flat, dark-flat and shortest bias integration; exact exposure microseconds, 0.1/1.5/2.5 seconds, SDK ROI/origin read/write failure and exposure-control failure. Assert dark flag where SDK accepts it; video limitations explicit. | 1, 4, 5, 7 |
| F05 Single acquisition | BUSY/start/pending/success, status IDLE/FAILED/invalid/error, setup/start/readout failure, watchdog expiry and fresh acquisition; exactly one image or failure, ASI120 cooldown and unity-gain branch. | 1, 4, 5, 7 |
| F06 Streaming | Exact finite counts and indefinite counts at short and >=2-second exposure, inter-frame yielding, bounded timeout/not-ready retry, start/read/stop failures including read-fail/stop-success, primary error retention, stop/restart and exactly-once video finalization. | 1, 5, 7 |
| F07 Abort/countdown | Abort idle, queued start, exposure wait, status/readout, stream wait and final frame in both orders; no frame after terminal abort, no stale work on restart; distinguish in-flight frame after request from post-terminal frame. Shared countdown progresses with busy master queue; display rounding never changes hardware duration. | 5, 6, 7 |
| F08 Controls/presets | Gain/gamma/offset, all three presets, egain refresh and ASI120 unity gain; individual/partial write and readback failures, absent/invalid metadata, advanced dynamic names/permissions, bandwidth 45 and GPS/rolling exclusions. SDK work off bus, cross-property busy rejection and accepted-value preservation. | 1, 4, 5, 7 |
| F09 Persistence/strings | Pixel/advanced and relevant standard-setting save/change/load with SDK readback in isolated storage; new property names; suffix empty, 1/8/>8 bytes, unsupported device, write failure, nonterminated SDK id/model/control name, literal percent, replug naming/collisions. No flash cache update on failure. | 1, 3, 4, 7 |
| F10 Cooling | Sensor-only, cooled and absent capabilities; tenths-to-degrees, target versus measured, cooler on/off, power readback, settling tolerance, each read/write failure and recovery, slow initialization crossing first poll, polling during acquisition and cancellation at disconnect. | 1, 4, 5, 7 |
| F11 Guide/races/teardown | All directions and ms conversion, zero/opposed items, same-axis busy/replacement policy, simultaneous axes, ON/OFF failure, no cross-axis stop; imaging plus guiding, pending pulse disconnect, removal idle/exposure/readout/video/pulse/poll, rejected versus accepted shutdown with queued discovery, INIT/SHUTDOWN and reload. Assert no post-close calls, post-detach updates, double close/free or deadlocks. | 1, 4, 5, 6, 7 |
| F12 Timing evidence | Monotonic ON-to-OFF intervals at fake SDK entry for 20–500 ms in all directions, idle and acquisition workloads; repeats/warm-up and signed error statistics per guider standard. Functional liveness stays in normal tests; host-dependent precision/load statistics are separate opt-in measurements. | 5, 7 |

Use bounded condition-variable gates for races, an outer deadline for deadlocks, and guaranteed gate release/thread join on failures. Record opened/initialized/closed sessions, active calls, relay masks, queue/bus thread identity, USB references, image and terminal-event counts. Temporary config/output must not alter HOME or user profiles. Fake SDK timing establishes software call timing, not electrical pulse accuracy or a real SDK blocking bound.

### Implemented scenario-to-test mapping

All functions are in `indigo_test/integration/test_ccd_asi_sdk.c`. This mapping identifies coverage, not blanket completion of every requirement in each family.

| Family | Named test functions |
| --- | --- |
| F01 | `discovery_identity_and_reordering`, `discovery_filter_failures_retry`, `discovery_attach_and_init_rollback`, `malformed_identity_recovery`, `shutdown_pending_discovery_and_capacity` |
| F02 | `properties_and_exposure`, `open_init_rollback`, `initialization_read_rollback`, `guider_shared_session`, `capability_rebuild_and_cooling`, `control_metadata_rollback` |
| F03 | `formats_and_roi`, `limited_formats_and_bins`, `malformed_geometry_and_recovery`, `acquisition_conflicts_preserve_configuration` |
| F04 | `frame_types_and_fractional_countdown`, `setup_write_origin_and_control_failures`, `setup_read_failure_recovers` |
| F05 | `properties_and_exposure`, `start_failure_is_immediate`, `failed_readout_recovers`, `exposure_status_failures`, `snapshot_watchdog_recovery`, `asi120_family_cooldown_and_unity` |
| F06 | `finite_stream`, `long_stream_abort_restart`, `video_error_paths`, `finite_video_abort_and_timeout` |
| F07 | `abort_overtakes_pending_start`, `idle_abort_terminal`, `readout_abort_and_restart`, `video_final_frame_abort_orders`, `frame_types_and_fractional_countdown` |
| F08 | `control_failures_and_presets`, `advanced_failure_is_alert`, `unchanged_advanced_controls_are_not_written`, `malformed_numeric_readback`, `control_metadata_rollback`, `acquisition_conflicts_preserve_configuration` |
| F09 | `configuration_roundtrip`, `suffix_boundaries_and_failure`, `discovery_identity_and_reordering`, `malformed_identity_recovery` |
| F10 | `temperature_read_failure`, `capability_rebuild_and_cooling`, `cooling_reads_writes_and_settling`, `initialization_read_rollback`, `sensor_only_and_sensor_absent` |
| F11 | `guide_all_axes_and_disconnect`, `guide_failures_and_overlap`, `disconnect_readout_with_sibling`, `unplug_active_and_replug`, `shutdown_pending_discovery_and_capacity` |
| F12 | `guide_timing_measurements (--timing only)` |

Additional cases now pass: `discovery_retry_exhaustion`, `opposed_guide_requests`, and `unplug_during_sdk_read` (snapshot/video/temperature poll). `property_inventory` now checks inherited/common/CCD/guider visibility and interface bits; `finite_stream` also verifies zero count makes no SDK capture or image. Electrical guide timing, optical color correctness and real vendor blocking behavior require hardware evidence.

## Physical hardware acceptance matrix

Available and connected equipment confirmed by the user: ASI120MC-S and ASI294MC Pro. Physical tests are in progress; see checkpoint 8 checkboxes and execution records. The user requested final hardware testing; establish model/serial/capabilities and availability when preparing the hardware checkpoint. Record OS/architecture, source revision plus dirty state, driver/SDK/firmware versions where available, exact commands, timings and results. Use discovered capabilities and restore settings even on failure.

| ID | Required workflow | Evidence / applicability |
| --- | --- | --- |
| H01 | Model/serial and CCD/guider association; each logical interface alone, both connection orders, disconnect either with sibling operational, reconnect and fresh operation. | All available cameras; ST4 cases only if present. |
| H02 | Repeated short and completed long exposures, fractional >1 s exposure, every available SDK format, representative bins and nonzero ROI; supported frame types. | Dimensions/BPP/payload/file integrity, Bayer/channel layout and usable image content; no optical calibration requirement. |
| H03 | Abort long exposure and reacquire; finite exact-count stream, sustained indefinite short and >=2 s stream, abort/restart. | Record duration/frame counts, request/BUSY/terminal abort latency and post-terminal image count, including readout delay. |
| H04 | Gain/gamma/offset/presets and available advanced controls, bandwidth, configuration save/change/load with readback. | Isolated configuration; restore original values; fan/heater only where exposed and applicable. |
| H05 | Cooled-camera target/temperature/power observation, cooler toggle, settling state and reconnect readback. | Cooled profile required when available; sensor-only profile checks temperature. No thermometer or cooling-rate calibration. |
| H06 | All guide directions, simultaneous axes, same-axis overlap and zero policy, imaging plus guiding, disconnect during pulse, reconnect/pulse. | ST4 models; command/state/cleanup evidence, no external electrical measurement required. |
| H07 | Actual USB unplug/replug during idle, long exposure, readout where practically reproducible, streaming and pending guide pulse. | Detach/re-enumerate/reconnect then new image/pulse; second camera survives when available. Synthetic events are fake coverage only. |
| H08 | Supported eight-byte flash suffix write/replug/name verification, clear/replug and restoration; disconnected INIT/SHUTDOWN and actual dynamic-library unload/reload followed by fresh image. | Record original suffix, use a bounded restoration cycle; unsupported USB2 flash feature is N/A. Distinguish driver reload from whether host unloads the SDK image. |

Available ASI120-family hardware should additionally verify the retained cooldown/unity-gain workaround. Mono/color/cooled/ST4/multi-camera and platform gaps remain explicitly unavailable until exercised; successful fake coverage is required even for supported features absent from available hardware.

## Execution record

| Date | Checkpoint | Evidence | Result |
| --- | --- | --- | --- |
| 2026-09-11 | Planning | Clean baseline recorded; handwritten source, SDK header contracts, Player One structure, test rules and existing build/test registrations inspected; driver make dry run resolved targets. | This document added. No compilation, fake runtime or physical hardware validation claimed. |

For each implementation checkpoint append source revision, affected test case names/commands, pass/fail counts, SDK/platform details, regressions fixed, outstanding matrix rows and cleanup outcome. Completion requires synchronized generated files, a version greater than baseline and truthful migration status; this planning document alone changes none of those statuses.

### Checkpoint 1 — executable baseline (2026-09-11)

- Source remains byte-identical to baseline commit. Existing user Xcode registration of this document was preserved. New integration source is also registered; root AGENTS now requires project registration for every new persistent file.
- macOS 26.6.2 (25G83), Apple clang 21.0.0, native arm64. Forced both driver translation units with `make -C indigo_drivers/ccd_asi -f ../../Makefile.drv -W indigo_ccd_asi.c -W indigo_ccd_asi_main.c all`; universal arm64/x86_64 archive, dylib and standalone built. Vendor minimum-macOS linker warnings remain. Bundled runtime `ASIGetSDKVersion()` returned `1, 41, 0, 0`.
- An initial fake attach crash was caused by stale framework objects compiled against older headers. Recompiling all framework C/C++ translation units removed it without changing source. Do not attribute that crash to ASI behavior.
- Added `test_ccd_asi_sdk`/`test-ccd-asi-sdk` with separately compiled real driver/framework, fake camera/libusb boundary, real timers/queues, bus observation with fresh revisions and temporary configuration. Seven initial named cases cover properties/image/save, finite video, shared guider, Open/Init rollback, formats/ROI, configuration roundtrip and read failure/reacquire.
- Initial execution: properties/image/save, finite video, shared guider, Open/Init rollback and read failure/reacquire passed functional assertions. Cleanup consistently detected redundant global unlock at original ccd_detach line 1639 after asi_close line 512. Full suite is not yet green. RGB fixture originally used RGB order although original image handoff requests BGR conversion; corrected the fixture. Y8 incorrectly carries Bayer metadata in the original driver. Configuration LOAD needs investigation with a wait spanning the five-second cooler polling interval; no successful roundtrip claimed yet.
- SDK manual pages 8–12 checked against the header: Open/Init/Close lifecycle, binned ROI, image size, video timeout in ms, distinct snapshot/video paths, explicit ST4 OFF and eight-byte USB3 flash ID. Header is authoritative for newer signatures absent from this older manual.
- Physical baseline is deferred to the hardware checkpoint; confirmed equipment is ASI120MC-S and ASI294MC Pro. No physical behavior claimed from SDK-version lookup.
- Deviation: retain the newly reproduced failing checks while proceeding through isolated formatting and then correctness fixes. Do not encode defects as passing expectations or call the full baseline suite passed.

### Checkpoint 2 — isolated formatting (2026-09-11)

Formatted only ASI handwritten C/header/main, preserved the license and extended copyright through 2026. Added mandatory control-flow braces, normalized whitespace and single-line calls, removed blank lines inside functions. Non-comment token comparison against baseline, ignoring brace tokens, is identical. Production universal build and test compilation pass; `git diff --check` passes. Baseline functional results are unchanged: six cases reach their functional completion, while the format case is rejected by the image observer for original Y8 Bayer metadata; CONFIG roundtrip still lacks terminal completion within 20 seconds and all fixtures expose redundant unlock. These remain active failures, not formatting regressions. Proceeding to the planned rename and correctness checkpoints.

### Checkpoint 3 — custom property names (2026-09-11)

Renamed the four custom properties and internal handles to X_PIXEL_FORMAT/X_ADVANCED/X_PRESETS/X_CUSTOM_SUFFIX, updated README and PROPERTIES.md together, and incremented DRIVER_VERSION to 0x0300002E. Universal production build succeeds. Seven seed cases execute against new names; configuration SAVE/change/LOAD now passes with verified SDK WB_R restoration after correctly isolating CCD_LOCAL_MODE.DIR in the test. The earlier CONFIG result was ALERT (state 3), not a missing completion: restoring the default unwritable image directory failed framework validation. This was a fixture issue, not an ASI defect. Remaining full-suite failures are the original redundant unlock and Y8 Bayer metadata; no new rename failure observed.

### Checkpoint 4 — SDK correctness (2026-09-11)

Version advanced through 0x0300002F (redundant detach unlock and Y8 metadata) to 0x03000030 (SDK errors). The complete 12-case suite passed against the handwritten implementation with cleanup assertions enabled. `Start failure immediate recovery` and `Advanced failure ALERT recovery` both failed before their fixes and passed afterward. Added initialization read rollback across exposure/gain/gamma/offset/power/bandwidth, ROI read failure/recovery and temperature read failure tests. Production universal build passed before the queue conversion.

Changes: fail immediately on setup/start/read/status errors; roll back successful Open/Init when later control initialization fails; do not use failed SDK outputs; retain advanced write errors after readback; preserve suffix cache on failed flash writes; use readback for gain/offset/presets and validate egain; propagate cooling read failures. Added bounded SDK identity/metadata checks and capability-output checks, and selected the first supported format rather than assuming RAW8. Test fixture BGR input now explicitly matches the original image handoff; physical channel verification remains pending.

Scope refinement: complete discovery reservation/attach/unplug ownership, PID registration rollback and additional malformed/capability adversarial cases are assigned to checkpoints 6/7, where generated lifecycle replaces those paths. This checkpoint does not claim those remaining matrix cases passed. All supported capability acceptance still gates final completion.

### Checkpoint 5 — queued operations (2026-09-11)

Version 0x03000031: all 16 fake SDK cases passed, including 2.5-second indefinite video, abort/restart, abort overtaking a queued start, idle abort and all guide directions with disconnect. Bounded acquisition finalizers replace blocking exposure/video loops; the shared CCD countdown handles snapshots and streaming publishes a monotonic ceil countdown independently of exact SDK microseconds. Controls and periodic temperature polling execute on the device queue. Guide OFF uses TIME-priority finalizers.

The queue already locks the master device; removing duplicated master locking in handwritten connection handlers fixed a reproduced deadlock. Generated connection handlers run on the driver queue and therefore explicitly lock the master around SDK initialization/cleanup. Indefinite streaming abort is OK per indigo_ccd_abort_exposure_cleanup; finite stream abort remains ALERT. The test was corrected to this framework contract. No physical timing claim or complete F01–F12 acceptance is made at this checkpoint.

### Checkpoint 6 — generator cutover (in progress, 2026-09-11)

Created indigo_ccd_asi.driver as the source of truth (version 50 / 0x03000032), registered it in Xcode, and regenerated C/header/main with the unchanged generator. Generator-owned SDK hot-plug, optional ST4 guider, shared connection references and property lifecycle replace handwritten scaffolding. PID filtering uses the bundled SDK ASICameraCheck API, and missing SDK ids determine removal independently of event order. No MAX_DEVICES override or generator implementation change.

Validation so far: all 16 existing fake SDK cases pass against generated code, including balanced teardown/USB refs; universal production build passes. The initial generator invocation stalled on a legacy block comment outside DSL code; converted that input comment to line comments and reran successfully. Finalizer handlers and connection rollback output have been inspected.

Still in progress: capability/geometry/readback edge cases, complete discovery/race/failure matrix expansion (checkpoint 7), source formatting and regeneration consistency checks. Hardware acceptance has not started. Do not interpret this checkpoint entry as final migration acceptance.

Checkpoint 6/7 progress: author corrected to Peter Polakovic at user request. Version 0x03000033 adds SDK geometry/readback validation, sparse-bin rejection, first-supported-mode selection and cooler/target initialization readback. All 16 Matrix cases passed; Edges checks found fixture expectations needing correction for the actual five-logical-device generator capacity and suppressed unchanged property notifications. Those corrections are being verified. Real SDK inventory outside the sandbox sees ASI120MC-S (1280x960, ST4, uncooled) and ASI294MC Pro (4144x2822, cooled, no ST4). Physical acquisition is still pending.

### Hardware/sanitizer regressions in progress (2026-09-11)

- Physical ASI120MC-S: RAW8/RGB24/RAW16/Y8 payload geometry and ROI 256x256 at (16,24), bin 1/2 passed. CONFIG restore exposed unconditional advanced writes: SDK 1.41 advertises OverCLK 0..30 writable but rejects writes. Fake `Unchanged advanced controls` reproduced the collateral failure before the fix. Version 0x03000034 reads current controls, writes only changed values, then checks readback and retains write failure. A deliberate OverCLK request still reports ALERT; no control is silently hidden. Further hardware acceptance remains unchecked.
- ASan/UBSan instrumented driver/fake plus bus, driver base, CCD, guider, timer, I/O and RAW helpers. Found a framework queue use-after-free: queue_func freed running_task before taking queue->mutex while indigo_queue_remove inspected it under that mutex. Moved retirement/free under the same mutex. Also fixed framework INFO name/model initialization to treat device names containing percent as data, and an INT_MAX+1 overflow in the fake serial-number fixture. These are scoped prerequisite fixes reproduced by the ASI acceptance suite; the final 47-case ASan/UBSan rerun passes without diagnostics.

Original advanced-handler comparison verified from baseline d373999: handle_advanced_property (857–893) iterates the incoming request items; ccd_change_property (1550–1559) passes that request before copying it into the cached property, then unconditionally publishes OK. Thus ordinary partial changes did not write every cached item; CONFIG LOAD passed the saved full property. Neither OverCLK nor HighSpeedMode was excluded (only GPS/ROLLING_INTERVAL at 1045–1052). The initial queued extraction widened partial writes; this is corrected by only writing changed accepted targets, with SDK readback and truthful ALERT. ASI120 hardware now passes CONFIG roundtrip and explicitly confirms SDK rejection/value retention for OverCLK and HighSpeedMode. A subsequent acquisition failed after the control sweep; diagnosis is ongoing, so H02–H04 are not marked fully accepted.

Physical original-driver comparison: compiled the unchanged d373999 source/header into a temporary executable against the same framework and SDK 1.41. Used the same fractional-exposure harness, with a finite count of 100 because the original indefinite countdown incorrectly converts negative count to zero. Both initial snapshots passed; abort requested after three stream frames delivered one additional frame and completed in 4.999 s. The following 0.1 s snapshot reported ASI_EXP_FAILED (3), matching the migrated failure path. This establishes a pre-existing problem; it does not establish SDK root cause or successful recovery. Temporary baseline sources/binary remain outside the repository.

ASI294MC Pro default hardware run: macOS 26.6.2 arm64, SDK 1.41.0.0, dirty worktree driver 0x03000035; `INDIGO_TEST_DEVICE='ZWO ASI294MC Pro' indigo_test/build/hardware/test_ccd_asi_hw --run` passed. Full 4144x2822 RAW8 frames are 11694405 bytes including RAW header. Long-stream terminal abort 0.350 s, subsequent short snapshot passed; sustained 0.1 s stream delivered 109 frames in approximately 10 seconds. Driver shutdown/reinitialization and final disconnect passed. SDK library stayed loaded; real dlclose/dlopen, cable cycles and suffix flash acceptance remain separate.

The full 47-case normal and ASan/UBSan suites passed after the final discovery/guide/removal cases. An initial opposed-guide assertion read the relay before the queued handler ran (framework BUSY precedes SDK entry); the test now waits for the SDK relay mask. No production behavior change was needed. Physical ASI294 idle hotplug was not exercised: the bounded wait expired without a cable removal and the harness disconnected cleanly; this is missing user-performed hardware input, not a driver hotplug failure.

ASI294MC Pro original-driver cross-check: unchanged d373999 baseline passed 1.5 s/2.5 s snapshots, a 100-frame 2.5 s stream aborted after three received frames, and the subsequent 0.1 s snapshot. Original abort delivered a fourth streaming frame and completed in 3.058 s. Unlike ASI120MC-S, this sequence passes on ASI294MC Pro with both original and migrated drivers.

### Fake SDK guide timing results

macOS 26.6.2 arm64, production guide callbacks unchanged between this measurement and version 0x03000035. Four measured samples after one warm-up per row, monotonic time at SDK ON/OFF entry; signed errors in milliseconds. Streaming workload is 0.05 s indefinite acquisition with normal temperature polling. These measurements do not establish physical relay timing or a maximum real SDK delay.

| Workload | Direction | Requested ms | Min error | Mean error | Median error | p95 error | p99 error | Max error | Std dev |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| idle | EAST | 20 | 0.822 | 3.138 | 3.360 | 5.011 | 5.011 | 5.011 | 1.884 |
| idle | EAST | 100 | 0.632 | 2.967 | 3.108 | 5.021 | 5.021 | 5.021 | 1.588 |
| idle | EAST | 500 | 0.049 | 3.768 | 5.001 | 5.022 | 5.022 | 5.022 | 2.147 |
| idle | WEST | 20 | 3.545 | 4.141 | 4.013 | 4.993 | 4.993 | 4.993 | 0.564 |
| idle | WEST | 100 | 0.668 | 2.629 | 2.928 | 3.994 | 3.994 | 3.994 | 1.374 |
| idle | WEST | 500 | 3.726 | 4.567 | 4.763 | 5.016 | 5.016 | 5.016 | 0.496 |
| idle | NORTH | 20 | 3.635 | 4.204 | 4.155 | 4.870 | 4.870 | 4.870 | 0.464 |
| idle | NORTH | 100 | 1.885 | 3.181 | 2.995 | 4.850 | 4.850 | 4.850 | 1.074 |
| idle | NORTH | 500 | 0.317 | 2.239 | 2.066 | 4.506 | 4.506 | 4.506 | 1.930 |
| idle | SOUTH | 20 | 0.069 | 2.960 | 3.378 | 5.017 | 5.017 | 5.017 | 2.138 |
| idle | SOUTH | 100 | 0.139 | 3.798 | 5.015 | 5.023 | 5.023 | 5.023 | 2.112 |
| idle | SOUTH | 500 | 0.547 | 2.806 | 2.833 | 5.010 | 5.010 | 5.010 | 2.203 |
| streaming | EAST | 20 | 2.002 | 2.683 | 2.252 | 4.224 | 4.224 | 4.224 | 0.912 |
| streaming | EAST | 100 | 3.055 | 3.872 | 3.806 | 4.823 | 4.823 | 4.823 | 0.629 |
| streaming | EAST | 500 | 0.957 | 4.284 | 4.451 | 7.278 | 7.278 | 7.278 | 2.278 |
| streaming | WEST | 20 | 0.204 | 1.987 | 1.378 | 4.988 | 4.988 | 4.988 | 1.851 |
| streaming | WEST | 100 | 0.469 | 2.376 | 2.005 | 5.023 | 5.023 | 5.023 | 1.676 |
| streaming | WEST | 500 | 2.082 | 4.562 | 5.034 | 6.098 | 6.098 | 6.098 | 1.496 |
| streaming | NORTH | 20 | 1.404 | 3.531 | 3.878 | 4.963 | 4.963 | 4.963 | 1.470 |
| streaming | NORTH | 100 | 0.870 | 3.494 | 4.324 | 4.459 | 4.459 | 4.459 | 1.519 |
| streaming | NORTH | 500 | 0.599 | 3.186 | 3.346 | 5.455 | 5.455 | 5.455 | 2.095 |
| streaming | SOUTH | 20 | 0.226 | 2.046 | 2.076 | 3.805 | 3.805 | 3.805 | 1.295 |
| streaming | SOUTH | 100 | 0.119 | 1.269 | 0.378 | 4.199 | 4.199 | 4.199 | 1.697 |
| streaming | SOUTH | 500 | 0.058 | 4.312 | 4.025 | 9.141 | 9.141 | 9.141 | 3.558 |

Final fake acceptance: 48/48 normal cases and 48/48 ASan/UBSan cases pass at driver 0x03000035. `property_inventory` validates the common, CCD and guider contracts; repeated disconnected requests are accepted without requiring a redundant property notification. Corrected the testing reference typo CCD_REMOVE_FITS_HEADERS to its actual published name CCD_REMOVE_FITS_HEADER, verified in indigo_names.h. No property was added or removed by that documentation fix.

Final formatting: integration source changed only whitespace; hardware source changed only whitespace/preprocessor line splicing, verified by token comparison. No production source semantics changed after version 0x03000035 validation. Xcode registers all persistent new ASI files; temporary diagnostic executables/logs are outside the repository.

ASI120 recovery diagnosis (resumed hardware acceptance): direct SDK tests reproduce failure on shortening snapshots and after video; recovery succeeds after stopping the failed exposure, waiting 150 ms, reapplying the exact requested duration and restarting. An immediate restart without that complete sequence was insufficient in the INDIGO hardware run. Version 56 / 0x03000038 implements that complete sequence once, on ASI120 ASI_EXP_FAILED only. The queued retry is canceled by abort/disconnect; SDK stop/control/start errors and a second FAILED status remain ALERT. The fake models a failed snapshot requiring StopExposure and covers exact 1.5 s/dark retry, bounded repeated failure, abort/disconnect with sibling guider survival, control/start/stop failure and later recovery. Final physical acceptance is being verified.

Version 57 / 0x03000039: a full real-SDK diagnostic reproduced three failed short snapshots followed by success on the fourth attempt without reconnecting. The previous single retry was insufficient. The fake regression now injects three FAILED results before a successful exact 1.5 s dark exposure and four failures for bounded ALERT; it fails before the change. Recovery now allows at most three queued StopExposure/cooldown/control/start retries. Full hardware and sanitizer verification is in progress.

Version 58 / 0x0300003A: version 57 passed long-stream abort followed by the 0.1 s snapshot (two retries, 3.853 s), then exposed a distinct short-video timeout after aborting a 5 s snapshot. Direct SDK calls measured video read intervals of 0.001, 0.958 and 1.673 s at a 0.1 s exposure setting. The former ~0.7 s watchdog was too short. ASI120 video now has at least a 5 s readout allowance per frame, with unchanged 20 ms SDK transactions and bounded abort/disconnect handling. New `asi120_video_settling` (50th normal case) fails before the fix, models delayed successive video frames, and verifies watchdog ALERT and subsequent recovery.

## Final acceptance — version 58 / 0x0300003A

Final source is the migration worktree over repository HEAD `23d1f926b`; the unchanged original-driver comparison remains `d373999270f5070ced1a7d5fcd3958adf357a696`. macOS 26.6.2 arm64, bundled SDK 1.41.0.0. Universal arm64/x86_64 production driver, archive and standalone build passed; only arm64 runtime was exercised.

| Acceptance | Final evidence |
| --- | --- |
| H01 | Both cameras: connection, disconnect/reconnect, fresh image and shutdown/reinitialization. ASI120: CCD/guider alone, both connection orders and sibling survival; simultaneous physical cameras preserve image acquisition when either is removed. |
| H02 | Both full suites pass all four pixel formats, all five frame types, nonzero ROI, bins 1/2, short/fractional/5 s exposures and image payload checks. ASI120 duration-transition retry is bounded and verified. |
| H03 | Both pass abort/reacquire, exact five-frame streaming, sustained 0.1 s streaming, three 2.5 s frames then abort/reacquire. Final ASI120: 102 sustained frames/~10 s, long-stream abort 2.502 s, following short snapshot 2.645 s including one retry. ASI294: 108 sustained frames/~10 s, long-stream abort 0.345 s. |
| H04 | Both pass exposed controls, presets and isolated configuration roundtrip with restoration. ASI120 SDK rejects OverCLK/HighSpeedMode writes; ALERT and unchanged readback are verified. |
| H05 | ASI294 target 21 °C settles at 21.4 °C/5% power, then original cooler/target restored. ASI120 is sensor-only, active cooling N/A. |
| H06 | ASI120 passes all directions, simultaneous axes, silent BUSY reversal rejection, zero requests, guiding alongside imaging, pulse disconnect/reconnect and CCD survival. ASI294 has no ST4, N/A. |
| H07 | User-operated idle/exposure/stream cable cycles pass on both; active guide cycle passes on ASI120. Restored images/pulses and other-camera survival verified. Exact cable removal inside a brief SDK readout cannot be reliably synchronized manually; gated fake SDK snapshot/video/poll removals cover that boundary. |
| H08 | Both pass eight-byte suffix write/replug/name/image, clear/replug/original-name/image and original empty suffix restoration. Final dynamic dlclose/dlopen followed by fresh image passes on both; this asserts driver reload, not unloading the host's shared vendor SDK image. |

Reproduction: build `make -C indigo_test build/hardware/test_ccd_asi_hw build/hardware/test_ccd_asi_reload_hw`; select the exact camera with `INDIGO_TEST_DEVICE` and run the hardware executable with `--run`. `INDIGO_TEST_PHASE=reload` selects focused dynamic reload; `hotplug-exposure`, `hotplug-stream`, `hotplug-guide` and `--run --suffix` require coordinated USB cycles. Do not run two real-SDK processes concurrently. Fake coverage: `make -C indigo_test test-ccd-asi-sdk`; all 50 cases and the fully documented ASan/UBSan instrumentation scope passed. Hardware and diagnostic logs were inspected under the temporary `indigo-asi-refactor` workspace; persistent acceptance evidence is summarized here and in TESTING.md.

Linux/Windows builds and runtime, Intel runtime, monochrome/non-S USB2 models and other physical ASI models were not available. Fake profiles cover absent capabilities. No external electrical ST4 timing or optical calibration is claimed or required for this driver acceptance. The original ASI120 SDK failure remains reproducible in the unchanged driver; the migrated driver supplies bounded recovery and preserves ALERT if recovery is exhausted.
