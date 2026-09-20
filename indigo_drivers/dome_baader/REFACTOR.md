# dome_baader refactoring record

## Scope and baseline

This record covers migration of `indigo_dome_baader` (Baader Planetarium Classic rotating dome) from its hand-written INDIGO 2.0 implementation (legacy `indigo_io`, POSIX `select`/`read`, timer thread, synchronous bus handlers) to `indigo_generator` with portable `indigo_uni_io` and the device handler queue, the production fixes proven by regression tests, and complete applicable hardware-free simulator coverage.

Baseline date and source: 2026-09-17, commit `bf636afc1` (`focuser_astroasis: test added for failing connect`), branch `refactoring`, clean working tree. Host: macOS 26 (`Darwin 25.6.0`), Apple Silicon arm64; repository universal build (x86_64 + arm64).

Baseline build command:

```sh
cd indigo_drivers/dome_baader
make -B -f ../../Makefile.drv
```

Result: passed without compiler or linker warnings; object, archive, dynamic library and executable built for x86_64 and arm64.

Baseline automated tests:

```sh
make -C indigo_test build/integration/test_dome_baader_simulator
cd indigo_test && ./build/integration/test_dome_baader_simulator
```

Result: 1 run, 1 passed (`baader_passes_serial_compliance_checks`, 5.8 s). `MIGRATION_STATUS.md` records `1 / 0`.

`build/bin/indigo_generator` was not present in the build tree; it was built from the unchanged `indigo_tools/indigo_generator.c` with `make -C indigo_tools /Users/polakop/Development/indigo/build/bin/indigo_generator` (no source change).

## Hardware-test decision

No hardware testing will be performed. The user has no Baader Classic dome (no physical hardware is available). No statement in this record implies physical validation; all automated evidence is simulator-backed software behaviour. The README status is "Untested" (developed against the Arduino simulator sketch only).

## Studied sources

- `indigo_dome_baader.c`, `.h`, `_main.c`, `README.md`, `BAADER-PROTOCOL-LICENSE.md` in this directory.
- `dome_baader_simulator/`: `dome_baader_simulator.ino` (original Arduino simulator by the driver author, the most detailed protocol reference available: 9-byte frames without terminator, command set, state machines, reply formats for new and `OLD_FIRMWARE`), `baader_command_set.txt` and `baader_shutter.txt` (command scripts), `baader_simulator_test.py`, and the host-side `dome_baader_simulator.c`.
- `indigo_test/integration/test_dome_baader_simulator.c`, `indigo_test/Makefile`, `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (dome section and shared scope).
- No manufacturer protocol document is bundled; the protocol licence file covers usage only. Commands and replies below are taken from the driver and the `.ino` sketch; everything else is a recorded simulator assumption.
- `indigo_tools/indigo_generator.c`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_libs/indigo_dome_driver.c`, `indigo_libs/indigo_uni_io.c` (`indigo_uni_read_section2()` with empty terminators reads exactly `length` bytes or returns a short count on timeout; `indigo_uni_discard()` drains with a 10 ms wait), sibling migrations `focuser_dsd` (serial simulator with event/fault/control files, forked cases, reference traces), `dome_skyroof` and `dome_simulator` (generated dome drivers).

## Current-state audit

### Protocol

Fixed 9-byte ASCII frames without terminator in both directions.

| Command | Reply | Driver use |
| --- | --- | --- |
| `d#ser_num` | `d#<serial>` (sketch `d#3141592`), `d#domerro` | connection identification, INFO serial number |
| `d#getazim` | `d#azi%04d` (tenths of degree); old firmware `d#azr%04d` when stopped | position poll, before relative moves, at connection |
| `d#azi%04d` | `d#gotmess`, `d#domerro` | absolute, relative and park moves (sketch rotates the shortest way) |
| `d#stopdom` | `d#gotmess`, `d#domerro` | abort (sketch stops rotation and aborts moving shutter and flap) |
| `d#getshut` | `d#shutope`, `d#shutclo`, `d#shut_%02d`; old firmware `d#shutrun` | shutter poll |
| `d#opeshut` / `d#closhut` | `d#gotmess`, `d#domerro` | shutter open/close (reverse request while moving aborts in the sketch) |
| `d#getflap` | `d#flapope`, `d#flapclo`, `d#flaprop`, `d#flaprcl`, `d#flaprim`; old firmware `d#flaprun` | flap poll |
| `d#opeflap` / `d#cloflap` | `d#gotmess`, `d#err_sht` (shutter below 5 %), `d#domerro` | flap open/close |
| `d#get_eme` | `d#emeRWTP` (`0`/`1` rain, wind, operation timeout, power cut) | emergency flags poll |
| `d#mvshtNN`, `d#opefull`, `d#chk_aon` (`d#automod`) | `d#gotmess` | not used by the driver |
| unknown | `d#comerro` | |

### Architecture and implementation

- `DRIVER_VERSION 0x020000005` (nine hex digits: the published value is `0x20000005`, major version 32), label/device `Baader Classic Dome` (`DOME_BAADER_NAME`), author Rumen G. Bogdanovski. One dome device, `ADDITIONAL_INSTANCES` supported, no hot plug, serial transport or `baader://host[:port]` / `tcp://` URL opened as TCP (default port 8080) via legacy `indigo_open_network_device()`.
- Legacy non-portable I/O: file descriptor, `select()`, `read()`, `close()`, `indigo_write()`; a `pthread` port mutex serializes single commands only. There is no Windows project; the driver cannot build on Windows.
- `baader_command()`: drains input (10 ms `select` loop), writes the command without checking the result, sleeps 100 µs, reads up to 9 bytes with a 3.1 s first-byte and 0.1 s inter-byte timeout and returns success even when fewer bytes (or none) arrived; the callers compare or `sscanf` the reply.
- Connection runs on a zero-delay timer: global lock, open, `d#ser_num` (any `d#…` reply except `d#domerro` is accepted as serial number), INFO serial number, define `X_EMERGENCY_CLOSE` (IDLE), `d#getazim` (failure only logged), position/target initialized, park azimuth 0, `DOME_PARK` PARKED when the azimuth is within 0.01° of 0 otherwise UNPARKED, status poll after 0.5 s.
- Status poll (timer thread, every 1 s after the previous run): `d#getazim`, `d#getshut`, `d#getflap`, `d#get_eme`, always in this order, failures only logged.
  - Rotation: while `DOME_HORIZONTAL_COORDINATES` or `DOME_PARK` is BUSY, or the azimuth differs from the cached target by ≥ 0.1°, publishes `DOME_HORIZONTAL_COORDINATES` (and `DOME_STEPS`) BUSY with the current azimuth when away from the target, otherwise OK. Inside that branch, a requested park completes (PARKED OK) when the azimuth is within 0.1° of 0.
  - Shutter: when the position changed since the previous poll or the property is BUSY: 100 → OPENED OK "Shutter open", 0 → CLOSED OK "Shutter closed", otherwise OPENED selected, state unchanged. The previous position is a function-`static` variable.
  - Flap: when the state changed or the property is BUSY: open → OPENED OK "Flap open", closed → CLOSED OK "Flap closed", `flaprim` → no switch, OK, moving → no switch, state unchanged. The previous state is a function-`static` variable.
  - After an abort: publishes `DOME_HORIZONTAL_COORDINATES` (target := current), `DOME_STEPS`, `DOME_SHUTTER`, `DOME_FLAP` OK.
  - Emergency: when `X_EMERGENCY_CLOSE` is IDLE or a flag changed, publishes OK with ALERT/OK lights.
