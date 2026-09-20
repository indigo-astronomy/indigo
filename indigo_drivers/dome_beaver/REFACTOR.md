# dome_beaver refactoring record

## Scope and baseline

This record covers migration of `indigo_dome_beaver` (NexDome dome with the Lunatico Beaver rotator controller) from its hand-written INDIGO 2.0 implementation to `indigo_generator`, the production fixes proven by regression tests, and complete applicable hardware-free simulator coverage.

Baseline date and source: 2026-09-17, commit `2d2e7b19a` (`dome_baader: migrated to code generator`), branch `refactoring`. The working tree contained one unrelated pre-existing modification of `indigo.xcodeproj/project.pbxproj` (removal of a duplicated `aux_cloudwatcher_simulator.c` reference) that is not part of this work and is preserved. Host: macOS 26 (`Darwin 25.6.0`), Apple Silicon arm64; repository universal build (x86_64 + arm64).

Baseline build command:

```sh
cd indigo_drivers/dome_beaver
make -B -f ../../Makefile.drv
```

Result: passed without compiler or linker warnings; object, archive, dynamic library and executable built for x86_64 and arm64.

Baseline automated tests:

```sh
make -C indigo_test build/integration/test_dome_beaver_simulator
cd indigo_test && ./build/integration/test_dome_beaver_simulator
```

Result: 1 run, 1 passed (`beaver_passes_serial_compliance_checks`, 9.7 s). `MIGRATION_STATUS.md` records `1 / 0`.

`build/bin/indigo_generator` is present in the build tree (built from the unchanged `indigo_tools/indigo_generator.c`).

## Hardware-test decision

No hardware testing will be performed. The user has no NexDome/Beaver controller (no physical hardware is available). No statement in this record implies physical validation; all automated evidence is simulator-backed software behaviour. `MIGRATION_STATUS.md` currently records `✅ HW` for retesting of the original driver by its author; that historical status is not evidence produced by this work.

## Studied sources

- `indigo_dome_beaver.c`, `.h`, `_main.c`, `README.md`, `indigo_dome_beaver.vcxproj` in this directory.
- `BeverAPI.txt`: Lunatico protocol API (dome, seletek, domerot, shutter, tmc2660 and wifi groups; `dome status` bit field; return value convention "0 OK, negative error"; `savefs` stops a turning motor). It is the only manufacturer protocol document; the reply framing `!<request>:<result>#` is taken from its examples and the driver.
- `dome_beaver_simulator/dome_beaver_simulator.c` (host PTY simulator), `indigo_test/integration/test_dome_beaver_simulator.c`, `indigo_test/Makefile`, `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (dome section and shared scope).
- `indigo_tools/indigo_generator.c`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_libs/indigo_dome_driver.c` (`DOME_ON_COORDINATES_SET` count 1, `DOME_PARK_POSITION` and `DOME_HOME` hidden by default), `indigo_libs/indigo_uni_io.c` (`indigo_uni_read_section()` has no inter-byte timeout; serial handles use `VMIN 0`/`VTIME 50`, so a started but unterminated reply blocks up to 5 s per read), `indigo_libs/indigo_bus.c` (`indigo_set_switch()` with `false` touches only the given item; `indigo_property_copy_values()` ignores properties of another type), the sibling generated migration `dome_baader` (simulator hooks, forked test cases, reference trace method).

## Current-state audit

### Protocol

ASCII requests `!<group> <command> [arguments]#`, replies `!<request without #>:<result>#`; a result `0` means success and a negative result an error. Commands used by the driver:

| Command | Reply payload | Driver use |
| --- | --- | --- |
| `!seletek version#` | integer `OMFNN` (operation, model digit, firmware major, minor) | connection identification (model 7 = Beaver rotator, 8 = Beaver shutter), INFO model and firmware |
| `!dome shutterisup#` | `0`/`1` | connection: hides shutter properties when no shutter is connected |
| `!dome getaz#` | float azimuth | connection, status poll, relative moves, parked GOTO |
| `!dome gotoaz %f#` | `0` | absolute and relative GOTO |
| `!dome setaz %f#` + `!seletek savefs#` | `0` | sync (only reachable with `DOME_ON_COORDINATES_SET.SYNC`) |
| `!dome status#` | bit field (0 rotating, 1 shutter moving, 2 rotator error, 3 shutter error, 4 shutter comm error, 5 unsafe CW, 6 unsafe Hydreon, 7…12 shutter/home/park) | status poll |
| `!dome atpark#`, `!dome athome#` | `0`/`1` | connection park state, poll |
| `!dome gopark#`, `!dome gohome#` | `0` | park, home |
| `!dome setpark#` + `!seletek savefs#`, `!domerot getpark#` | `0`, float azimuth | `DOME_PARK_POSITION` (current position becomes park), park position readback |
| `!dome openshutter#`, `!dome closeshutter#` | `0` | shutter |
| `!dome shutterstatus#` | `0` open, `1` closed, `2` opening, `3` closing, `4` error | status poll |
| `!dome abort 1#` | `0` | abort all motion |
| `!dome autocalrot 2#`, `!dome autocalshutter#` | `0` | calibrations |
| `!dome sendtoshutter "shutter getcalibrationstatus"#` | `0` none, `1` running, `2` ok, `3` error | shutter calibration completion |
| `!seletek getfailurecode#`, `!seletek getfailuremsg#`, `!seletek clearfailure#` and the same through `!dome sendtoshutter "seletek …"#` | integer, text, `0` | failure reporting and clearing (shutter variants only with a detected shutter) |

