# focuser_dsd refactoring record

## Scope and baseline

This record covers migration of `indigo_focuser_dsd` (Deep Sky Dad AF1/AF2/AF3) from its hand-written INDIGO 3.0 implementation to `indigo_generator`, the production fixes proven by regression tests, and complete applicable hardware-free simulator coverage.

Baseline date and source: 2026-09-16, commit `a446108c5` (`gps_gpsd: migrated to code generator`). The working tree already contained unrelated, uncommitted `ccd_uvc` work in `MIGRATION_STATUS.md`, `indigo.xcodeproj/project.pbxproj`, `indigo_docs/PROPERTIES.md`, `indigo_test/Makefile`, `indigo_drivers/ccd_uvc/` and `indigo_test/`. This work must preserve those edits and add only its own entries. Host: macOS 26.6.2 (`Darwin 25.6.0`), Apple Silicon arm64.

Baseline build command:

```sh
cd indigo_drivers/focuser_dsd
make -B -f ../../Makefile.drv
```

Result: passed without compiler or linker warnings; object, archive, dynamic library and executable built for x86_64 and arm64.

Baseline automated tests:

```sh
make -C indigo_test build/integration/test_focuser_dsd_simulator
cd indigo_test && ./build/integration/test_focuser_dsd_simulator
```

Result: 1 run, 1 passed (`dsd_focuser_passes_serial_compliance_checks`, 6.8 s). `MIGRATION_STATUS.md` records `1 / 0`.

## Hardware-test decision

No hardware testing will be performed. The user has no Deep Sky Dad focuser available. No statement in this record implies physical validation; all automated evidence is simulator-backed software behaviour. The `✅ HW` retesting status in `MIGRATION_STATUS.md` refers to earlier validation of the hand-written driver (README: "developed and tested with DSD AF1") and must not be carried over to the migrated driver.

## Studied sources

- `indigo_focuser_dsd.c`, `.h`, `_main.c`, `README.md`, `.vcxproj` in this directory; `focuser_dsd_simulator/focuser_dsd_simulator.c`; `indigo_test/integration/test_focuser_dsd_simulator.c`.
- No manufacturer protocol document is bundled in the repository. The protocol was cross-checked against the open-source INDI Deep Sky Dad AF1/AF2/AF3 drivers (`indilib/indi`, `drivers/focuser/deepskydad_af{1,2,3}.cpp`): commands are framed `[XXXX…]`, replies `(…)` terminated by `)`, `[STRG…]` answers `(OK)` or `!101)` (movement exceeds maximum move), `[SMOV]` answers `(OK)`, `[STOP]` reply is not read, `[GMOV]` answers `(0)`/`(1)`, AF1/AF2 currents answer `(50%)`, AF3 uses current multipliers `[GMMM]`/`[GMHM]`, temperature is `(%lf)`. Undocumented details (error replies other than `!101)`, device behaviour beyond the maximum position, value validation, reset timing) are simulator assumptions and are recorded as such.
- `indigo_tools/indigo_generator.c`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_libs/indigo_focuser_driver.c`, `indigo_libs/indigo_driver.c`, `indigo_libs/indigo_uni_io.c` (`indigo_uni_read_section` returns `0`, not an error, on timeout and returns partial unterminated data as success).
- `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (focuser standard), sibling migrations `focuser_qhy` (serial simulator with fault/control/event files), `focuser_robofocus`, `focuser_astroasis` and `gps_gpsd` (reference traces).

## Current-state audit

### Architecture and implementation

- Version `0x03000010`, label `Deep Sky Dad Focuser`, device `Focuser DSD AF` (`FOCUSER_DSD_NAME`), author Rumen G. Bogdanovski. One logical focuser, `ADDITIONAL_INSTANCES` supported, no hot plug, serial transport.
- `DEVICE_PORT`, `DEVICE_PORTS` and `DEVICE_BAUDRATE` are visible; baud rate defaults to `9600` (AF1/AF2); `DSD_MODEL_HINT` switches it between `9600` and `115200` (AF3).
- Transport helper `dsd_command()` holds a port mutex, discards input, writes the command, optionally reads up to `)` and sleeps 50 ms after a successful read. Callers pass a timeout of `100`, which the helper converts with `INDIGO_DELAY(100)`: a 100 second read timeout.
- `[SMOV]` and `[STOP]` are written without reading their reply; the next command discards the pending `(OK)`.
- Connection runs on a zero-delay timer: global lock, open serial (or an `asi://` URL as TCP, a copy-paste from the ASI driver), unconditional 2 s sleep (the device resets on RTS), `[GPOS]` identification, then `[GFRM]` (board/firmware and model version 1/2/3), capability adjustment, `[GPOS]`, `[GMXP]`, `[GSPD]`, `[SMXM<POSITION.max>]`, `[SREV<reverse>]`, `[GSTP]`, define step mode, AF1/AF2: `[GCLM]`, define coils mode, `[GCMV%]`, `[GCHD%]`; AF3: `[GMMM]`, `[GMHM]`; define current control, `[GBUF]`, AF1/AF2: `[GIDC]`, define timings, position timer after 0.5 s; AF2/AF3: unhide mode/temperature/compensation, `[GTMC]`, temperature timer after 1 s.
- Disconnect: synchronous cancel of both timers, `[STOP]`, delete custom properties, close, global unlock.
- All property changes except CONNECTION run synchronously on the calling bus thread. Position polling (0.5 s) and temperature polling (2 s) run on independent timer threads.

