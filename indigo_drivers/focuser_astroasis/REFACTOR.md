# focuser_astroasis refactoring record

## Scope and baseline

This record covers migration of `indigo_focuser_astroasis` from its hand-written INDIGO 3.0 implementation to `indigo_generator`, the production fixes proven by regression tests, and complete applicable hardware-free fake-SDK coverage.

Baseline date and source: 2026-09-16, commit `e39a08159` (`ccd_svb: mode change fixed`). The working tree already contained unrelated, uncommitted `ccd_uvc` work in `indigo.xcodeproj/project.pbxproj`, `indigo_test/Makefile`, `indigo_drivers/ccd_uvc/` and `indigo_test/`. This work must preserve those edits and add only its own entries. Host: macOS 26.6.2 (`Darwin 25.6.0`), Apple Silicon arm64.

Baseline build command:

```sh
cd indigo_drivers/focuser_astroasis
make -B -f ../../Makefile.drv
```

Result: passed without compiler or linker warnings. The object, archive, dynamic library and executable were built for x86_64 and arm64 against the bundled `liboasisfocuser.a` (macOS).

Baseline automated tests: none. No simulator, fake SDK, driver-specific integration test or hardware test exists; `MIGRATION_STATUS.md` records `0 / 0`.

## Hardware-test decision

No hardware testing will be performed. The user has no Astroasis Oasis focuser available. No statement in this record implies physical validation; all automated evidence is fake-SDK software behaviour. The retesting column in `MIGRATION_STATUS.md` must not claim hardware validation for this migration.

## Studied sources

- `indigo_focuser_astroasis.c`, `.h`, `_main.c`, `Makefile.inc`, `README.md`, `.vcxproj` in this directory.
- `bin_externals/liboasisfocuser/include/AOFocus.h`: the only SDK documentation bundled. It documents API names, return codes, config masks, the status structure, `AO_FOCUSER_MAX_NUM = 32`, 32-byte name/version buffers and stable scan ids. It does not document motion timing, thread safety, reset behaviour or firmware capability detection.
- `../wheel_astroasis/` (`.driver`, `REFACTOR.md`) and `../focuser_asi/indigo_focuser_asi.driver`: generated Astroasis SDK discovery and generated SDK focuser patterns.
- `indigo_tools/indigo_generator.c`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_libs/indigo_focuser_driver.c`, `indigo_libs/indigo_driver.c`, `indigo_libs/indigo_timer.c`.
- `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (focuser standard), `integration/test_wheel_astroasis_sdk.c`, `integration/test_focuser_asi_sdk.c`.

## Current-state audit

### Architecture and implementation

- Version `0x03000006`, label `Astroasis Oasis Focuser`, original author Frank Chen, 3.0 refactoring by Peter Polakovic. Everything except INFO returns `INDIGO_UNSUPPORTED_ARCH` on `__i386__`.
- One logical focuser per SDK focuser id. Hot plug uses a hand-written libusb callback filtered by VID `0x338f` / PID `0xa0f0`. Every arrival or removal schedules an unowned global timer (`indigo_set_timer(NULL, 0.5, ...)`) that performs a complete `AOFocuserScan()` refresh under a pthread mutex: new ids are probed and attached, missing ids are detached. The device list capacity is `AO_FOCUSER_MAX_NUM` (32).
- Discovery probe per new id: `AOFocuserOpen`, `AOFocuserGetVersion`, `AOFocuserGetProductModel`, `AOFocuserGetFriendlyName`, `AOFocuserGetBluetoothName`, `AOFocuserGetConfig`, then device allocation, `AOFocuserGetSDKVersion`, `indigo_attach_device`, and finally `AOFocuserClose`. Any query failure closes the id and skips it. Name is `Oasis Focuser` or `Oasis Focuser #<friendly name>`, made unique by id.
- INIT: `AOFocuserGetSDKVersion`, `AOFocuserSetLogLevel(DEBUG or QUIET)`, `indigo_start_usb_event_handler`, `libusb_hotplug_register_callback(... ENUMERATE ...)`. SHUTDOWN refuses with `INDIGO_BUSY` while a focuser is connected; otherwise deregisters and detaches/frees all devices.
- Connection runs on a zero-delay timer: global lock, `AOFocuserOpen`, `AOFocuserGetConfig`, publishes limits/backlash/reverse/beep/backlash direction, calls `AOFocuserGetConfig` again (Bluetooth, result only logged), `AOFocuserGetBluetoothName` (result only logged), defines custom properties, starts a 0.5 s position timer and a 0.1 s (then 2 s) temperature timer. Disconnect cancels both timers synchronously, deletes custom properties, `AOFocuserStopMove`, `AOFocuserClose`, global unlock.
- Property changes other than CONNECTION run synchronously on the calling bus thread. Position/temperature polling runs on independent timer worker threads.

