# dome_talon6ror refactoring record

## Scope and baseline

This record covers migration of `indigo_dome_talon6ror` (Talon6 RoR roll-off roof controller by Observatorios SPAG) from its hand-written INDIGO 2.0 implementation (legacy `indigo_io`, POSIX `select`/`read`/`close`, `pthread` mutex, timer threads, blocking handlers) to `indigo_generator` with portable `indigo_uni_io` and the device handler queue, the production fixes proven by regression tests, and complete applicable hardware-free simulator coverage.

Baseline date and source: 2026-09-17, commit `73f9eadc2` (`dome_nexdome3: refactored to code generator`), branch `refactoring`. Working tree: clean except an unrelated `MIGRATION_STATUS.md` change of the `aux_joystick` row that was not made by this work and is left untouched. Host: macOS 26 (`Darwin 25.6.0`), Apple Silicon arm64; repository universal build (x86_64 + arm64).

Baseline build command:

```sh
cd indigo_drivers/dome_talon6ror
make -B -f ../../Makefile.drv
```

Result: passed with 0 lines containing `warning`; object, archive, dynamic library and executable built for x86_64 and arm64.

Baseline automated tests:

```sh
make -C indigo_test build/integration/test_dome_talon6ror_simulator
cd indigo_test && ./build/integration/test_dome_talon6ror_simulator
```

Result: 1 run, 1 passed (`talon6ror_passes_serial_compliance_checks`, 11.0 s). `MIGRATION_STATUS.md` records `1 / 0`.

`build/bin/indigo_generator` is present in the build tree (built from the unchanged `indigo_tools/indigo_generator.c`).

The original `indigo_dome_talon6ror.c` and `.h` of commit `73f9eadc2` were copied to the session scratch directory before any change; they are used for the original-driver baseline binary (see "Original-driver baseline binary").

## Hardware-test decision

No hardware testing will be performed. The user has no Talon6 controller (no physical hardware is available). No statement in this record implies physical validation; all automated evidence is simulator-backed software behaviour.

## Studied sources

- `indigo_dome_talon6ror.c`, `.h`, `_main.c`, `README.md` in this directory (no Windows project exists).
- `dome_talon6ror_simulator/dome_talon6ror_simulator.ino` (Arduino simulator by the driver author: `&V%#`, `&G%#`, `&O%#`, `&C%#`, `&P%#`, `&S%#`, `&p%#`, `&a…%#`, `&ERROR#` for unknown commands, 21-byte status with checksum, 55-byte configuration, status state 4 when stopped between the end positions, last action 13 after `S`) and the host PTY simulator `dome_talon6ror_simulator.c`.
- No manufacturer protocol document is bundled. Public references consulted (not copied into the repository): the Talon6 RoR user manual 2019 (observatoriosspag.es: roof states Open/Closed/Opening/Closing/Error, last-action codes 1…14 — code 13 is "Stop. Motor Stalled", 14 "Stop: Emergency" —, safety conditions that prevent opening, closing that first orders the mount to park and waits "Del Park" seconds (0 = until parked), timeout closing countdown in seconds, keypad OPEN/STOP/CLOSE, MGM and COM inputs, encoder ticks for 100 % open, motor setup parameters), the INDI `indi-talon6` driver (status byte layout: state/last action, 3-byte position, 2-byte voltage in 15/1024 V, 3-byte closing timer, 2-byte power and weather timers, switch byte with close/COM/MGM bits and sensor byte with PWL/CWL/MAP/ROP/RCL/OPEN/STOP bits; INDI frames its commands without `%`) and the `telescopio-montemayor/roof-control` Talon6-compatible firmware library (same status layout). Where these sources and the INDIGO driver differ only in framing, the INDIGO driver and its `.ino` are taken as the contract for this driver.
- `indigo_test/integration/test_dome_talon6ror_simulator.c`, `indigo_test/Makefile`, `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (shared scope, dome section), `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`.
- `indigo_libs/indigo_dome_driver.c` (disconnect resets `DOME_SHUTTER` and other dome property states to OK before deleting them), `indigo_libs/indigo_io.c` (`indigo_open_serial()` 9600 8N1, `VMIN 0`, `VTIME 50`: a read returns 0 after 5 s of silence), `indigo_libs/indigo_bus.c` (`indigo_send_message()` with a NULL format sends an empty message), `indigo_libs/indigo_uni_io.c` (`indigo_uni_read_section2()`, `indigo_uni_discard()`, `indigo_uni_vprintf()`), `indigo_tools/indigo_generator.c` output of sibling drivers.
- Sibling migrations `dome_baader` (serial request/response dome with status poll on the device queue, harness, simulator hooks, reference trace method), `dome_skyroof` (generated roll-off roof), `dome_nexdome3`.

## Current-state audit

### Protocol

Serial 9600 8N1. Every command is `&<payload>%#`; every reply is `&<payload>#`. Payload bytes other than the leading letter carry 7-bit values with bit 7 set, so they never collide with `&`, `%` or `#`. Values are packed MSB first in 7-bit groups (3 bytes: 21 bits, 2 bytes: 14 bits). Checksums: `0x80 | -(sum % 128)` over the payload bytes after the letter.