### Public properties and behaviour

INFO count 6: model and firmware from `[GFRM]` (`(Board=DSD AF1, Version=1.3.3)`).

Inherited focuser properties: `FOCUSER_LIMITS` visible (max 10000..1000000 step 10000, min fixed 0), `FOCUSER_SPEED` visible (1..5, max 3 for AF1/AF2), `FOCUSER_POSITION` (0..1000000 step 100), `FOCUSER_STEPS` (min 0 step 1), `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_BACKLASH` visible; AF2/AF3 additionally `FOCUSER_MODE`, `FOCUSER_TEMPERATURE`, `FOCUSER_COMPENSATION` (count 2, −10000..10000 steps/°C and threshold).

Custom properties:

| Current wire name | Type / items | Group, label | Scope / protocol |
| --- | --- | --- | --- |
| `DSD_MODEL_HINT` | switch one-of-many `AF1_2` (default), `AF3` | Main, Focuser model hint | always defined; sets `DEVICE_BAUDRATE`; saved on CONFIG save |
| `DSD_STEP_MODE` | switch one-of-many `FULL`, `HALF`, `FOURTH`, `EIGTH`, `16TH`, `32TH`, `64TH`, `128TH`, `256TH` (count 4 on AF1/AF2) | Advanced, Step mode | connected; `[SSTP<n>]` then `[GSTP]`; saved |
| `DSD_COILS_MODE` | switch one-of-many `OFF_WHEN_IDLE`, `ALWAYS_ON`, `TIMEOUT_OFF` | Advanced, Coils Power | connected, hidden on AF3; `[SCLM<n>]` then `[GCLM]`; saved |
| `DSD_CURRENT_CONTROL` | number `MOVE_CURRENT`, `HOLD_CURRENT` 10..100 % (AF3: 1..100, multiplier labels) | Advanced, Coils current control | connected; AF1/AF2 `[SCMV<n>%]`, `[SCHD<n>%]`, `[GCMV%]`, `[GCHD%]`; AF3 `[SMMM<n>]`, `[SMHM<n>]`, `[GMMM]`, `[GMHM]`; saved |
| `DSD_TIMINGS` | number `SETTLE_TIME` 0..99999 ms, `COILS_POWER_TIMEOUT` 9..999999 ms (count 1 on AF3) | Advanced, Timing settings | connected; `[SBUF%06d]`, `[GBUF]`, AF1/AF2 `[SIDC%06d]`, `[GIDC]`; saved |

None of the custom names complies with the mandatory `X_` prefix rule. The migration renames them by prefixing `X_` only, keeping item names, labels, groups, rules and scope.

Motion and settings behaviour:

- GOTO: target equal to the cached position publishes OK without commands; otherwise publishes STEPS/POSITION BUSY with the cached position, sends `[STRG%06d]` with `indigo_compensate_backlash()` applied to the cached position, then `[SMOV]` unless the reply was `!101)`, and polls `[GMOV]`, `[GPOS]` every 0.5 s until not moving or position equals target, then STEPS/POSITION OK.
- SYNC (same no-op check): `[SPOS%06d]`, `[GPOS]`, publishes OK or ALERT.
- STEPS: publishes BUSY, `[GPOS]`, target = position ∓ steps clamped to the POSITION range, backlash, `[STRG]`/`[SMOV]`, polling as GOTO. Reversal is device-side (`[SREV]`).
- ABORT: asynchronous timer cancel, `[STOP]` (reply not read), `[GPOS]`, publishes POSITION, STEPS OK, ABORT OK/ALERT.
- LIMITS: `[SMXP<n>]`, `[GMXP]`. SPEED: `[SSPD<n>]`, `[GSPD]`. REVERSE: `[SREV<n>]`. Each ALERT on write failure.
- MODE: manual defines, automatic deletes the manual properties and redefines POSITION read-only. BACKLASH/COMPENSATION: accept values.
- Temperature poll: `[GTMC]`; `≤ −127` means no sensor (IDLE, one message). In automatic mode, when POSITION is OK and threshold ≤ |ΔT| < 100, target = cached position + (int)(ΔT × steps/°C), `[GPOS]`, clamp, backlash, `[STRG]`/`[SMOV]`, POSITION BUSY and polling. Manual mode resets the baseline.

### Supported platforms, build and integration

