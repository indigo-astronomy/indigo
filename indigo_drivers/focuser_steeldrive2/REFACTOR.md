# SteelDrive II generated migration

Status: complete, 2026-09-13. Baseline version: 0x0200000C; migrated version: 0x0300000D. Hardware tests are explicitly out of scope. The user-facing README remains unchanged.

## Sources and protocol confidence

- Repository guidance: root and `indigo_test/AGENTS.md`, driver-development basics, generator migration guide, serial-simulator contract, common driver-testing scope, focuser standard, AUX/powerbox standard, and completed generated multi-device focuser migrations.
- Production sources: the hand-written driver, public header and standalone wrapper; the host PTY and Arduino simulators; the existing integration test; property, simulator, test, Makefile, Xcode, Windows and migration records.
- Manufacturer documentation: bundled 31-page *SteelDrive II Focuser Technical documentation*, 25 June 2019, firmware documentation version 0.770. Chapter 3 and the CRC appendix were inspected as extracted text and rendered pages.
- The documented virtual COM port is 19200 baud, 8 data bits, no parity, one stop bit and no flow control. Commands and replies end in CR+LF; every received character is echoed. Valid protocol messages start with `$BS`. Boot emits `$BS Hello World!`.
- CRC is optional but encouraged. `CRC_ENABLE` turns on Dallas/Maxim CRC8; `*HH` is appended to messages, invalid input CRC is ignored, and RESET, REBOOT and CRC_DISABLE are exceptions. The manual documents unknown-command errors, `$BS OK` SET acknowledgements and `$BS STATUS VARIABLE:VALUE` GET replies.
- The implemented feature set is documented: INFO/SUMMARY polling; absolute GO/STOP; end-stop selection and zeroing; position sync and upper limit; saved focus/jog/single-step/backlight values; temperature compensation mode/factor/period/delta/pause/sensor; two temperature sensors and offsets; heater PWM; PID enable/target/sensor/dew offset; ambient sensor and automatic dew control. GO clamps to 0..LIMIT. ZEROING is asynchronous when the end stop is enabled. Missing temperature sensors report -128.
- Simulator-backed results can validate command grammar, CRC, parsing, state transitions and shared-device lifecycle, but cannot certify firmware timing/quirks, USB serial electrical behavior, physical motor direction/current/travel/end stop, temperature accuracy, heater load, Linux/Windows runtime or real hardware concurrency.

## Existing behavior and findings