### Public properties and behaviour

INFO count 7: model, `DEVICE_FW_REVISION` = firmware `major.minor.patch` from the high three bytes, `DEVICE_HW_REVISION` = SDK version with label `SDK version`.

Inherited focuser properties used: `FOCUSER_LIMITS` (visible; maximum is the device maximum step, minimum fixed at 0), `FOCUSER_BACKLASH` (0–10000), `FOCUSER_POSITION`/`FOCUSER_STEPS` (maximum from probe-time `maxStep`), `FOCUSER_ON_POSITION_SET` (GOTO/SYNC), `FOCUSER_TEMPERATURE` (label `Temperature 2 (Ambient)`), `FOCUSER_REVERSE_MOTION`, `FOCUSER_COMPENSATION` (count 2, −10000..10000 steps/°C and threshold), `FOCUSER_MODE`, `FOCUSER_DIRECTION`, `FOCUSER_ABORT_MOTION`. `FOCUSER_SPEED` is hidden.

Custom connected-only properties (all RW except board temperature):

| Current wire name | Type / items | Group, label | SDK |
| --- | --- | --- | --- |
| `BEEP_ON_POWER_UP_PROPERTY` | switch one-of-many `ON`, `OFF` | Advanced, Beep on power up | config `MASK_BEEP_ON_STARTUP` |
| `BEEP_ON_MOVE_PROPERTY` | switch one-of-many `ON`, `OFF`; saved on CONFIG save | Advanced, Beep on move | config `MASK_BEEP_ON_MOVE` |
| `BACKLASH_DIRECTION_PROPERTY` | switch one-of-many `INWARD`, `OUTWARD` | Main, Backlash compensation overshot direction | config `MASK_BACKLASH_DIRECTION` (0 = IN) |
| `CUSTOM_SUFFIX` | text `SUFFIX` | Advanced, Device name custom suffix | `AOFocuserSetFriendlyName`, max 32 bytes |
| `BLUETOOTH_PROPERTY` | switch one-of-many `ENABLED`, `DISABLED` | Advanced, Bluetooth | config `MASK_BLUETOOTH` |
| `BLUETOOTH_NAME_PROPERTY` | text `BLUETOOTH_NAME` | Advanced, Bluetooth name | `AOFocuserSetBluetoothName`, max 32 bytes |
| `FACTORY_RESET_PROPERTY` | switch any-of-many `RESET`, hint `warn_on_set:"Confirm focuser factory reset?";` | Advanced, Factory reset | `AOFocuserFactoryReset` |
| `BOARD_TEMPERATURE_PROPERTY` | RO number `Internal Temp.` −50..50 | Main, Temperature 1 (Board) | status `temperatureInt / 100` |

None of the custom names complies with the mandatory `X_` prefix rule; `indigo_docs/PROPERTIES.md` documents the legacy names. The migration renames them by prefixing `X_` only, keeping every item name, label, group, rule and hint.

Motion and settings behaviour:

- GOTO: target equal to cached status position publishes OK without an SDK call; otherwise publishes POSITION/STEPS BUSY with the cached position, calls `AOFocuserMoveTo(target)`, and polls `AOFocuserGetStatus` every 0.5 s until `moving == 0`, then POSITION/STEPS OK.
- SYNC: publishes BUSY, `AOFocuserSyncPosition(target)`, `AOFocuserGetStatus`, publishes OK or ALERT with the read position.
- STEPS: publishes BUSY, `AOFocuserMove(±steps)` (inward negative; reversal is device-side), polls as GOTO.
- ABORT: cancels the poll timer (not synchronously), `AOFocuserStopMove`, `AOFocuserGetStatus`, publishes POSITION, STEPS OK and ABORT OK/ALERT.
- LIMITS / BACKLASH / REVERSE / beep / backlash direction / Bluetooth: one `AOFocuserSetConfig` with the single corresponding mask; OK or ALERT.
- Suffix / Bluetooth name: reject > 32 bytes with ALERT and message, else setter; OK or ALERT.
- Factory reset: RESET=ON calls `AOFocuserFactoryReset` and publishes OK `Factory reset completed` or ALERT `Factory reset failed`; RESET=OFF publishes nothing.
- MODE: manual defines, automatic deletes the manual properties and redefines POSITION read-only. COMPENSATION: accepts values.
- Temperature poll: `AOFocuserGetStatus`; board temperature always, ambient from the external probe when `temperatureDetection` and not `TEMPERATURE_INVALID`, otherwise the board value; a message on probe connection change. In automatic mode, when position is idle and |ΔT| ≥ threshold (and < 100), `AOFocuserMove((int)(ΔT * steps/°C))`, POSITION BUSY and polling. Failure publishes both temperature properties ALERT.