### Architecture and implementation

- `DRIVER_VERSION 0x02000003`, label and device `Nexdome Beaver Dome` (`DOME_BEAVER_NAME` in the public header), author Rumen G. Bogdanovski. One dome device, `ADDITIONAL_INSTANCES` supported, no hot plug. Transport: serial at a fixed 115200 baud (hidden `DEVICE_BAUDRATE`) or a `nexdome://host[:port]` URL opened as TCP (default port 8080), both through `indigo_uni_io`. A Windows project exists.
- `beaver_command()`: `indigo_uni_discard()`, write, `indigo_uni_read_section()` up to `#` with a 3 s first-byte timeout and no inter-byte timeout, then a 5 ms pause. On any failure over TCP it queues `network_disconnection()`, which disconnects and publishes `CONNECTION` ALERT "Device disconnected unexpectedly".
- Result helpers build a `sscanf` format from the request (`…:%d#`, `…:%f#`, `…:%[^#]`); the trailing `#` is not verified and trailing garbage is accepted. Several helpers store the parsed value directly into driver state before validating it.
- Connection (device queue): `beaver_open()` (reference count, global lock, open, `!seletek version#`, rejects shutter and other boards with a CONNECTION message, INFO model/firmware), `!dome shutterisup#` (no shutter → message "Shutter not detected", `DOME_SHUTTER` and `X_SHUTTER_CALIBRATE` hidden), definition of the five custom properties, `!dome getaz#`, `!dome atpark#` (DOME_PARK PARKED/UNPARKED), `!domerot getpark#` (`DOME_PARK_POSITION`), first status poll after 0.5 s.
- Status poll `dome_timer_callback` (device queue, 1 s after the end of each run): `!dome status#`, `!dome getaz#`; X_CONDITIONS_SAFETY when bits 5/6 differ from the previous status; rotation block (when HORIZONTAL or PARK is BUSY or the status changed): HORIZONTAL BUSY while bit 0 is set, otherwise HORIZONTAL and STEPS OK; `!dome atpark#` (park request completes when at park, then `!domerot getpark#`), `!dome athome#` (home request OK at home, ALERT "Failed to find home." when stopped elsewhere, otherwise `DOME_HOME` item follows at-home); rotator calibration completes when bit 0 is clear (ALERT with bit 2); `!dome shutterstatus#` published on change; shutter calibration status while calibrating; abort block; clear failures when requested; failure codes (and messages when codes change) every poll.
- Property changes: CONNECTION, `DOME_STEPS`, `DOME_HORIZONTAL_COORDINATES`, `DOME_SHUTTER`, `DOME_PARK`, `DOME_PARK_POSITION`, `DOME_HOME`, `X_ROTATOR_CALIBRATE`, `X_SHUTTER_CALIBRATE` are queued handlers; `DOME_ABORT_MOTION` is an urgent handler; `X_CLEAR_FAILURES` is handled synchronously (BUSY, performed by the poll). Every motion/calibration handler ends with `indigo_sleep(0.5)`, which blocks the device queue and delays the next status poll until 0.5 s after the command.
- Disconnect: cancel handlers, delete custom properties, close (reference count, global unlock), INFO model/firmware reset. The dome is not stopped. Detach disconnects and calls `indigo_global_unlock()` a second time.

### Public properties and behaviour

- `DEVICE_PORT`, `DEVICE_PORTS` visible; `DEVICE_BAUDRATE` hidden (115200); INFO count 6 (model and firmware shown); `DOME_SPEED` hidden; `DOME_ON_COORDINATES_SET`, `DOME_SLAVING_PARAMETERS`, `DOME_HOME`, `DOME_PARK_POSITION` visible; `DOME_HORIZONTAL_COORDINATES` RW; `DOME_STEPS` label "Relative move (°)"; `DOME_FLAP` hidden.
- Custom properties (group `Misc`, defined only while connected; all names already have the `X_` prefix): `X_SHUTTER_CALIBRATE` (`CALIBRATE`), `X_ROTATOR_CALIBRATE` (`CALIBRATE`), `X_FAILURE_MESSAGES` (RO text `ROTATOR`, `SHUTTER`), `X_CLEAR_FAILURES` (`CLEAR`), `X_CONDITIONS_SAFETY` (lights `CLOUD_WATCHER`, `HYDREON`, initially IDLE).
- GOTO and steps are rejected with "Dome is moving: request can not be completed" while `DOME_HORIZONTAL_COORDINATES` (or `DOME_STEPS`) is BUSY; shutter requests with "Shutter is moving: request can not be completed" while BUSY. Parked dome: GOTO, steps, home and rotator calibration end ALERT "Dome is parked, please unpark". Unpark sends no command. Abort: `!dome abort 1#`, PARK ALERT when it was BUSY, ABORT OK, SHUTTER OK; the next poll publishes HORIZONTAL and STEPS OK.
- `DOME_PARK_POSITION` change: the requested value is not copied (the handler copies into `DOME_SHUTTER`, a no-op for a number property) and `!dome setpark#` makes the current azimuth the park position; the published value is read back from the controller.

### Supported platforms, build and integration

- README: platform independent; `indigo_dome_beaver.vcxproj` and solution entry exist; listed in root `UNTESTED_DRIVERS`. Built by `Makefile.drv` auto-discovery. Xcode `dome_beaver` group (sources, simulator) and test group. No `REFACTOR.md`, no `.driver`, no generated outputs.
- `indigo_docs/PROPERTIES.md` documents the five custom properties and names the `.c` source.