- The driver exposes two logical devices sharing one serial controller: `SteelDriveII (focuser)` and `SteelDriveII (aux)`. The focuser owns the visible serial port and exposes position, relative steps, direction/reversal, GOTO/SYNC, abort, temperature, limits, temperature compensation, controller identity/saved values, two sensor readings, compensation-sensor selection, reset/reboot, end-stop selection and zeroing. AUX exposes heater PWM, automatic-dew mode, PID enable/settings/sensor and ambient-sensor selection.
- The driver is hand-written API 2 style. Property work uses zero-delay timers plus a shared mutex instead of handler queues. Serial I/O uses `indigo_printf()`, `indigo_read_line()` and POSIX `close()`, so the driver is not portable to Windows. Responses are held in many stack buffers.
- `steeldrive2_command()` does not use bounded uniform-I/O line reads. Its echo prefix check can accept extra trailing content; `strncpy()` may omit NUL termination; reply lines without CRC remain accepted after CRC is enabled; CRC text length and hex digits are not strictly validated; and reply prefixes/payloads are left to permissive `sscanf`, `atoi`, `atof` and token mutation.
- Connection is not transactional. The shared reference count is incremented before open and only the transport probe determines connection success. Individual initialization failures merely mark properties ALERT while CONNECTION becomes OK. A failed or partially initialized logical device can therefore retain shared ownership/state. The first connected logical device owns the polling timer, making polling lifetime and property context dependent on connection order.
- SUMMARY polling mutates a shared response with `strtok_r` and permissively accepts missing, duplicate, malformed and non-finite fields. Poll failures are silent. AUX PWM is updated only when the timer was started from the AUX logical device; a focuser-first connection does not publish external PWM changes. Disconnecting the timer-owning logical device while its sibling stays connected leaves ambiguous timer/property ownership.
- GOTO and relative handlers publish OK on command acknowledgement, before physical motion completes. Polling may later change the same properties back to BUSY, producing an invalid OK-to-BUSY transition. There is no named motion finalizer, completion deadline or deterministic recovery after poll/transport loss. ZEROING likewise publishes OK immediately even when documented end-stop homing is still moving. Abort is not an urgent queued operation and does not explicitly reconcile the actual stopped position.
- Change dispatch does not use the INDIGO process-change helpers, so framework BUSY conflict protection is not consistently applied. Overlapping moves, zeroing, settings, disconnect and shared-device operations are not covered by tests.
- Most SET handlers contain the inverted condition `!steeldrive2_command(...) && !strcmp(response, "$BS OK")`. An ERROR acknowledgement is treated as success; transport failure can compare an uninitialized response; and the affected property often remains incorrectly OK. This affects saved values, compensation, limits, sensor selections, end-stop, zeroing and most AUX controls.
- The saved-values handler sends six focuser values and then incorrectly sends the AUX PID offset and target too. Temperature offsets and PID settings are formatted as integers even though the protocol requires floats with a decimal point, losing fractional values. Documented 32-bit focus/jog ranges and the SINGLESTEPS <= JOGSTEPS relationship are not represented accurately.
- Initialization reads `TCOMP_DELTA` twice. The driver does not expose documented TCOMP_PAUSE or expert motor-current/RCX controls; these remain non-applicable unless preserving driver scope requires otherwise.
- A successful RESET returns while holding the shared mutex, causing a deadlock. REBOOT uses an unchecked raw write and disconnects only the focuser context even if AUX shares the session. Reset/reboot handling does not consume or model the documented reboot sequence safely.
- AUX property allocation checks the wrong pointer after creating `X_PID_SETTINGS`. AUX initialization assigns PID-settings failures to the focuser's `X_SAVED_VALUES` state. The custom item `PID TARGET` contains a space and must be preserved only if compatibility requires it.
- `X_USE_AUTO_DEW` is initialized from `GET AUTO_DEW`, but its change handler sends `SET PID_CTRL` rather than `SET AUTO_DEW`; automatic dew cannot be changed correctly. `X_USE_PID` already controls PID. The PID offset handler sends the undocumented typo `SET PID_DEV_OFSL` instead of `SET PID_DEW_OFS`.
- Setting PWM is documented to disable PID. The current driver re-reads PID and AUTO_DEW afterwards, but no test verifies the linked property reconciliation or failure behavior.
- The host simulator implements many nominal commands but strips and ignores request CRC without verifying it, accepts malformed/range-invalid values, and has no missing/bad-reply injection. Motion is completed at the start of the next command, so SUMMARY never exposes meaningful elapsed BUSY progress. Zeroing is instantaneous; reset/reboot semantics, missing-sensor behavior, split replies, command journal, external mutation, stalled motion and connection-order/resource evidence are absent. It does not use `serial_motion.h`.
- The Arduino sketch is a historical hardware fixture and is not suitable as the automated-test oracle.
- The integration suite has only two broad smoke cases, one per logical device. It does not independently validate simulator protocol/CRC, initial readback, fresh revisions, command parameters/order, motion progress/overlap/abort/recovery, zeroing, boundaries, external changes, per-setting failures/reconciliation, malformed replies, transactional connection, disconnect during work, reconnect, both shared connection orders or resource release.
- Build metadata already contains the host simulator and test targets, and both existing sources are in Xcode. `REFACTOR.md` and the future `.driver` are missing from Xcode. No Visual Studio project exists. The simulator inventory still lists only the Arduino sketch as a future candidate despite the existing PTY simulator. Migration status remains API 2 / no Windows / no generator / no queues / not retested.

## Atomic plan and progress