| Command | Reply | Driver use |
| --- | --- | --- |
| `V` | `&V3.20N--#` (letter + 7 character firmware version) | connection handshake, INFO firmware revision |
| `p` | `&p` + 54 configuration bytes + checksum `#` | connection, configuration properties |
| `a` + 54 bytes + checksum | `&#` (`.ino`) | `X_MOTOR_CONF`, `X_DELAY_CONF`, `X_CLOSE_COND` writes |
| `G` | `&G` + 20 status bytes + checksum `#` | 0.5 s status poll |
| `O` | `&#` | open roof |
| `P` | `&#` | close roof (with mount park, per manual) |
| `S` | `&#` | abort |
| `C`, `A` | `&#` | close without park, go to percentage (unused by the driver) |
| unknown | `&ERROR#` (`.ino`) | |

Status bytes after `G` (driver indices, 1-based): 1 state (bits 6…4: 0 open, 1 closed, 2 opening, 3 closing, 4 error/stopped between ends) and last action (bits 3…0); 2…4 position (encoder ticks); 5…6 voltage ADU (15/1024 V); 7…9 timeout closing timer (s); 10…11 power condition timer; 12…13 weather condition timer; 14 switches (bit 0 close button, bit 1 COM input, bit 2 MGM input); 15 sensors (bit 0 power lost, 1 weather condition, 2 mount at park, 3 roof open, 4 roof closed, 5 open button, 6 stop button); 16…20 not used by the driver; 21 checksum.

Configuration bytes after `p` (1-based): 1 KP, 4 KI, 7 KD, 10 maximum speed, 13 minimum speed, 16 acceleration, 19 park delay, 22 weather delay, 25 power delay, 28 communication delay, 31 maximum aperture, 34 IP switch, 37 full travel encoder ticks, 40 deceleration ramp, 43 timeout delay (all 3 bytes), 46 encoder factor, 47 reverse (low nibble), 48 close conditions (bit 0 power, 1 weather, 2 timeout), 49 dummy (3 bytes), 52 TW time (3 bytes), 55 checksum.

### Architecture and implementation

- `DRIVER_VERSION 0x02000002`, device and label `Talon6 ROR`, author Peter Polakovic. One dome device, `ADDITIONAL_INSTANCES` supported, no hot plug, serial only (no network URL). Listed in root `STABLE_DRIVERS`, registered in `indigo_server`. POSIX-only I/O; no Windows project.
- Transport: `talon6ror_command()` locks a `pthread` mutex, writes `&<command>%#` with `indigo_printf()` (command passed as a C string, so the configuration frame ends at the first zero byte), then reads byte by byte with `indigo_read()` (5 s per byte): bytes before the first `&` are skipped, further `&` bytes are dropped, `#` ends the reply, at most 64 payload bytes. Success means a `#` terminated reply — the payload (for example `ERROR`) is not checked. A reply of 64 or more bytes without `#` leaves the buffer unterminated, and `dump_hex()` (debug formatting into a static 192-byte buffer) then reads and writes past its buffers.
- Connection (timer thread, `indigo_set_timer(…, 0, dome_connect_handler)`): `indigo_open_serial()`; flush that waits up to 5 s for a first byte and then discards bytes until 10 ms of silence (a controller banner after the Arduino reset); `V` (reply letter checked, 7 characters copied to INFO firmware revision, INFO updated) or ALERT message "Handshake failed"; `p` (letter and checksum checked, configuration copied into the write buffer with letter `a`, item values unpacked) or messages "Checksum error, handshake failed" / "Handshake failed"; on success define the seven custom properties, CONNECTION OK, status timer at 0 s; on failure close, CONNECTION ALERT with DISCONNECTED selected. No global device lock.
- Status timer (`talon6ror_get_status`, timer thread, rescheduled 0.5 s after each run): `G`; on a valid checksum publishes:
  - `DOME_SHUTTER`: state 0 → OPENED OK "Roof opened", 1 → CLOSED OK "Roof closed", 2 → OPENED BUSY "Roof opening", 3 → CLOSED BUSY "Roof closing", each only when the state or the selected switch differs; other → ALERT "Error reported" when not ALERT.
  - Last action text sent as an `IDLE_PROPERTY` message when the text pointer changed (codes 0 and 3 have no text).
  - `X_POSITION_PROPERTY`, `X_STATUS_PROPERTY` (voltage `round(adu × 150 / 1024) / 10`), `X_TIMER_COND` when a value changed (OK).
  - `X_SENSORS` lights (power/weather ALERT, park/open/closed sensors OK, buttons BUSY) when the 14-bit sensor word changed.
  - Invalid checksum → `DOME_SHUTTER` ALERT "Checksum error" when not ALERT. A failed read publishes nothing.
- Property changes (client thread): `DOME_SHUTTER` ignored while BUSY; OPENED → BUSY update and `dome_open_handler` on a new timer thread (`O`, then sleeps in 0.5 s steps while `DOME_SHUTTER` is BUSY, then updates `DOME_SHUTTER` again; failed command → ALERT); CLOSED → `P` likewise. `DOME_ABORT_MOTION`: only while `DOME_SHUTTER` is BUSY, BUSY update and `dome_abort_handler` (`S`; success → both switches off, `DOME_SHUTTER` ALERT, abort OK; failure → abort ALERT); otherwise abort ALERT with the item left ON. `X_MOTOR_CONF`, `X_DELAY_CONF`, `X_CLOSE_COND` (ignored while BUSY): values packed into the shared configuration buffer on the client thread, BUSY, `write_configuration_handler` on a timer thread (checksum `-(sum % 128)` without bit 7, `a` frame, any `#` terminated reply completes every BUSY configuration property OK, otherwise ALERT). No readback.
- Disconnect: synchronous timer cancel, `DOME_SHUTTER` ALERT if BUSY (overwritten to OK by the dome base class), close, delete custom properties. The roof is not stopped. Detach disconnects synchronously.