### Supported platforms, build and integration

- Bundled static SDK for Linux x64/arm/arm64, macOS universal, Windows DLL/import library. `Makefile.inc` links C++ and on Linux libudev. `README.md` declares Linux and macOS; a Windows `.vcxproj` exists and compiles the single C source.
- Registered in `Makefile.drv` auto-discovery, the Xcode project (`focuser_astroasis` group) and the Windows project.
- No `.driver`, no generated outputs, no fake SDK or tests.

### Defects and risks found by source audit

Identifiers are used in the found-defects section below; each is either reproduced by a dedicated reproducer against the original driver or explicitly marked audit-only.

- **FOC-01** Connection open failure returns ALERT without releasing the acquired global lock.
- **FOC-02** `AOFocuserGetConfig` failure after a successful open leaves the SDK handle open and the global lock held.
- **FOC-03** `AOFocuserMoveTo` / `AOFocuserMove` start failures (manual and temperature compensation) are only logged; polling then reports POSITION/STEPS OK.
- **FOC-04** `FOCUSER_POSITION`/`FOCUSER_STEPS` maxima are fixed at probe-time `maxStep`: neither the connection config read nor a `FOCUSER_LIMITS` change updates them.
- **FOC-05** SDK calls are unsynchronized: bus handlers, the position timer and the temperature timer call the SDK concurrently and share `PRIVATE_DATA->status` without a lock.
- **FOC-06** Pending global hot-plug refresh timers are not cancelled by SHUTDOWN; a refresh scheduled by INIT can attach focusers after SHUTDOWN returned.
- **FOC-07** A failed INIT hot-plug registration leaves `last_action == INIT`, so a retried INIT returns OK without registering.
- **FOC-08** Failed settings writes (config setters, suffix, Bluetooth name, including over-length text) leave the rejected value published instead of the last confirmed value.
- **FOC-09** A POSITION/STEPS request while BUSY is copied into the property (overwriting target and value) and never answered.
- **FOC-10** `FACTORY_RESET.RESET=OFF` requests are never answered.
- **FOC-11** `indigo_attach_device()` failure is ignored; the unattached device stays in the list, is never retried by later arrivals, and a later SHUTDOWN dereferences its NULL device context in `VERIFY_NOT_CONNECTED` (segmentation fault).
- Audit-only risks: re-arming `focuser_timer` while a poll is already pending orphans the earlier timer (position poll started at connection plus an immediate move); abort cancels the poll asynchronously so an in-flight poll can publish after abort; cached status used for the GOTO no-op comparison is zero until the first 0.5 s poll; factory reset leaves settings properties stale until reconnect (reset/re-enumeration timing is undocumented and hardware-dependent, so no readback is added); a BOARD temperature allocation is not NULL-checked; suffix/Bluetooth length is checked with the version constant instead of the name constant (both 32).

## Atomic plan