### Existing simulator and tests (gaps)

`dome_beaver_simulator.c` moves instantly (rotation reported moving for one reply, shutter for two), has no elapsed motion, no park/home geometry beyond exact equality, no calibration behaviour, no failures, no unsafe conditions, no shutter-less or non-rotator board, fixed firmware, no abort semantics beyond clearing counters, `sscanf` into the global azimuth (a malformed command corrupts it), no fault injection, external control, event log or TCP listener. The single test is a smoke/compliance pass without timing, failure, reconnect, lifecycle, multi-instance or concurrency coverage.

### Defects and risks found by source audit

Identifiers are used in the found-defects section; each is either reproduced by a dedicated reproducer against the original driver or explicitly marked audit-only.

- **BVR-01** `X_CONDITIONS_SAFETY` is never published when both unsafe bits are set in the first status after connection: the previous status is initialized to `-1` (all bits set), so no change is detected and the lights stay IDLE.
- **BVR-02** A shutter request that does not change the reported shutter status (open while open, close while closed) leaves `DOME_SHUTTER` BUSY forever; every later shutter request is rejected as "Shutter is moving".
- **BVR-03** A failed shutter command sends its failure message as an update of `DOME_STEPS` and leaves the rejected switch selected indefinitely.
- **BVR-04** A failed park command (`!dome gopark#` error) is only logged: `DOME_PARK` stays BUSY forever.
- **BVR-05** An aborted park request is not cleared: a later move that ends at the park position marks the dome PARKED.
- **BVR-06** `X_CLEAR_FAILURES` always ends ALERT without a detected shutter: the shutter result variable keeps its `-1` initializer.
- **BVR-07** Error replies are stored before validation: a negative `!dome getaz#` result becomes the published azimuth (−1) and the base of relative moves.
- **BVR-08** Relative moves compute the target by truncating a floating-point sum (`(int)(10 × (0.7f + 10))` = 106): the dome is sent 0.1° short for many start/step combinations.
- **BVR-09** A rotation stopped by a rotator error (status bit 2) ends `DOME_HORIZONTAL_COORDINATES` and `DOME_STEPS` OK, and a park stopped that way stays BUSY forever.
- **BVR-10** Aborting a rotator calibration publishes "Rotator calibration complete" with OK.
- **BVR-11** `DOME_ON_COORDINATES_SET` is shown with its base-class count of 1, so the SYNC item is not published and the implemented sync (`!dome setaz`) is unreachable.
- **BVR-12** Requests queued behind a running handler are not BUSY-guarded: a second request overwrites the values of a queued first request, which is silently lost (shutter open followed by close sends `closeshutter` twice).
- **BVR-13** Handlers block the device queue for 0.5 s after each command, so an urgent abort (and disconnect) is delayed until the sleep ends.
- **BVR-14** A started but unterminated reply blocks the serial read for the 5 s `VTIME` instead of the intended 3 s timeout (no inter-byte timeout).
- **BVR-15** (found in step 3) A `DOME_HOME` request that arrives while the status poll refreshes the at-home switch is overwritten by the poll (`DOME_HOME_ITEM` reset to the at-home state); the handler then sees no request, sends nothing and publishes OK.
- Audit-only risks: the shutter calibration poll uses an uninitialized `cal_status` when the status request fails (undefined behaviour); failure messages are read directly into the published items even when the request fails; result parsers accept trailing garbage and do not verify the terminating `#`; `indigo_global_unlock()` is called twice on detach; the `DOME_PARK_POSITION` change handler copies the request into `DOME_SHUTTER`; the park position request value is ignored (controller semantics of `dome setpark`).

## Atomic plan