- README declares Linux (Intel 32/64, ARM v6+) and macOS; a Windows `.vcxproj` and solution entry exist. Portable `indigo_uni_io` is used; `pthread` is used directly for the port mutex.
- Registered in `Makefile.drv` auto-discovery, the Xcode project (`focuser_dsd` group with simulator) and the Windows solution. Stale local build products (`*.o`, `Debug/`) are untracked and not touched.
- No `.driver`, no generated outputs.

### Existing simulator and tests (gaps)

`focuser_dsd_simulator.c` models only AF2, moves instantly (`[STRG]` sets position, `[GMOV]` always `(0)`), does not reply to `[SMOV]`, replies `(50)` instead of `(50%)` for currents, lacks AF1/AF3 commands (`[GMMM]`, `[GMHM]`, `[SMMM]`, `[SMHM]`), `!101)` maximum-move validation, temperature control, fault injection, external control and an event log. The single test is a smoke/compliance pass: no motion timing, abort in motion, failure paths, model variants, compensation, reconnect, lifecycle or concurrency coverage.

### Defects and risks found by source audit

Identifiers are used in the found-defects section; each is either reproduced by a dedicated reproducer against the original driver or explicitly marked audit-only.

- **DSD-01** Read timeout is 100 s (`INDIGO_DELAY(100)`): a silent device blocks connection (and every synchronous property change) for 100 s per command.
- **DSD-02** Malformed numeric replies with trailing data (for example `(123x)`) are accepted because `sscanf("(%d)")` does not check the closing parenthesis; unterminated replies are returned as success after the timeout.
- **DSD-03** Move start failures (`[STRG]` answering `!101)`, another error or nothing) are only logged; polling then reports POSITION/STEPS OK. Any non-`!101)` failure still sends `[SMOV]`.
- **DSD-04** Poll read failures do not end in ALERT: a failed `[GPOS]` when motion ends publishes OK with a stale position; a failed `[GMOV]` evaluates an uninitialized value (UB, audit-only part).
- **DSD-05** Relative inward moves beyond zero and negative compensation near zero underflow the unsigned target, which the range clamp then turns into the maximum position: the focuser drives outward to the maximum.
- **DSD-06** The cached position used for GOTO/SYNC no-op checks and backlash is not initialized at connection (only by the first poll 0.5 s later, and stale from a previous session): an early GOTO to the cached value is silently ignored.
- **DSD-07** Model-dependent capabilities are changed on connection but never restored: after reconnecting to another model (port or model change within one driver lifetime) speed maximum, step-mode count, coils visibility, timings count, current ranges/labels, temperature/mode/compensation visibility and the model version from the previous device persist.
- **DSD-08** Readback failures after writes are reported OK (SPEED, LIMITS, step mode, coils mode, move current, settle time); SPEED and connection readbacks use uninitialized values on failure (UB part audit-only).
- **DSD-09** `DSD_CURRENT_CONTROL` and `DSD_TIMINGS` readback updates only `target`; the published value stays the requested value, so a rejected write shows the rejected value.
- **DSD-10** A failed `FOCUSER_REVERSE_MOTION` write leaves the rejected switch published with ALERT.
- **DSD-11** `DSD_MODEL_HINT` is defined for every enumeration request regardless of the requested property (and twice at attach).
- **DSD-13** (found by the characterization harness) A move requested while a poll is in flight is not tracked: the running poll timer cannot be rescheduled (`Attempt to reschedule timer without reference or canceled timer!`), the poll's `[GPOS]` consumes the unread `(OK)` reply of `[SMOV]` as its position reply, and POSITION/STEPS are published OK within ~0.1 s while the motor keeps moving.
- **DSD-12** Property changes block the calling bus thread for their whole serial exchange (≥ 50 ms per command, ≥ 200 ms for current control, up to 400 s with a silent device).
- Audit-only risks: bus handlers, the position timer and the temperature timer share `PRIVATE_DATA` without synchronization (the port mutex protects I/O only) and abort cancels the poll timer asynchronously; a failed temperature read passes an uninitialized value to compensation; the `asi://` URL scheme is a copy-paste artefact (no network DSD exists) with a TCP-only unexpected-disconnection path; log statements print a handle pointer with `%d`; `dsd_command_get_value` scans `%d` into `uint32_t`.

## Atomic plan