### Public properties and behaviour

- `DEVICE_PORT`, `DEVICE_PORTS` visible; INFO count 6 (model "Talon6 ROR", firmware revision); `DOME_SPEED`, `DOME_DIRECTION`, `DOME_HORIZONTAL_COORDINATES`, `DOME_STEPS`, `DOME_PARK`, `DOME_DIMENSION`, `DOME_SLAVING_PARAMETERS` hidden; `DOME_SHUTTER` rule at most one, labels "Roof state", "Roof opened", "Roof closed"; `DOME_ABORT_MOTION`, `DOME_STATE`, `GEOGRAPHIC_COORDINATES` (and the base-class time properties) visible.
- Custom properties, defined only while connected (all names already carry the mandatory `X_` prefix):
  - `X_SENSORS` (light, group `Dome`, "Sensors"): `POWER_CONDITION`, `WEATHER_CONDITION`, `PARKED_SENSOR`, `OPEN_SENSOR`, `CLOSE_SENSOR`, `OPEN_BUTTON`, `CLOSE_BUTTON`, `STOP_BUTTON`.
  - `X_MOTOR_CONF` (number RW, group `Configuration`, "Motor configuration"): `KP`, `KI`, `KD` 1…1000; `MAX_SPEED`, `MIN_SPEED`, `ACCELERATION`, `RAMP` 0…100; `FULL_ENC` 0…INT32_MAX; `ENC_FACTOR` 2…4 step 2; `REVERSE` 0…1.
  - `X_DELAY_CONF` (number RW, `Configuration`, "Delay configuration"): `PARK`, `WEATHER`, `POWER`, `TIMEOUT` 1…0x377 (887) s.
  - `X_CLOSE_COND` (switch RW any of many, `Configuration`, "Close conditions"): `WEATHER`, `POWER`, `TIMEOUT`.
  - `X_TIMER_COND` (number RO, `Dome`, "Close timers"): `WEATHER`, `POWER`, `TIMEOUT` 0…887.
  - `X_POSITION_PROPERTY` (number RO, `Dome`, "Roof position"): `POSITION` 0…10000.
  - `X_STATUS_PROPERTY` (number RO, `Dome`, "System status"): `VOLTAGE` 0…10000.

### Supported platforms, build and integration

- README: "platform independent" (in fact POSIX only). Built by `Makefile.drv` auto-discovery; Xcode `dome_talon6ror` group (sources, simulator `.c` and `.ino`) and test group. No `REFACTOR.md`, `.driver`, generated outputs or Windows project.
- `indigo_docs/PROPERTIES.md` documents the seven custom properties and names the `.c` source.

### Existing simulator and tests (gaps)

`dome_talon6ror_simulator.c` jumps between the end positions after three status requests (no elapsed-time motion, no intermediate positions, `S` never leaves the roof between the ends), uses a full travel of 819100 ticks unrelated to the configured encoder ticks, reports fixed voltage, timers and sensors, has no safety conditions, mount-park wait, keypad buttons, stall, fault injection, external control or event log, and does not validate configuration writes. The single test is a smoke/compliance pass without failure, timing, reconnect, lifecycle, multi-instance or concurrency coverage.

### Defects and risks found by source audit

Identifiers are used in the found-defects section; each is either reproduced by a dedicated reproducer against the original driver or explicitly marked audit-only.

- **T6R-01** Command replies are not validated: an `&ERROR#` reply to `O`, `P` or `S` is treated as accepted — a rejected open completes OK with the old closed state and a rejected abort reports OK.
- **T6R-02** A status poll that is in progress when an open or close request arrives completes the request OK with the previous end state ("Roof closed" for an open request) before the command has been sent; the roof then starts moving while the request was already reported complete.
- **T6R-03** An accepted command that does not move the roof at once completes OK with the previous end state: an open refused by active safety conditions (per the manual the roof does not move) is reported OK "Roof closed", and a close that waits for the mount to park is reported OK "Roof opened" while the roof is still open.
- **T6R-04** The configuration write checksum is stored without bit 7; when the payload sum is a multiple of 128 it is a zero byte that terminates the command string, so the frame is sent without checksum.
- **T6R-05** Configuration write replies are not validated (`&ERROR#` completes the properties OK) and rejected values stay in the write buffer and the properties, so the next successful write of another property sends them again.
- **T6R-06** A reply of 64 or more bytes without `#` leaves the reply buffer unterminated; the debug dump then reads past the stack buffer and writes past its static 192-byte buffer.
- **T6R-07** Open and close handlers block a timer thread while the roof moves; a disconnect during motion lets the handler publish `DOME_SHUTTER` after the property was deleted.
- **T6R-08** The last announced action is not reset at connection: after a reconnection the current last action is not announced again.
- **T6R-09** Last action 13 is announced as "Stop requested"; the manufacturer manual defines code 13 as a motor stall (mechanical/electrical problem).
- **T6R-10** Property ranges are narrower than the protocol fields and the documented use: delays and timers are limited to 887 s (a timeout closing after hours is clamped by the framework), delays cannot be 0 (the manual documents park delay 0 = wait until parked) and the roof position is limited to 10000 ticks although the full travel is configurable (the `.ino` configuration uses 50000).
- Audit-only risks: property states and the shared configuration buffer are modified from the client thread, timer threads and the status timer without serialization (the mutex protects single transactions only), so two quick configuration changes can race with a write in progress; the attach code checks `X_MOTOR_CONF_PROPERTY` for NULL after allocating `X_DELAY_CONF` and `X_CLOSE_COND` (copy/paste, only relevant on allocation failure); the reader keeps a 5 s per-byte timeout under the mutex, so a silent controller blocks every other request for 5 s; POSIX-only I/O prevents a Windows build.