1. **Done — record.** This file: audit, baseline, hardware decision, plan (written before any production change).
2. **Done — simulator audit and rewrite.** `dome_beaver_simulator.c` rewritten from `BeverAPI.txt`: `serial_motion.h` elapsed-time rotation (shortest path, a half turn clockwise, `--rotation-speed`, default 5 °/s), shutter travel (`--shutter-time`, default 20 s), `dome status` bits 0…12, park and home positions (`--park`, `--home`; `atpark` needs a stopped rotator within 0.5°, `athome` is a 1° sensor), `setaz`, `setpark`, `domerot setpark/getpark/sethome/gethome/getpos/setpos/goto/athome/atpark/getcalibrationstatus`, `savefs` stopping a turning rotator (documented), `abort` with the three flags (`abort 1` and `abort 0 0 0` abort all), rotator calibration (one revolution ending at home, `--calibration-time`) and shutter calibration (`--shutter-calibration-time`) with status, failure codes/messages and clearing for rotator and shutter (directly and through `sendtoshutter`), unsafe CW/Hydreon bits (`--unsafe`), shutter detection (`--no-shutter`), board/firmware (`--version`), battery voltage, `seletek echo`, request framing (`!`…`#`, a new `!` discards an unterminated request, 0.5 s stale reset). Test hooks: event log `INDIGO_BEAVER_EVENTS` (`RX`/`TX`/`MOVE`/`STATE`/`FAULT`/`CONTROL`/`REJECT`), fault injection `INDIGO_BEAVER_FAULT` (`silent`, `error` → `-1` without executing, `reply <result>`, `raw <reply>`, `partial` → reply without `#`, `slow <ms>`, `close`, with count and request prefix or `ANY`), external control `INDIGO_BEAVER_CONTROL` (`azimuth`, `rotate`, `shutter`, `unsafe`, `rotfault`, `shutfault`, `shutterup`, `calfail`, `park`, `home`, `version`, `drop`) and an optional loopback TCP listener (`--tcp-port`, `INDIGO_SIMULATOR_TCP_URL`). Simulator assumptions (undocumented) are listed in the source header. Verification: `clang -std=gnu11 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -fsyntax-only` clean; direct PTY self-check `simulator_protocol` 1 run, 1 passed (not counted as driver coverage).
3. **Done — characterization suite against the original driver.** `indigo_test/integration/test_dome_beaver_simulator.c` rewritten on the `dome_baader` harness: every case runs in a forked child with its own simulator(s), temporary `HOME` and alarm; the in-process client observes define/update/delete and `send_message` under a mutex with fresh revisions and BUSY counts; the simulator event log (with timestamps) provides ordered protocol assertions and status-poll synchronization (a poll starts with `!dome status#` and ends with the failure-code/message queries). The baseline binary `test_dome_beaver_simulator_original` is compiled (arm64) from the same test source with `-DEXPECTED_VERSION=0x02000003 -DBEAVER_REFERENCE_TRACE_PATH="fixtures/dome_beaver/original_reference_trace.txt"`, the original header and the original `indigo_dome_beaver.c` saved from commit `2d2e7b19a`. First complete run: 31 run, 26 passed (5 min 57 s). Failures: three test mistakes (the shutter-less dome started parked at 0°; the park move from 180° to 0° turns clockwise, not counterclockwise; `go_home` requested home immediately after a GOTO completed and hit a real race, see BVR-15), `reference_trace` (no fixture yet) and `urgent_abort_cancels_queued_goto` (new queued-handler behaviour: the original has no BUSY state at request time, so a GOTO queued behind a shutter command is still sent after the abort; expected to fail against the original). After fixing the three tests they passed 3 of 3 against the original.
4. **Done — defect reproducers.** `--known-defects` runs 15 dedicated cases (BVR-01…BVR-15; BVR-15 was found in step 3). Result against the original driver: 15 run, 15 failed as expected, each at its defect assertion (evidence below).
5. **Done — original reference trace.** `indigo_test/fixtures/dome_beaver/original_reference_trace.txt` (293 lines): per step the ordered simulator exchange (`S RX`/`S TX`/`S FAULT`; complete status polls collapsed into one `S POLL` line per run of identical polls with the `getaz` digits masked) followed by property definitions/updates/deletions and messages (`D`/`U`/`X`/`M`; BUSY azimuth values masked, repeated identical BUSY updates dropped; the abort-in-motion step masks all digits and keeps first occurrences). Steps: connect, GOTO 180, steps clockwise/counterclockwise, direction, park, GOTO and steps while parked, unpark, home, GOTO 200, abort in motion, GOTO after abort, idle abort, failed abort, park position, shutter open/close, rotator and shutter calibration, unsafe Cloud Watcher, rotator failure, clear failures, disconnect. 3 of 3 captures identical; `reference_trace` against the fixture 1 run, 1 passed.
6. **Done — `.driver` and regeneration.** Added `indigo_dome_beaver.driver` (version 4, `DRIVER_VERSION 0x03000004`), `serial;`, transactional `beaver_open`/`beaver_close` owning the port (serial at the hidden 115200 `DEVICE_BAUDRATE` or `nexdome://` TCP with default port 8080), the global lock and the identification; the INFO reset on close is kept. Transport helper `beaver_vcommand()` keeps the original manner (discard, write, 3 s first-byte timeout, 5 ms pause after a reply, `network_disconnection` on TCP transport failure, now queued once per session) and adds an inter-byte timeout of 0.5 s (`indigo_uni_read_section2()`) and full reply validation (echoed request, `:`, terminating `#`, complete integer/float payload). Property handlers are queued generated handlers; the status poll `dome_status_poll` runs on the device queue with the original command order. The 0.5 s sleeps after device commands are replaced by a non-blocking settle window: a handler records `poll_hold_until` and a poll due earlier is rescheduled to that time, so polls still never run earlier than 0.5 s after a command while abort and disconnect are no longer blocked. Fixes for BVR-01…BVR-15 (table below). Generated with `build/bin/indigo_generator indigo_dome_beaver.driver` (no generator warnings); `make -B -f ../../Makefile.drv` passes for x86_64 + arm64 with zero warnings; strict check `clang -arch arm64 -std=gnu11 -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wshadow -Wformat=2 -Wno-format-nonliteral -Wunreachable-code -fsyntax-only` clean. No generator change and no `MAX_DEVICES` override. The generated public header no longer defines `DOME_BEAVER_NAME`; no repository code used it (the test uses its own names).
7. **Done — post-migration suite and trace comparison.** First run against the generated driver (the generated fixture was still a copy of the original trace): 31 run, 30 passed, the only failure `reference_trace`; `--known-defects` 15 run, 14 passed. The remaining reproducer BVR-15 still failed: the poll decided at its start whether a home request was queued, and the request arrived during the slow at-home query. Fix: queued-request checks for rotation, home and shutter are evaluated at the point of use (`beaver_rotation_queued()`, `DOME_HOME` state at the refresh, shutter after the status reply); after regeneration BVR-15 passed. Generated trace captured 3 times: identical; every `S` (protocol) line equals the original trace (analysis below). `generated_reference_trace.txt` (304 lines) is the checked-in contract of the suite.
8. **Done — defect fixes promoted and generated-behaviour coverage.** All 15 reproducers pass and were promoted into the ordinary suite (`--known-defects` selects no cases). Generated-behaviour cases: `urgent_abort_cancels_queued_goto` (a GOTO queued behind a shutter command with a 1.5 s reply is cancelled by the urgent abort: no `gotoaz`), `queued_request_survives_status_poll` (a shutter close queued behind a poll delayed by 1.2 s is neither completed early nor lost) and `busy_guard_holds_while_command_runs` (GOTO, steps and shutter requests during a 0.8 s command reply are ignored). Complete suite: 46 run, 46 passed (7 min 32 s). ASan + UBSan (`make -C indigo_test test-dome-beaver-simulator-sanitize`, arm64, driver source compiled into the test): 46 run, 46 passed, no sanitizer report. Original driver built from the saved source with the final harness: 46 run, 30 passed, 16 failed — exactly the 15 reproducers and `urgent_abort_cancels_queued_goto`; all preservation cases including `reference_trace`, `queued_request_survives_status_poll` and `busy_guard_holds_while_command_runs` passed. Mutation checks on scratch copies of the generated source: without the queued-home guard BVR-15 failed; without completion of a request for the current shutter state BVR-02 failed.
9. **Done — repository integration.** `indigo.xcodeproj/project.pbxproj`: `REFACTOR.md` and `indigo_dome_beaver.driver` in the `dome_beaver` group, new `fixtures/dome_beaver` group with both traces (`plutil -lint` passes; the pre-existing unrelated edit is preserved). `indigo_dome_beaver.vcxproj`: `.driver` and `REFACTOR.md` as `None` items (`xmllint` passes; no Windows build was possible). `indigo_test/Makefile`: simulator depends on `serial_motion.h`, test depends on both fixtures and the driver archive, targets `test-dome-beaver-simulator`, `test-dome-beaver-simulator-sanitize` and opt-in `test-dome-beaver-simulator-network`. `indigo_docs/PROPERTIES.md`: source now names the `.driver` (property set unchanged). `MIGRATION_STATUS.md`: generator `✅ Yes`, retested `✅ Sim`, tests `49 / 0` (46 default cases including the simulator self-check and 3 opt-in network cases); API, Windows and async columns unchanged, Comment column unchanged. `README.md` unchanged.
10. **Done — final verification.** Generator run twice more after the final `.driver` edit: SHA-1 unchanged (`.c` `fdc250d9d17e911b6fac33f51506e3ccee41b7e3`, `.h` `f51a4bca99872ec2a0909e9190f9701004e64955`, `_main.c` `ea26ee17103d57d5b5e7bef01d64043545419c9a`). Opt-in TCP transport (`--network`: `nexdome://` URL, default port 8080, refused connection, silent identification, connection dropped by the controller → CONNECTION ALERT "Device disconnected unexpectedly", no further I/O, reconnect): generated 3/3, generated under ASan/UBSan 3/3, original 3/3. `git diff --check` clean; formatting audit of the `.driver`, simulator and test (tabs, no trailing whitespace, no blank lines in function bodies). Linux and Windows builds and physical hardware were unavailable and are not claimed.