- Disconnect: synchronous timer cancel, delete `X_EMERGENCY_CLOSE`, close, global unlock; no stop command.

### Public properties and behaviour

- `DEVICE_PORT`, `DEVICE_PORTS` visible; INFO count 8 (serial number shown); `DOME_SPEED` and `DOME_ON_COORDINATES_SET` hidden; `DOME_FLAP` and `DOME_SLAVING_PARAMETERS` visible; `DOME_HORIZONTAL_COORDINATES` RW (count 1); `DOME_STEPS` label "Relative move (°)".
- Custom `X_EMERGENCY_CLOSE` (light, group `Dome` (`DOME_MAIN_GROUP`), label "Energency close flags", items `RAIN` "Rain alert", `WIND` "Wind alert", `OPERATION_TIMEOUT` "Operation timeout alert", `POWER_CUT` "Power coutage alert"), defined only while connected. The name already has the mandatory `X_` prefix.
- All property changes except CONNECTION run synchronously on the calling bus thread:
  - `DOME_HORIZONTAL_COORDINATES`: rejected with "Dome is moving: request can not be completed" while BUSY. Values copied; parked → `d#getazim`, value := current, HORIZONTAL/STEPS ALERT "Dome is parked". Otherwise target := requested, `d#azi`, failure → STEPS/HORIZONTAL ALERT ("Goto azimuth failed[ with DOME_ERROR…]"), success → STEPS and HORIZONTAL BUSY.
  - `DOME_STEPS`: rejected while STEPS or HORIZONTAL is BUSY; parked → ALERT "Dome is parked"; `d#getazim`; target := current ∓ steps (clockwise +) via `(int)(10 × …) % 3600`; `d#azi`; failure → ALERT both with message; success → BUSY both (HORIZONTAL value = current).
  - `DOME_PARK`: UNPARKED → OK (no device command). PARKED → switch forced to UNPARKED, `d#azi0000` (failure only logged), target 0, park requested, PARK/STEPS/HORIZONTAL BUSY.
  - `DOME_ABORT_MOTION`: `d#stopdom`; failure → ALERT; success → PARK ALERT if it was BUSY, target := cached current, abort flag, ABORT OK, SHUTTER OK.
  - `DOME_SHUTTER`: rejected while BUSY; `d#opeshut`/`d#closhut`; failure → SHUTTER ALERT, failure message sent with an update of `DOME_STEPS`; success → BUSY "Opening shutter…"/"Closing shutter…".
  - `DOME_FLAP`: rejected while BUSY; `d#opeflap`/`d#cloflap`; failure → switches restored, ALERT "Flap open/close failed. Is the shutter open enough?" (or DOME_ERROR text); success → BUSY "Opening flap…"/"Closing flap…".

### Supported platforms, build and integration

- README: platform independent. In practice POSIX only (legacy I/O); no `.vcxproj`, no solution entry.
- Built by `Makefile.drv` auto-discovery; listed in root `UNTESTED_DRIVERS`; Xcode `dome_baader` group (sources, simulator `.c`/`.ino`) and test group. No `REFACTOR.md`, no `.driver`, no generated outputs.
- `indigo_docs/PROPERTIES.md` documents `X_EMERGENCY_CLOSE` and names the `.c` source.

### Existing simulator and tests (gaps)

`dome_baader_simulator.c` (host PTY) moves everything instantly (azimuth, shutter, flap), never reports intermediate/moving states, has no old-firmware mode, fixed emergency flags `0000`, `d#stopdom` has no effect, no fault injection, no external control and no event log. The single test is a smoke/compliance pass: no motion timing, no rotation/park/steps coverage, no abort in motion, no failure paths, no reconnect, lifecycle, multiple instances or concurrency coverage.

### Defects and risks found by source audit

Identifiers are used in the found-defects section; each is either reproduced by a dedicated reproducer against the original driver or explicitly marked audit-only.

- **BDR-01** Replies are not validated: short/garbage azimuth replies (`d#azi12x4`, partial frames) are published as azimuth values, and error replies such as `d#comerro` to `d#ser_num` are accepted as a serial number (connection succeeds).
- **BDR-02** After a failed GOTO or relative move the cached target is left at the requested azimuth: the next poll turns `DOME_HORIZONTAL_COORDINATES` BUSY and it stays BUSY forever, rejecting every later GOTO.
- **BDR-03** A dome rotated externally (hand controller, other software) is reported BUSY forever because the stale target is never reached; GOTO requests are then rejected as "Dome is moving".
- **BDR-04** After an aborted park the park request is not cleared: a later move that ends at azimuth 0 marks the dome PARKED and further GOTOs are refused.
- **BDR-05** The previous shutter position and flap state are function-`static`: all instances share them, so a second instance connected to a dome in the same state never publishes its real shutter/flap state (a default CLOSED switch remains while the shutter is open).
- **BDR-06** A failed shutter command leaves the rejected switch selected indefinitely (the poll does not refresh an unchanged position) and sends its failure message as an update of `DOME_STEPS`.
- **BDR-07** A failed park move (`d#azi0000` error) is only logged: PARK, STEPS and HORIZONTAL stay BUSY forever.
- **BDR-08** Property changes block the calling client/bus thread for their serial exchange (≥ 10 ms drain plus reply, up to 3.1 s per command with a silent device).
- **BDR-09** `DRIVER_VERSION` has nine hex digits; INFO reports version `0x20000005` (major 32) instead of 2.0.0.5.
- **BDR-10** Relative moves compute the target by truncating a float sum (`(int)(10 × (0.7f + 10))` = 106): in about 40 % of start/step combinations the dome is sent 0.1° short.
- **BDR-11** (found in review after the migration, confirmed by reproducers against both drivers) Emergency flags only light `X_EMERGENCY_CLOSE`: when the controller stops an operation because of rain, wind, operation timeout or power cut, a rotation or park stays BUSY forever (later GOTOs are rejected) and a shutter or flap request that the controller answers by closing ends OK ("Shutter closed"/"Flap closed") although the requested operation failed.
- Audit-only risks: bus handlers and the timer thread share `PRIVATE_DATA` and property states without synchronization (the port mutex protects single commands only); the shutter/flap completion is based purely on the polled position, so a stale "open" reading right after a close request would complete the operation early (not reproducible with an immediately reacting controller); a failed `d#getazim` before a relative move silently uses the cached azimuth; the write result is never checked; `indigo_global_unlock()` is called twice on detach; legacy `indigo_io` prevents a Windows build.