## Atomic plan

1. **Done — record.** This file: audit, baseline, hardware decision, plan (written before any production change).
2. **Done — simulator audit and rewrite.** `dome_talon6ror_simulator.c` rewritten from the driver, the author's `.ino`, the user manual and the INDI status layout: `&…%#` framing with out-of-frame bytes ignored, `V`, `G`, `O`, `C`, `P`, `S`, `p` and `a` (length, data bits and checksum validated) with `&ERROR#` for everything else; `serial_motion.h` elapsed-time roof travel over the configured full encoder ticks (`--travel-time`, `--position`); roof states open/closed/opening/closing and error when stopped between the end positions; last-action codes of the manual; safety conditions that refuse an open and close the roof (with mount park) after the configured delay when the matching close condition is enabled; timeout closing countdown after an accepted open; close with mount park waiting for the park sensor or the park delay (last action 15); keypad buttons, COM/MGM inputs, voltage, stall; status timers in whole seconds. Test hooks: event log `INDIGO_TALON6ROR_EVENTS` (`RX`/`TX`/`MOVE`/`STATE`/`REJECT`/`FAULT`/`CONTROL`/`OPEN`/`CLOSE`, frames rendered with printable characters as is and other bytes as hex), fault injection `INDIGO_TALON6ROR_FAULT` (`silent`, `error`, `reply`, `slow`, `checksum`, `short`, `garbage`, `long`, `ignore`, `close` with repeat count per command letter or `ANY`) and external control `INDIGO_TALON6ROR_CONTROL` (`position`, `weather`, `power`, `parked`, `button`, `com`, `mgm`, `voltage`, `stall`, `travel-time`, `noise`). Simulator assumptions (undocumented) are listed in the file header. Verification: `clang -std=gnu11 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -fsyntax-only` clean; PTY probe of the framing, motion, states and checksums; `simulator_protocol` self-check 1 run, 1 passed after two test mistakes (the safety condition blocks an open only when its close condition is enabled, and the configuration reply payload starts at byte 2).
3. **Done — characterization suite against the original driver.** `indigo_test/integration/test_dome_talon6ror_simulator.c` rewritten on the `dome_baader` harness (forked cases with a fresh simulator, temporary `HOME`, alarm and process-group teardown; observed definitions, updates, deletions and messages with `force_property_updates`; simulator event log for ordered protocol assertions and poll synchronization; normalized reference trace). Baseline binary compiled from the saved original sources (see "Original-driver baseline binary"). First complete run: 23 run, 17 passed. All six failures were test mistakes, fixed and rerun individually: driver messages sent through `IDLE_PROPERTY`/`ALERT_PROPERTY` arrive with the property names `IDLE`/`ALERT` (four cases), a request that already selects the direction of the reported motion publishes no intermediate "Roof opening"/"Roof closing" message (`open_and_close_roof`), and the trace step for a rejected configuration write expected an ALERT that the original driver cannot produce (replaced by an unanswered write, which both drivers report as ALERT). `urgent_abort_cancels_queued_open` is a generated-behaviour case and is expected to fail against the original. After the fixes: 23 run, 22 passed (only the queue case failing).
4. **Done — defect reproducers.** `--known-defects` ran T6R-01…T6R-05 and T6R-07…T6R-10 (ten cases; T6R-06 needs a sanitizer, see below): 10 run, 10 failed as expected, each at its defect assertion (evidence below). T6R-06 was reproduced by running `overlong_reply_is_bounded` against the original driver compiled with ASan: `stack-buffer-overflow ... READ of size 1 ... in dump_hex indigo_dome_talon6ror.c:224` from `talon6ror_command` in the status poll.
5. **Done — original reference trace.** `indigo_test/fixtures/dome_talon6ror/original_reference_trace.txt` (128 lines): per step the ordered simulator exchange (`S RX`/`S TX`; a status request and its reply are reduced to `S POLL state=<n> action=<n>` and consecutive identical polls are collapsed) followed by property definitions, updates, deletions and messages (`D`/`U`/`X`/`M`; `X_POSITION_PROPERTY`, `X_TIMER_COND` and `X_STATUS_PROPERTY` values masked because they depend on elapsed time, `DEVICE_PORT` masked, repeated identical updates dropped). Steps: connect, open, open again, close, abort idle, abort in motion, close after abort, motor configuration, delay configuration, close conditions, unanswered configuration, checksum error, weather condition, disconnect. The first capture set differed in one step because a short close after an abort could finish between two polls; with a four second travel time and an abort after three polls, 3 of 3 captures were identical.
6. **Done — `.driver` and regeneration.** Added `indigo_dome_talon6ror.driver` (version 3, `DRIVER_VERSION 0x03000003`), `serial;`, transactional `talon6ror_open`/`talon6ror_close` (serial open, banner drain, `V` and `p` handshake with checksum validation, INFO firmware revision) and portable `indigo_uni_io` transport (`indigo_uni_vtprintf()` with the `%#` terminator, `indigo_uni_read_section2()` up to `#` with 5 s first-byte and inter-byte timeouts as in the original, out-of-frame bytes and further `&` bytes ignored, overlong or unterminated replies discarded with `indigo_uni_discard()`). The status poll runs as a device-queue callback (`dome_status_poll`, immediately after connection, then 0.5 s after each run) and owns all status publications; `DOME_SHUTTER`, `DOME_ABORT_MOTION` and the three configuration properties are generated queued handlers. Fixes for T6R-01…T6R-10. Generated with `build/bin/indigo_generator indigo_dome_talon6ror.driver`; `make -B -f ../../Makefile.drv` for x86_64 + arm64 with zero warnings; strict check `clang -arch arm64 -std=gnu11 -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wshadow -Wformat=2 -Wunreachable-code -fsyntax-only` clean. One generator-semantics correction was needed while bringing the driver up: the first version used early `return` statements in `on_change` blocks, which skip the generated final `indigo_update_property()`, so failure states would never have been published; the handlers were restructured into if/else chains that leave the final publication to the generator. No generator change and no `MAX_DEVICES` override.
7. **Done — post-migration suite and trace comparison.** Suite against the generated driver: 23 run, 21 passed; `reference_trace` failed only because the fixture was still the original one and `urgent_abort_cancels_queued_open` asserted an ALERT that neither driver produces (the roof never moved, so the poll correctly reports it closed; the case now asserts that no `&O%#` reaches the controller, that `&S%#` does, and that the roof settles OK closed — it passes against the generated driver and fails against the original). `--known-defects` against the generated driver: 9 of 10 passed; `T6R-03 close_waits_for_mount_park` compared the total number of "Roof opened" messages instead of the number published after the close request and was corrected (it then fails against the original and passes against the generated driver). Generated trace captured (`generated_reference_trace.txt`, 132 lines, 3 of 3 identical captures); comparison in "Reference trace comparison".
8. **Done — defect fixes promoted.** The ten T6R cases were promoted into the ordinary suite (`--known-defects` now selects none), so every reproducer runs in the default target.
9. **Done — repository integration.** `indigo.xcodeproj/project.pbxproj`: `REFACTOR.md` and `indigo_dome_talon6ror.driver` in the `dome_talon6ror` group, new `fixtures/dome_talon6ror` group with both trace files (`plutil -lint` passes). Windows: `indigo_dome_talon6ror.vcxproj`, `.filters` and `.user` copied from the `dome_nexdome3` template with a new project GUID, UTF-8 BOM and CRLF preserved, `.driver` and `REFACTOR.md` as `None` items (`xmllint` passes), and the project and configuration entries in `indigo_windows.sln` (CRLF preserved); no Windows build was possible in this environment. `indigo_test/Makefile`: the simulator now depends on `simulator_common/serial_motion.h`, the test depends on both fixtures and the driver archive, new targets `test-dome-talon6ror-simulator` and `test-dome-talon6ror-simulator-sanitize`. `indigo_docs/PROPERTIES.md`: source now names the `.driver`, and the property list uses the real names `X_POSITION_PROPERTY` and `X_STATUS_PROPERTY` and adds `DOME_SHUTTER` to the driver-specific properties (the property set itself is unchanged). `MIGRATION_STATUS.md` row updated (API 3️⃣, Windows ✅ Yes, generator ✅ Yes, async queues ✅ Yes, retested ✅ Sim, tests `33 / 0`); the Comment column is unchanged. `README.md` unchanged.
10. **Done — final verification.** Evidence in "Post-migration evidence". Linux and Windows builds and physical hardware were unavailable and are not claimed.