1. **Create the complete refactoring ledger — complete.** Inventory repository rules, production code, both simulators, current tests, bundled protocol documentation, properties and all build/project/status wiring. Record findings, acceptance coverage, exclusions and every atomic implementation/validation/integration obligation here before modifying production code.
2. **Register this ledger — complete.** Add `REFACTOR.md` to the existing SteelDrive II Xcode group and validate the project file. Do not change README.
3. **Capture the baseline — complete.** Record scoped git status and the exact pre-change driver/generated-file hashes; build the untouched driver, PTY simulator and existing test; run both original smoke scenarios; record results without treating them as complete coverage.
4. **Design the generator source — complete.** Inspect the closest generated multi-device serial drivers and generator semantics. Define one focuser master plus one AUX slave with shared private data, generator-owned connection reference counting and queues, independent logical-device polling contexts, additional-instance behavior only where valid, and no `MAX_DEVICES` override. Document any generator limitation and request approval before changing the generator.
5. **Rebuild the protocol layer — complete.** In the authoritative `.driver`, implemented variadic `steeldrive2_command()` over uniform I/O, explicit first/inter-byte timeouts, echoed-line validation, required CRC after enable, strict two-digit hex/checksum validation, complete CR+LF lines, bounded debug/irrelevant-line handling, exact acknowledgements/prefixes, strict finite integer/float parsing and reusable private buffers. Implemented transactional `steeldrive2_open()` / `steeldrive2_close()`.
6. **Migrate lifecycle and shared ownership — complete.** The `.driver` is authoritative and generated C/header/main preserve both logical devices and compatible public properties. Version is 0x0300000D and copyright reaches 2026. Tests cover first/second connection rollback, both connection orders, survivor polling, final close, cancellation and connected shutdown refusal.
7. **Migrate focuser properties and fixes — complete.** Preserved the supported focuser surface and corrected inverted reply tests, target/value ownership, duplicate reads, float formatting, documented ranges and device-specific constraints. TCOMP_PAUSE and expert motor-current/RCX commands remain intentionally non-applicable because the original driver did not expose them.
8. **Implement asynchronous focuser operations — complete.** GO and end-stop ZEROING use a queued handler plus named `motion_finalizer`; BUSY/progress and bounded OK/ALERT completion are explicit. Tests cover no-op/boundaries, overlap rejection, queued abort with pending-start/finalizer cancellation, stopped-position reconciliation, stalled/poll/transport failure, recovery and disconnect cancellation without blocking the queue.
9. **Migrate AUX properties and fixes — complete.** Preserved heater/PID/automatic-dew controls and initial readback; corrected AUTO_DEW, PID_DEW_OFS, allocation/state ownership and decimal formatting. PWM-disables-PID and linked-state reconciliation are simulator-backed, including failures and external mutation.
10. **Make reset/reboot safe — complete.** Removed the mutex deadlock and unchecked raw write. RESET/REBOOT have deterministic teardown and reconnect behavior; shared-session reset is rejected while a sibling remains connected and is covered by tests.
11. **Rebuild the host simulator as an independent oracle — complete.** The Arduino sketch is unchanged. The host simulator uses `serial_motion.h`, validates framing/grammar/ranges/CRC, implements every driver-used command and supplies deterministic journal, fault, split, stall, mutation, profile and reset/reboot facilities.
12. **Add direct simulator protocol tests — complete.** The `simulator_protocol` scenario independently covers echo/framing, CRC transitions and rejection, unknown/malformed/range-invalid commands, movement/status/stop/sync/zeroing, missing sensors, heater/PID/dew side effects and reset/reboot state.
13. **Expand focuser integration coverage — complete.** Fork-isolated named scenarios cover the complete applicable focuser checklist, including every required initialization decision point and rollback/retry, all operations/settings, asynchronous transitions, failures, recovery, disconnect/reconnect, external state and additional instances.
14. **Expand AUX and shared-lifecycle coverage — complete.** Fork-isolated named scenarios cover the full applicable AUX contract, each initialization failure, linked-state behavior, both shared orders, disconnect/sibling-failure isolation, one shared transport, final close, reconnect and shutdown behavior.
15. **Add sanitizer and strict-build targets — complete.** The Makefile tracks `serial_motion.h` and provides a scoped ASan target instrumenting generated production code and the suite. Current generated code and simulator compile with `-Wall -Wextra -Werror` at O0/O3 respectively; no shared warnings were weakened.
16. **Run complete hardware-free validation — complete.** Two regenerations were byte-identical. Archive/library/executable and strict objects built; all 49 ordinary and all 49 ASan scenarios passed. Generated queue/master-slave/finalizer/rollback/timer behavior was inspected; no `MAX_DEVICES` override exists. Project, XML and diff checks pass. No hardware tests were run.
17. **Add Windows project without Xcode registration — complete.** Added `.vcxproj` and `.vcxproj.filters` from the iOptron pattern with Debug/Release x64/ARM64, unique GUID, generated sources, `.driver` and `REFACTOR.md`; registered them in the solution and server project. XML/GUID/configuration/path checks pass. Neither Visual Studio file is in Xcode. Windows compilation/runtime remains unverified.
18. **Synchronize repository records — complete.** Added both new non-Visual-Studio files to Xcode; updated only the SteelDrive II migration status columns with its Comment preserved; synchronized property/simulator/test records and coverage gaps. README is byte-identical and unrelated edits were preserved.
19. **Final audit — complete.** Re-read the complete checklist. Persistent files have the required project registrations, VS files are absent from Xcode, records agree, the version increased, README is unchanged and hardware/Windows runtime are not claimed as tested. Exact final pass counts are recorded below.