## Atomic plan

1. **Done — record.** This file: audit, baseline, hardware decision, plan (written before any production change).
2. **Done — simulator audit and rewrite.** `dome_baader_simulator.c` rewritten from the `.ino` semantics: elapsed-time rotation with `serial_motion.h` (shortest path and direction rule of the sketch, `--rotation-speed`, default 5 °/s like the sketch), shutter and flap travel (`--shutter-time`, `--flap-time`, default 20 s), `d#stopdom` stopping rotation, shutter and flap, reverse shutter/flap request aborting motion, `d#err_sht` below 5 % shutter, `d#mvshtNN`, `d#opefull`, `d#chk_aon`, `d#EMERWTP`, moving shutter reported `d#shut_01`…`d#shut_99`, flap `flaprop`/`flaprcl`/`flaprim`, old-firmware replies (`--old-firmware`: `d#azr` when stopped, `d#shutrun`, `d#flaprun`), start state (`--azimuth`, `--shutter`, `--flap`), frame resynchronization on a leading `d` and a 0.5 s incomplete-frame reset. Test hooks: event log `INDIGO_BAADER_EVENTS` (`RX`/`TX`/`MOVE`/`STATE`/`FAULT`/`CONTROL`/`REJECT`), fault injection `INDIGO_BAADER_FAULT` (`silent`, `error` → `d#comerro`, `domerro`, `partial` → first 5 bytes, `slow <ms>`, `reply <frame>`, `close`, with repeat count and command prefix or `ANY`) and external control `INDIGO_BAADER_CONTROL` (`azimuth`, `rotate`, `shutter`, `flap`, `eme`, `serial`). The simulator uses no threads, so the Makefile rule no longer passes `-pthread`; it now depends on `serial_motion.h`. Simulator assumptions (undocumented): azimuth argument four digits `0000`…`3600`, other argument formats rejected with `d#comerro`, linear motion. Verification: `clang -std=gnu11 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -fsyntax-only` clean; direct PTY self-checks `simulator_protocol` and `simulator_old_firmware_protocol` 2 run, 2 passed (elapsed motion, wrap, stop, shutter/flap motion and abort, err_sht, emergency flags, faults and control). These two cases are simulator verification only and are not counted as driver coverage in the mapping.
3. **Done — characterization suite against the original driver.** `indigo_test/integration/test_dome_baader_simulator.c` rewritten: every case runs in a forked child with its own simulator(s), temporary `HOME` and alarm; the parent kills the child's process group. The in-process client observes define/update/delete and `send_message` under a mutex with `force_property_updates`, tracks fresh revisions and BUSY/ALERT counts; the simulator event log provides ordered protocol assertions and status-poll synchronization. The baseline binary was compiled from the same source with `-DEXPECTED_VERSION=0x20000005 -DBAADER_REFERENCE_TRACE_PATH=\"fixtures/dome_baader/original_reference_trace.txt\"` and linked against the unchanged `build/drivers/indigo_dome_baader.a`. Harness discoveries during the first runs (test-only fixes, driver unchanged): (a) property messages reach clients only through `send_message`, not the `update_property` callback; (b) the original publishes `float` azimuths (`123.4f`), so azimuth comparisons use a 1e-4 tolerance; (c) the original does not stop the dome on disconnect and a reconnection while it still rotates hits BDR-03, so `disconnect_during_motion` reconnects after the rotation has finished; (d) the post-failure BUSY of BDR-02 makes post-failure state assertions part of the reproducers, not of the preservation cases. Baseline results are recorded in the original-driver baseline evidence section.
4. **Done — defect reproducers.** `--known-defects` runs 11 dedicated cases (BDR-01 has two). All 11 failed against the original driver (evidence below).
5. **Done — original reference trace.** `indigo_test/fixtures/dome_baader/original_reference_trace.txt` (181 lines): per step the normalized ordered simulator exchange (`S RX`/`S TX`/`S FAULT`; complete status polls collapsed to one `S POLL` line per run of identical polls with azimuth and shutter digits masked) followed by property definitions/updates/deletions and messages (`D`/`U`/`X`/`M`; BUSY azimuth values masked, repeated identical BUSY updates dropped). Steps: connect, GOTO 180, steps clockwise/counterclockwise, direction, park, GOTO and steps while parked, unpark, abort in motion (all digits masked), GOTO after abort, abort idle, failed abort, flap without shutter, shutter open, flap open, flap close, shutter close, emergency flag, disconnect. Each step starts right after a complete status poll and ends after the next complete poll. First three captures differed in one line (the idle abort republished the random stop azimuth of the previous step); a deterministic "goto 300 after abort" step was added, after which 3 of 3 captures were identical.
6. **Done — `.driver` and regeneration.** Added `indigo_dome_baader.driver` (version 6, `DRIVER_VERSION 0x03000006`), `serial;`, transactional `baader_open`/`baader_close` owning the port, the TCP URL and the global lock, `indigo_uni_io` frame helper (`indigo_uni_discard()`, `indigo_uni_vprintf()`, 100 µs pause, `indigo_uni_read_section2()` with empty terminators, 3.1 s first-byte and 0.1 s inter-byte timeout), queued property handlers, the status poll as a device-queue callback (`dome_status_poll`, first run after 0.5 s, then 1 s after each run), fixes for BDR-01…BDR-10. Generated with `build/bin/indigo_generator indigo_dome_baader.driver` (no generator warnings); `make -B -f ../../Makefile.drv` passes for x86_64 + arm64 with zero warnings. Strict check `clang -arch arm64 -std=gnu11 -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wshadow -Wformat=2 -Wunreachable-code -fsyntax-only` first reported three unreachable generated epilogues after trailing `return;` statements (abort, shutter and flap success paths); those paths were restructured to use the generated final update (shutter/flap start messages sent with `indigo_send_message()` for the property), after which the check is clean. No generator change and no `MAX_DEVICES` override.
7. **Done — post-migration suite and trace comparison.** First run against the generated driver (suite compiled with the original trace path): 26 run, 25 passed; the only failure was `reference_trace`. Every `S` (protocol) line of the trace was identical; the property differences are analysed below. `--known-defects`: 11 run, 11 passed (all fixes effective). A second capture of the generated trace revealed a migration regression: one capture completed the clockwise move at `AZ=209.9/180` instead of 210. The original stored azimuths as `float`, where 210 − 209.9f ≥ 0.1, so the dome stayed BUSY; the migrated code used `double`, where 210 − 209.9 = 0.0999… < 0.1, so a poll taken 0.1° before the target completed the move and the final 210.0 was never published. The deterministic preservation case `rotation_completes_at_target` (the simulator pauses 0.1° before the target for two polls, then finishes the move) passed against the original driver (compiled from the saved original source) and failed against the generated driver (value 209.9). Fix: arrival, park arrival and external-movement detection compare whole tenths of a degree (`baader_tenths()`, the protocol resolution, also used to format `d#aziNNNN`); after the fix the case passed against both drivers. A further capture showed a duplicated masked `DOME_HORIZONTAL_COORDINATES`/`DOME_STEPS` OK publication in the abort step (the post-abort poll can see the stopped dome at the cached target and publish OK before the abort block publishes OK again; the original can do the same). The trace normalization now keeps first occurrences of property lines in masked steps; both traces were recaptured with the final harness (original driver compiled from the saved original source): 3 of 3 identical captures for each driver. `original_reference_trace.txt` (180 lines, differs from the step 5 capture only by the removed duplicate `U DOME_SHUTTER OK CLOSED` of the abort step) and `generated_reference_trace.txt` (190 lines, the contract compared by the suite) are checked in.
8. **Done — defect fixes and generated-behaviour coverage.** All 11 reproducers pass and were promoted into the ordinary suite (the `--known-defects` mode now selects no cases). Added `urgent_abort_cancels_queued_goto` (a GOTO queued behind a running shutter handler whose reply is delayed 1.5 s is overtaken by the urgent abort: no `d#azi` reaches the device, `d#stopdom` follows the running handler, azimuth, steps and shutter end OK) and `queued_requests_survive_status_poll` (a shutter close and a flap open queued behind a status poll delayed by 1.2 s: the poll must not overwrite their requested switches, so `d#closhut` and `d#opeflap` are sent and no `d#cloflap`/second `d#opeshut`). Mutation checks on scratch copies of the generated source: without the queued-request guard in the poll, `queued_requests_survive_status_poll` failed (`d#closhut` count 0); without cancellation of the queued GOTO handler, `urgent_abort_cancels_queued_goto` first passed because the queued handler sent `d#azi0900` (the settled target) instead of `d#azi1800`; the assertion was strengthened to "no `d#azi` command at all", after which the mutant failed and the real driver passed.
8a. **Done — second migration regression (sanitizer run).** The first ASan/UBSan run (40 run, 38 passed, no sanitizer report) failed `moves_refused_when_parked` and `busy_requests_rejected`. The first was a harness race: a property message arrives through a separate `send_message` callback right after the ALERT update, so `message_seen()` now waits up to 2 s. The second was a real migration defect: the generated handler prologue sets the property OK before the user code runs, so while `d#opeshut` (or `d#azi`, `d#opeflap`) waited for its reply the framework BUSY guard was open and a second request was accepted and queued; the debug log showed "Opening shutter..." followed by "Closing shutter...", `d#closhut` aborted the shutter at 50 % and `DOME_SHUTTER` stayed BUSY. The original driver handled requests synchronously and had no such window. Added the deterministic case `busy_guard_holds_while_command_runs` (device replies to `d#azi`, `d#opeshut` and `d#opeflap` delayed by 0.8 s, second GOTO/steps, shutter and flap requests sent while the first command runs); it passed against the original driver and failed against the generated driver (azimuth 300 instead of 120). Fix: the GOTO, steps, park, shutter and flap `on_change` blocks start with `<PROPERTY>->state = INDIGO_BUSY_STATE;`, which suppresses the generated OK prologue, and set OK/ALERT/BUSY explicitly on every path; after regeneration the case passes. Park requests during an active rotation remain accepted (retarget to 0°) in both drivers.
8b. **Environment change outside this work.** During final verification the whole `indigo_libs/` directory appeared as a staged move to `indigo_optional_drivers/indigo_libs/` in the working tree (1446 staged additions, 1468 deletions). It was not made by this refactoring and was left untouched. Because `Makefile.drv` and `indigo_test/Makefile` include `../indigo_libs`, the final driver archive, the normal test binary, the original-driver comparison binary and the sanitizer binary were built with the same compiler flags but with the include path redirected to `indigo_optional_drivers/indigo_libs`; `build/lib/libindigo` was not rebuilt.
8c. **Done — final trace normalization.** A final 3-capture stability check of the generated driver differed in one masked line of the abort step (`AZ=#.#/#` versus `AZ=#/#`: the dome stopped on a whole degree). Masked steps now drop the fractional separator of masked numbers; the same transformation was applied to the single affected line of both fixtures. `reference_trace` then passed 4 of 4 against the generated driver and 2 of 2 against the original driver. Only test-harness normalization changed.
9. **Done — repository integration.** `indigo.xcodeproj/project.pbxproj`: `REFACTOR.md` and `indigo_dome_baader.driver` in the `dome_baader` group, new `fixtures/dome_baader` group with both trace files; `plutil -lint` passes. Windows: `indigo_dome_baader.vcxproj` (+ `.filters`, `.user`, copied from `dome_skyroof` with a new project GUID, UTF-8 BOM and CRLF like the source; `.driver` and `REFACTOR.md` as `None` items) and the `indigo_windows.sln` project and configuration entries; `xmllint` passes; no Windows build was possible. `indigo_test/Makefile`: simulator dependency on `serial_motion.h` without `-pthread`, test dependencies on both fixtures and the driver archive, `test-dome-baader-simulator`, `test-dome-baader-simulator-network` (opt-in, step 11) and `test-dome-baader-simulator-sanitize` (arm64 ASan + UBSan with the generated driver source compiled into the test). `indigo_docs/PROPERTIES.md`: source now names the `.driver` (property set unchanged). `MIGRATION_STATUS.md`: API `3️⃣`, Windows, generator and async queues `✅ Yes`, retested `✅ Sim`, tests `46 / 0` (43 default cases and 3 opt-in network cases); Comment column unchanged. `README.md` unchanged.
10. **Done — final verification.** Evidence below. Linux and Windows builds and physical hardware were unavailable and are not claimed.
11. **Done — opt-in TCP transport coverage.** At the user's request the network transport was tested outside the sandbox. The simulator gained `--tcp-port PORT` (loopback listener serving the same protocol to one client, `0` selects a free port, ready file adds `INDIGO_SIMULATOR_TCP_URL`) and a `drop` control that closes the client connection; the PTY path is unchanged in behaviour (the main loop now multiplexes both sources with a frame buffer per source). Three cases run only with `--network` (make target `test-dome-baader-simulator-network`), so the normal integration target still opens no sockets: `network_baader_and_tcp_urls` (`baader://127.0.0.1:<port>` and `tcp://127.0.0.1:<port>`: identification, GOTO, shutter, disconnect closes the socket, reconnect, descriptor balance), `network_default_port` (`baader://127.0.0.1` uses port 8080) and `network_failures_and_transport_loss` (refused connection and silent identification end in ALERT with balanced descriptors; after the controller drops the connection the driver sends nothing further, publishes no false state, reports a GOTO failure as ALERT, disconnects without hanging and reconnects). Results: generated driver 3 run, 3 passed in each of three runs and 3 run, 3 passed under ASan/UBSan; original driver (saved pre-migration source) 3 run, 3 passed, so the transport behaviour is preserved.
12. **Done — emergency close ends operations in ALERT (BDR-11).** Raised in user review of the remaining limitations: an operation stopped by an emergency must end in ALERT, not stay BUSY. Decisions confirmed by the user: use the protocol's emergency flags (no stall timeout), and a shutter/flap request stopped by an emergency stays ALERT, with switches following the real position, until the next request on that property. Simulator: a newly raised emergency flag (control `eme` or `d#EMERWTP`) now performs an emergency close — rotation stops, flap and shutter close (simulator assumption, the `.ino` sketch only stores the flags); `simulator_protocol` verifies it. Reproducers `BDR-11 emergency_close_alerts_rotation_and_park` (GOTO and park stopped by rain/wind) and `BDR-11 emergency_close_alerts_shutter_and_flap` (flap open and shutter open stopped by operation timeout/power cut) failed against the original driver and against the generated driver before the fix (rotation did not settle in ALERT; flap settled OK). Fix in the status poll: each GOTO, relative move, park, shutter and flap request records the emergency flags known when it starts; a flag raised afterwards while the operation is active publishes ALERT with "Emergency close: <flags>" (rotation: azimuth and steps, park ALERT/UNPARKED when a park was requested; shutter/flap: switches from the polled position), and later position changes update the switches without leaving ALERT until the next request or an abort. Flags already active when an operation starts do not trigger ALERT. After regeneration both reproducers pass; strict check clean.