1. **Done — instructions, sources and baseline.** Read governing instructions, driver/SDK/build/project files, generator semantics and sibling migrations. Built the original driver as recorded above; confirmed zero existing tests.
2. **Done — fake SDK and harness.** Added `indigo_test/integration/test_focuser_astroasis_sdk.c` and Makefile targets `test-focuser-astroasis-sdk` / `test-focuser-astroasis-sdk-sanitize`. The production driver is compiled separately with link-level seams for libusb hot plug, attach/detach, global lock and timer/handler delays (scaled to 10 %, preserving relative order); `indigo_driver.c` and `indigo_focuser_driver.c` are compiled into the test so configuration files go to a temporary directory. The fake `AOFocus` SDK models eight focusers with scan ids that change on replug, per-call counters and call logs, per-call error injection with skip counts, open-state checking (calls after close), per-focuser concurrent-entry detection, a status-read gate, held motion, poll-driven motion, board/ambient temperatures and config masks. The client uses `force_property_updates` so unchanged OK publications are observed. Xcode registration is performed with the other project entries in step 8.
3. **Done — original-driver characterization suite.** 18 registered preservation cases (listed in the scenario mapping below) pass against the unchanged driver. Eleven dedicated reproducers (`--known-defects`) run in isolated child processes and all fail on the original driver as expected.
4. **Done — original reference trace.** `indigo_test/fixtures/focuser_astroasis/original_reference_trace.txt` (normalized, ordered SDK/lock/property trace for connect, GOTO, no-op GOTO, relative moves, SYNC, abort, reverse, backlash, limits, custom switches, suffix/Bluetooth name, failed write, reset, compensation and disconnect) is compared byte-for-byte by `reference_trace`. Nine consecutive runs produced identical traces.
5. **Done — create `.driver` and regenerate.** Added `indigo_focuser_astroasis.driver` (version 6 → 7, `supported_architecture = "!defined(__i386__)"`, `sdk { hotplug = true; discovery_retries = 6; ... plug / unplug_match }`, transactional `astroasis_open` / `astroasis_close`, generated per-driver and per-device queues, `_finalizer` motion polling, `X_` custom names). Generated `.c`, `.h`, `_main.c` with `build/bin/indigo_generator indigo_focuser_astroasis.driver`; `make -B -f ../../Makefile.drv` passes for x86_64 + arm64 without warnings. The generator initially warned about `->hidden = false` assignments on declared inherited properties; these were moved to declarative `hidden = false` attributes. The default `MAX_DEVICES` (5) is used; no override exists. The 32-element SDK scan buffer is an SDK API requirement and independent of that capacity.
6. **Done — post-migration characterization and trace comparison.** Only the property-name prefix, the expected version constant and the reference-trace fixture path changed in the suite. First run: all SDK behaviour passed, but the late-update invariant failed in six cases because the generated `on_timer` was started with `indigo_execute_handler()` inside the connection handler, before the base focuser class defined `FOCUSER_TEMPERATURE` and before `CONNECTION` was published; a temperature update could precede its definition. The original driver started the first temperature read after 0.1 s. The `.driver` now schedules `focuser_temperature_poll` 0.1 s after connection (original timing) and reschedules it every 2 s; both delayed callbacks return when `CONNECTION.CONNECTED` is no longer requested. After this, 17/17 preservation cases passed and the only failing case was the reference trace, whose diff is analysed below. `generated_reference_trace.txt` was captured and five consecutive runs matched it.
7. **Done — defect fixes.** All eleven reproducers, which had failed on the original driver, passed on the generated driver without further changes, because their fixes were designed into step 5. They were promoted from the isolated `--known-defects` mode into the ordinary suite with the full resource invariants; the separate mode was removed. Invariants now also require zero concurrent SDK entries, zero invalid detaches and zero attachments after SHUTDOWN in every case. Three generated-behaviour cases were added: `capacity_and_slot_reuse`, `delayed_sdk_discovery_retry` and `urgent_abort_cancels_queued_move`; a duplicate-arrival check was added to `multiple_focusers_and_removal`, and the probe-failure case now checks balanced open/close instead of an exact close count because bounded discovery retries repeat the probe. A mutation check (scratch copy of the generated source without cancellation of the queued position handler) made `urgent_abort_cancels_queued_move` fail with one unexpected `MoveTo`, confirming the case detects the overtaking race. Strict compilation found one driver-owned `-Wsign-compare` in the temperature validity check inherited from the original; it now compares `(unsigned int)temperatureExt` with `TEMPERATURE_INVALID`, which is the same conversion the original comparison performed.
8. **Done — repository integration and documentation.** `indigo.xcodeproj/project.pbxproj`: `.driver` and `REFACTOR.md` in the `focuser_astroasis` group, `test_focuser_astroasis_sdk.c` in the integration test group, `fixtures/focuser_astroasis` with both trace files; `plutil -lint` passes and the pre-existing uncommitted `ccd_uvc` project entries are unchanged. `indigo_focuser_astroasis.vcxproj`: `.driver` and `REFACTOR.md` added as `None` items using the existing CRLF convention; the generated C file remains the only compiled driver source. `indigo_test/Makefile`: `test_focuser_astroasis_sdk` added to `INTEGRATION_TESTS`, plus `test-focuser-astroasis-sdk` and `test-focuser-astroasis-sdk-sanitize`. `indigo_docs/PROPERTIES.md`: `X_` names, restored-value semantics, position range semantics and `.driver` source. `MIGRATION_STATUS.md`: generator/async `✅ Yes`, retested `✅ Sim`, tests `32 / 0`; Comment column unchanged. `README.md` unchanged.
9. **Done — final verification.** Evidence below. Windows and Linux builds and physical hardware were unavailable and are not claimed.
10. **Done — post-abort communication retry regression (2026-09-17).** Preserved the version 8 post-`StopMove` status retry and driver version unchanged. Added `abort_status_communication_retry`, whose fake SDK activates errors only after a successful stop: two communication failures followed by success require exactly three status attempts and finish with POSITION/STEPS/ABORT OK; three communication failures require exactly three attempts, leave POSITION ALERT and STEPS/ABORT OK, and reset the abort switch. Corrected the suite's stale expected driver version from 7 to 8. `make -C indigo_test test-focuser-astroasis-sdk`: 33/33 passed. `make -C indigo_test test-focuser-astroasis-sdk-sanitize`: 33/33 passed on arm64 with no ASan/UBSan report; the five existing shared-framework `sprintf` deprecation warnings remain. No production-driver change or new physical-hardware test was performed.