1. **Done — record.** This file: audit, baseline, hardware decision, plan (written before any production change).
2. **Done — simulator audit and rewrite.** `focuser_dsd_simulator.c` rewritten against the protocol summary: AF1/AF2/AF3 models (`--model`, `--position`, `--temperature`), `serial_motion.h` elapsed-time motion at 2000 steps/s per speed unit, `[SMOV]` `(OK)`, `[STOP]` without reply, `!101)` maximum-move check, AF1/AF2 currents with `%`, AF3 multipliers, per-model speed/step-mode validation, `!100)` for unsupported commands and `!102)` for invalid arguments (both simulator assumptions), temperature and `-127` sensor-absent value, event log (`RX`/`TX`/`STATE`/`MOVE`/`FAULT`/`CONTROL`/`REJECT`), fault injection (`silent`, `error`, `malformed` `(12x)`, `partial` `(12`, `garbage` `(ERR)`, `close`, with repeat count) and external control (`position`, `temperature`, `stall`, `max_move`, `board`) through files named by `INDIGO_DSD_EVENTS`, `INDIGO_DSD_FAULT` and `INDIGO_DSD_CONTROL`. The test Makefile dependency now includes `serial_motion.h`. Simulator behaviour itself is verified by `simulator_protocol` and `simulator_af1_protocol` (direct PTY exchanges, including elapsed motion, stop and sync); these two cases are not counted as driver coverage in the mapping.
3. **Done — characterization suite against the original driver.** `indigo_test/integration/test_focuser_dsd_simulator.c` rewritten: every case runs in a forked child with its own simulator(s), temporary `HOME` (configuration files) and alarm; the parent kills the child's process group. The in-process client observes define/update/delete under a mutex with `force_property_updates`, tracks fresh revisions, BUSY/ALERT counts and definitions; the simulator event log provides ordered protocol assertions. Makefile: `test-focuser-dsd-simulator` and `test-focuser-dsd-simulator-sanitize` (arm64 ASan + UBSan with the driver source compiled into the test). 26 preservation cases (mapping below) pass against the unchanged driver. Harness discoveries: (a) unchanged OK publications are only delivered with `force_property_updates`; (b) a GOTO sent while the post-connection poll was still running reproduced DSD-13, so helpers now wait for the poll's publication; (c) concurrent runs of this suite (or any process using the device name `Focuser DSD AF`) collide on INDIGO's `/tmp/indigo_lock_Focuser DSD AF` global lock and must not be started in parallel.
4. **Done — defect reproducers.** `--known-defects` runs 14 dedicated cases (DSD-01..DSD-13; DSD-05 has two). All 14 fail against the original driver (evidence below). DSD-04 was initially too weak (the original publishes ALERT transiently during motion) and was strengthened to require that the state is still ALERT after motion has ended.
5. **Done — original reference trace.** `indigo_test/fixtures/focuser_dsd/original_reference_trace.txt` (428 lines): normalized ordered simulator exchange (`S RX`/`S TX`/`S FAULT`) followed by property definitions/updates/deletions (`D`/`U`/`X`) per step for AF1 (connect, GOTO, no-op GOTO, direction, relative inward/outward, SYNC, abort in motion, abort idle, speed, limits, reverse, backlash GOTO, step mode, coils, currents, timings, failed speed write, model hint, disconnect), AF2 (connect, automatic/manual mode, coils, disconnect) and AF3 (connect, multipliers, timings, step mode, disconnect). Normalization: temperature exchanges and `FOCUSER_TEMPERATURE` updates are excluded (2 s timer), consecutive moving polls collapse to `S POLL moving`, BUSY position values are masked, `X_` prefixes are removed, ports are masked. The abort step masks digits, drops poll exchanges and keeps one `[GPOS]` readback after `[STOP]` plus first occurrences of property lines, because the original cancels its poll timer asynchronously and a poll can interleave with `[STOP]` (observed under the sanitizer build). Repeated captures: normal build 3/3 identical; sanitizer build see step 5a.
5a. **Done — trace stability.** After the abort-step normalization (including collapsing masked digit runs, because the abort position has four or five digits depending on timing) the fixture was recaptured from the unchanged driver; `reference_trace` then passed 2/2 in the normal build and 4/4 in the sanitizer build. Only test-harness normalization changed; the driver was not modified.
6. **Done — `.driver` and regeneration.** Added `indigo_focuser_dsd.driver` (version 16 → 17, `DRIVER_VERSION` `0x03000010` → `0x03000011`; `serial { configurable_speed = true; }`; transactional `dsd_open`/`dsd_close` owning the port and the global lock; `X_` custom names; `motion_finalizer` completion polling; temperature polling via `indigo_execute_handler_in` on the device queue; generator default capacity, no `MAX_DEVICES` override). Generated with `build/bin/indigo_generator indigo_focuser_dsd.driver` (no generator warnings); `make -B -f ../../Makefile.drv` passes for x86_64 + arm64 with zero compiler/linker warnings. The strict check (`clang -arch arm64 -std=gnu11 -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wshadow -Wformat=2 -fsyntax-only`) first reported `-Wformat-nonliteral` in a `vsnprintf` forwarding helper; the command helpers were restructured to forward the `va_list` directly to `indigo_uni_vprintf()` (`dsd_vcommand`), after which the strict check is clean. Generator attribute finding: every declared inherited property is unhidden at attach unless it has a `hidden` attribute, so `FOCUSER_MODE` and `FOCUSER_COMPENSATION` declare `hidden = true` (connection reapplies model visibility).
7. **Done — post-migration suite and trace comparison.** First run against the generated driver: 25/26 preservation cases passed; the only failure was `reference_trace`, whose differences are analysed below (the ordered protocol exchange is identical). `--known-defects`: 13/14 reproducers passed; DSD-03 failed only because the harness helper `last_reply_to()` expected the reply immediately after the request while the simulator logs a `STATE` line before `!101)`; the helper now takes the next `TX` before the next request. The generated trace is checked in as `generated_reference_trace.txt` and is the contract compared by the suite; `original_reference_trace.txt` is kept for comparison.
8. **Done — defect fixes and generated-behaviour coverage.** The fixes were designed into step 6; all 14 reproducers pass and were promoted into the ordinary suite (the `--known-defects` mode now selects no cases). Added `urgent_abort_cancels_queued_move` (abort queued behind a running settings handler overtakes a queued GOTO: no `[STRG]`/`[SMOV]` reaches the device, `[STOP]` follows the running handler, POSITION ends OK at the unchanged position) and `busy_motion_requests_rejected` (POSITION request while POSITION is BUSY and STEPS request while POSITION is BUSY send no motion command and keep the published target). Mutation check: a scratch copy of the generated source without cancellation of the queued position handler made `urgent_abort_cancels_queued_move` fail with two unexpected motion commands, confirming the case detects the overtaking race.
9. **Done — repository integration.** `indigo.xcodeproj/project.pbxproj`: `indigo_focuser_dsd.driver` in the `focuser_dsd` group (the group already referenced `REFACTOR.md`), new `fixtures/focuser_dsd` group with both trace files; `plutil -lint` passes; concurrent uncommitted `mount_rainbow` project entries were left untouched. `indigo_focuser_dsd.vcxproj`: `.driver` and `REFACTOR.md` as `None` items (this file uses LF line endings); the generated C file remains the only compiled driver source. `indigo_test/Makefile`: simulator dependency on `serial_motion.h`, test dependencies on the driver archive and both fixtures, `test-focuser-dsd-simulator` and `test-focuser-dsd-simulator-sanitize`. `indigo_docs/PROPERTIES.md`: `X_` names, model-dependent shape, readback semantics, `.driver` source. `MIGRATION_STATUS.md`: generator and async queues `✅ Yes`, retested `✅ Sim`, tests `42 / 0`; Comment column unchanged. `README.md` unchanged.
10. **Done — final verification.** Evidence below. Linux and Windows builds and physical hardware were unavailable and are not claimed.