## Original-driver baseline binary

The original-driver baseline binary is not in the repository. To rebuild it, take `indigo_drivers/dome_talon6ror/indigo_dome_talon6ror.c` and `.h` from commit `73f9eadc2` into a temporary directory `<tmp>` and run from `indigo_test` the normal test compile command (`make -n -B build/integration/test_dome_talon6ror_simulator` prints it) with the driver archive replaced by `<tmp>/indigo_dome_talon6ror.c` and with `-DEXPECTED_VERSION=0x02000002 -DTALON6ROR_REFERENCE_TRACE_PATH=\"fixtures/dome_talon6ror/original_reference_trace.txt\"` added. For the T6R-06 evidence add `-arch arm64 -O1 -fsanitize=address,undefined -fno-omit-frame-pointer` and drop the universal `-arch` flags.

## Original-driver baseline evidence

Environment: macOS 26 arm64, baseline binary above.

- Characterization suite (preservation cases only, after the harness fixes of step 3): 23 run, 22 passed; the only failure is `urgent_abort_cancels_queued_open`, which specifies generated queue behaviour.
- `--known-defects` before promotion: 10 run, 10 failed as expected:
  - T6R-01: after `&O%#` was answered with `&ERROR#`, `DOME_SHUTTER` settled OK instead of ALERT.
  - T6R-02: the open request settled OK with the CLOSED switch selected (the status reply captured before the command completed it).
  - T6R-03 (refused open): with an active weather condition and the weather close condition enabled the roof did not move, but `DOME_SHUTTER` settled OK.
  - T6R-03 (close with mount park): the close request settled OK ("Roof opened") while the roof was still open and waiting for the mount to park.
  - T6R-04: the configuration frame was 58 instead of 59 bytes (the zero checksum byte truncated the command).
  - T6R-05: after `&a…%#` was answered with `&ERROR#`, `X_MOTOR_CONF` settled OK.
  - T6R-07: `DOME_SHUTTER` was published once more after it had been deleted by the disconnect.
  - T6R-08: after a reconnection the current last action was not announced again.
  - T6R-09: last action 13 was announced as "Stop requested" instead of "Motor stalled".
  - T6R-10: `X_DELAY_CONF.TIMEOUT` had the range 1…887 instead of 0…2097151.