## Required acceptance coverage

The final test inventory must map every applicable focuser and AUX requirement to named scenarios, with fresh property revisions and bounded waits. At minimum it must cover protocol/framing/CRC and error grammar; both logical-device capability contracts; pre/connected/post visibility; transactional initialization at every decision point; all readbacks; GOTO/SYNC/relative/reverse/limits/no-op/boundaries; elapsed BUSY/progress/completion; zeroing modes; overlap/abort/disconnect/failure/recovery; temperature and external mutation; every saved/compensation/sensor setting; heater/PID/dew controls and their documented side effects; both shared connection/disconnection orders; sibling failure isolation; reconnect, shutdown and resource release.

Generic framework validation of numeric range, finite value, integer step and BUSY guards must not be duplicated. Hardware-only validation is excluded. TCOMP_PAUSE, expert motor-current and RCX commands are not exposed by the existing driver and are provisionally non-applicable; the final record must preserve that rationale or update it if scope changes.

## Evidence log

- 2026-09-13: Inspected all 31 PDF pages by text extraction and visually inspected the complete rendered communication-protocol section (PDF pages 12-27, document pages 11-26), including command examples and CRC requirements.
- 2026-09-13: Confirmed the current driver is 1,332 lines, the PTY simulator 373 lines and the integration test 170 lines with only two registered smoke cases.
- 2026-09-13: Confirmed existing test Makefile and Xcode references for simulator/test, absent `.driver`/REFACTOR/Visual Studio files, stale simulator inventory, and migration row `2 / No / No / No / No / Yes`.
- 2026-09-13: Added `REFACTOR.md` to the existing SteelDrive II Xcode group. Xcode project syntax validation is recorded with the baseline checks.
- 2026-09-13: Baseline SHA-256: README `8135402ea6782dacf46b4c6ed9cff24190f3e95de6c82c973b06d6bf3673f74c`; driver C `6da1ee62a3c8afb28aed599af88c1260421f5d5b2db6f52103ddcb8e52a59924`; header `14bf8e3295f5d93e8944aecfcf30513f03997f0c2b09a9803b3452701dc90271`; main `311b4354a4e01722ba4772aa51b3ec97ee2c8ba2a76ba9bfbde710209032c79d`; PTY simulator `b6fb7ed7f6bcc43fd8a5b0e63a856695a5323e1d3f74f403d5f643dc38907067`; test `f9f6cdaa43267ede497d87decb5d421228edfa0e857ede3501dfcd402bfb49bd`.
- 2026-09-13: A clean baseline rebuild produced the universal macOS archive, dynamic library and executable plus the PTY simulator/test. Both original smoke cases passed. An initial run had linked a stale pre-existing driver archive and crashed in `focuser_attach`; forcing clean driver and test rebuild removed the ABI-stale artifact and the source-identical baseline passed 2/2. This is build-artifact evidence, not a production-source defect. `plutil` accepted the Xcode project.
- 2026-09-13: Inspected generated `focuser_prodigy` as the closest serial focuser+AUX ownership model and generated single-/multi-class finalizer patterns. The unchanged generator already provides transactional open/reference rollback, master-queue dispatch for slave changes and per-logical-device handler cancellation. SteelDrive II will use separate focuser and AUX periodic handlers so polling remains valid in either connection order and after either sibling disconnects. No generator change or `MAX_DEVICES` override is needed.
- 2026-09-13: Ran reverse extraction safely in `/private/tmp` with `indigo_generator -c <temporary>.driver`; the resulting 408-line skeleton confirmed all inherited/custom property mappings and two logical device blocks without touching the hand-written repository source.
- 2026-09-13: Added authoritative `.driver` version 0x0300000D and regenerated C/header/main twice byte-identically: C `3379be708e98a57c69de63d0a3734b843e3eb9c8fded5a6d7a2b39ee2838770c`, header `69125a035e0655807289e7ea43e2fdb673b0bdb3a703207c5d4a249da28d9ff3`, main `f26a09d244c17a18e2fb35de0b3b5bdae4324fcc1e630277abc6afeb29820270`.
- 2026-09-13: Rebuilt the protocol/lifecycle/property implementation and independent host simulator; verified queue/finalizer ownership, strict parsing, transactional shared ownership and persistent ALERT after uncertain motion until abort reconciliation.
- 2026-09-13: Replaced the two smoke cases with 49 fork-isolated scenarios. Final ordinary result: 49/49 passed. Final ASan result with `ASAN_OPTIONS=abort_on_error=1`: 49/49 passed. Leak detection is not claimed because this macOS ASan runtime does not support the requested leak option.
- 2026-09-13: Clean driver archive/dylib/executable build passed. Generated code passed `-Wall -Wextra -Werror -O0`; simulator passed `-Wall -Wextra -Werror -O3`. Xcode plist, Visual Studio XML, solution/GUID/path and `git diff --check` validation passed.
- 2026-09-13: Added and wired the Windows x64/ARM64 Debug/Release project without registering its two Visual Studio files in Xcode. Windows compilation/runtime remains unverified. No hardware tests were run.
- 2026-09-13: README SHA-256 remains `8135402ea6782dacf46b4c6ed9cff24190f3e95de6c82c973b06d6bf3673f74c`; project/property/simulator/migration/test records are synchronized.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard are now declared with the generator's `reject_change` block. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values instead of an `INDIGO_OK_STATE` update carrying no items, which left the refused value visible in the client.