## Original-driver baseline evidence

Environment: macOS 26.6.2 arm64; normal test build is the repository universal x86_64/arm64 configuration linked against `build/drivers/indigo_focuser_dsd.a`; sanitizer build is arm64 only (`detect_leaks=0`, LeakSanitizer is unsupported on this macOS runtime).

- `./build/integration/test_focuser_dsd_simulator` (from `indigo_test`): 26 run, 26 passed (4 min 08 s).
- `make -C indigo_test test-focuser-dsd-simulator-sanitize`: 26 run, 25 passed; the failure was `reference_trace` (abort-step race, see step 5); all other cases passed with no sanitizer report. After the test-only trace normalization `reference_trace` passed 4/4 in this build (step 5a).
- `./build/integration/test_focuser_dsd_simulator --known-defects`: 14 run, 14 failed as expected:
  - DSD-01: CONNECTION did not reach ALERT within 10 s with a silent `[GPOS]` (100 s read timeout).
  - DSD-02: SYNC readback `(12x)` published POSITION OK instead of ALERT.
  - DSD-03: `[STRG]` error reply ended POSITION in OK instead of ALERT.
  - DSD-04: with `[GPOS]` failing during motion POSITION ended OK (state 1) instead of ALERT after motion stopped.
  - DSD-05 (steps): inward 5000 steps from 1000 did not settle at 0 (drove toward the maximum).
  - DSD-05 (compensation): −200 step compensation from 100 did not reach 0.
  - DSD-06: GOTO 0 immediately after connection sent no `[STRG000000]`.
  - DSD-13: GOTO 9000 during the post-connection poll settled OK after less than 1.8 s (motion needs 2 s).
  - DSD-07: after AF1 → AF3 reconnection `FOCUSER_SPEED` maximum stayed 3.
  - DSD-08: `[GSPD]` readback failure after a speed write settled OK instead of ALERT.
  - DSD-09: failed move-current write published value 60 instead of device value 50.
  - DSD-10: failed reverse write left `ENABLED` selected.
  - DSD-11: `DSD_MODEL_HINT` defined twice during attach.
  - DSD-12: `DSD_CURRENT_CONTROL` change request returned after 0.207 s.

## Post-migration evidence

Environment as for the baseline.