## Original-driver baseline evidence

Environment: macOS 26.6.2 arm64; test build is the repository universal x86_64/arm64 test configuration, sanitizer build is arm64 only.

- `make -C indigo_test test-focuser-astroasis-sdk`: 18 run, 18 passed.
- `make -C indigo_test test-focuser-astroasis-sdk-sanitize` (ASan + UBSan, `detect_leaks=0` because LeakSanitizer is unsupported on this macOS runtime): 18 run, 18 passed, no sanitizer report. Driver sanitizer compilation reports only pre-existing `sprintf` deprecation warnings.
- `indigo_test/build/integration/test_focuser_astroasis_sdk --known-defects`: 11 run, 11 failed as expected:
  - FOC-01: lock count 1 after failed open.
  - FOC-02: SDK handle still open after failed connection config read.
  - FOC-03: POSITION reached OK instead of ALERT after `MoveTo` failure.
  - FOC-04: POSITION maximum 10000 after connection read 20000.
  - FOC-05: one concurrent SDK entry while a status read was in progress.
  - FOC-06: one attachment after SHUTDOWN returned.
  - FOC-07: second INIT did not register hot plug (1 registration instead of 2).
  - FOC-08: BACKLASH shows rejected value 50 instead of confirmed 12.
  - FOC-09: POSITION target 7000 published while the device completed the 3000 move.
  - FOC-10: `RESET=OFF` produced no update.
  - FOC-11: terminated by SIGSEGV in SHUTDOWN (`VERIFY_NOT_CONNECTED` on the unattached device).

## Post-migration evidence

Environment as for the baseline.

- Reproducible generation: after the final `.driver` edit the generator was run twice more; SHA-1 of the checked-in outputs was unchanged: `.c` `e24cbbacad04bcad165cf9db94ba3fcb0c7facb9`, `.h` `5be0ae4b2b14eb97b74023a4df8aa8798bddbbdc`, `_main.c` `69ff6f23b71590b5470deabb2b4f25f656c8db21`.
- `make -B -f ../../Makefile.drv` in this directory: passed, x86_64 + arm64, zero compiler/linker warnings.
- Strict syntax check (`clang -arch arm64 -std=gnu11 -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wshadow -Wformat=2 -fsyntax-only`): the driver-owned sign-compare warning was fixed; two remaining warnings are in generator-emitted SDK hot-plug scaffolding (`-Wshadow` of `device` in the generated shutdown loop and `-Wcast-function-type-mismatch` for the generated retry handler cancellation) and were not hand-edited.
- `make -C indigo_test test-focuser-astroasis-sdk`: 32 run, 32 passed in each of seven complete runs during steps 7–9 (the final three after the last `.driver` edit).
- `make -C indigo_test test-focuser-astroasis-sdk-sanitize` (arm64 ASan + UBSan, `detect_leaks=0`): 32 run, 32 passed after the last `.driver` edit, no sanitizer report and no driver compiler warning. Instrumentation covers the test, driver, `indigo_driver.c` and `indigo_focuser_driver.c`; the shared `libindigo` is not instrumented.
- `reference_trace` against `generated_reference_trace.txt`: 5/5 consecutive passes plus every full-suite run.
- `git diff --check` for the changed text files and a formatting audit of the `.driver` and test source: no trailing whitespace, tab indentation, no blank lines inside function bodies. The `.vcxproj` additions use that file's existing CRLF line endings.
- Unavailable: Linux (x64/arm/arm64) and Windows builds, i386 fallback build, physical Oasis focuser. The `supported_architecture` fallback is generator-owned and was not compiled for i386 here.