Covered by the `rejected_change` scenario in `indigo_test/integration/test_focuser_steeldrive2_simulator.c`: while `FOCUSER_POSITION` is BUSY, `FOCUSER_STEPS` and the `X_START_ZEROING` switch both end in ALERT with their values restored, and motion is accepted again after the abort.

```sh
cd indigo_test && STEELDRIVE2_TEST_FILTER=rejected_change ./build/integration/test_focuser_steeldrive2_simulator
```

## Switch target adoption: aux heater controls and FOCUSER_POSITION (3.0.0.18, 2026-09-27)

Findings TGT-011, TGT-012 and the focuser_steeldrive2 parts of TGT-B03 and TGT-B05 in `indigo_drivers/REVIEW_SWITCH_TARGETS.md`, all reproduced before the fix.

- **Defects (reproduced):** the 1 s aux poll called `steeldrive2_read_aux()`, which wrote X_USE_PID, X_USE_AUTO_DEW and the AUX_HEATER_OUTLET value and target from the device, then set the three properties OK (or ALERT) and published them without a BUSY check. Polls are `INDIGO_TASK_PRIORITY_TIME` tasks and run ahead of a queued change handler, so a request copied while the poll came due was overwritten and shown OK before it was sent; the handler then read the overwritten value, sent the old setting, read it back as matching and reported OK. The AUX_HEATER_OUTLET handler (which reads the aux state back, and the device disables PID control when the PWM is set) overwrote a queued X_USE_PID request the same way. The 0.5 s focuser poll wrote FOCUSER_POSITION `number.target` from the device and published FOCUSER_POSITION / FOCUSER_STEPS OK, so a queued GOTO read the current position as its target and never moved.
- **Fix:** `steeldrive2_read_aux()` keeps the PID and dew settings in private data and leaves value and target of a BUSY AUX_HEATER_OUTLET, X_USE_PID or X_USE_AUTO_DEW alone; the aux poll sets and publishes only the properties that are not BUSY. The X_USE_PID and X_USE_AUTO_DEW handlers send the request read with `indigo_get_switch_target()`, apply it with `indigo_apply_switch_targets()` when the device reports it and otherwise show the setting the device last reported with ALERT (the command order SET, GET, GET is unchanged). The AUX_HEATER_OUTLET handler reads `number.target` and on failure shows the PWM the device last reported. The focuser poll skips its status read and publication while FOCUSER_POSITION or FOCUSER_STEPS is BUSY for a pending request; a BUSY the poll published itself for a motion the driver did not start (`external`, set by `steeldrive2_publish_focuser()`) is still refreshed and ended by the poll.
- **Simulator:** the PTY is set to raw mode at start. With the default settings it echoed the periodic `$BS Hello World!` boot banner back to the simulator until the driver opened the port, the simulator answered its own output, and a partial echo corrupted the driver's first `$BS CRC_DISABLE`; on Linux this failed all five aux cases whose connect path waits more than 100 ms (and one further, uncaptured case in one of two baseline runs). A real serial line does not echo. The new `external_pid_ctrl` control models PID control switched on the device itself. The test Makefile now links the simulator with `-lm` (`lround`), which Linux needs.
- **Regression tests:** `aux_pid_request_survives_poll`, `aux_auto_dew_request_survives_poll` and `aux_heater_request_survives_poll` hold the device queue with a gate handler, send the request, change another aux value on the device so the poll publishes a witness, let the poll come due behind the gate, and check that the first result after the request is the handler's requested value with OK, published after the poll ran, with exactly one `SET`. `aux_pid_request_survives_heater_change` queues an AUX_HEATER_OUTLET and an X_USE_PID request behind the gate. `position_request_survives_poll` holds the queue for more than two focuser polls behind a GOTO and checks that the first result is OK at the requested position with exactly one `$BS GO`. Against 3.0.0.17 all five failed (poll published the old value OK first and no `SET PID_CTRL:1`, `SET AUTO_DEW:1`, `SET PWM:40` or `GO 1500` was sent; X_USE_PID reported DISABLED/OK behind the heater change); all pass with 3.0.0.18, also in the ASan build.
- **Verification (Linux x64):** `TZ=Europe/Bratislava python3 tools/run_driver_test.py focuser_steeldrive2` 55/55 OK. Regeneration reproduces the checked-in output.