- Reproducible generation: the generator was run twice more after the final `.driver` edit; SHA-1 of the checked-in outputs unchanged: `.c` `ad1cf9b6361750b940049532fdd475189d1f96c5`, `.h` `a741e35eb1e4b4dc14cf1bbe97d2b3d8e64e5106`, `_main.c` `3afcb8540e900e94913ab1edc515224a7d9fe8ef`. No generator warnings.
- `make -B -f ../../Makefile.drv` in this directory: passed, x86_64 + arm64, zero warnings. Strict syntax check clean (step 6).
- `./build/integration/test_focuser_dsd_simulator`: 42 run, 42 passed (5 min 51 s).
- `make -C indigo_test test-focuser-dsd-simulator-sanitize` (arm64 ASan + UBSan, `detect_leaks=0`): 42 run, 42 passed (5 min 55 s), no sanitizer report, no driver compiler warning. Instrumentation covers the test and the driver; `libindigo` is not instrumented.
- `git diff --check` and a formatting audit of the `.driver`, simulator and test sources: tab indentation, no trailing whitespace, no blank lines inside function bodies.
- Unavailable: Linux (x64/arm/arm64) and Windows builds, physical AF1/AF2/AF3 focusers.

## Reference trace comparison

`original_reference_trace.txt` (unchanged driver) and `generated_reference_trace.txt` (current contract, 447 lines) differ in 29 diff lines: 5 STEPS BUSY lines moved after POSITION BUSY (10 lines) and 19 additional BUSY lines; property names are normalized by removing `X_`. Every `S` (protocol) line is identical: connection sequences for all three models, every `[STRG]`/`[SMOV]` with backlash targets, polls, `[SPOS]`, `[STOP]` and readback, settings writes and readbacks in order, the failed write and its readback, and disconnection `[STOP]`. All differences are property publications:

1. Queued changes publish BUSY before the handler runs (`INDIGO_COPY_*_PROCESS_CHANGE`): an additional BUSY line for `FOCUSER_SPEED`, `FOCUSER_LIMITS`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_ABORT_MOTION`, `FOCUSER_MODE`, the custom settings properties and the synchronous model hint. For preserved-value properties the BUSY line shows the old value and the requested target.
2. For GOTO, no-op GOTO, SYNC and relative moves the framework publishes POSITION (or STEPS) BUSY before the handler publishes STEPS BUSY, so the first BUSY lines appear as POSITION, STEPS instead of STEPS, POSITION; the no-op GOTO additionally shows the POSITION BUSY acknowledgement before OK. Device commands and completion order are unchanged.

Not visible in the normalized trace but intentionally changed: custom connect-scoped properties are defined after the whole connection sequence (generator order) instead of interleaved with their readbacks; the failed-open message is the generated "Failed to connect" message instead of the CONNECTION update message "Deep Sky Dad AF did not respond" (still logged).

## Intentional behaviour differences

- **Property names:** five custom properties gained the mandatory `X_` prefix (`X_DSD_MODEL_HINT`, `X_DSD_STEP_MODE`, `X_DSD_COILS_MODE`, `X_DSD_CURRENT_CONTROL`, `X_DSD_TIMINGS`); items, labels, groups, rules and scope are unchanged. Saved configurations and clients using the old names must be updated.
- **Serialization:** property handlers, the completion poll and temperature polling run on the device queue; the port mutex was removed. The original ran handlers on bus threads and polls on independent timer threads (DSD-12, DSD-13, audit concurrency risk).
- **Transport timeout:** replies must end with `)` within 3 s (the fallback value of the original helper) instead of 100 s; timeouts and unterminated replies are failures (DSD-01).
- **Reply validation:** numeric replies must be exactly `(<int>)` or `(<int>%)`, temperature `(<float>)`, firmware `(Board=…, Version=…)`, set commands `(OK)`. `[STRG]` now requires `(OK)` before `[SMOV]` is sent; any other reply is a start failure (DSD-02, DSD-03). The replies of `[SMOV]` and `[STOP]` are still not read and are discarded by the next command, as in the original.
- **Busy requests:** a POSITION or STEPS request while that property is BUSY is ignored by the framework guard; a STEPS request while POSITION is BUSY (including a compensation move) is answered with "Motion already in progress". The original re-targeted a running move.
- **Abort:** runs at urgent priority and cancels queued position/steps handlers and the pending completion poll before `[STOP]`; command order `[STOP]`, `[GPOS]` is unchanged.
- **Completion polling:** a failed `[GMOV]` or `[GPOS]` read ends the move with POSITION/STEPS ALERT and stops polling (DSD-04); a started move replaces a pending connection or compensation poll so only one completion poll runs.
- **Generic range checks:** the original rejected positions/steps outside the published range; the framework already clamps requests, so the duplicate checks were removed. Targets are clamped with signed arithmetic and backlash targets below zero are clamped to 0 (DSD-05).
- **Readback semantics:** speed, limits, currents and timings publish the device readback as value (and target for currents/timings) and keep the last confirmed value on readback failure; any write or readback failure reports ALERT (DSD-08, DSD-09). Failed reverse writes restore the confirmed switch (DSD-10). Step-mode/coils readback values outside the published items are failures.
- **Connection:** capabilities, labels, visibility and model version are reset before every detection (DSD-07); the cached position is initialized from the connection readback (DSD-06); the 2 s RTS-reset delay is applied only after a successful open (the original also slept after a failed open); the `asi://` TCP URL path and its unexpected-disconnection handler were removed (copy-paste from the ASI driver; no network DSD device exists).
- **Model hint:** defined once at attach and only for matching enumerations (DSD-11).
- **Temperature failure:** a failed `[GTMC]` read publishes ALERT and skips compensation instead of passing an uninitialized value.