- T6R-06 under ASan: `overlong_reply_is_bounded` aborted with `AddressSanitizer: stack-buffer-overflow ... READ of size 1 ... #0 dump_hex indigo_dome_talon6ror.c:224, #1 talon6ror_command …:264, #2 talon6ror_get_status …:280`.
- Complete suite with the final harness (all reproducers promoted): 33 run, 21 passed, 12 failed — the ten promoted T6R cases, `urgent_abort_cancels_queued_open`, and one timeout of `additional_instance` (alarm, no assertion failure). That case was rerun in isolation against the original driver 10 times: 7 passed, 3 timed out without any assertion output; with driver debug logging enabled it passed 5 of 5. The hang is consistent with the unsynchronized blocking handlers of the original driver (an open/close handler spins on the property state while the status timer and the client thread publish it) and is recorded as an audit-only observation, not as a separate reproducer. The generated driver passed the same case 5 of 5 in isolation plus every complete run.

## Post-migration evidence

Environment as for the baseline.

- Reproducible generation: `build/bin/indigo_generator indigo_dome_talon6ror.driver` in a scratch copy produced `.c`, `.h` and `_main.c` byte-identical to the checked-in files (`cmp` clean).
- Build: `make -B -f ../../Makefile.drv` in this directory, x86_64 + arm64, zero warnings. Strict syntax check (flags in step 6) produced no diagnostics.
- Simulator: `clang -std=gnu11 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -fsyntax-only` clean.
- Complete suite `./build/integration/test_dome_talon6ror_simulator` (generated driver, 33 cases including the ten promoted reproducers): 33 run, 33 passed, 0 failed (6 min 23 s).
- Reference trace: 3 of 3 identical captures for each driver; every protocol line of `original_reference_trace.txt` and `generated_reference_trace.txt` is identical (analysis below).
- ASan + UBSan (`make -C indigo_test test-dome-talon6ror-simulator-sanitize`, arm64, `-O1`, driver source compiled into the test, `detect_leaks=0`, `halt_on_error=1`): see the final test summary.
- Not run: Linux and Windows builds and tests (environment unavailable), hardware tests (no device).

## Reference trace comparison

Every `S` line (protocol exchange) of the two fixtures is identical: the connection sequence `&V%#`, `&p%#` and the first status request, the 0.5 s status polls with their roof state and last action, `&O%#`, `&P%#`, `&S%#`, the three configuration frames byte for byte, the unanswered configuration write and the injected checksum error. All differences are property publications:

1. `INFO` reports driver version 3.0.0.3 instead of 2.0.0.2 (generated `DRIVER_VERSION`).
2. The configuration properties are defined and published with `target` equal to the value read from the controller; the original left the targets at their compile-time defaults (for example `FULL_ENC=50000/10000`).
3. Generated connection messages "Connected to Talon6 ROR on <port>" and "Disconnected from Talon6 ROR".
4. `DOME_ABORT_MOTION` publishes its requested value (`ABORT_MOTION` on) with the BUSY update, because the framework publishes the copied values before the queued handler clears the item.
5. The failed configuration write publishes the message "Configuration write failed" and restores the rejected item value (`KD=2` instead of the rejected `KD=3`, T6R-05).

## Intentional behaviour differences

- **Transport:** portable `indigo_uni_io` replaces the legacy descriptor I/O and the `pthread` mutex (a Windows project was added). The command manner is preserved: one write of `&<payload>%#`, then a read up to `#` with 5 s first-byte and inter-byte timeouts, bytes before `&` and further `&` bytes ignored. New: a reply longer than 64 payload bytes or without `#` is discarded and the pending input is drained instead of being formatted from an unterminated buffer (T6R-06); the connection banner drain keeps the original 5 s first-byte and 10 ms gap timing.
- **Serialization:** the status poll and all property handlers run on the device queue, so property states and the configuration buffer are no longer shared between threads. Property changes no longer block the calling client thread, and open/close handlers no longer spin while the roof moves (T6R-07); the request is completed by the status poll.
- **Request completion:** an accepted open or close stays BUSY until the controller reports the requested end state (OK), a motion state (BUSY), or — after 3 s without motion, and for a close only while the controller reports "mount park requested" — fails with ALERT and the message "Roof did not open"/"Roof did not close" (T6R-03). A rejected command (`&ERROR#`) restores the switches from the last reported state and publishes ALERT (T6R-01). A status reply captured before the command was sent no longer completes the request (T6R-02). A request failure keeps its ALERT until the controller reports a different roof state.
- **Configuration:** the write checksum always has bit 7 set, so a payload sum that is a multiple of 128 no longer truncates the frame (T6R-04); a rejected or unanswered write restores the buffer and the item values from the last confirmed configuration and reports ALERT with the message "Configuration write failed" (T6R-05).
- **Ranges (T6R-10):** delays and the roof position follow the protocol fields — `X_DELAY_CONF` and `X_CLOSE_TIMER.TIMEOUT` 0…2097151 s, `X_CLOSE_TIMER.WEATHER`/`POWER` 0…16383 s, `X_POSITION.POSITION` and `X_MOTOR_CONF.FULL_ENC` 0…2097151 ticks. The other item ranges, names, labels, groups and permissions are unchanged.
- **Messages:** last action 13 is announced as "Motor stalled" (the manual defines it as a motor stall, T6R-09) and the announced action is reset at connection so a reconnection announces the current action again (T6R-08). Everything else keeps the original texts.
- **Version:** `0x03000003` (the original was `0x02000002`).
- **Unchanged by decision:** the abort is refused with ALERT (and with the abort item left on) while the roof is not moving, exactly as before; the roof is not stopped on disconnect; the status poll interval stays 0.5 s; the connection still waits up to 5 s for the controller banner; the acknowledgement check accepts any reply except one starting with `ERROR`, because no manufacturer document defines the acknowledgement payload.