```sh
cd indigo_test && STEELDRIVE2_TEST_FILTER=request_survives ./build/integration/test_focuser_steeldrive2_simulator
```

## Position request copied during the poll's status read (TGT-D17, 3.0.0.19, 2026-09-27)

Found by reading the code while fixing the focuser_qhy part of TGT-B03 (`indigo_drivers/REVIEW_SWITCH_TARGETS.md`, TGT-D17), reproduced on Linux x64 with the simulator.

- Impact: a FOCUSER_POSITION request copied on the bus thread while the 0.5 s focuser poll waited for its `$BS SUMMARY` reply was replaced by the current position. The queued handler then read the current position as its target, sent no `$BS GO` and reported OK, so the GOTO never moved.
- Root cause: 3.0.0.18 checked FOCUSER_POSITION / FOCUSER_STEPS for a pending request (BUSY not published by the poll itself) only before the `steeldrive2_summary()` round trip, and after it wrote `number.target` from the device and published both properties with the poll's state. The same window focuser_qhy 3.0.0.10 closed.
- Fix: the poll checks for a pending request again after the read and then leaves value, target and state to the handler; it no longer writes `number.target` at all (`steeldrive2_publish_focuser()` writes only the value). The `external` handling is unchanged: a BUSY the poll published for a motion the driver did not start is still refreshed and ended by the poll, and no request can be copied while it is BUSY.
- Simulator: new fault action `slow` holds a reply back 0.6 s (within the driver's 1 s first-byte timeout); `SUMMARY slow` delays the poll's status reply.
- Regression test `position_request_survives_poll_read`: with the focuser idle at 1000 the next SUMMARY is delayed, and a GOTO to 1500 is requested as soon as the simulator takes the fault; the first FOCUSER_POSITION result after the request must be OK at 1500 with exactly one `$BS GO 1500`. On 3.0.0.18 it failed 3/3 (first result OK at 1000, no `$BS GO 1500` sent); with 3.0.0.19 it passed 5/5 and in the ASan build.
- Verification (Linux x64): `TZ=Europe/Bratislava python3 tools/run_driver_test.py focuser_steeldrive2` 56/56 OK. Regeneration reproduces the checked-in output. No hardware test was run.

```sh
cd indigo_test && STEELDRIVE2_TEST_FILTER=poll_read ./build/integration/test_focuser_steeldrive2_simulator
```

Final test summary: 56 simulated tests run, 56 passed; 0 hardware tests run, 0 passed.

## Focuser testing rules alignment (3.0.0.20, 2026-10-05)

The suite was checked against the "Focuser Drivers" chapter of
`indigo_test/DRIVER_TESTING_RULES.md` and extended where a rule applies. No hardware test was run.

### Found defects

| # | Defect | Fix | Regression test (fails against 3.0.0.19) |
|---|---|---|---|
| R1 | An aborted move (running or still queued) ended OK. | Both motion properties end ALERT with value and target equal to the stopped position; an abort while idle still sends no STOP. | `focuser_abort_overlap_disconnect`, `rejected_change` |
| R2 | A refused or lost STOP during a running move ended the move ALERT and cancelled its finalizer although the focuser kept moving. | `FOCUSER_ABORT_MOTION` ends ALERT, the move stays BUSY under its finalizer and an immediate retry stops it. | `abort_refused` |
| R3 | After a stall, a persistent status failure or a refused SYNC the driver set `uncertain`, which stopped polling and refused every move and sync until an explicit abort. | The motor is stopped and its position read back; `uncertain` remains only when that stop fails. A failed move stays ALERT through later idle polls, a fresh move is accepted directly, and a refused SYNC keeps the real position and accepts the next SYNC. | `stall_failure`, `sync_failure` |
| R4 | A refused GO left the requested target published, and the controller's error did not reach the client. | The target returns to the position the focuser has and the `ERROR` reply is the message. | `go_failure` |
| R5 | Uncommanded motion (hand controller, external position change) kept the old target. | The poll sets the target to the measured position whenever no request is pending (the pending-request check of TGT-D17 is unchanged and runs after the read). | `external_focuser_state`, `uncommanded_motion` |
| R6 | A missing sensor (TEMP_AVG -128) was published as a temperature. | `FOCUSER_TEMPERATURE` is IDLE and keeps its value. | `focuser_missing_sensor` |
| R7 | Reverse, limits, temperature compensation mode, compensation settings and end-stop selection were accepted during a move; a new limit below the current position was sent to the controller. | `reject_change` refuses them without a command while a move runs; a limit below the position ends ALERT without a command and keeps the old value. | `settings_during_motion` |
| R8 | In temperature compensation mode the controller moves the focuser, yet `FOCUSER_POSITION` stayed writable and relative moves and zeroing were accepted. | `FOCUSER_POSITION` is read-only in automatic mode (defined again when the mode changes); `FOCUSER_STEPS` and `X_START_ZEROING` are refused there. | `automatic_mode` |
| R9 | `X_START_ZEROING` with the item OFF ended ALERT. | It is answered OK without a command. | `automatic_mode` |

The pre-fix results come from a separate binary built against a copy of the 3.0.0.19 sources; the
shared tree was not reverted.

### Simulator additions

- Fault `external_move <position>`: the focuser moves at 500 steps/s without a command.
- A refused or lost `STOP` leaves the motor running, and a refused or lost `SET POS` leaves the
  position unchanged (previously both were applied before the injected reply).

### Rule mapping (additions)

| Rule | Test |
|---|---|
| Missing-sensor sentinel IDLE once, never as a value | `focuser_missing_sensor` |
| Abort ending ALERT at the stopped position, proven by later readbacks; idle abort without STOP; disconnect during a move stops it and reconnect is OK at the stopped position | `focuser_abort_overlap_disconnect` |
| Refused STOP and retry | `abort_refused` |
| Stall: STOP, ALERT kept through idle polls, fresh move without abort | `stall_failure` |
| Idle poll failure ALERT with the last position, recovery, move | `poll_failure` |
| Refused GO keeps the position | `go_failure` |
| Uncommanded motion BUSY then OK with target equal to the measurement, later relative move from it | `uncommanded_motion`, `external_focuser_state` |
| SYNC of the published value, refused SYNC keeps the position, next SYNC accepted | `sync_failure` |
| Settings during a move, limit excluding the position | `settings_during_motion` |
| Automatic mode: position read-only, moves refused, manual mode restores moves; momentary switch OFF answered without a command | `automatic_mode` |

### Rules not applicable

- Speed and backlash: not exposed by the driver.
- Driver-owned temperature compensation: the controller compensates itself (`TCOMP`); the driver
  writes factor, period and threshold and switches the mode.

### Validation

`python3 tools/run_driver_test.py focuser_steeldrive2`: 61/61 passed (3.0.0.20, macOS arm64).

Final test summary: 61 simulated tests run, 61 passed; 0 hardware tests run, 0 passed.