## Original-driver baseline evidence

Environment: macOS 26 arm64; normal test build is the repository universal x86_64/arm64 configuration.

- Characterization suite (`test_dome_baader_simulator_original`): 26 run, 25 passed on the first complete run after the harness fixes (a) and (c); the failure was `connect_unparked_open_shutter` (harness tolerance (b)); after the tolerance fix the case passed 1 of 1. All other 25 cases passed in that run, including `reference_trace` against the checked-in original fixture.
- `--known-defects`: 11 run, 11 failed as expected:
  - BDR-01 (azimuth): the poll published the malformed `d#azi12x4` reply (DOME_HORIZONTAL_COORDINATES revision 1 → 3).
  - BDR-01 (serial): CONNECTION settled OK after `d#comerro` answered `d#ser_num`.
  - BDR-02: DOME_HORIZONTAL_COORDINATES was BUSY (2) instead of ALERT three polls after a failed GOTO.
  - BDR-03: DOME_HORIZONTAL_COORDINATES was BUSY (2) three polls after an external move to 120°.
  - BDR-04: after an aborted park and a GOTO to 0 the dome was reported PARKED.
  - BDR-05: the second instance connected to an open dome still showed DOME_SHUTTER CLOSED.
  - BDR-06: after a failed `d#opeshut` the rejected OPENED switch remained selected.
  - BDR-07: DOME_PARK did not settle in ALERT after a failed park move (stayed BUSY).
  - BDR-08: the shutter change request returned after 1.017 s (device reply delayed by 1 s).
  - BDR-09: INFO version `0x20000005`.
  - BDR-10: a 10° clockwise move from 0.7° sent no `d#azi0107` (it sent `d#azi0106`).