## Reference trace comparison

Both traces are checked in: `original_reference_trace.txt` (unchanged driver) and `generated_reference_trace.txt` (current contract, compared by the suite). Property names are normalized by removing the `X_` prefix so the traces are comparable.

The ordered SDK and lock trace is identical: connection (`lock`, `Open`, `GetConfig`, `GetConfig`, `GetBluetoothName`), every `MoveTo`/`Move`/`SyncPosition`/`StopMove`/`SetConfig` with its mask and value, the name setters, `FactoryReset`, and disconnection (`StopMove`, `Close`, `unlock`). Every difference is a property publication:

1. Generated asynchronous handlers publish BUSY before queuing work (`INDIGO_COPY_*_PROCESS_CHANGE`), adding a BUSY line for `FOCUSER_REVERSE_MOTION`, `FOCUSER_BACKLASH`, `FOCUSER_LIMITS`, `FOCUSER_ABORT_MOTION`, all custom writable properties, and the no-op GOTO. This is generator semantics required by the queue design and gives clients an immediate acknowledgement.
2. For GOTO and relative moves the framework publishes `FOCUSER_POSITION` BUSY before the handler publishes `FOCUSER_STEPS` BUSY, so the first two BUSY lines appear as POSITION, STEPS instead of STEPS, POSITION. The subsequent SDK call and completion order are unchanged.
3. The failed backlash write publishes ALERT with the confirmed value 25 instead of the rejected value 30 (FOC-08 fix).

Not visible in the normalized trace but intentionally changed: disconnect calls `AOFocuserStopMove` before, rather than after, deleting the custom properties (generator `on_disconnect` placement; the `StopMove` → `Close` → unlock order is preserved); the SDK version string is read after the probe `Close` instead of before (no id argument, no device interaction); discovery attaches after closing the probe handle instead of before closing it.

## Intentional behaviour differences

- **Property names:** eight custom properties gained the mandatory `X_` prefix; items, labels, groups, rules, hint and connect-scope are unchanged. Clients using the old names must be updated.
- **Serialization:** property handlers, motion completion polling and temperature polling run on the device queue; connection, discovery and removal run on the per-driver queue. The original ran handlers on bus threads and polls on independent timer threads (FOC-05).
- **Discovery:** the original rescanned all ids after a fixed 0.5 s delay on every USB event. Generated discovery scans immediately and attaches the first unattached, successfully probed id per arrival event; if nothing can be attached it retries up to six times at 0.5 s intervals (up to 3 s), which covers the original readiness delay and additionally tolerates slower SDK enumeration or temporary capacity exhaustion. Removal uses SDK scan membership (`unplug_match`); a failed scan is treated as inconclusive. Duplicate arrival for an already attached USB device is ignored.
- **Capacity:** the generator default of five attached focusers replaces the original 32-entry list, as required by the repository rules. A focuser beyond capacity attaches if a slot is freed during the retry window or on its next arrival event.
- **Generic range checks:** the original handlers rejected positions/steps outside `0..max`; the framework already clamps requests to the published range, so these duplicate checks were removed per the repository rules. `FOCUSER_POSITION` and `FOCUSER_STEPS` maxima now track the device maximum (FOC-04).
- **Abort:** runs at urgent priority and cancels queued position/steps handlers and the pending completion poll before `StopMove`, so an abort can overtake a queued move that has not reached the SDK.
- **Busy requests:** a `FOCUSER_POSITION`/`FOCUSER_STEPS`/settings request arriving while that property is BUSY is ignored by the framework guard instead of overwriting the published values (FOC-09).
- **Temperature polling start:** first read 0.1 s after connection as in the original; `on_timer` is not used because it can publish before the base class defines `FOCUSER_TEMPERATURE` (see generator finding).

## Found defects

Every defect below was reproduced by a dedicated case that failed against the original driver (evidence in the baseline section) and passes against the generated driver.