## Original-driver baseline evidence

Environment: macOS 26 arm64, baseline binary described in step 3.

- Characterization suite: 31 run, 26 passed on the first complete run (analysis in step 3); the three corrected cases 3 run, 3 passed; `reference_trace` 1 run, 1 passed after the fixture was recorded; `urgent_abort_cancels_queued_goto` fails as expected (a queued GOTO is sent after the abort).
- `--known-defects`: 15 run, 15 failed as expected:
  - BVR-01: `X_CONDITIONS_SAFETY` stayed IDLE (state 0) with both unsafe bits set at connection.
  - BVR-02: the open request on an open shutter did not settle (BUSY) within 5 s.
  - BVR-03: after a failed `openshutter` the OPENED switch remained selected.
  - BVR-04: `DOME_PARK` did not settle in ALERT after a failed `gopark` (stayed BUSY).
  - BVR-05: after an aborted park and a GOTO to the park position the dome was reported PARKED.
  - BVR-06: `X_CLEAR_FAILURES` settled in ALERT without a shutter.
  - BVR-07: the azimuth −1 (error reply to `getaz`) was published during a GOTO.
  - BVR-08: a 10° clockwise move from 0.7° did not send `gotoaz 10.700000` (it sent `10.600000`).
  - BVR-09: a GOTO stopped by a rotator failure settled OK instead of ALERT.
  - BVR-10: an aborted rotator calibration settled OK ("Rotator calibration complete").
  - BVR-11: `DOME_ON_COORDINATES_SET` count 1 instead of 2.
  - BVR-12: the shutter open request queued behind a slow GOTO was lost (no `openshutter`).
  - BVR-13: the abort reached the controller 0.517 s after the GOTO reply.
  - BVR-14: the next request followed an unterminated reply after 5.006 s.
  - BVR-15: a home request sent during the at-home query sent no `gohome`.

## Reference trace comparison

`original_reference_trace.txt` (293 lines) and `generated_reference_trace.txt` (304 lines): every `S` (protocol) line is identical — connection sequence, poll content and order in every step (including conditional `atpark`/`athome`/`getpark`, shutter calibration status and failure messages), `gotoaz` arguments, `getaz` before relative moves and parked GOTO, `gopark`, `gohome`, `abort 1`, `setpark`/`savefs`/`getpark`, shutter, calibration and clear-failure commands. All differences are property publications:

1. Queued changes publish BUSY at request time (`INDIGO_COPY_VALUES_PROCESS_CHANGE` / `…_URGENT_CHANGE`): additional BUSY lines for `DOME_PARK`, `DOME_HOME`, `DOME_PARK_POSITION`, `DOME_ABORT_MOTION`, `DOME_HORIZONTAL_COORDINATES` (GOTO while parked) and `DOME_STEPS` (steps while parked); some first BUSY lines of `DOME_STEPS`/`DOME_HORIZONTAL_COORDINATES` change order because the requested property is published first.
2. Connection: the custom properties are defined by generated code after the connection sequence (after the `DOME_PARK` and `DOME_PARK_POSITION` updates); generated messages "Connected to Nexdome Beaver Dome on PORT" and "Disconnected from Nexdome Beaver Dome" are added.
3. `DOME_ON_COORDINATES_SET` is defined with count 2 (BVR-11).
4. A successful abort publishes `DOME_SHUTTER` OK before `DOME_ABORT_MOTION` OK (the generated final update publishes the abort property last).

Not visible in the normalized trace but intentionally changed: a failed connection reports the identification message with `indigo_send_message()` for `CONNECTION` plus the generated "Failed to connect to …" message instead of a CONNECTION update message; calibration start messages are sent with `indigo_send_message()` for the property.

## Intentional behaviour differences

- **Serialization and timing:** handlers no longer sleep 0.5 s; the status poll still never runs earlier than 0.5 s after a motion, shutter or calibration command (non-blocking settle window), so the protocol timing of polls is preserved while an urgent abort reaches the controller immediately (BVR-13) and requests queued behind a command are no longer delayed by 0.5 s.
- **Reply validation (BVR-07, BVR-14):** replies must echo the request, contain `:` and end with `#`; integer and float payloads must be complete; `getaz`/`getpark` values outside 0…360, negative `status`, `shutterstatus` outside 0…4 and negative `atpark`/`athome`/calibration values are treated as failures and ignored. A started reply times out after 0.5 s without a further byte. The reply buffer is `INDIGO_VALUE_SIZE` for every command (the original used 100 bytes except for failure messages).
- **Busy requests:** handlers keep their property BUSY until they publish the result; GOTO and shutter requests while BUSY are ignored by the framework guard without the original "… request can not be completed" message (the steps rejection message while `DOME_HORIZONTAL_COORDINATES` is BUSY is kept); requests are no longer lost when queued (BVR-12) and the poll does not overwrite queued requests (BVR-15).
- **Failure recovery:** a failed park (`gopark`) publishes `DOME_PARK` ALERT "Goto park failed" and returns `DOME_STEPS`/`DOME_HORIZONTAL_COORDINATES` to OK (BVR-04); a failed `gohome` publishes `DOME_HOME` ALERT "Failed to find home." immediately (the original reached the same state at the next poll); a failed shutter command restores the switches from the last polled status and reports on `DOME_SHUTTER` (BVR-03); a failed `DOME_PARK_POSITION` change restores the last park position value (the original never copied the request).
- **Completion:** a shutter request for the state the shutter is already in completes OK (BVR-02); a rotation, park or home stopped by a newly raised rotator failure bit ends ALERT "Rotation stopped by rotator failure" (BVR-09; a failure bit already set at the start does not trigger it); an aborted calibration ends ALERT "Rotator/Shutter calibration aborted" (BVR-10); an abort clears a pending park request (BVR-05) and cancels queued motion, shutter and calibration handlers, settling the properties they left BUSY.
- **Other fixes:** `X_CONDITIONS_SAFETY` is published after the first successful status read (BVR-01); `X_CLEAR_FAILURES` without a shutter only needs the rotator result (BVR-06); relative moves use integer tenths (BVR-08); `DOME_ON_COORDINATES_SET` shows SYNC (BVR-11); failure messages are copied only when read successfully; the shutter calibration status is never read uninitialized; the redundant second global unlock on detach is gone.
- **Version:** `0x03000004` (original `0x02000003`).
- **Unchanged by decision:** message texts including their typos ("falied", "prperty", "controler") and property labels; the shutter switch shows OPENED while the shutter is closing, opening or in error; the park position request value is not used (`dome setpark` makes the current azimuth the park position); the dome is not stopped on disconnect.

## Found defects

Every defect below was reproduced by a dedicated case that failed against the original driver (evidence in the baseline section) and passes against the generated driver.