## Post-migration evidence

Environment as for the baseline.

- Reproducible generation: the generator was run twice more after the final `.driver` edit; SHA-1 of the checked-in outputs unchanged: `.c` `2a9c6a876697a19d188c317d5270cb306287828c`, `.h` `58b70eeeed56f85484a6d4e5df1fe15f0dc4d511`, `_main.c` `9a5bacdb58f518262413a4c23a8e165adfbea2e2`. No generator warnings.
- `make -B -f ../../Makefile.drv` in this directory: passed, x86_64 + arm64, zero warnings, before the external `indigo_libs` move (step 8b); the final archive after step 8a was built with the identical compile command and redirected include path, zero warnings. Strict syntax check (step 6 flags) clean on the final source.
- Final driver, normal build (`./build/integration/test_dome_baader_simulator` from `indigo_test`): 41 run, 41 passed (5 min 21 s); second complete run 41 run, 41 passed (5 min 25 s). Earlier complete run before step 8a: 40 run, 40 passed, twice.
- ASan + UBSan (arm64, `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`, command of `test-dome-baader-simulator-sanitize` with the redirected include path of step 8b): 41 run, 41 passed, no sanitizer report. Instrumentation covers the test and the driver; `libindigo` is not instrumented. The earlier sanitizer run before step 8a: 40 run, 38 passed (step 8a).
- Original driver compiled from the saved pre-migration source with the final harness: 41 run, 29 passed, 12 failed — exactly the 11 promoted defect reproducers and `urgent_abort_cancels_queued_goto` (the original has no queue, so the GOTO request is sent before the abort); all preservation cases including `reference_trace`, `rotation_completes_at_target`, `busy_guard_holds_while_command_runs` and `queued_requests_survive_status_poll` passed.
- `reference_trace` after step 8c: generated 4 run, 4 passed; original 2 run, 2 passed.
- Opt-in TCP transport (step 11, run outside the sandbox): generated 3/3 in three runs and 3/3 under ASan/UBSan (no sanitizer report); original 3/3. After the simulator main-loop change one complete default run was also made: 41 run, 41 passed.
- Emergency close (step 12): complete default suite 43 run, 43 passed (6 min 12 s); ASan/UBSan for the affected cases (`BDR-11` ×2, `emergency_flags`, `reference_trace`) 4 run, 4 passed, no sanitizer report; original driver `BDR-11` 2 run, 0 passed (expected) and `emergency_flags` 1/1. Mutation check: without the "ALERT until the next request" rule the shutter/flap reproducer failed (flap OK instead of ALERT). The generator was run twice more after the final `.driver` edit with unchanged output (hash above). The opt-in network cases were not rerun because the transport code did not change.
- `git diff --check`: clean except the CRLF line endings of the added `indigo_windows.sln` lines, which match the existing file. Formatting audit of the `.driver`, simulator and test sources: tab indentation, no trailing whitespace, no blank lines inside function bodies.
- Unavailable: Linux (x64/arm/arm64) and Windows builds, physical Baader dome.