| ID | Observable impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- |
| FOC-01 | After a failed connection open, the global lock stays held; other drivers/clients cannot claim it and later attempts depend on lock re-entry. | Open failure branch set ALERT without `indigo_global_unlock`. | `astroasis_open` releases the lock on open failure; transactional open/close contract. | `FOC-01 open_failure_releases_lock` |
| FOC-02 | A failed config read during connection leaves the SDK handle open and the lock held while CONNECTION reports ALERT. | No `AOFocuserClose`/unlock in the failure path after a successful open. | `on_connect` closes the successful open via `astroasis_close` when the config read fails. | `FOC-02 connect_config_failure_closes_sdk` |
| FOC-03 | A rejected `MoveTo`/`Move` (manual or temperature compensation) is reported as a successful completed move. | Start result only logged; polling then saw an idle motor and published OK. | Start failure publishes POSITION/STEPS ALERT and schedules no completion poll. | `FOC-03 move_start_failure_alerts` |
| FOC-04 | Clients see a stale position/steps range after connection or a `FOCUSER_LIMITS` change; requests are clamped to the probe-time maximum. | Maxima were set only at attach from probe-time config. | `focuser_update_limits` applies the connection read and every successful limit write, redefining POSITION/STEPS while connected. | `FOC-04 limits_update_position_range` |
| FOC-05 | SDK calls from bus handlers and both timers overlap and share `PRIVATE_DATA->status` without synchronization (possible corrupted status or unsafe proprietary SDK use). | Handlers ran on caller threads; polls on independent timer workers. | Generated queues serialize all per-device SDK work; disconnect cancels and waits for running work. | `FOC-05 sdk_calls_serialized` and the zero-concurrency invariant in every case |
| FOC-06 | A focuser can be attached after SHUTDOWN returned. | Unowned global refresh timers were not cancelled by SHUTDOWN. | Generated driver queue is drained and hot-plug work is owned by the queue. | `FOC-06 shutdown_cancels_pending_discovery` and the attach-after-shutdown invariant |
| FOC-07 | After a failed INIT registration, retrying INIT returns OK but no focuser is ever discovered. | `last_action` stayed INIT on failure. | Generated INIT resets `last_action` on registration failure. | `FOC-07 failed_init_can_retry` |
| FOC-08 | A rejected settings write leaves the rejected value displayed with ALERT (config setters, suffix, Bluetooth name, over-length text). | Values were copied before the SDK call and never restored. | Config writes use a local masked copy; on failure the confirmed cached value is restored before ALERT. | `FOC-08 failed_writes_restore_values`, trace step `failed-write` |
| FOC-09 | A POSITION request during motion overwrites the published target (and momentarily value) while the device continues to the original target. | Values were copied before the BUSY check; the request was then silently ignored. | Framework BUSY guard with `preserve_values` copies nothing while BUSY. | `FOC-09 busy_motion_request_preserves_target` |
| FOC-10 | A `RESET=OFF` request is never answered. | No update outside the RESET=ON branch. | Generated handler epilogue publishes OK; no SDK call. | `FOC-10 reset_off_request_answered` |
| FOC-11 | After a failed attachment the focuser is never retried, and the next SHUTDOWN crashes (SIGSEGV dereferencing the NULL device context in `VERIFY_NOT_CONNECTED`). | Attachment result ignored; unattached device kept in the device list. | Generated attach rolls back ownership on failure; discovery retry/next arrival attaches the focuser. | `FOC-11 attach_failure_not_detached` and the invalid-detach invariant |
| FOC-12 | A transient `AO_ERROR_COMMUNICATION` from the first status read after a successful abort can leave the current position in ALERT even though the device becomes readable immediately afterward. | Abort performed only one post-stop `AOFocuserGetStatus()` read. | Retry only `AO_ERROR_COMMUNICATION` for up to three total status calls; keep ABORT tied to the stop result and POSITION tied to status readback. | `abort_status_communication_retry` |

Audit-only risks resolved by the migration without a dedicated reproducer (timing-dependent on the original): orphaned position timers when a move started while the connection poll was pending, and polls publishing after an asynchronous abort cancel — both are replaced by queued finalizers with explicit cancellation. The NULL-check of the board-temperature allocation and the use of the name-length constant are handled by generated allocation and `AO_FOCUSER_NAME_LEN`.

Remaining known limitations (not changed): factory reset does not refresh the published settings until reconnect, because reset/re-enumeration timing is undocumented and hardware-dependent; a `FOCUSER_STEPS` request during a compensation move (which marks only POSITION BUSY, as in the original) is not rejected; SDK behaviour for concurrent use, real motion timing, name-length/encoding limits and Bluetooth semantics is unverified without hardware.

