# dome_nexdome3 refactoring record

## Scope and baseline

This record covers migration of `indigo_dome_nexdome3` (NexDome dome with the official firmware 3.x by Tim Long) from its hand-written INDIGO 2.0 implementation to `indigo_generator`, the production fixes proven by regression tests, and complete applicable hardware-free simulator coverage.

The user asked for `dome_nexstar3`; no such driver exists. The request was interpreted as `dome_nexdome3`, the next dome driver after the just completed `dome_nexdome` migration.

Baseline date and source: 2026-09-17, commit `32dfeaf5f` (`dome_nexdome: migrated to code generator`), branch `refactoring`, clean working tree. Host: macOS 26 (`Darwin 25.6.0`), Apple Silicon arm64; repository universal build (x86_64 + arm64).

Baseline build command:

```sh
cd indigo_drivers/dome_nexdome3
make -B -f ../../Makefile.drv
```

Result: passed without compiler or linker warnings (0 lines containing `warning`); object, archive, dynamic library and executable built for x86_64 and arm64.

Baseline automated tests:

```sh
make -C indigo_test build/integration/test_dome_nexdome3_simulator
cd indigo_test && ./build/integration/test_dome_nexdome3_simulator
```

Result: 1 run, 1 passed (`nexdome3_passes_serial_compliance_checks`, 8.8 s). `MIGRATION_STATUS.md` records `1 / 0`. The test passes although the simulator does not implement the protocol (see gaps below).

`build/bin/indigo_generator` is present in the build tree (built from the unchanged `indigo_tools/indigo_generator.c`).

## Hardware-test decision

No hardware testing will be performed. No NexDome controller is available (the user has no physical hardware). No statement in this record implies physical validation; all automated evidence is simulator-backed software behaviour.

## Studied sources