## Reference trace comparison

`original_reference_trace.txt` (180 lines) and `generated_reference_trace.txt` (190 lines) differ in 26 diff lines. Every `S` (protocol) line is identical: connection sequence, first poll timing position, every `d#azi` argument, `d#getazim` before relative moves and parked GOTO, `d#stopdom`, shutter and flap commands, the failed abort and the flap `d#err_sht` reply, and the collapsed status polls in every step. All differences are property publications:

1. Queued changes publish BUSY before the handler runs (`INDIGO_COPY_VALUES_PROCESS_CHANGE` / `…_URGENT_CHANGE`): additional BUSY lines for `DOME_PARK` (park and unpark), `DOME_HORIZONTAL_COORDINATES` (GOTO while parked), `DOME_STEPS` (steps while parked), `DOME_ABORT_MOTION` (three abort steps) and `DOME_FLAP` (flap without shutter).
2. Because the framework publishes the requested property BUSY first and repeated identical BUSY lines are dropped, the first BUSY lines of GOTO appear as `DOME_HORIZONTAL_COORDINATES`, `DOME_STEPS` instead of `DOME_STEPS`, `DOME_HORIZONTAL_COORDINATES`, and of relative moves as `DOME_STEPS`, `DOME_HORIZONTAL_COORDINATES` instead of the reverse. Device commands and completion order are unchanged.
3. `X_EMERGENCY_CLOSE` is defined by generated code after the connection sequence (after the `DOME_PARK` update) instead of before `d#getazim`; generated `OK` messages "Connected to Baader Classic Dome on <port>" and "Disconnected from Baader Classic Dome" are added.
4. A successful abort publishes `DOME_SHUTTER` OK before `DOME_ABORT_MOTION` OK (the generated final update of the handler publishes the abort property last).

Not visible in the normalized trace but intentionally changed: a failed connection reports the generated "Failed to connect to …" message instead of the CONNECTION update message "Baader dome did not respond" (still logged); the shutter and flap start messages are sent with `indigo_send_message()` for the property before the handler's BUSY update instead of with that update.

## Intentional behaviour differences

- **Transport:** `indigo_uni_io` replaces legacy file-descriptor I/O (Windows project added). The frame exchange keeps the original manner: drain, write, 100 µs pause, read 9 bytes with 3.1 s first-byte and 0.1 s inter-byte timeouts; `baader://` and `tcp://` URLs open TCP with default port 8080.
- **Serialization:** property handlers and the status poll run on the device queue; the port mutex and timer thread were removed. Property changes no longer block the caller (BDR-08). The status poll still sends `d#getazim`, `d#getshut`, `d#getflap`, `d#get_eme` in every run, 0.5 s after connection and 1 s after each run, but defers publishing a property whose request is queued behind it.
- **Reply validation (BDR-01):** replies must be complete 9-byte frames starting with `d#`; azimuth `d#az?NNNN` with four digits 0…3600 (3600 is 0), shutter `d#shut_NN` with two digits, flap states exact, emergency flags `0`/`1`, serial number not a status reply (`d#comerro`, `d#gotmess`, `d#err_sht`). Invalid replies are logged and ignored like other read failures.
- **Azimuth arithmetic:** commands, relative targets and arrival use whole tenths of a degree (BDR-10, migration regression of step 7); absolute GOTO rounds to the nearest tenth instead of truncating a `float` (identical for 0.1° grid values).
- **Failure recovery:** a failed GOTO or relative move leaves the cached target unchanged and publishes the current azimuth with ALERT (BDR-02); a failed park publishes `DOME_PARK` ALERT with the GOTO failure message and does not start the rotation state (BDR-07); a failed shutter request restores the switches from the last polled shutter position and reports on `DOME_SHUTTER` (BDR-06); a failed flap request restores the switches from the last polled flap state (the original restored the pre-request switches; identical while the published switches reflect the device).
- **External movement (BDR-03):** while no rotation is active, a changed azimuth is published with the property state unchanged and becomes the cached target.
- **Abort:** runs at urgent priority, cancels queued GOTO, steps, park, shutter and flap handlers and settles the properties they left BUSY (queued park: ALERT, UNPARKED), always clears a pending park request (BDR-04), then sends `d#stopdom` as before.
- **Per-device state (BDR-05):** the previous shutter position and flap state are per device and reset at connection, so the first successful poll after connection always publishes shutter and flap.
- **Busy requests:** handlers keep their property BUSY until they publish the result; GOTO, shutter and flap requests while their property is BUSY are ignored by the framework guard without the original "… request can not be completed" message; a relative move while `DOME_HORIZONTAL_COORDINATES` is BUSY is still rejected with "Dome is moving: request can not be completed"; a park request while `DOME_PARK` is BUSY is ignored (the original re-sent `d#azi0000`).
- **Emergency close (BDR-11):** a newly raised emergency flag during an active rotation, park, shutter or flap operation ends it in ALERT with an "Emergency close: …" message; a stopped shutter or flap request stays ALERT until the next request on that property or an abort. The emergency flags are reset at connection.
- **Version (BDR-09):** `0x03000006`. The original constant `0x020000005` was numerically `0x20000005`; the intended 2.0 build 5 is lower than the new version.
- **Labels:** typos fixed in `X_EMERGENCY_CLOSE` ("Emergency close flags", "Power outage alert"); names, items, group and permissions unchanged.
- **Detach:** the redundant second global unlock of the original detach was removed; the lock is owned by `baader_open`/`baader_close`.