| ID | Observable impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- |
| BVR-01 | Unsafe weather present at connection is never reported (`X_CONDITIONS_SAFETY` IDLE). | Previous status initialized to −1 (all bits set), change detection only. | Publish after the first successful status read. | `BVR-01 unsafe_at_connection_reported` |
| BVR-02 | Open request on an open shutter (close on closed) stays BUSY forever; later shutter requests are rejected. | Shutter published only on status change. | An active request completes when the status equals its target. | `BVR-02 repeated_shutter_request_completes` |
| BVR-03 | Failed shutter command leaves the rejected switch selected; message sent on `DOME_STEPS`. | Values copied before the command, wrong property in the failure update. | Switches restored from the polled status; message on `DOME_SHUTTER`. | `BVR-03 failed_shutter_request_restores_switch` |
| BVR-04 | Failed park command leaves `DOME_PARK` BUSY forever. | `gopark` failure only logged. | `DOME_PARK` ALERT, rotation not started. | `BVR-04 failed_park_alerts` |
| BVR-05 | After an aborted park a later move to the park position marks the dome PARKED. | Abort did not clear the park request. | Abort clears the park request. | `BVR-05 aborted_park_is_cleared` |
| BVR-06 | `X_CLEAR_FAILURES` always ALERT without a shutter. | Shutter result kept its −1 initializer. | Shutter result defaults to 0. | `BVR-06 clear_failures_without_shutter` |
| BVR-07 | Error replies published as values (azimuth −1, park position −1). | Parsed values stored before validation; `getpark` never validated. | Range validation before storing. | `BVR-07 error_replies_not_published` |
| BVR-08 | Relative moves 0.1° short (0.7° + 10° → `gotoaz 10.600000`). | Truncating floating-point conversion. | Integer tenths arithmetic. | `BVR-08 relative_move_is_exact` |
| BVR-09 | Rotation stopped by a rotator failure ends OK; park stopped that way stays BUSY. | Error bit used only for calibration. | Newly raised error bit ends rotation and park in ALERT. | `BVR-09 rotator_error_alerts_motion` |
| BVR-10 | Aborted calibration reported "calibration complete" OK. | Completion decided only by stopped rotation / calibration status 0. | Abort ends active calibrations in ALERT. | `BVR-10 aborted_calibration_not_complete` |
| BVR-11 | Sync (`dome setaz`) unreachable; `DOME_ON_COORDINATES_SET` shows only GOTO. | Base-class count 1 kept. | Count 2. | `BVR-11 sync_is_reachable` |
| BVR-12 | A request queued behind a running handler is silently lost (shutter open then close sends `closeshutter` twice). | No BUSY state at request time. | Generated BUSY guard at request; handlers keep BUSY. | `BVR-12 queued_request_not_lost` |
| BVR-13 | Abort delayed by 0.5 s after a command. | `indigo_sleep(0.5)` in handlers on the device queue. | Non-blocking settle window. | `BVR-13 abort_not_delayed` |
| BVR-14 | An unterminated reply stalls the driver for 5 s. | No inter-byte timeout (`VTIME` 5 s). | `indigo_uni_read_section2()` with 0.5 s inter-byte timeout. | `BVR-14 partial_reply_is_bounded` |
| BVR-15 | A home request arriving during the status poll is ignored (no `gohome`, OK). | The poll's at-home refresh overwrote the requested switch. | No refresh while `DOME_HOME` is BUSY; queued checks at the point of use. | `BVR-15 home_request_survives_status_poll` |

Migration regression found and fixed (not an original defect): the first generated poll evaluated queued requests at its start, so a home request arriving during the poll was still overwritten (step 7).

Audit-only risks resolved without a dedicated reproducer: uninitialized shutter calibration status on a failed read; failure messages written into published items on failed reads; parsers accepting trailing garbage and missing `#`; double global unlock on detach; the no-op copy of `DOME_PARK_POSITION` requests into `DOME_SHUTTER`.

Remaining known limitations (not changed): no stall or timeout detection — a rotation, park or shutter move that stops away from its target without a newly raised error bit stays BUSY until aborted, and completion right after a command relies on the controller reporting motion within the 0.5 s settle window (as in the original); a failure bit already set when an operation starts cannot be distinguished from a new failure; the park position cannot be set to a requested value; the real controller's reply formats beyond `BeverAPI.txt` examples, azimuth resolution, shutter-less status replies, abort/calibration semantics and network-attached controllers are simulator assumptions (TCP covered only against the loopback simulator); the dome is not stopped on disconnect.

## Scenario-to-test mapping

Dome class checklist (`indigo_test/DRIVER_TESTING_RULES.md`) and shared scope:

| Area | Cases |
| --- | --- |
| Simulator protocol self-check (not driver coverage) | `simulator_protocol` |
| Metadata, INFO, interface bit, common visible properties, hidden baud rate, no dome properties and no I/O before connection | `metadata_before_connection` |
| Connection sequence and first poll, INFO model/firmware and count, visible/hidden dome properties, azimuth/steps ranges, RW coordinates, park state and position, home state, shutter state, custom property shapes | `connect_parked_at_home`, `connect_unparked_open_shutter`, `reference_trace` |
| Open/identification failures (missing port, silent, shutter board, other board, malformed), descriptor balance, recovery | `connection_failures_and_recovery` |
| Shutter not detected: hidden shutter properties, no shutter queries, reconnect with shutter | `no_shutter_connection`, BVR-06 |
| Absolute GOTO BUSY→OK with elapsed motion, shortest path across 0°, fractional target, repeated GOTO | `goto_and_wrap` |
| Relative moves both directions, wrap, fractional steps, `getaz` before the move, tenth arithmetic | `relative_steps`, BVR-08 |
| Sync | BVR-11 |
| Parked dome refuses GOTO, steps, home and rotator calibration | `moves_refused_when_parked` |
| Park/unpark, repeated park, park at park position | `park_and_unpark` |
| Park position (setpark/savefs/getpark, failures) | `park_position_set`, BVR-07 |
| Home (success, repeated, at home, not reached) | `go_home`, BVR-15 |
| Abort while rotating, idle, during park, during shutter motion, during calibrations | `abort_rotation`, `abort_park`, `abort_shutter`, BVR-05, BVR-10 |
| Shutter open/close BUSY→OK with messages and switch semantics | `shutter_open_close`, BVR-02 |
| Shutter and rotator failures, failure messages, clear failures (success and failure) | `shutter_fault_and_clear`, `rotator_failure_and_clear`, BVR-09 |
| Rotator and shutter calibration (success, failure, command failure, status polling) | `rotator_calibration`, `shutter_calibration` |
| Observing conditions safety | `conditions_safety`, BVR-01 |
| Command failures and messages (GOTO, steps, abort, home, shutter, park) | `command_failure_messages`, BVR-03, BVR-04 |
| Poll read failures (malformed, silent, error, unterminated replies) without false publications, recovery | `poll_read_failures`, BVR-07, BVR-14 |
| BUSY conflicts, including requests during a running command | `busy_requests_rejected`, `busy_guard_holds_while_command_runs` |
| Queued-handler races: urgent abort versus queued GOTO, poll versus queued requests, lost queued requests, abort latency | `urgent_abort_cancels_queued_goto`, `queued_request_survives_status_poll`, BVR-12, BVR-13, BVR-15 |
| Disconnect during motion, no I/O after close, reconnect | `disconnect_during_motion` |
| INIT/SHUTDOWN idempotence, shutdown refused while connected, redundant disconnect, repeated connect cycles | `lifecycle_and_shutdown` |
| Additional instance with its own port and state | `additional_instance` |
| Ordered protocol/property compatibility contract | `reference_trace` |
| Network transport (opt-in `--network`): `nexdome://` URL, default port 8080, refused connection, silent device, dropped connection and recovery | `network_nexdome_url`, `network_default_port`, `network_failures_and_transport_loss` |