## Found defects

Every defect below was reproduced by a dedicated case that fails against the original driver and passes against the generated driver.

| ID | Observable impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- |
| T6R-01 | A rejected open, close or abort is reported as success. | Any `#` terminated reply accepted as acknowledgement. | Replies starting with `ERROR` are failures; switches restored, ALERT published. | `T6R-01 rejected_commands_alert` |
| T6R-02 | A request completes OK with the previous end state when a status poll was already in progress. | The poll publishes the shutter property without knowing about the queued request. | The poll defers to the handler while a request is queued. | `T6R-02 queued_request_survives_status_poll` |
| T6R-03 | An open refused by a safety condition completes OK ("Roof closed"); a close waiting for the mount to park completes OK ("Roof opened"). | Completion is taken from the first status after the command. | Completion requires the requested end state; a 3 s start timeout ends a motionless request in ALERT, and "mount park requested" keeps it BUSY. | `T6R-03 refused_open_alerts`, `T6R-03 close_waits_for_mount_park` |
| T6R-04 | A configuration write whose payload sum is a multiple of 128 is sent without its checksum. | Checksum stored without bit 7 becomes a zero byte and terminates the command string. | Checksum always with bit 7 set, like the status and configuration replies. | `T6R-04 configuration_checksum_not_truncated` |
| T6R-05 | A rejected configuration write reports OK and its values are sent again by the next write. | Reply not validated, write buffer never restored. | Reply validated; buffer and item values restored from the last confirmed configuration. | `T6R-05 failed_configuration_write_restores_values` |
| T6R-06 | The driver reads and writes past its buffers when a reply is longer than 64 bytes without a terminator (ASan: stack-buffer-overflow in `dump_hex`). | The reader leaves the buffer unterminated and the debug dump scans it. | Bounded reader; overlong or unterminated replies are discarded. | `overlong_reply_is_bounded` (ASan evidence above) |
| T6R-07 | An open or close handler blocks a timer thread while the roof moves and publishes the shutter property after a disconnect deleted it. | Handlers spin on the property state instead of completing asynchronously. | Queued handlers; the status poll completes the request; the generator cancels pending handlers on disconnect. | `T6R-07 no_updates_after_disconnect` |
| T6R-08 | After a reconnection the current last action is not announced. | The announced action is not reset at connection. | Reset in `on_connect`. | `T6R-08 last_action_announced_after_reconnect` |
| T6R-09 | A motor stall is announced as "Stop requested". | Wrong text for last action 13. | Text follows the manufacturer manual. | `T6R-09 motor_stall_message` |
| T6R-10 | Delays above 887 s cannot be set (a timeout closing after hours is clamped), delays cannot be 0 and positions above 10000 ticks are outside the published range. | Item ranges narrower than the protocol fields. | Ranges follow the 21-bit and 14-bit protocol fields. | `T6R-10 ranges_cover_protocol` |

Audit-only risks resolved by the migration without a dedicated reproducer: unsynchronized access to property states and the configuration buffer from the client thread, the status timer and handler threads (including the intermittent `additional_instance` hang of the original driver recorded above); the 5 s per-byte read under the shared mutex blocking every other request; the copy/paste NULL checks in the original attach code; POSIX-only I/O preventing a Windows build.

Remaining known limitations: the acknowledgement payload, the reaction to safety conditions, the mount-park wait, the keypad buttons, the stall behaviour and the status bytes 16…20 are simulator assumptions taken from the author's `.ino`, the user manual and the INDI driver, not from a manufacturer protocol document; a roof that stops between the end positions without a driver request is reported ALERT ("Error reported") as before; the driver still has no timeout for a motion that starts and then stalls without the controller changing its state; `C` (close without mount park) and `A` (go to percentage) are not exposed by the driver; no hardware, Linux or Windows validation.

## Scenario-to-test mapping

Dome class checklist (`indigo_test/DRIVER_TESTING_RULES.md`) and shared scope:

| Area | Cases |
| --- | --- |
| Simulator protocol self-check (not driver coverage) | `simulator_protocol` |
| Metadata, INFO, interface bit, common visible properties, no I/O before connection | `metadata_before_connection` |
| Connection sequence, banner drain, handshake, configuration readback, property shapes and ranges, first status publication | `connect_sequence`, `connect_with_open_roof`, `reference_trace`, `T6R-10 ranges_cover_protocol` |
| Open/handshake/checksum failures, descriptor balance, recovery | `connection_failures_and_recovery` |
| Roof open and close with elapsed-time motion, BUSY → OK, position and sensor publication, last action messages | `open_and_close_roof`, `open_when_already_open`, `T6R-08 last_action_announced_after_reconnect` |
| Requests that the controller accepts but does not execute (safety condition, mount park wait) | `T6R-03 refused_open_alerts`, `T6R-03 close_waits_for_mount_park` |
| Rejected commands | `T6R-01 rejected_commands_alert` |
| Abort while moving, abort when idle, movement after an abort | `abort_while_opening`, `abort_when_idle_alerts` |
| Roof moved by the keypad or by the controller itself (conditions, timers) | `manual_open_by_button`, `condition_close` |
| Motor stall and error state | `T6R-09 motor_stall_message` |
| Status failures: checksum error, silent, short, noise before the reply, overlong unterminated reply | `status_checksum_error_and_recovery`, `status_read_failures`, `overlong_reply_is_bounded` |
| Sensors, buttons, COM/MGM inputs, change-only publication | `sensors_and_buttons` |
| Configuration writes (frame bytes, checksum, preserved unrelated fields, readback after reconnection), failed write, zero checksum | `configuration_writes`, `T6R-04 configuration_checksum_not_truncated`, `T6R-05 failed_configuration_write_restores_values` |
| BUSY conflicts, including a request arriving while the device command runs | `busy_requests_rejected`, `busy_guard_holds_while_command_runs` |
| Queued-handler races: poll racing a queued request, urgent abort overtaking a queued request | `T6R-02 queued_request_survives_status_poll`, `urgent_abort_cancels_queued_open` |
| Disconnect during motion, no I/O after close, no publication of deleted properties, reconnect | `disconnect_during_motion`, `T6R-07 no_updates_after_disconnect` |
| INIT/SHUTDOWN idempotence, shutdown refused while connected, repeated connect cycles | `lifecycle_and_shutdown` |
| Additional instance with its own port and state | `additional_instance` |
| Ordered protocol/property compatibility contract | `reference_trace` |

Not applicable or not covered: hot plug and multiple physical devices (serial, no enumeration); network transport (the driver opens a serial port only); guider timing (no guider interface, so no guiding-pulse measurement applies); azimuth rotation, park, steps, flap, home and slaving properties (hidden by this roll-off roof driver); driver-owned persistent settings (the configuration is controller-owned and covered by `configuration_writes`); hardware acceptance (no device).

## Final audit

- `git diff --check`: clean for the driver, simulator, test, project and documentation files; the added `indigo_windows.sln` lines are reported only because of their CRLF terminators, which match the existing file. The new `.vcxproj` files keep the template's UTF-8 BOM and CRLF.
- Formatting audit of the `.driver`, the simulator and the test: tab indentation, no trailing whitespace, single-line calls, `{ 0 }` initializers, no blank lines inside function bodies.
- Driver version `0x03000003` is higher than the original `0x02000002`; no `MAX_DEVICES` override; no generator change; the `.driver` is the source of the checked-in `.c`, `.h` and `_main.c`; `README.md` unchanged.
- Project registration: Xcode group entries and the fixtures group, Windows project and solution entries, `indigo_test/Makefile` targets and dependencies.
- Cleanup: `make -C indigo_test test-clean` after the final runs; no simulator or test processes and no `/tmp/indigo-talon6ror.*` fixtures left behind. The scratch baseline binaries, the regeneration copy and the downloaded reference documents stay outside the repository.
- Unrelated working-tree changes not made by this work and left untouched: the `aux_joystick` row of `MIGRATION_STATUS.md` and the untracked `indigo_test/hardware/test_ccd_sx_hw.c`.

## Final test summary

Counts include development runs; a registered case run inside a complete suite run counts once per run. Trace captures count as runs of `reference_trace`.

- Simulated tests, original driver: 120 run, 83 passed. Breakdown: pre-existing smoke test 1/1; `simulator_protocol` development runs 3 run, 1 passed (2 test mistakes); first complete characterization run 23 run, 17 passed (6 harness failures, step 3) and 4 reruns after the fixes 4/4; defect reproducers 10 run, 0 passed (all failed as expected); the T6R-06 ASan run 1 run, 0 passed (expected, stack-buffer-overflow); trace captures 6/6; complete run with the installed fixture 23 run, 22 passed; `T6R-03 close_waits_for_mount_park` rerun 1 run, 0 passed and `urgent_abort_cancels_queued_open` rerun 1 run, 0 passed (both expected); complete run with the final harness 33 run, 21 passed (12 expected failures); `additional_instance` isolation runs 14 run, 11 passed (3 timeouts of the original driver, analysed above).
- Simulated tests, generated driver: 148 run, 145 passed. Breakdown: bring-up cases 3/3; defect reproducers 10 run, 9 passed (the tenth exposed a test mistake, fixed) and its rerun 1/1; first complete run 23 run, 21 passed (the fixture and the queue case, both resolved in step 7); `urgent_abort_cancels_queued_open` rerun 1/1; trace captures 3/3; complete suite after promotion 33/33; `additional_instance` isolation runs 8/8; ASan + UBSan complete run 33/33 with no sanitizer report; final verification run of the checked-in state after `test-clean` 33/33.
- Hardware tests: 0 run, 0 passed.