## Found defects

Every defect below was reproduced by a dedicated case that failed against the original driver (evidence in the baseline section) and passes against the generated driver. Audit-only risks are listed separately.

| ID | Observable impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- |
| DSD-01 | A silent device blocks connection (and synchronous handlers) for 100 s per command; unterminated replies are accepted after the timeout. | `dsd_command()` passed `100` to `INDIGO_DELAY()` and treated `indigo_uni_read_section()` result `0` as success. | 3 s timeout; reply must be non-empty and end with `)`. | `DSD-01 silent_device_times_out` (silent connect and unterminated SYNC readback) |
| DSD-02 | Malformed replies such as `(12x)` are published as valid values (position 12, speed 12). | `sscanf("(%d)")` does not verify the closing parenthesis. | Exact reply parsers. | `DSD-02 malformed_reply_rejected` |
| DSD-03 | A rejected move start (`!101)`, error or no reply) is reported as a completed OK move; non-`!101)` failures still send `[SMOV]`. | Start result only logged; only `!101)` checked; polling then saw an idle motor. | `[STRG]` requires `(OK)`; start failure publishes ALERT and schedules no poll. | `DSD-03 move_start_failure_alerts` |
| DSD-04 | Read failures during motion end in OK with a stale position. | Poll failure state overwritten by the `!moving` completion branch; failed `[GMOV]` evaluated an uninitialized value. | Poll failure publishes ALERT and stops polling. | `DSD-04 poll_failure_alerts` |
| DSD-05 | Inward steps beyond zero or negative compensation near zero drive the focuser outward to the maximum position. | Unsigned target underflow, then the upper clamp. | Signed target arithmetic and clamping; backlash target clamped to 0. | `DSD-05 inward_move_below_zero`, `DSD-05 negative_compensation_below_zero` |
| DSD-06 | A GOTO to the cached value right after connection (or after reconnecting to a moved focuser) is ignored. | Cached position initialized only by the first poll. | Connection readback initializes the cached and target position. | `DSD-06 goto_right_after_connect` |
| DSD-13 | A move requested while a poll runs is reported OK after ~0.1 s while the motor keeps moving; the position readback consumes the `[SMOV]` reply. | Timer reschedule of a running poll fails; poll and handler threads interleave on the port. | All I/O serialized on the device queue; a new move replaces the pending poll. | `DSD-13 move_during_poll_is_tracked` |
| DSD-07 | After reconnecting to another model, ranges, item counts, visibility, labels and the model version of the previous device remain. | Connection only narrowed capabilities, never restored them. | Reset before every detection. | `DSD-07 model_change_resets_capabilities` |
| DSD-08 | Failed readbacks after successful writes are reported OK (with an uninitialized speed value). | Readback results ignored or not reflected in state. | Readback failure keeps the confirmed value and reports ALERT. | `DSD-08 readback_failure_alerts` |
| DSD-09 | Rejected current/timing writes show the rejected value as the device value. | Readback updated only `target`. | `preserve_values` plus readback to value and target. | `DSD-09 current_value_matches_device` |
| DSD-10 | A rejected reverse write leaves the rejected switch selected. | Values copied before the device command, never restored. | Confirmed state cached and restored on failure. | `DSD-10 reverse_failure_restores` |
| DSD-11 | `DSD_MODEL_HINT` is defined twice at attach and for unrelated enumeration requests. | Unconditional `indigo_define_property()` in attach and enumerate. | Generated `always_defined` matching enumeration. | `DSD-11 model_hint_enumeration` |
| DSD-12 | Property changes block the calling client/bus thread for the serial exchange (0.207 s measured for current control). | Synchronous handlers on the bus thread. | Queued generated handlers. | `DSD-12 property_change_does_not_block` |

Audit-only risks resolved by the migration without a dedicated reproducer: unsynchronized `PRIVATE_DATA` access between bus handlers and two timer threads; asynchronous abort cancel allowing a poll to interleave with `[STOP]` (observed in the original trace under the sanitizer, now serialized and cancelled on the queue); uninitialized temperature passed to compensation after a failed read; `%d` logging of a handle pointer and `%d` scans into `uint32_t`; the `asi://` URL path.