Not applicable or not covered: hot plug and multiple physical devices (serial/TCP, no enumeration); guider timing (no guider interface, so no guiding-pulse measurement applies); driver-owned persistent settings (none; slaving parameters, dimension and geographic coordinates belong to the dome base class); `DOME_SPEED`, `DOME_FLAP`, UTC properties (hidden or not implemented); `DEVICE_BAUDRATE` changes (hidden); transport loss during active work on real hardware; hardware acceptance (no device).

## Final test summary

Counts include development runs; a registered case run inside a complete suite run counts once per run. Trace captures (`--capture-trace`) count as runs of `reference_trace`.

- Simulated tests, original driver: 107 run, 68 passed. Breakdown: pre-existing smoke test 1/1; simulator self-check 1/1; first complete characterization run 31 run, 26 passed; two `go_home` debug runs 0/2; the three corrected cases 3/3; defect reproducers 15 run, 0 passed (all failed as expected); trace captures 3/3; `reference_trace` against the fixture 1/1; `urgent_abort_cancels_queued_goto` 1 run, 0 passed (expected); final complete run with promoted reproducers 46 run, 30 passed (16 expected failures); network cases 3/3.
- Simulated tests, generated driver: 149 run, 146 passed. Breakdown: first complete run 31 run, 30 passed (trace compared with the original fixture copy); reproducers 15 run, 14 passed; BVR-15 debug run 0/1 and after the fix 1/1; trace captures 3/3; final complete run 46/46; ASan/UBSan complete run 46/46; network cases 3/3 and under ASan/UBSan 3/3.
- Mutation checks (not counted above): 2 runs against deliberately broken scratch copies of the generated driver; both failed as intended.
- Hardware tests: 0 run, 0 passed.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard are now declared with the generator's `reject_change` block. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values instead of an `INDIGO_OK_STATE` update carrying no items, which left the refused value visible in the client.

Covered by the `rejected_change` scenario in `indigo_test/integration/test_dome_beaver_simulator.c`: while `DOME_HORIZONTAL_COORDINATES` is BUSY, `DOME_STEPS` ends in ALERT with unchanged value and target, and is accepted again after the abort.

```sh
cd indigo_test && BEAVER_TEST_FILTER=rejected_change ./build/integration/test_dome_beaver_simulator
```

## Rejected DOME_STEPS change was invisible (2026-09-20)

`rejected_change_alerts_and_keeps_values` failed with "DOME_STEPS did not settle in ALERT". The
`reject_change` branch did run — the client received the "Dome is moving: request can not be
completed" message — but the property was published as BUSY, so the rejection never became
observable and the status poll kept republishing BUSY for the rest of the rotation.

Two threads wrote `DOME_STEPS_PROPERTY->state` without any ordering. The generated `reject_change`
branch runs on the bus thread inside `change_property`, while `DOME_HORIZONTAL_COORDINATES.on_change`
runs on the device queue. `INDIGO_COPY_VALUES_PROCESS_CHANGE` publishes BUSY synchronously and only
then queues the handler, so `wait_state(DOME_HORIZONTAL_COORDINATES, BUSY)` in the test succeeds
before the handler has run at all; the test's `DOME_STEPS` request then lands exactly while the
handler is starting, and the handler's `state = INDIGO_BUSY_STATE` overwrote the ALERT the rejection
had just assigned.

The claim on `DOME_STEPS` moved from the queued handler into `on_change_request`, which runs on the
bus thread like the rejection branch, so the two are now strictly ordered. Rotation still owns
`DOME_STEPS` while it runs. The remaining writes to that state on the rotation path only ever set
ALERT, which is what the rejection publishes anyway, so they cannot reintroduce the defect.

`fixtures/dome_beaver/generated_reference_trace.txt` was regenerated: `U DOME_STEPS BUSY` now
precedes `U DOME_HORIZONTAL_COORDINATES BUSY` in four places. No serial command changed.

47/47 simulator scenarios pass.