## Generator finding (not changed)

Generated `on_timer` callbacks are started with `indigo_execute_handler(device, <device>_timer_callback)` inside the connection handler before `CONNECTION_PROPERTY->state` is set to OK and before the base class defines its connected properties. The callback therefore races with the connection handler: it can publish an update for a property that is not yet defined (observed here for `FOCUSER_TEMPERATURE`), and if it runs before the state assignment its `IS_CONNECTED` guard returns without rescheduling, silently stopping polling until reconnect. This affects other generated drivers that use `on_timer` (for example `focuser_asi`). No generator change was made because it requires explicit approval; this driver avoids the race with an explicitly scheduled poll.

## Scenario-to-test mapping

Focuser class standard (`indigo_test/DRIVER_TESTING_RULES.md`) and shared scope:

| Area | Cases |
| --- | --- |
| Metadata, INFO, interface, property inventory, visibility, permissions, ranges, custom names/items/rules/groups/hint, disconnected inventory | `metadata_and_property_inventory` |
| Discovery probe order, every probe failure with balanced open/close and recovery on arrival, SDK retry, capacity, duplicate/reordered ids, duplicate arrival, names/uniqueness, removal/replug with new id, attach failure | `discovery_probe_order_and_failures`, `delayed_sdk_discovery_retry`, `capacity_and_slot_reuse`, `multiple_focusers_and_removal`, `FOC-11` |
| INIT/SHUTDOWN idempotence, registration arguments, shutdown refused while connected, re-INIT, failed INIT retry, pending discovery at shutdown | `init_shutdown_cycles`, `connection_lifecycle_and_shutdown`, `FOC-06`, `FOC-07` |
| Connect/disconnect/reconnect, SDK call order at connection, lock/handle ownership, open/config failures | `connection_lifecycle_and_shutdown`, `FOC-01`, `FOC-02`, `reference_trace` |
| Absolute GOTO, zero/no-op, SYNC without motion, sync/readback failures, measured position vs target | `goto_sync_and_noop`, `FOC-09` |
| Relative inward/outward steps, sign with device-side reversal, zero steps | `relative_steps_and_direction` |
| Abort in motion and idle, stop failure, transient/exhausted post-stop communication errors, fresh move after abort, abort overtaking queued start | `abort_motion`, `abort_status_communication_retry`, `urgent_abort_cancels_queued_move` |
| Start/poll failures and recovery | `FOC-03`, `motion_poll_failure_and_recovery` |
| Limits, backlash, reverse, beep, backlash direction, Bluetooth (masks, values, failures, restore), range propagation | `settings_config_writes`, `FOC-04`, `FOC-08` |
| Suffix/Bluetooth name boundaries 0/1/32/33 bytes, setter failures, suffix used after replug | `suffix_and_bluetooth_name`, `FOC-08` |
| Factory reset success/failure/OFF | `factory_reset`, `FOC-10` |
| Manual/automatic mode property sets and permissions, compensation parameters | `mode_and_compensation_settings` |
| Board/ambient temperature sources, probe messages, invalid ambient, read failure/recovery | `temperature_sources` |
| Compensation baseline, threshold, positive/negative correction, ≥100 °C rejection, manual reset of baseline, failed compensation move | `temperature_compensation`, `FOC-03` |
| CONFIG save/load of `X_BEEP_ON_MOVE_PROPERTY` in a temporary directory | `configuration_save_and_load` |
| Disconnect and USB removal during motion, no polling after close, reconnection/replug | `disconnect_and_removal_during_motion` |
| Serialization, no SDK after close, no late updates, balanced lock/USB references, no invalid detach | `FOC-05`, invariants in every case |
| Ordered SDK/property compatibility contract | `reference_trace` |

Not applicable: speed control (hidden, SDK field reserved), heater/stall/USB-power configuration, serial number, zero-position, clear-stall and firmware upgrade APIs (present in the SDK but not exposed by the driver; no features were added), guider timing (no guider interface), hardware-only acceptance (no device).

## Final test summary

- Simulated (fake-SDK) tests, original driver: 18 preservation cases run, 18 passed (ordinary); 18 run, 18 passed (ASan/UBSan); 11 defect reproducers run, 0 passed (all failed as expected).
- Simulated (fake-SDK) tests, final generated driver: 33 registered cases; final ordinary run 33 run, 33 passed; final ASan/UBSan run 33 run, 33 passed.
- Hardware tests: 0 run, 0 passed.