Remaining known limitations (not changed): `FOCUSER_POSITION` maximum stays fixed at 1000000 instead of following the device maximum, and `[SMXM]` is still set to that value at connection, because device behaviour beyond the maximum position is undocumented; `FOCUSER_COMPENSATION` and temperature polling behaviour of real sensors, real AF1/AF2 current granularity (INDI offers 25/50/75/100 %), RTS reset timing and replies to `[SMOV]`/`[STOP]` are unverified without hardware; generated connection failure no longer carries the "Deep Sky Dad AF did not respond" CONNECTION message.

## Scenario-to-test mapping

Focuser class standard (`indigo_test/DRIVER_TESTING_RULES.md`) and shared scope:

| Area | Cases |
| --- | --- |
| Simulator protocol self-check (not driver coverage) | `simulator_protocol`, `simulator_af1_protocol` |
| Metadata, INFO, interface, visible serial properties, model hint inventory, disconnected inventory, no I/O before connection | `metadata_before_connection`, DSD-11 |
| Model hint and baud-rate selection | `model_hint_selects_baudrate`, `reference_trace` |
| Connection sequence, capability discovery per model (speed range, step-mode count, coils visibility, timings count, current ranges/labels, temperature/mode/compensation visibility), initial poll, temperature timer | `connect_af1_capabilities`, `connect_af2_capabilities`, `connect_af3_capabilities`, DSD-07, `reference_trace` |
| Open/identification failure, descriptor balance, reconnect, firmware query failure, silent device | `connection_failures_and_recovery`, `firmware_query_failure`, DSD-01 |
| Absolute GOTO with elapsed motion, BUSY→OK, no-op GOTO, target beyond device maximum | `goto_and_noop`, `goto_beyond_device_maximum`, DSD-06, DSD-13 |
| SYNC without motion commands, sync failure and recovery, malformed readback | `sync_position`, DSD-02 |
| Relative inward/outward, zero steps, device-side reversal, lower boundary | `relative_steps_and_reverse`, DSD-05 |
| Backlash overshoot semantics | `backlash_overshoot`, `reference_trace` |
| Abort in motion and idle, fresh move after abort, abort readback failure | `abort_motion_and_idle`, `reference_trace` |
| Overlapping motion requests (final consistency) | `overlapping_motion_requests` |
| Move start and poll failures | DSD-03, DSD-04 |
| Speed, limits, reverse, step mode, coils mode, currents (AF1/AF2 and AF3 commands), timings, direction/compensation/backlash acceptance without I/O | `settings_af2`, `settings_af3`, `reference_trace` |
| Write failures and readback restoration, readback failures | `write_failures`, DSD-08, DSD-09, DSD-10 |
| Temperature polling, sensor absent/restored, read failure/recovery | `temperature_polling` |
| Manual/automatic mode permissions and property sets, compensation threshold, positive/negative correction, ≥100 °C and absent-sensor rejection, manual mode inhibits compensation | `compensation_modes`, DSD-05 |
| Disconnect during motion (stop, no I/O after close), reconnect | `disconnect_during_motion` |
| INIT/SHUTDOWN idempotence, shutdown refused while connected, redundant disconnect, repeated connect cycles | `lifecycle_and_shutdown` |
| Additional instance with its own port | `additional_instance` |
| CONFIG save of custom and inherited properties | `configuration_save` |
| Non-blocking property changes | DSD-12 |
| Ordered protocol/property compatibility contract | `reference_trace` |

Not applicable or not covered: hot plug and multiple physical devices (serial, no enumeration), guider timing (no guider interface), network transport (the `asi://` URL path is removed, see intentional differences), RTS reset timing (a PTY has no reset; only the fixed 2 s delay is observed), behaviour of real firmware for undocumented replies (simulator assumptions), hardware acceptance (no device).

## Final test summary

- Simulated tests, original driver: preservation suite 26 run, 26 passed (normal build); 26 run, 25 passed (ASan/UBSan build, `reference_trace` abort race before normalization), followed by `reference_trace` 6 run, 6 passed after the test-only normalization (2 normal, 4 sanitizer); defect reproducers 14 run, 0 passed (all failed as expected).
- Simulated tests, generated driver: first comparison run 26 run, 25 passed (trace differences analysed) and 14 reproducers run, 13 passed (harness helper defect, fixed); final 42 registered cases: normal build 42 run, 42 passed in each of two complete runs, ASan/UBSan build 42 run, 42 passed; mutation check 1 run against a deliberately broken scratch driver, failed as intended (not counted as a pass).
- Hardware tests: 0 run, 0 passed.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard are now declared with the generator's `reject_change` block. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values instead of an `INDIGO_OK_STATE` update carrying no items, which left the refused value visible in the client.

Covered by the extended `busy_motion_requests_rejected` scenario in `indigo_test/integration/test_focuser_dsd_simulator.c`: while `FOCUSER_POSITION` is BUSY, `FOCUSER_STEPS` now has to settle in ALERT with unchanged value and target.

```sh
make -C indigo_test test-focuser-dsd-simulator
```