## Found defects

Every defect below was reproduced by a dedicated case that failed against the original driver (evidence in the baseline section) and passes against the generated driver. The migration regression found in step 7 is not an original defect and is listed separately.

| ID | Observable impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- |
| BDR-01 | A malformed azimuth reply (`d#azi12x4`) is published as 1.2° (and turns the property BUSY); `d#comerro` to `d#ser_num` connects with serial number "comerro". | `sscanf()` prefixes accept trailing garbage and short reads; any `d#…` reply accepted as serial. | Exact frame and payload validation. | `BDR-01 malformed_azimuth_rejected`, `BDR-01 serial_error_reply_rejected` |
| BDR-02 | After a failed GOTO or relative move the dome is reported BUSY forever and every later GOTO is rejected. | Cached target set to the requested azimuth before the command and never restored. | Target stored only after the command is accepted; ALERT publishes the current azimuth. | `BDR-02 failed_goto_does_not_stay_busy` |
| BDR-03 | An externally rotated dome is reported BUSY forever; GOTO requests are rejected. | The poll treats any difference to the stale target as motion. | Rotation completion only for an active driver move; otherwise the new azimuth is published and becomes the target. | `BDR-03 external_rotation_is_not_busy` |
| BDR-04 | After an aborted park, a later move ending at 0° marks the dome PARKED and refuses further moves. | Abort did not clear the park request. | Abort clears the park request. | `BDR-04 aborted_park_is_cleared` |
| BDR-05 | A second instance never publishes the real shutter/flap state when it equals the first instance's (CLOSED shown for an open shutter). | Previous shutter position and flap state were function-`static`. | Per-device state reset at connection. | `BDR-05 instances_do_not_share_state` |
| BDR-06 | A failed shutter request leaves the rejected switch selected indefinitely; its message is sent on `DOME_STEPS`. | Values copied before the command and never restored; wrong property in the failure update. | Switches restored from the polled position; message on `DOME_SHUTTER`. | `BDR-06 failed_shutter_request_restores_switch` |
| BDR-07 | A failed park move leaves PARK, STEPS and HORIZONTAL BUSY forever. | GOTO failure only logged. | PARK ALERT with message; rotation not started. | `BDR-07 failed_park_alerts` |
| BDR-08 | Property changes block the calling client/bus thread for the serial exchange (1.017 s with a 1 s reply delay). | Synchronous handlers on the bus thread. | Queued generated handlers. | `BDR-08 property_change_does_not_block` |
| BDR-09 | INFO reports version `0x20000005` (major 32). | Nine hex digits in `DRIVER_VERSION`. | Generated `0x03000006`. | `BDR-09 version_is_valid` |
| BDR-11 | A rotation or park stopped by an emergency close stays BUSY forever; a shutter/flap request answered by an emergency close ends OK. | Emergency flags only updated `X_EMERGENCY_CLOSE`; completion was based on position only. | Flags raised during an active operation end it in ALERT with a message; shutter/flap stay ALERT until the next request. | `BDR-11 emergency_close_alerts_rotation_and_park`, `BDR-11 emergency_close_alerts_shutter_and_flap` |
| BDR-10 | Relative moves go 0.1° short for many start/step combinations (0.7° + 10° → `d#azi0106`). | Truncating conversion of a `float` sum. | Integer tenths arithmetic. | `BDR-10 relative_move_is_exact` |

Migration regressions found and fixed (not original defects): rotation completed 0.1° before the target with `double` azimuths and the final azimuth was not published (step 7, whole-tenth comparison, `rotation_completes_at_target`); the generated OK prologue opened the BUSY guard while a device command was running, so overlapping GOTO, steps, shutter and flap requests were accepted (step 8a, handlers keep their property BUSY, `busy_guard_holds_while_command_runs`). Both cases pass against the original and the generated driver.

Audit-only risks resolved by the migration without a dedicated reproducer: unsynchronized access to `PRIVATE_DATA` and property states between bus handlers and the timer thread; unchecked write results; double global unlock on detach; non-portable I/O preventing a Windows build.

Remaining known limitations (not changed): the driver has no stall or timeout detection, so a rotation, shutter or flap move that stops short of its end position without a newly raised emergency flag (for example a mechanical obstruction, an emergency flag that was already active when the operation started, or an old-firmware flap stopped midway which still reports `d#flaprun`) stays BUSY until aborted; the real controller's reaction to emergency flags (which motions it stops or closes) is a simulator assumption; shutter and flap completion is decided from the polled position only, so a stale "open" reading immediately after a close request would complete early (not reproducible with an immediately reacting controller); a failed `d#getazim` before a relative move still uses the cached azimuth; the dome is not stopped on disconnect; the park position is fixed at 0°; the serial line is assumed 9600 8N1; real firmware replies beyond the `.ino` sketch, the serial number format and network-attached Baader controllers are unverified (the TCP transport is covered only against the loopback simulator).

## Scenario-to-test mapping

Dome class checklist (`indigo_test/DRIVER_TESTING_RULES.md`) and shared scope:

| Area | Cases |
| --- | --- |
| Simulator protocol self-check (not driver coverage) | `simulator_protocol`, `simulator_old_firmware_protocol` |
| Metadata, INFO, interface bit, common visible properties, no dome properties and no I/O before connection | `metadata_before_connection`, BDR-09 |
| Connection sequence, first poll, INFO serial number and count, visible/hidden dome properties, azimuth/steps ranges, RW coordinates, park state from azimuth, shutter/flap initial state, emergency property | `connect_parked_at_zero`, `connect_unparked_open_shutter`, `reference_trace` |
| Open/identification failures, silent device, descriptor balance, recovery | `connection_failures_and_recovery`, BDR-01 (serial) |
| Absolute GOTO BUSY→OK with elapsed motion, shortest path across 0°, 0.1° targets, repeated GOTO | `goto_and_wrap`, `rotation_completes_at_target` |
| Relative moves clockwise/counterclockwise, wrap below 0°, fractional steps, tenth arithmetic | `relative_steps`, BDR-10 |
| Parked dome refuses GOTO and steps | `moves_refused_when_parked` |
| Park/unpark (logical unpark without command, repeated park) | `park_and_unpark` |
| Abort while rotating and idle, fresh move afterwards, aborted park | `abort_rotation`, `abort_park`, BDR-04 |
| Shutter open/close BUSY→OK with messages | `shutter_open_close` |
| Flap refused below 5 % shutter, flap open/close | `flap_requires_shutter_and_moves` |
| Abort of moving shutter and flap | `abort_shutter_and_flap` |
| Old firmware replies (`d#azr`, `d#shutrun`, `d#flaprun`) | `old_firmware_replies` |
| Emergency flags, change-only publication, malformed flags | `emergency_flags` |
| Emergency close during rotation, park, shutter and flap operations (ALERT, message, ALERT kept until the next request) | BDR-11 (2 cases) |
| Command failures and messages (GOTO, steps, park, abort, shutter, flap; `d#comerro`, `d#domerro`, partial replies) | `goto_failure_messages`, `steps_failure_messages`, `abort_and_flap_failures`, BDR-02, BDR-06, BDR-07 |
| Poll read failures (error, partial, silent) without false publications, recovery | `poll_read_failures`, BDR-01 (azimuth) |
| External rotation | BDR-03 |
| BUSY conflicts (GOTO, steps, shutter, flap), including requests arriving while the first device command is still running | `busy_requests_rejected`, `busy_guard_holds_while_command_runs` |
| Queued-handler races: urgent abort overtaking a queued GOTO, poll racing queued shutter/flap requests | `urgent_abort_cancels_queued_goto`, `queued_requests_survive_status_poll` |
| Non-blocking property changes | BDR-08 |
| Disconnect during motion, no I/O after close, reconnect | `disconnect_during_motion` |
| INIT/SHUTDOWN idempotence, shutdown refused while connected, redundant disconnect, repeated connect cycles | `lifecycle_and_shutdown` |
| Additional instance with its own port and state | `additional_instance`, BDR-05 |
| Ordered protocol/property compatibility contract | `reference_trace` |
| Network transport (opt-in `--network`): `baader://` and `tcp://` URLs, default port 8080, refused connection, silent device, transport loss and recovery | `network_baader_and_tcp_urls`, `network_default_port`, `network_failures_and_transport_loss` |

Not applicable or not covered: hot plug and multiple physical devices (serial, no enumeration); guider timing (no guider interface, so no guiding-pulse measurement applies); driver-owned persistent settings (the driver has none; `DOME_SLAVING_PARAMETERS`, `DOME_DIMENSION` and geographic coordinates are saved by the dome base class); `DOME_SPEED`, `DOME_ON_COORDINATES_SET`, `DOME_PARK_POSITION`, `DOME_HOME`, UTC properties (hidden or not implemented by the driver); transport loss during active work on real hardware; hardware acceptance (no device).

## Final test summary

Counts include development runs; a registered case run inside a complete suite run counts once per run. Trace captures (`--capture-trace`) count as runs of `reference_trace`.

- Simulated tests, original driver: 177 run, 128 passed. Breakdown: pre-existing smoke test 1/1; simulator self-checks 2/2; first complete characterization run 26 run, 16 passed (10 harness failures, see step 3) and 8 reruns of those cases 8/8; defect reproducers 11 run, 0 passed (all failed as expected); trace captures 6/6 (step 5) and 3/3 (step 7); baseline complete run 26 run, 25 passed and the tolerance rerun 1/1; `rotation_completes_at_target` 2/2; `busy_guard_holds_while_command_runs` 2 run, 1 passed (the first variant wrongly expected a park request to be rejected); complete runs with the final harness 40 run, 28 passed and 41 run, 29 passed (12 expected failures each: 11 reproducers and the urgent-abort queue case); `reference_trace` 2/2 after step 8c; network cases 3/3 (step 11); `BDR-11` 2 run, 0 passed (expected) and `emergency_flags` 1/1 (step 12).
- Simulated tests, generated driver: 414 run, 404 passed. Breakdown: first comparison run 26 run, 25 passed (trace differences analysed) and reproducers 11/11; queue cases 2/2 and 1/1 after strengthening; trace captures 3/3, 3/3 and 3/3; `rotation_completes_at_target` 1 run, 0 passed before and 1/1 after the step 7 fix; complete runs 40/40 twice; first sanitizer run 40 run, 38 passed and 2 sanitizer debug reruns failed (step 8a); `busy_guard_holds_while_command_runs` 2 run, 0 passed before the fix and the 3 affected cases 3/3 after it; final trace captures 3/3; final complete runs 41/41 twice; final sanitizer run 41/41; `reference_trace` 4/4 after step 8c; network cases 3/3 in three runs and 3/3 under the sanitizer, and one complete default run 41/41 after the simulator change (step 11); step 12: before the fix `simulator_protocol` 1/1, `emergency_flags` 1/1 and `BDR-11` 2 run, 0 passed, after the fix `BDR-11` 2/2, the complete default suite 43/43 and 4/4 affected cases under the sanitizer.
- Mutation checks (not counted above): 6 runs against deliberately broken scratch copies of the generated driver; 4 failed as intended, 1 passed and exposed a too-weak assertion that was then strengthened, and 1 unrelated case selected by the same name filter passed.
- Hardware tests: 0 run, 0 passed.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard are now declared with the generator's `reject_change` block. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values instead of an `INDIGO_OK_STATE` update carrying no items, which left the refused value visible in the client.

Covered by the `rejected_change` scenario in `indigo_test/integration/test_dome_baader_simulator.c`: while `DOME_HORIZONTAL_COORDINATES` is BUSY, `DOME_STEPS` ends in ALERT with unchanged value and target, and is accepted again after the abort.

```sh
cd indigo_test && BAADER_TEST_FILTER=rejected_change ./build/integration/test_dome_baader_simulator
```

## Rejected DOME_STEPS change could be lost (2026-09-20)

The driver had the same unsynchronised write to `DOME_STEPS_PROPERTY->state` that was found in
dome_beaver: the generated `reject_change` branch runs on the bus thread inside `change_property`,
while `DOME_HORIZONTAL_COORDINATES.on_change` claimed `DOME_STEPS` as BUSY from the device queue. A
rejection arriving in that window is overwritten and never becomes observable.

Unlike dome_beaver the defect did not reproduce here, because the claim came after
`baader_goto_azimuth()` and its serial round-trip gave the rejection time to publish first. That is
timing, not design, so the fix was applied identically: the claim moved into `on_change_request`,
which runs on the bus thread like the rejection branch. Rotation still owns `DOME_STEPS` while it
runs, and the remaining writes on that path only ever set ALERT.

`fixtures/dome_baader/generated_reference_trace.txt` was regenerated: `U DOME_STEPS BUSY` now
precedes `U DOME_HORIZONTAL_COORDINATES BUSY` in three places. No serial command changed.

44/44 simulator scenarios pass.