- `indigo_dome_nexdome3.c`, `.h`, `_main.c`, `README.md` in this directory (no Windows project exists).
- `dome_nexdome3_simulator/Documents/Firmware-Protocol.md` (official firmware command protocol and event notifications), `email_communication.txt` (firmware author's answers: `#\n` terminators are deliberately undocumented, undocumented diagnostic output must be ignored, `:BV` scale 15 V / 1023, position updates about four times a second, `XB->Online` repeated every 10 s, `ARR` replies `:ARR<value>#`, semantic version strings such as `3.1.0-iss8-home-sensor.28`), `nexdome_v3_additional_commands.txt` (simulator-only commands).
- `dome_nexdome3_simulator/dome_nexdome3_simulator.ino` (the author's Arduino simulator of firmware 3.2: 30 character command buffer, `@` restarts a command, echo replies, `:Err#`, dead zone, shortest-way rotation, `:left#`/`:right#`/`:open#`/`:close#` before motion, `P`/`S` position reports while moving, `:SER`/`:SES` when a motor stops, `:BV` and XBee state reports, rain, EEPROM load/save/defaults, `GSR` only in 3.2), the Python scripts next to it and `nexdome_v3_command_set.txt`; `dome_nexdome3_simulator.c` (current host PTY simulator).
- `indigo_test/integration/test_dome_nexdome3_simulator.c`, `indigo_test/Makefile`, `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (dome section and shared scope), `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`.
- `indigo_libs/indigo_dome_driver.c` (`DOME_ON_COORDINATES_SET` count 1, `DOME_STEPS` 0…180, `DOME_PARK` initially PARKED, disconnect resets dome property states to OK), `indigo_libs/indigo_driver.c` (INFO driver version from `version >> 16`; `indigo_execute_handler*`), `indigo_libs/indigo_io.c` (`indigo_open_serial()` sets `VMIN 0`, `VTIME 50`: a read returns 0 after 5 s of silence; TCP sockets have 5 s receive timeouts), `indigo_libs/indigo_timer.c` (`indigo_cancel_timer()` of a running timer only sets a flag), `indigo_libs/indigo_uni_io.c`, `indigo_tools/indigo_generator.c` (generated connection handler shape).
- Sibling migrations `dome_nexdome` (harness, simulator hooks, reference trace method) and `mount_rainbow` (generated driver with a reader thread for an event-stream protocol).

## Current-state audit

### Protocol

ASCII at 9600 8N1 or a `nexdome://host[:port]` TCP URL (default port 8080). Commands are `@<verb><target>[,<parameter>]` terminated by LF (the driver sends `\n`). Replies start with `:` and end with `#` (the firmware appends an undocumented `\n`); `:Err#` reports an invalid command. Event notifications can arrive at any time: `XB-><state>`, `P<steps>` and `S<steps>` (about every 250 ms while a motor moves, LF terminated, no `#`), `:SER,p,a,c,h,d#` and `:SES,p,l,o,c#` (when a motor stops or on request), `:left#`, `:right#`, `:open#`, `:close#` (before motion), `:BV<adu>#`, `:Rain#`, `:RainStopped#`; undocumented diagnostic output must be ignored.

Commands used by the driver:

| Command | Reply / effect | Driver use |
| --- | --- | --- |
| `FRR` | `:FR<semver>#` | identification at connection, INFO firmware revision; `<major>.<minor>` selects `GSR` for 3.2 and later |
| `SRR` / `SRS` | `:SER,…#` / `:SES,…#` | connection, 3 s after GOTO/steps/sync and shutter requests |
| `ARR`/`ARS`, `DRR`, `HRR`, `PRR`/`PRS`, `VRR`/`VRS`, `RRR`/`RRS` | `:ARR<v>#` … | settings readback at connection and after writes/EEPROM operations |
| `AWR`/`AWS`, `DWR`, `HWR`, `VWR`/`VWS`, `RWR`/`RWS` | echo `:AWR#` … | settings properties |
| `GAR,<deg>` | echo, rotation | GOTO (firmware < 3.2), relative move, park at 0° |
| `GSR,<steps>` | echo, rotation | GOTO with firmware ≥ 3.2 |
| `PWR,<steps>` | echo | sync (unreachable, see NX3-02) |
| `GHR` | echo, rotation to home sensor | `NEXDOME_FIND_HOME` |
| `OPS` / `CLS` | echo, shutter motion | `DOME_SHUTTER` |
| `SWR` / `SWS` | echo, hard stop | `DOME_ABORT_MOTION` |
| `ZRR`/`ZRS`, `ZWR`/`ZWS`, `ZDR`/`ZDS` | echo | `NEXDOME_SETTINGS` (EEPROM load, save, factory defaults) |

Documented but unused: `FRS`, `PWS`. The `NEXDOME_COMMAND` property exists only with the compile-time `CMD_AID` switch, which is disabled; it is not part of the driver.

### Architecture and implementation

- `DRIVER_VERSION 0x02000000B`, device and label `NexDome3` (`DOME_NEXDOME3_NAME` in the public header), author Rumen G. Bogdanovski. One dome device, `ADDITIONAL_INSTANCES` supported, no hot plug. Legacy `indigo_io` API with POSIX `read()`, `close()`, `sleep()` and `pthread` in the driver; not portable to Windows and no Windows project. Listed in root `STABLE_DRIVERS`, registered in `indigo_server`.
- Transport: `nexdome_command()` formats `@<command>\n` and writes it under a write mutex without checking the result (fire and forget). `nexdome_get_message()` reads byte by byte under a read mutex up to `\n`, `\r` or `#`, skipping leading line breaks; a read failure or the 5 s `VTIME` silence returns false.
- Connection (timer thread, `indigo_set_timer(…, 0, dome_connect_callback)`): global lock; serial open followed by `sleep(1)` or TCP open; `FRR` and up to 30 messages until one starts with `:FR` (failure: close, CONNECTION ALERT "NexDome did not respond. Are you using the correct firmware?"); INFO model `NexDome` and firmware; custom properties defined; `steps_per_degree = 153`; `DOME_PARK` UNPARKED OK and `park_requested = true` (park detection at 0°); CONNECTION OK; reader started (`indigo_set_timer(…, 0, dome_event_handler)`); `SRR`, `SRS`, then `ARR ARS DRR HRR PRR PRS VRR VRS RRR RRS`.
- Reader `dome_event_handler` (timer thread, loops while `IS_CONNECTED`): dispatches messages by prefix to handlers that publish properties directly from the reader thread: `P`/`:PRR` heading; `S`/`:PRS` logged only; `:left#`/`:right#` HORIZONTAL and STEPS BUSY, find home BUSY "Going home...", park BUSY "Going to park position..."; `:open#`/`:close#` shutter BUSY "Shutter is opening..."/"Shutter is closing..."; `:SER` steps per degree, heading, HORIZONTAL and STEPS OK, find home OK "Dome is at home." when at home, park OK PARKED within 1° of 0°, abort completion (find home/park ALERT); `:SES` closed/open (with or without end switch message), stopped ALERT "Shutter stopped.", otherwise BUSY; `:BV` voltage (0.01465 V/ADU, published when changed by 0.01 V, "Dome power is low! (U = …V)" below 7.5 V and "Dome power is normal! (U = …V)"); `:HRR`, `:DRR` (also raises `DOME_SLAVING_PARAMETERS` to the dead zone with a message), `:AR`, `:VR`, `:RR` settings; `XB->` state text (OK when `Online`, else BUSY); `:Rain#`/`:RainStopped#` light ALERT/OK. Echoes, `:Err#`, `:FR` and unknown output are ignored.
- Property changes (`dome_change_property`, client thread) write commands synchronously and never wait for replies: `DOME_STEPS` (refused ALERT "Dome is parked." when parked; `(int)` whole-degree target, `GAR`, BUSY, `SRR` after 3 s), `DOME_HORIZONTAL_COORDINATES` (parked: ALERT "Dome is parked." and `PRR`; sync `PWR` then BUSY; GOTO `GAR`/`GSR`, BUSY, `SRR` after 3 s), `DOME_ABORT_MOTION` (`SWR`, `SWS`, stop requests for BUSY rotation/shutter), `DOME_SHUTTER` (`OPS`/`CLS`, BUSY, `SRS` after 3 s), `DOME_PARK` (unpark OK; park OK when within 1° of 0°, else `GAR,0`, `PRR`, BUSY), `NEXDOME_FIND_HOME` (`GHR`), settings writes each followed by readback commands, `DOME_SLAVING_PARAMETERS` (threshold raised to the dead zone with a message), `NEXDOME_SETTINGS` (`ZRR ZRS` + readback, `ZWR ZWS`, `ZDR ZDS` + readback). No request is BUSY-guarded. The 3 s status request timers have no reference and are never cancelled.
- Disconnect: `indigo_cancel_timer()` of the reader (flag only), delete custom properties, lock the read and write mutexes (waits for the reader's current read), close, global unlock. The dome is not stopped. Detach disconnects and calls `indigo_global_unlock()` a second time.

### Public properties and behaviour

- `DEVICE_PORT`, `DEVICE_PORTS` visible; INFO count 6; `DOME_SPEED` hidden; `DOME_ON_COORDINATES_SET` visible with count 1; `DOME_SLAVING_PARAMETERS` visible; `DOME_HORIZONTAL_COORDINATES` RW; `DOME_STEPS` item label "Relative move (°)"; `DOME_PARK_POSITION`, `DOME_HOME`, `DOME_FLAP` hidden.
- Custom properties (group `Settings`, defined only while connected): `NEXDOME_FIND_HOME` (`FIND_HOME` "Find home sensor", at most one), `NEXDOME_HOME_POSITION` (`POSITION` 0…100000 steps), `NEXDOME_MOVE_THRESHOLD` (`THRESHOLD` 0…10000 steps, default 300), `NEXDOME_BATTERY_POWER` (RO `VOLTAGE` 0…500 V `%.2f`), `NEXDOME_ACCELERATION_TIME` (`ROTATOR`/`SHUTTER` 100…10000 ms, 1500), `NEXDOME_VELOCITY` (`ROTATOR`/`SHUTTER` 32…5000 steps/s, 600/800), `NEXDOME_RANGE` (`ROTATOR` 30000…100000 default 55080, `SHUTTER` 20000…90000 default 46000), `NEXDOME_SETTINGS` (`LOAD_EEPROM`, `SAVE_EEPROM`, `LOAD_DEFAULT`, at most one), `NEXDOME_RAIN_SENSOR` (light `RAIN_ALERT`, initially IDLE), `NEXDOME_XB_STATE` (RO text `XB_STATE`, initially IDLE). None has the required `X_` prefix.

### Supported platforms, build and integration

- README: "platform independent" (in fact POSIX only). Built by `Makefile.drv` auto-discovery; Xcode `dome_nexdome3` group (sources, `.ino`, simulator, test). No `REFACTOR.md`, `.driver`, generated outputs or Windows project.
- `indigo_docs/PROPERTIES.md` lists the custom properties (including the disabled `NEXDOME_COMMAND`) and names the `.c` source.

### Existing simulator and tests (gaps)

`dome_nexdome3_simulator.c` does not implement the firmware protocol: it answers `Open`, `Close`, `Abort`, `Home` instead of `OPS`, `CLS`, `SWR`/`SWS`, `GHR`, completes motion immediately, answers unknown commands and writes with `:OK#` instead of echoes or `:Err#`, never sends `P`/`S` position events, `:left#`/`:open#` notifications, battery, XBee or rain events, has no dead zone, EEPROM, `GSR` or version selection, no fault injection, external control, event log or TCP listener. The single test is a smoke/compliance pass without timing, failure, reconnect, lifecycle, multi-instance or concurrency coverage.

### Defects and risks found by source audit

Identifiers are used in the found-defects section; each is either reproduced by a dedicated reproducer against the original driver or explicitly marked audit-only.

- **NX3-01** GOTO and relative moves complete OK 3 s after the request while the dome is still rotating: the delayed `SRR` reply is handled like the stop report.
- **NX3-02** `DOME_ON_COORDINATES_SET` keeps its base count of 1, so the implemented sync (`PWR`) is unreachable; the sync branch also overwrites its OK state with BUSY.
- **NX3-03** Park detection at connection leaves `park_requested` set: a later rotation not requested by the driver (toggle switch, weather) is announced as "Going to park position..." with `DOME_PARK` BUSY, and stopping near 0° marks the dome PARKED.
- **NX3-04** A park that does not move (within the dead zone but more than 1° from 0°) or that stops away from 0° stays BUSY forever (no status request, no failure).
- **NX3-05** A rotator range change is not applied to the steps-per-degree conversion until the next `:SER`: `GSR`, `PWR` and heading reports use the stale factor.
- **NX3-06** `NEXDOME_HOME_POSITION` readback compares step counts with `indigo_azimuth_distance()` (modulo 360): a controller home position that differs by a multiple of 360 steps (for example 360 instead of 0) is never published.
- **NX3-07** A shutter stopped between the end positions (after an abort, manually or after reconnection) is reported BUSY indefinitely by every later `:SES`.
- **NX3-08** The low-voltage state is a `static` variable shared by all instances and sessions: a low battery on a second dome instance is not reported.
- **NX3-09** A rejected motion command (`:Err#`) is ignored: GOTO, relative move and shutter requests complete OK after the delayed status request without any motion.
- **NX3-10** Relative moves truncate the current heading to whole degrees (`(int)(10.7 + 10) % 360`): the dome is sent up to 1° short.
- **NX3-11** `DRIVER_VERSION 0x02000000B` has an extra digit: INFO reports driver version 32.0.0.11.
- **NX3-12** The delayed status request timers are not cancelled by a disconnect: after a quick reconnection the old request is written to the new session (or to a closed descriptor).
- **NX3-13** Disconnect blocks while the reader waits for data: up to 5 s (serial `VTIME`) when the controller is silent; after transport loss the reader spins on immediate read failures.
- **NX3-14** The identification reads up to 30 messages of 5 s each: a controller that streams events but does not answer `FRR` delays the connection failure for up to 30 events.
- **NX3-15** A find home request rejected by the controller (`:Err#`) or not moving leaves the request pending: a later stop at the home sensor publishes "Dome is at home.".
- **NX3-16** A shutter open or close request completes OK with the previous end state when the delayed `SRS` reply arrives before the shutter reported its motion over the wireless link.
- Audit-only risks (the first was later reproduced as NX3-17, a crash): the message reader writes the terminating NUL one byte past the buffer for a 100 (connection 255) byte unterminated message and splits it into further messages; `:SER` with circumference 0 divides by zero and publishes NaN; negative or out-of-range positions are published unnormalized; malformed messages are only logged; `GAR` targets are rounded with `%.0f` and 359.5° and more is sent as `GAR,360` (outside the documented 0…359); the reader thread and client-thread change handlers update the same properties without serialization (partial `PROPERTY_LOCK()` use) and the reader may update properties that are being deleted; `indigo_global_unlock()` is called twice on detach; a dropped TCP connection is not reported (CONNECTION stays OK); POSIX-only I/O.

## Atomic plan

1. **Done — record.** This file: audit, baseline, hardware decision, plan (written before any production change).
2. **Done — simulator audit and rewrite.** `dome_nexdome3_simulator.c` rewritten from `Firmware-Protocol.md`, the e-mail notes and the author's `.ino`: `@` framing with CR/LF, 30 character buffer and `:Err#` for empty, short, overlong, unknown and invalid commands (`.ino` range checks); echoes and value replies for every documented command; firmware version option (`GSR` only for 3.2 and later, `FRS` shutter version); `serial_motion.h` elapsed-time rotation in steps (velocity × `--speed-factor`, `.ino` integer `GAR` conversion, dead zone, shortest way, wrap, `GHR` clockwise to the home position, home sensor, `PWR` sync while moving) and shutter motion; `:left#`/`:right#`/`:open#`/`:close#` after the command reply, `P`/`S` reports every 0.25 s, `:SER`/`:SES` when a motor stops or on `SWR`/`SWS`; optional shutter latency; `:BV` and `XB->` reports; rain; EEPROM load/save/defaults; simulator-only commands; replies without the undocumented LF (`--no-newline`). Test hooks: event log (`RX`, `TX` replies, `EV` notifications), fault injection (`lost`, `error`, `reply`, `mute`, `raw`, `slow`, `close`), external control (manual rotation and shutter motion, stall with and without status report, arbitrary and undocumented output, rain, battery, XBee, home, range, dead zone, version, latency, noise, TCP drop), optional loopback TCP listener. PTY output while the port is closed is discarded like on a real serial port (checked with a Python PTY probe). Verification: `clang -std=gnu11 -Wall -Wextra -Wshadow -Wno-unused-parameter -fsyntax-only` clean; `simulator_protocol` self-check (framing, errors, values, EEPROM, dead zone, rotation timing, wrap, stop, home, shutter, rain, battery, XBee, version gating, control and faults): first run failed twice on test mistakes (expected the next XBee state instead of waiting for `Online`; a manual rotation from 0.65° to 10° turns right), then 1 run, 1 passed.
3. **Done — characterization suite against the original driver.** `indigo_test/integration/test_dome_nexdome3_simulator.c` rewritten on the `dome_nexdome` harness (forked cases with fresh simulators, observed publications and messages, simulator event log, generic observation helpers copied from `test_dome_nexdome_simulator.c`). Baseline binary compiled (arm64) from the same source with `-DNEXDOME3_ORIGINAL_DRIVER` (original property names, version `0x02000000B`, original trace fixture) and the original `indigo_dome_nexdome3.c`/`.h` of commit `32dfeaf5f` (see "Original-driver baseline binary"). First complete run: 33 run, 21 passed, 12 failed. Test mistakes, fixed and rerun individually (5 run, 4 passed; `connect_reads_settings` then failed on a further test mistake — the `.ino` converts `GAR,90` with the integer 183 steps per degree — fixed, 1/1): `connect_sequence`, `connect_reads_settings`, `battery_status` (the battery voltage is published only after the first periodic `:BV`), `goto_firmware_32` (a 10.4° target is within the dead zone of 10°), `rain_sensor` (waited for the closed message before the shutter arrived). New finding: `long_undocumented_output_ignored` crashed the original with `SIGABRT` (stack protector) on a 150 character diagnostic line — moved to the reproducer NX3-17. `reference_trace` failed only because its fixture did not exist yet. Five cases specify generated behaviour and are expected to fail against the original: `busy_requests_rejected` (the original retargets a moving dome), `malformed_status_ignored` (`:SER` with circumference 0 publishes NaN), `stalled_rotation_recovers` (no recovery without a stop report), `status_request_unanswered_alerts` (stays BUSY), `shutter_without_motion_times_out` (completes OK with the closed shutter).
4. **Done — defect reproducers.** `--known-defects` runs NX3-01…NX3-17 (NX3-17 found in step 3). Against the original: 17 run, 17 failed as expected. NX3-01 first failed at the wrong assertion (the simulator started at 0°, so the dome was parked and the GOTO refused); the case now starts at 5°, rerun 1/1 failed at its defect assertion. Evidence per defect is listed below.
5. **Done — original reference trace.** `indigo_test/fixtures/dome_nexdome3/original_reference_trace.txt` (402 lines): per step the ordered simulator exchange (`S RX` commands, `S TX` replies, `S EV` notifications without periodic `P`, `S`, `:BV`, `XB->` and diagnostic output; `:PRR` values masked because a position read during motion depends on timing; the abort step masked and followed by a GOTO to a known heading) followed by property definitions/updates/deletions and messages with original property names (repeated identical BUSY updates collapsed, azimuth masked while BUSY). The first three captures differed in timing-dependent positions (`:PRR` during park, the heading after abort); after masking them, 3 captures were identical.
6. **Done — `.driver` and regeneration.** Added `indigo_dome_nexdome3.driver` (version 12, `DRIVER_VERSION 0x0300000C`), `serial;`, transactional `nexdome3_open`/`nexdome3_close` (global lock, serial open with the 1 s delay or `nexdome://` TCP with default port 8080, `FRR` identification reading at most 30 messages within 5 s). Portable `indigo_uni_*` transport: `nexdome3_command()` keeps the fire-and-forget manner (`@<command>\n`, no wait for replies) and records the command code for reply matching; a reader thread (`indigo_set_timer` + `reader_running`, like `mount_rainbow`) reads messages up to `#`, LF or CR with 0.5 s timeouts, discards the rest of overlong lines, backs off 0.5 s after an immediate read failure, and hands messages to the device queue (`nexdome3_process_messages`) so that all property changes are serialized with the generated handlers. Message handlers keep the original prefixes, texts and publications. Delayed status requests are queued finalizers (`rotator_status_finalizer`, `shutter_status_finalizer`, 3 s as before) followed by bounded watch finalizers (`rotation_finalizer`, `shutter_finalizer`). Custom properties renamed with the `X_` prefix (items, labels, groups, ranges and formats unchanged); the disabled `NEXDOME_COMMAND` is not generated. Fixes for NX3-01…NX3-17. Generator-order issues found while bringing the driver up and fixed in the `.driver` before the suite ran: the device `on_attach` runs before custom properties are allocated (segmentation fault setting the XBee property state; moved into the property's `on_attach`), and the identification skipped the undocumented line feed after `:FR…#` as an empty read and failed (now ignored explicitly). `build/bin/indigo_generator indigo_dome_nexdome3.driver`; `make -B -f ../../Makefile.drv` x86_64 + arm64 with zero warnings; `clang -Wall -Wextra -Wno-unused-parameter -Wshadow -fsyntax-only` reports only the generator-emitted `VERIFY_NOT_CONNECTED` shadowing present in every generated driver. No generator change, no `MAX_DEVICES` override. The generated header no longer defines `DOME_NEXDOME3_NAME` (only the old test used it).
7. **Done — post-migration suite and trace comparison.** First complete run: 32 run, 29 passed. `simulator_protocol` failed on a test race (`SWR` right after `:left#` can stop at the start position; the check now allows it, then 1/1), `settings_writes` depended on NX3-05 (the old expectation used 153 steps per degree after a range change; the case now keeps the circumference and rejects a home position above it, then 1/1 on both drivers), `reference_trace` had no generated fixture. `--known-defects`: 17 run, 15 passed; NX3-04 failed because park detection at connection used the dead-zone tolerance (connection detection restored to the original 1°, the dead-zone tolerance applies only to park requests) and NX3-09 because the delayed status reply turned the rejected GOTO back to OK (a status reply without an active request no longer clears ALERT); regenerated, both 1/1. Generated trace captured (`generated_reference_trace.txt`, analysis below); a complete run with the fixture: 32 run, 32 passed.
8. **Done — defect fixes promoted.** The 17 NX3 cases were promoted into the ordinary suite (`defect = false`). A shared `static` buffer in the message handler (two instances process on different queues) was moved into the private data before the final runs.
9. **Done — repository integration.** `indigo.xcodeproj/project.pbxproj`: `REFACTOR.md` and `.driver` in the `dome_nexdome3` group, new `fixtures/dome_nexdome3` group (`plutil -lint` OK). New `indigo_dome_nexdome3.vcxproj`, `.filters`, `.user` from the `dome_nexdome` template (UTF-8 BOM, CRLF, `.driver` and `REFACTOR.md` as `None` items, `xmllint` OK) and `indigo_windows.sln` entry (CRLF preserved); no Windows build was possible. `indigo_test/Makefile`: simulator depends on `serial_motion.h` (no longer `-pthread`), test depends on both fixtures and the driver archive, targets `test-dome-nexdome3-simulator`, `test-dome-nexdome3-simulator-sanitize`, opt-in `test-dome-nexdome3-simulator-network`. `indigo_docs/PROPERTIES.md`: renamed properties, removed the disabled `NEXDOME_COMMAND`, source is the `.driver`. `MIGRATION_STATUS.md` row updated (see step 10). `README.md` unchanged.
10. **Done — final verification.**
    - Generator reproducibility: `build/bin/indigo_generator indigo_dome_nexdome3.driver` in a scratch copy; generated `.c`, `.h`, `_main.c` byte-identical to the checked-in files.
    - Build: `make -B -f ../../Makefile.drv` (x86_64 + arm64) without warnings.
    - Complete suite `./build/integration/test_dome_nexdome3_simulator` (generated driver, promoted reproducers): 49 run, 49 passed, 0 failed.
    - Opt-in network cases `--network`: 3 run, 3 passed (`network_nexdome_url`, `network_default_port`, `network_failures`; loopback simulator only).
    - ASan + UBSan: `make -C indigo_test test-dome-nexdome3-simulator-sanitize` (arm64, `-O1`, driver source compiled into the test, `detect_leaks=0`, `halt_on_error=1`): 49 run, 49 passed, no sanitizer report.
    - Original driver with the final harness (baseline binary described below): 49 run, 27 passed, 22 failed — exactly the 17 promoted NX3 reproducers and the five generated-behaviour cases listed in step 3. Every preservation case, including `settings_writes` after its step 7 change and `reference_trace` against `original_reference_trace.txt`, passed. Original driver `--network`: 3 run, 3 passed.
    - `MIGRATION_STATUS.md` row: API 3️⃣, Windows ✅ Yes (project added, no Windows build or test possible in this environment), Generator ✅ Yes, Async Queues ✅ Yes, Retested ✅ Sim (simulator only, no hardware), tests `52 / 0` = 32 default cases + 17 promoted reproducers + 3 opt-in network cases; Comment unchanged.
    - Diff and format audit, versioning, cleanup: see "Final audit" below.
    - Not run: Linux and Windows builds/tests (environment unavailable), hardware tests (no device).

## Original-driver baseline binary

The original-driver baseline binary is not in the repository. To rebuild it, take `indigo_drivers/dome_nexdome3/indigo_dome_nexdome3.c` and `.h` from commit `32dfeaf5f` into a temporary directory `<tmp>`, put a copy of the header both next to the `.c` and at `<tmp>/indigo_drivers/dome_nexdome3/indigo_dome_nexdome3.h`, and run from `indigo_test`: `clang -std=gnu11 -arch arm64 -g -O1 -DINDIGO_MACOS -Duint=unsigned -I<tmp> -I.. -I../indigo_libs -I../build/include -I../indigo_drivers -DNEXDOME3_ORIGINAL_DRIVER -w -o <tmp>/test_original integration/test_dome_nexdome3_simulator.c <tmp>/indigo_dome_nexdome3.c -L../build/lib -Wl,-rpath,$(pwd)/../build/lib -lindigo -lm -lpthread -lindigocat`. Never run it concurrently with another NexDome3 test process (global device lock).

## Original-driver baseline evidence

Environment: macOS 26 arm64, baseline binary above, `--known-defects` before promotion: 17 run, 17 failed as expected, each at its defect assertion:

- NX3-01: the GOTO settled OK while the simulator was still moving (after the 3 s status reply).
- NX3-02: `DOME_ON_COORDINATES_SET` count 1 instead of 2.
- NX3-03: "Going to park position..." published for a toggle-switch rotation after connection.
- NX3-04: a park request 1.5° from the park position (within the dead zone) did not settle.
- NX3-05: after the circumference was changed to 66000 steps, GOTO 180 did not send `GSR,33000`.
- NX3-06: controller home position 360 published as 0.
- NX3-07: a shutter stopped at 20000 of 46000 steps was BUSY after connection.
- NX3-08: no low-voltage message for the second instance.
- NX3-09: a GOTO rejected with `:Err#` settled OK.
- NX3-10: a 10° clockwise move from 10.699° did not send `GAR,21` (it sent `GAR,20`).
- NX3-11: driver version major/minor `0x2000`.
- NX3-12: a disconnection and reconnection within 3 s of a GOTO produced 3 `SRR` instead of 2.
- NX3-13: disconnect of a silent controller took 3.188 s.
- NX3-14: a failed identification with a streaming controller took 28.187 s.
- NX3-15: `NEXDOME_FIND_HOME` did not settle in ALERT after `GHR` was rejected.
- NX3-16: the open request completed OK with CLOSED selected before the shutter moved (latency 4 s).
- NX3-17: the process aborted (`SIGABRT`, stack protector) on a 150 character diagnostic line.

## Reference trace comparison

`original_reference_trace.txt` (402 lines) and `generated_reference_trace.txt` (423 lines): every `S` (protocol) line is identical — connection sequence and readback order, `GSR`/`GAR` arguments, delayed `SRR`/`SRS` 3 s after requests, `GAR,0` and `PRR` for park, `PRR` for GOTO while parked, `GHR`, `SWR`/`SWS`, `OPS`/`CLS`, settings writes and readbacks, EEPROM commands — except one intentional addition: the park request also sends the delayed `SRR` (NX3-04), with its reply and the resulting HORIZONTAL/STEPS OK updates. All other differences are property publications:

1. Queued changes publish BUSY at request time (`INDIGO_COPY_VALUES_PROCESS_CHANGE` / `…_URGENT_CHANGE`): additional BUSY lines for `DOME_PARK` (with the requested switch), `DOME_ABORT_MOTION`, `DOME_HORIZONTAL_COORDINATES` and `DOME_STEPS` while parked, the settings properties, `DOME_SLAVING_PARAMETERS`.
2. `X_FIND_HOME` is BUSY when the request is accepted (the original published the request OK and BUSY only at `:right#`).
3. Generated connection messages "Connected to NexDome3 on PORT" and "Disconnected from NexDome3".
4. `DOME_ON_COORDINATES_SET` is defined with count 2 (NX3-02).

## Intentional behaviour differences

- **Property names:** `NEXDOME_*` custom properties renamed to the required `X_` names (`X_FIND_HOME`, `X_HOME_POSITION`, `X_MOVE_THRESHOLD`, `X_BATTERY_POWER`, `X_ACCELERATION_TIME`, `X_VELOCITY`, `X_RANGE`, `X_SETTINGS`, `X_RAIN_SENSOR`, `X_XB_STATE`); items unchanged. Clients using the old names must be updated. The trace maps the new names back to the original ones.
- **Serialization:** a reader thread feeds the device queue; message handling, handlers and finalizers no longer race. Requests arriving while their property is BUSY are ignored by the framework guard (the original retargeted a moving dome or reversed a moving shutter).
- **Transport:** portable `indigo_uni_*` I/O; reader timeouts 0.5 s (disconnect no longer waits up to 5 s, NX3-13); immediate read failures back off 0.5 s instead of spinning; identification bounded to 30 messages and 5 s (NX3-14); overlong lines are discarded (NX3-17); malformed `:SER`/`:SES` (missing fields, circumference 0) are ignored; headings are normalized to 0…360°.
- **Completion:** rotation completes on the stop report; a status reply during motion only updates the heading (NX3-01); a rotation without position reports for 5 s requests the status and completes on its reply; a missing reply to the delayed status request within 3 s ends the request in ALERT; `:Err#` for `GAR`/`GSR`/`GHR`/`PWR`/`OPS`/`CLS` ends the request in ALERT (NX3-09, NX3-15), matched by the command order; shutter requests wait for the shutter motion and end in ALERT "Shutter did not respond" after 30 s without motion (NX3-16); a shutter stopped between the end positions is ALERT with OPENED selected (NX3-07); park requests complete within the dead zone and fail when the rotation stops elsewhere (NX3-04); park detection is limited to the first status report after connection (NX3-03); find home that stops away from home fails.
- **Other fixes:** sync reachable and OK at once (NX3-02); circumference readback updates the step conversion (NX3-05); home position compared as steps (NX3-06); low-voltage state per instance and session (NX3-08); fractional heading kept for relative moves (NX3-10); version `0x0300000C` (NX3-11); delayed requests cancelled on disconnect (NX3-12); interrupted find-home and settings switches reset at connection.
- **Unchanged by decision:** message texts including the typos "synchrinization" and "swich"; the step value truncated to whole degrees; `GAR` targets formatted with `%.0f` (a target of 359.5° or more is sent as `GAR,360`); settings writes are published OK before the readback; the dome is not stopped on disconnect; a dropped TCP connection is not reported (CONNECTION stays OK).

## Found defects

Every defect below was reproduced by a dedicated case that failed against the original driver and passes against the generated driver.

| ID | Observable impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- |
| NX3-01 | GOTO/steps OK 3 s after the request while rotating. | Status reply handled as stop report. | Reply matching; replies during motion only update the heading. | `NX3-01 goto_completes_after_stop` |
| NX3-02 | Sync unreachable; sync BUSY for 3 s. | Base count 1; OK overwritten by BUSY. | Count 2; sync OK. | `NX3-02 sync_is_reachable` |
| NX3-03 | Toggle-switch rotation reported as park, may park the dome. | `park_requested` set at connection and never cleared. | Detection only on the first status report. | `NX3-03 manual_rotation_is_not_park` |
| NX3-04 | Park within the dead zone or stopped short stays BUSY. | No status request, completion only within 1°. | Dead-zone tolerance, delayed `SRR`, ALERT when stopped elsewhere. | `NX3-04 park_without_motion_completes` |
| NX3-05 | Wrong `GSR`/`PWR` steps after a circumference change. | Step conversion only from `:SER`. | `:RRR` updates the conversion. | `NX3-05 range_change_applies` |
| NX3-06 | Home position differing by 360 steps not published. | `indigo_azimuth_distance()` on steps. | Plain step difference. | `NX3-06 home_position_readback` |
| NX3-07 | Partially open shutter BUSY forever. | Intermediate `:SES` without stop request is BUSY. | ALERT unless the shutter is moving. | `NX3-07 stopped_shutter_not_busy` |
| NX3-08 | Second instance: no low-voltage message. | `static` low-voltage state. | Private data, reset per session. | `NX3-08 low_voltage_per_instance` |
| NX3-09 | Rejected GOTO/steps/shutter request completes OK. | `:Err#` ignored. | Command order matching, ALERT. | `NX3-09 rejected_commands_alert` |
| NX3-10 | Relative moves up to 1° short. | `(int)` truncation of the heading. | `fmod` on doubles. | `NX3-10 relative_move_keeps_fraction` |
| NX3-11 | INFO driver version 32.0.0.11. | Extra digit in `DRIVER_VERSION`. | Generated version. | `NX3-11 driver_version` |
| NX3-12 | Stale `SRR`/`SRS` written after disconnect. | Unreferenced 3 s timers. | Queued finalizers cancelled on disconnect. | `NX3-12 status_request_cancelled_on_disconnect` |
| NX3-13 | Disconnect waits up to 5 s; reader spins after transport loss. | 5 s `VTIME` read under the port mutex; failures retried at once. | 0.5 s reader timeout, back-off. | `NX3-13 disconnect_is_prompt` |
| NX3-14 | Failed identification takes up to 30 events. | 30 messages of up to 5 s each. | 5 s overall deadline. | `NX3-14 identification_is_bounded` |
| NX3-15 | Rejected find home completes later. | Request flag never cleared. | `:Err#` → ALERT, flag cleared. | `NX3-15 failed_find_home_cleared` |
| NX3-16 | Shutter request OK with the old end state. | Delayed `:SES` before the shutter moved completes it. | Wait for motion, 30 s timeout. | `NX3-16 shutter_waits_for_motion` |
| NX3-17 | Driver process aborts on a long diagnostic line. | Reader writes the terminator past its buffer. | Bounded reader, overlong lines discarded. | `NX3-17 overlong_output_ignored` |

Audit-only risks resolved without a dedicated reproducer: unserialized property updates from the reader thread and property deletion races, NaN heading from `:SER` with circumference 0 and unnormalized positions (covered by `malformed_status_ignored` for the generated driver), double global unlock on detach, POSIX-only I/O.

Remaining known limitations: a dropped TCP connection or unplugged serial port is not reported as a connection failure (the reader backs off and CONNECTION stays OK, as in the original); command/reply matching assumes replies in command order and expires unmatched commands after 3 s; `GAR,360` for targets of 359.5° and more; the real firmware's shutter latency, stall behaviour, XBee and diagnostic output are simulator assumptions from `Firmware-Protocol.md`, the e-mail notes and the author's `.ino`; network transport covered only against the loopback simulator; no hardware, Linux or Windows validation.

## Scenario-to-test mapping

| Area | Cases |
| --- | --- |
| Simulator protocol self-check (not driver coverage) | `simulator_protocol` |
| Metadata, INFO, interface, no I/O before connection | `metadata_before_connection`, NX3-11 |
| Connection sequence, delay, identification, readback, property shapes, park/shutter state at connection, older firmware version string | `connect_sequence`, `connect_reads_settings`, `reference_trace` |
| Open/identification failures, descriptor balance, recovery, bounded identification | `connection_failures_and_recovery`, NX3-14 |
| GOTO with `GSR` and `GAR`, wrap, dead zone, completion after stop, stall and missing status | `goto_firmware_32`, `connect_reads_settings`, NX3-01, `stalled_rotation_recovers`, `status_request_unanswered_alerts` |
| Relative moves, truncated steps, fractional heading | `relative_steps`, NX3-10 |
| Sync | NX3-02 |
| Parked dome refuses moves; park/unpark; park within dead zone and stopped short; park detection | `moves_refused_when_parked`, `park_and_unpark`, NX3-03, NX3-04 |
| Find home (success, at home, abort, rejection) | `find_home`, `abort_park_and_home`, NX3-15 |
| Abort while rotating, idle, during park, find home and shutter motion | `abort_rotation`, `abort_park_and_home`, `abort_shutter` |
| Shutter open/close, repeated requests, end switch messages, stopped between ends, latency, no motion, rejection, manual motion, rain | `shutter_open_close`, `shutter_end_switch_messages`, NX3-07, NX3-16, `shutter_without_motion_times_out`, NX3-09, `unsolicited_events`, `rain_sensor` |
| Settings writes/readback, rejected write, circumference conversion, home position readback, slaving threshold, EEPROM | `settings_writes`, NX3-05, NX3-06, `slaving_parameters`, `eeprom_settings` |
| Battery, XBee state, rain notifications | `battery_status`, `xb_state`, `rain_sensor`, NX3-08 |
| Unsolicited, undocumented, malformed, overlong output; replies without LF | `unsolicited_events`, `malformed_status_ignored`, NX3-17, `replies_without_newline` |
| BUSY conflicts | `busy_requests_rejected` |
| Disconnect during motion, reconnect, stale delayed requests, prompt disconnect, transport loss | `disconnect_during_motion`, NX3-12, NX3-13 |
| INIT/SHUTDOWN idempotence and repeated cycles; additional instance | `lifecycle_and_shutdown`, `additional_instance`, NX3-08 |
| Network transport (opt-in `--network`) | `network_nexdome_url`, `network_default_port`, `network_failures` |

Not applicable: hot plug, guider timing (no guider interface), driver-owned persistent settings (EEPROM is controller-owned and covered), `DOME_SPEED`/`DOME_FLAP`/`DOME_HOME`/`DOME_PARK_POSITION` (hidden), queued-abort overtaking (handlers only write commands and never block the queue), hardware acceptance.

## Final audit

- `git diff --check`: clean for driver, simulator, test and documentation files; `indigo_windows.sln` lines report only their CRLF terminators (preserved like the sibling `dome_nexdome` entry); the new `.vcxproj` files keep the template's BOM and CRLF.
- Formatting audit of `.driver`, simulator and test: single-line calls, comma spacing, `{ 0 }` initializers, no blank lines inside function bodies (the multi-line `simulator_driver_case` initializers follow the sibling test).
- Driver version `0x0300000C` > original `0x02000000B` major/minor; no `MAX_DEVICES` override; no generator change; `README.md` unchanged; the `.driver` is the source of the checked-in `.c`, `.h`, `_main.c`.
- Project registration: `.driver`, `REFACTOR.md` and fixtures in Xcode; Windows project and solution; Makefile targets. The old `dome_nexdome3_simulator.ino`, Python scripts and documents are unchanged.
- Cleanup: `make -C indigo_test test-clean`; scratch baseline binary, regeneration copy and probe files removed; no remaining simulator or test processes or `/tmp/indigo-nexdome3.*` fixtures.
- Unrelated working-tree change not made by this work and left untouched: `indigo_drivers/ccd_asi/README.md` (modified at 19:08 during the session).

## Final test summary

Trace captures count as runs of `reference_trace`; development runs of single cases are included.

- Simulated tests, original driver: 119 run, 64 passed. Breakdown: pre-existing smoke test 1/1; `simulator_protocol` development runs 3 run, 1 passed (2 test mistakes); first complete characterization run 33 run, 21 passed (12 failures analysed in step 3); reruns after test fixes 5 run, 4 passed and 1/1; defect reproducers 17 run, 0 passed (all failed as expected); NX3-01 rerun 1 run, 0 passed (failed as expected); trace captures 6/6; final harness with promoted reproducers 49 run, 27 passed (22 expected failures, step 10); network cases 3/3.
- Simulated tests, generated driver: 191 run, 183 passed. Breakdown: `connect_sequence` bring-up 4 run, 1 passed (two generator-order segmentation faults and one identification failure fixed in the `.driver`); first complete run 32 run, 29 passed; defect reproducers 17 run, 15 passed; reruns after fixes 4/4; trace capture 1/1; complete run with the fixture 32/32; final complete run 49/49; opt-in network cases 3/3; ASan + UBSan complete run 49/49.
- Hardware tests: 0 run, 0 passed.

## Rejected-change regression coverage (2026-09-18)

A change request that arrives while a motion is already running is now refused with the generator's
`reject_change` block on `DOME_STEPS`, with the condition that `DOME_HORIZONTAL_COORDINATES` is BUSY and the message
shown in the generated driver. The generated guard marks every item for update, sets
`INDIGO_ALERT_STATE` and publishes the property, so the client receives the actual driver-side values
instead of an update that carries no items.

Both motion properties are published BUSY together at motion start, so `INDIGO_COPY_*_PROCESS_CHANGE` dropped a concurrent request silently: the client received no update at all and kept showing the refused target. The guard runs before that macro and turns the silent drop into an explicit refusal.

Driver version is now `0x0300000D`. Regression coverage is the existing suite in
`indigo_test/integration/test_dome_nexdome3_simulator.c`, which was re-run after the change.

```sh
cd indigo_test && ./build/integration/test_dome_nexdome3_simulator
```

- Simulated tests run: 49; passed: 49.
- Hardware tests run: 0; passed: 0.
