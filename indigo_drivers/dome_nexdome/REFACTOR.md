# dome_nexdome refactoring record

## Scope and baseline

This record covers migration of `indigo_dome_nexdome` (NexDome dome with Gerry Rozema's 1.x firmware) from its hand-written INDIGO 2.0 implementation to `indigo_generator`, the production fixes proven by regression tests, and complete applicable hardware-free simulator coverage.

Baseline date and source: 2026-09-17, commit `3b7905386` (`dome_beaver: migrated to code generator`), branch `refactoring`, clean working tree. Host: macOS 26 (`Darwin 25.6.0`), Apple Silicon arm64; repository universal build (x86_64 + arm64).

Baseline build command:

```sh
cd indigo_drivers/dome_nexdome
make -B -f ../../Makefile.drv
```

Result: passed without compiler or linker warnings; object, archive, dynamic library and executable built for x86_64 and arm64.

Baseline automated tests:

```sh
make -C indigo_test build/integration/test_dome_nexdome_simulator
cd indigo_test && ./build/integration/test_dome_nexdome_simulator
```

Result: 1 run, 1 passed (`nexdome_passes_serial_compliance_checks`, 10.9 s). `MIGRATION_STATUS.md` records `1 / 0`.

`build/bin/indigo_generator` is present in the build tree (built from the unchanged `indigo_tools/indigo_generator.c`).

## Hardware-test decision

No hardware testing will be performed. No NexDome controller is available (the user has no physical hardware). No statement in this record implies physical validation; all automated evidence is simulator-backed software behaviour.

## Studied sources

- `indigo_dome_nexdome.c`, `.h`, `_main.c`, `README.md` in this directory (no Windows project exists).
- `dome_nexdome_simulator/nexdome_protocol.txt`: "Nexdome protocol 1.10, rough draft based on the firmware source code" — the only protocol document. `dome_nexdome_simulator/dome_nexdome_simulator.ino`: the author's Arduino simulator derived from the firmware (reply formats, line framing, 20 character command buffer, `ConfigureWireless()` blocking 1.1 s after `w`, rain handling, home sensor and calibration logic). `dome_nexdome_simulator/dome_nexdome_simulator.c`: host PTY simulator.
- `indigo_test/integration/test_dome_nexdome_simulator.c`, `indigo_test/Makefile`, `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (dome section and shared scope), `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`.
- `indigo_tools/indigo_generator.c`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_libs/indigo_dome_driver.c` (`DOME_ON_COORDINATES_SET` count 1; `DOME_STEPS` 0…180 step 1; `DOME_SHUTTER` initially CLOSED), `indigo_libs/indigo_driver.c` (INFO driver version is derived from `version >> 16`), `indigo_libs/indigo_uni_io.c` (`indigo_uni_discard()` is `tcflush()` for serial handles; `indigo_uni_read_section2()` with first-byte and inter-byte timeouts), the sibling generated migrations `dome_beaver` and `dome_baader` (simulator hooks, forked test cases, reference trace method).

## Current-state audit

### Protocol

Line based ASCII at 9600 8N1 (or a `nexdome://host[:port]` TCP URL, default port 8080). A command is a letter with optional arguments terminated by `\n` (the firmware accepts `\r` or `\n`; a command longer than 20 characters is discarded; unknown commands produce no reply). Replies are terminated by `\r\n` (`println`), except `m`, `p` and `q` which end with `\n` only. Commands used by the driver:

| Command | Reply | Driver use |
| --- | --- | --- |
| `v` | `VNexDome V 1.10` (plus ` NexShutter V <version>` and an extra empty line when a shutter version is known) | connection identification, INFO model and firmware |
| `y` / `y <0|1>` | `Y <0|1>` | reversed flag read at connection, `NEXDOME_REVERSED` |
| `q` | `Q <heading with one decimal>` | connection, status poll, relative moves, GOTO while parked |
| `n` | `N <park azimuth with two decimals>` | connection park state, park |
| `k` | `K <rotator cV> <shutter cV> <cutoff>` | status poll: `NEXDOME_POWER`, low-voltage messages (threshold 7.5 V) |
| `m` | `M <0 stopped|1 goto|2 finding home|3 calibrating>` | status poll |
| `u` | `U <0 not connected|1 open|2 opening|3 closed|4 closing|5 unknown> <rain: 1 dry, 0 raining>` | status poll |
| `g <az>` | `G`, `E` outside 0…360 | GOTO, relative move, park |
| `s <az>` | `S <az>`, `E ` outside 0…360 | sync (only reachable with `DOME_ON_COORDINATES_SET.SYNC`) |
| `a` | `A` | abort |
| `d` / `e` | `D` (`E` for `d` while raining) | shutter open/close |
| `h` | `H` | find home |
| `c` | `C`, `E` when not at home | calibration |
| `w` | `W` (the controller then reinitializes the XBee link) | reset shutter communication |

Documented but unused by the driver: `p` (raw position), `b`/`f` (shutter position), `x` (wake shutter), `o` (heading error), `t` (steps per turn), `i`/`j` (home azimuth), `l` (set park azimuth), `z` (at home), `r` (hibernate timer).

### Architecture and implementation

- `DRIVER_VERSION 0x020000009`, label and device `NexDome` (`DOME_NEXDOME_NAME` in the public header), author Rumen G. Bogdanovski. One dome device, `ADDITIONAL_INSTANCES` supported, no hot plug. Legacy `indigo_io` API with POSIX `select()`, `read()`, `close()`, `sleep()` and `pthread` in the driver; not portable to Windows and no Windows project.
- `nexdome_command()`: under a port mutex, drains input until 100 ms of silence (a silent flush of at least 100 ms before every command), writes the command, sleeps 100 µs, then reads byte by byte until `\r` or `\n` with a 3.1 s first-byte and 0.1 s inter-byte timeout. A timeout returns success with the partial (possibly empty) reply. The response helpers parse with `sscanf` and some store the parsed value before checking the parse result.
- Connection (timer thread via `indigo_set_timer(…, 0, dome_connect_callback)`): global lock, open serial and sleep 1 s (the Arduino resets when the port opens) or open TCP, `v` (failure: close, CONNECTION ALERT "NexDome did not respond. Are you using the correct firmware?"), INFO model/firmware, `y`, definition of the five custom properties, `q`, `n`, DOME_PARK PARKED when the heading is within 0.01° of the park azimuth, first status poll after 0.5 s.
- Status poll `dome_timer_callback` (timer thread, 1 s after the end of each run): `k` (power and low-voltage messages), `m`, `q`, rotation block (HORIZONTAL and STEPS BUSY while moving; after motion OK, or ALERT when the heading is 0.1° or more from the target unless a find-home/calibration or abort was requested; find-home and calibration completion), park completion within 0.1° of the park azimuth, `u` (shutter published when changed or while BUSY). The poll keeps `need_update`, `low_voltage` and `prev_shutter_state` in `static` variables.
- Property changes (`dome_change_property`) run synchronously on the caller's thread and perform device I/O there: `DOME_STEPS`, `DOME_HORIZONTAL_COORDINATES`, `DOME_ABORT_MOTION`, `DOME_SHUTTER`, `DOME_PARK`, `NEXDOME_REVERSED`, `NEXDOME_RESET_SHUTTER_COMM` (sleeps 2 s after `w` for the XBee reinitialization), `NEXDOME_FIND_HOME`, `NEXDOME_CALLIBRATE`. No request is BUSY-guarded; a GOTO while moving retargets the dome.
- Disconnect: cancel the poll timer synchronously, delete custom properties, close, global unlock. The dome is not stopped. Detach disconnects and calls `indigo_global_unlock()` a second time.

### Public properties and behaviour

- `DEVICE_PORT`, `DEVICE_PORTS` visible; INFO count 6; `DOME_SPEED` hidden; `DOME_ON_COORDINATES_SET` and `DOME_SLAVING_PARAMETERS` visible; `DOME_HORIZONTAL_COORDINATES` RW; `DOME_PARK_POSITION` and `DOME_HOME` hidden (base defaults); `DOME_FLAP` hidden.
- Custom properties (group `Settings`, defined only while connected): `NEXDOME_REVERSED` (one of many `YES`/`NO`), `NEXDOME_RESET_SHUTTER_COMM` (`RESET`), `NEXDOME_FIND_HOME` (`FIND_HOME`), `NEXDOME_CALLIBRATE` (`CALLIBRATE`), `NEXDOME_POWER` (RO numbers `ROTATOR_VOLTAGE`, `SHUTTER_VOLTAGE`, 0…500, `%.2f`). None has the required `X_` prefix.
- Parked dome: GOTO ALERT "Dome is parked" (after reading `q`), steps ALERT "Dome is parked"; find home, calibration and shutter are not refused. Unpark sends no command. Relative moves truncate the step value to whole degrees and send `g` with a whole-degree target. Failures: GOTO ALERT without message, steps ALERT "Goto azimuth failed.", calibration ALERT "Callibration failed. Is the dome in home position?", shutter/abort/reversed/reset ALERT without message. Completion messages "Home Found." and "Callibration complete.". Power messages "Dome power is low! (U_rotator = …V, U_shutter = …V)" and "Dome power is normal! …".

### Supported platforms, build and integration

- README: "platform independent" (in fact POSIX only); listed in root `UNTESTED_DRIVERS`; registered in `indigo_server`. Built by `Makefile.drv` auto-discovery. Xcode `dome_nexdome` group (sources, `.ino`, simulator, test). No `REFACTOR.md`, `.driver`, generated outputs or Windows project.
- `indigo_docs/PROPERTIES.md` lists the five custom properties and names the `.c` source.

### Existing simulator and tests (gaps)

`dome_nexdome_simulator.c` completes motion after two `m` polls (no elapsed time), has no find-home or calibration motion, home sensor, rain, voltage changes, shutter motion or latency, no shutter-less controller, answers unknown commands with `E` (the firmware is silent), uses the wrong reply terminators, has no fault injection, external control, event log or TCP listener. The single test is a smoke/compliance pass without timing, failure, reconnect, lifecycle, multi-instance or concurrency coverage.

### Defects and risks found by source audit

Identifiers are used in the found-defects section; each is either reproduced by a dedicated reproducer against the original driver or explicitly marked audit-only.

- **NXD-01** Property changes block the caller: device I/O (including the 100 ms flush per command and a 2 s sleep for the shutter communication reset) runs inside `change_property`.
- **NXD-02** `DOME_ON_COORDINATES_SET` keeps its base count of 1, so the implemented sync (`s`) is unreachable.
- **NXD-03** A failed park command is only logged: `DOME_PARK` stays BUSY forever.
- **NXD-04** An aborted park request is not cleared: a later move that ends at the park azimuth marks the dome PARKED.
- **NXD-05** Aborting a find-home or calibration publishes "Home Found." / "Callibration complete." with OK.
- **NXD-06** Relative moves truncate the current heading to whole degrees (`(int)(10.7 + 10) % 360`): the dome is sent up to 1° short.
- **NXD-07** The poll's `static` state is shared by all instances and sessions: a low-voltage condition on a second dome instance is not reported.
- **NXD-08** A shutter, reversed-flag or reset request that fails leaves the requested switch selected (for example OPENED while the shutter is closed after an open refused because of rain).
- **NXD-09** Operations interrupted by a disconnect survive reconnection: `NEXDOME_FIND_HOME` (and `NEXDOME_CALLIBRATE`) is defined BUSY again and completed by the first poll.
- **NXD-10** The firmware does not change the reported shutter state when it accepts `d` (the shutter reports over the wireless link later); the driver completes the open request OK at the first poll while the controller still reports CLOSED.
- **NXD-11** `DRIVER_VERSION 0x020000009` has an extra digit: INFO reports driver version 32.0.0.9.
- Audit-only risks: parse helpers store uninitialized values on malformed `m`, `u` and `y` replies (undefined behaviour); the reply reader writes the terminating NUL one byte past the 100-byte buffer for a 100-byte unterminated reply; replies without terminator are accepted; the poll (timer thread) and change handlers (client thread) update the same properties without serialization; a dropped TCP connection is not detected (every poll fails, CONNECTION stays OK); `indigo_global_unlock()` is called twice on detach; POSIX-only I/O.

## Atomic plan

1. **Done — record.** This file: audit, baseline, hardware decision, plan (written before any production change).
2. **Done — simulator audit and rewrite.** Rewrite `dome_nexdome_simulator.c` from `nexdome_protocol.txt` and the author's `.ino`: `serial_motion.h` elapsed-time rotation (shortest path) with states `M 0…3`, find home and home sensor (`z`, has-been-home), calibration (one revolution ending at home, only at home), sync with home adjustment, park/home azimuth get/set, shutter motion with optional wireless latency, rain (`d` refused, open shutter closes), shutter not connected, voltages, reversed flag, `w` blocking the controller, version with optional shutter version, firmware reply terminators, 20 character command buffer, silent unknown commands. Test hooks: event log, fault injection, external control, optional loopback TCP listener. Verification: strict syntax check and a direct PTY protocol self-check.
3. **Done — characterization suite against the original driver.** `indigo_test/integration/test_dome_nexdome_simulator.c` rewritten on the `dome_beaver` harness (forked cases, observed publications, simulator event log, poll synchronization on `k`…`u`). Baseline binary `test_dome_nexdome_simulator_original` compiled (arm64) from the same source with `-DNEXDOME_ORIGINAL_DRIVER` (original property names, version `0x020000009`, original trace fixture) and the original `indigo_dome_nexdome.c` saved from commit `3b7905386`. Simulator self-check `simulator_protocol` passed first. First complete run: 34 run, 26 passed. Failures: `connect_sequence` (test mistake: simulator event times cannot be compared with the test clock; now measured on the test clock, then 1/1), `reference_trace` (no fixture yet), `additional_instance` (the second instance never publishes its heading — root cause NXD-07, expected baseline failure), and five cases that specify generated behaviour and are expected to fail against the original: `status_read_failures_ignored` (the original publishes out-of-range/uninitialized shutter states), `busy_requests_rejected` and `busy_guard_holds_while_command_runs` (the original retargets a moving dome), `urgent_abort_cancels_queued_goto` (the original has no queue), `shutter_without_report_times_out` (NXD-10).
4. **Done — defect reproducers.** `--known-defects` runs NXD-01…NXD-12 (NXD-12, park stopped short, found while writing step 3). Against the original: 12 run, 12 failed as expected, each at its defect assertion (evidence below).
5. **Done — original reference trace.** `indigo_test/fixtures/dome_nexdome/original_reference_trace.txt` (218 lines): per step the ordered simulator exchange (`S RX`/`S TX`/`S FAULT`; complete status polls collapsed into one `S POLL` line per run of identical polls with the `Q` digits masked) followed by property definitions/updates/deletions and messages with original property names. Captured 3 times: identical. On user request further repetitions were dropped to save time; every later run below is a single run.
6. **Done — `.driver` and regeneration.** Added `indigo_dome_nexdome.driver` (version 10, `DRIVER_VERSION 0x0300000A`), `serial;`, transactional `nexdome_open`/`nexdome_close` (global lock, serial open with the 1 s controller reset delay or `nexdome://` TCP with default port 8080, `v` identification). Transport `nexdome_vcommand()` keeps the original manner (100 ms pause and input discard before every command, `\n` terminated command, 100 µs pause, reply up to `\r` or `\n` with 3.1 s first-byte and 0.1 s inter-byte timeouts) and adds validation (terminated reply, expected reply letter, complete numeric fields, value ranges). Queued generated handlers; status poll `dome_status_poll` on the device queue with the original command order `k`, `m`, `q`, `u` (0.5 s after connection, 1 s after each poll); the 2 s XBee wait is `reset_shutter_comm_finalizer`. Custom properties renamed to `X_REVERSED`, `X_RESET_SHUTTER_COMM`, `X_FIND_HOME`, `X_CALIBRATE` (item `CALIBRATE`), `X_POWER`; labels, items and messages otherwise unchanged. Fixes for NXD-01…NXD-12. `build/bin/indigo_generator indigo_dome_nexdome.driver` (no warnings); `make -B -f ../../Makefile.drv` x86_64 + arm64 with zero warnings; `clang -Wall -Wextra -Wno-unused-parameter -Wshadow -fsyntax-only` clean. No generator change, no `MAX_DEVICES` override. The generated header no longer defines `DOME_NEXDOME_NAME` (unused elsewhere).
7. **Done — post-migration suite and trace comparison.** Generated trace captured once (`generated_reference_trace.txt`, analysis below; every `S` line equals the original). First complete run was invalid: it ran concurrently with the trace capture and every connection failed on the global device lock (two processes with device `NexDome`) — never run two NexDome test processes at once. Sequential complete run: 34 run, 32 passed, 2 failed: `simulator_protocol` (test race: the rounded `Q` heading reaches the target shortly before the simulated motion ends; fixed in the test with a 100 ms wait, then 1/1) and `shutter_states_and_rain` (migration regression: the poll treated a BUSY shutter state that it had published itself as a request still waiting in the queue, so a rain-driven close never completed; fixed with `shutter_observed` in the `.driver`, regenerated; all 6 `shutter` cases then 6/6). `reference_trace` passed (1/1) after the fix. The complete sequential suite after the `shutter_observed` fix is recorded in step 8 (it includes `reference_trace`, which passed against `generated_reference_trace.txt`).
8. **Done — defect fixes promoted.** `--known-defects` against the generated driver after the `shutter_observed` fix: 12 run, 12 passed (all NXD reproducers fixed). Before promotion the checked-in `.c`, `.h` and `_main.c` were regenerated in a scratch directory from the `.driver` and compared byte by byte: identical. The 12 NXD cases were promoted into the ordinary suite (`defect = false` in `cases[]`; `--known-defects` now selects no case). Driver rebuilt (`make -B -f ../../Makefile.drv`, x86_64 + arm64, zero warnings) and test rebuilt. Complete sequential suite `./build/integration/test_dome_nexdome_simulator`: 46 run, 46 passed, 0 failed (11 min 3 s).
9. **Done — repository integration.** `indigo.xcodeproj/project.pbxproj`: `REFACTOR.md` and `.driver` in the `dome_nexdome` group, new `fixtures/dome_nexdome` group (`plutil -lint` OK). New `indigo_dome_nexdome.vcxproj`, `.filters`, `.user` (from the `dome_baader` template, `.driver` and `REFACTOR.md` as `None` items, `xmllint` OK) and `indigo_windows.sln` entry (CRLF preserved); no Windows build was possible. `indigo_test/Makefile`: simulator depends on `serial_motion.h` (no longer `-pthread`), test depends on both fixtures and the driver archive, targets `test-dome-nexdome-simulator`, `test-dome-nexdome-simulator-sanitize`, opt-in `test-dome-nexdome-simulator-network`. `indigo_docs/PROPERTIES.md`: renamed properties, source is the `.driver`. `MIGRATION_STATUS.md` row: API 3️⃣, Windows ✅ Yes (project added, no Windows build or test possible in this environment), Generator ✅ Yes, Async Queues ✅ Yes, Retested ✅ Sim (simulator only, no hardware), tests `49 / 0` = 34 default cases + 12 promoted reproducers + 3 opt-in network cases; Comment unchanged. `README.md` unchanged.
10. **Done — final verification.**
    - Generator reproducibility: `build/bin/indigo_generator indigo_dome_nexdome.driver` in a scratch copy; generated `.c`, `.h`, `_main.c` byte-identical to the checked-in files.
    - Build: `make -B -f ../../Makefile.drv` (x86_64 + arm64) without warnings.
    - ASan + UBSan: `make -C indigo_test test-dome-nexdome-simulator-sanitize` (arm64, `-O1`, driver source compiled into the test, `detect_leaks=0`, `halt_on_error=1`): 46 run, 46 passed, no sanitizer report.
    - Opt-in network cases: `./build/integration/test_dome_nexdome_simulator --network`: 3 run, 3 passed (`network_nexdome_url`, `network_default_port`, `network_failures`; loopback simulator only).
    - Original driver with the final harness (baseline binary rebuilt as described in the handover note): 46 run, 28 passed, 18 failed — exactly the 12 promoted NXD reproducers, `additional_instance` (NXD-07) and the five generated-behaviour cases listed in step 3 (`status_read_failures_ignored`, `busy_requests_rejected`, `busy_guard_holds_while_command_runs`, `urgent_abort_cancels_queued_goto`, `shutter_without_report_times_out`). Every preservation case, including `reference_trace` against `original_reference_trace.txt`, passed.
    - `git diff --check` clean for the driver, test and documentation files (the `.vcxproj` CRLF/BOM matches the sibling `dome_baader` project); formatting audit of `.driver`, simulator and test (single-line calls, comma spacing, `{ 0 }` initializers, no blank lines inside function bodies) found no violation; driver version `0x0300000A` > original `0x020000009` major/minor; no `MAX_DEVICES` override; no generator change; `README.md` unchanged.
    - `make -C indigo_test test-clean`; scratch baseline binary and regeneration copies removed; no remaining test processes or `/tmp/indigo-nexdome.*` fixtures.
    - Not run: Linux and Windows builds/tests (environment unavailable), hardware tests (no device).

## Original-driver baseline binary

The work was interrupted once on 2026-09-17 and completed on another computer from the recorded state. The original-driver baseline binary is not in the repository. To rebuild it, take `indigo_drivers/dome_nexdome/indigo_dome_nexdome.c` and `.h` from commit `3b7905386` into a temporary directory `<tmp>`, put a copy of the header both next to the `.c` and at `<tmp>/indigo_drivers/dome_nexdome/indigo_dome_nexdome.h` (so that the test's `#include <indigo_drivers/dome_nexdome/indigo_dome_nexdome.h>` resolves to the original header), and run from `indigo_test`: `clang -std=gnu11 -arch arm64 -g -O1 -DINDIGO_MACOS -Duint=unsigned -I<tmp> -I.. -I../indigo_libs -I../build/include -I../indigo_drivers -DNEXDOME_ORIGINAL_DRIVER -w -o <tmp>/test_dome_nexdome_simulator_original integration/test_dome_nexdome_simulator.c <tmp>/indigo_dome_nexdome.c -L../build/lib -Wl,-rpath,$(pwd)/../build/lib -lindigo -lm -lpthread -lindigocat`. Never run it concurrently with another NexDome test process (global device lock).

## Original-driver baseline evidence

Environment: macOS 26 arm64, baseline binary described in step 3. `--known-defects`: 12 run, 12 failed as expected:

- NXD-01: the GOTO request with a 1.5 s reply returned to the client after 1.602 s.
- NXD-02: `DOME_ON_COORDINATES_SET` count 1 instead of 2.
- NXD-03: `DOME_PARK` did not settle in ALERT after a failed `g`.
- NXD-04: after an aborted park and a GOTO to the park azimuth the dome was reported PARKED.
- NXD-05: the aborted find-home settled OK ("Home Found.").
- NXD-06: a 10° clockwise move from 10.7° did not send `g 20.70` (it sent `g 20.00`).
- NXD-07: no low-voltage message for the second instance.
- NXD-08: after an open refused because of rain OPENED stayed selected.
- NXD-09: `NEXDOME_FIND_HOME` was defined BUSY again after reconnection.
- NXD-10: the open request completed OK with CLOSED selected while the shutter had not yet reported.
- NXD-11: driver version major/minor `0x2000`.
- NXD-12: a park stopped short did not settle in ALERT (stayed BUSY).

## Reference trace comparison

`original_reference_trace.txt` (218 lines) and `generated_reference_trace.txt`: every `S` (protocol) line is identical — connection sequence, poll content and order in every step, `g` arguments, `q` before relative moves and GOTO while parked, `n` before park, `h`, `c`, `a`, `d`, `e`, `y 1`/`y 0`, `w`, failure paths. All differences are property publications:

1. Queued changes publish BUSY at request time (`INDIGO_COPY_VALUES_PROCESS_CHANGE` / `…_URGENT_CHANGE`): additional BUSY lines for `DOME_PARK`, `DOME_ABORT_MOTION`, `DOME_STEPS` (steps while parked), `NEXDOME_REVERSED`; some `DOME_HORIZONTAL_COORDINATES` BUSY lines change order because the requested property is published first.
2. Connection: `DOME_PARK` is updated before the custom properties are defined (they are defined by generated code after `on_connect`); generated messages "Connected to NexDome on PORT" and "Disconnected from NexDome" are added.
3. `DOME_ON_COORDINATES_SET` is defined with count 2 (NXD-02).

## Intentional behaviour differences

- **Property names:** `NEXDOME_*` custom properties renamed to the required `X_` names (`NEXDOME_CALLIBRATE`/`CALLIBRATE` became `X_CALIBRATE`/`CALIBRATE`); clients using the old names must be updated. The trace maps the new names back to the original ones for comparison.
- **Serialization:** handlers and the status poll run on the device queue; a change request no longer blocks the client (NXD-01). GOTO, steps, shutter and other requests arriving while their property is BUSY are ignored by the framework guard (the original retargeted a moving dome). An urgent abort cancels queued motion, park, shutter, find-home and calibration requests and settles their properties. The poll ignores properties that are BUSY only because their request is still queued.
- **Transport:** the 100 ms quiet-period drain is a 100 ms pause followed by `indigo_uni_discard()`; unterminated, malformed or out-of-range replies are failures and are not published (the original accepted partial replies and wrote uninitialized values for malformed `m`/`u`/`y`); the reply buffer is `INDIGO_VALUE_SIZE`. The 1 s reset delay is applied only after a successful serial open.
- **Failure recovery and completion:** failed park → `DOME_PARK` ALERT, no rotation BUSY (NXD-03); park stopped away from the park azimuth → ALERT (NXD-12); abort clears the park request (NXD-04) and ends active find-home/calibration in ALERT (NXD-05); failed shutter/reversed/reset requests restore the switches (NXD-08); a shutter request completes only when the requested state (or a motion, error, unknown state) is reported, and ends ALERT "Shutter did not respond" after 30 s (NXD-10); operations interrupted by disconnect are reset at connection (NXD-09); poll state is per instance and per session (NXD-07); relative moves keep the fractional heading (NXD-06).
- **Version:** `0x0300000A` (original `0x020000009`, NXD-11).
- **Unchanged by decision:** message texts and labels including "Callibrate"; the step value is truncated to whole degrees; the shutter switch shows OPENED while opening or closing; INFO model/firmware are not reset on disconnect; the dome is not stopped on disconnect.

## Found defects

Every defect below was reproduced by a dedicated case that failed against the original driver and passes against the generated driver.

| ID | Observable impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- |
| NXD-01 | Clients block during device I/O (≥1.6 s for a slow reply, 2 s for a shutter link reset). | Synchronous I/O and `sleep(2)` in `change_property`. | Generated queued handlers, finalizer for the reset wait. | `NXD-01 change_does_not_block` |
| NXD-02 | Sync (`s`) unreachable. | Base count 1 kept. | Count 2. | `NXD-02 sync_is_reachable` |
| NXD-03 | Failed park stays BUSY forever. | `g` failure only logged. | `DOME_PARK` ALERT. | `NXD-03 failed_park_alerts` |
| NXD-04 | Aborted park later reports PARKED. | Park request not cleared on abort. | Abort clears it. | `NXD-04 aborted_park_is_cleared` |
| NXD-05 | Aborted find-home/calibration reports completion OK. | Completion decided only by stopped rotation. | Abort ends them in ALERT. | `NXD-05 aborted_home_and_calibration_not_complete` |
| NXD-06 | Relative moves up to 1° short. | `(int)` truncation of the heading. | `fmod` on doubles. | `NXD-06 relative_move_keeps_fraction` |
| NXD-07 | Second instance: no low-voltage message, heading never published. | `static` poll state shared. | Poll state in private data, reset per session. | `NXD-07 poll_state_per_instance` |
| NXD-08 | Rejected request leaves the requested switch selected. | Values copied before the command, not restored. | Switches restored from the last known state. | `NXD-08 failed_requests_restore_switches` |
| NXD-09 | Interrupted find-home resumes and completes after reconnection. | Property state survives delete/define. | Operation states reset in `on_connect`. | `NXD-09 interrupted_operation_not_resumed` |
| NXD-10 | Shutter open completes OK while still closed. | Any polled state completes a BUSY request. | Wait for the requested state with a 30 s timeout. | `NXD-10 shutter_open_waits_for_report`, `shutter_without_report_times_out` |
| NXD-11 | INFO driver version 32.0.0.9. | Extra digit in `DRIVER_VERSION`. | Generated version. | `NXD-11 driver_version` |
| NXD-12 | Park stopped short stays BUSY. | Park completion only on reaching the park azimuth. | ALERT when the rotation ends away from the target. | `NXD-12 park_stopped_short_alerts` |

Audit-only risks resolved without a dedicated reproducer: uninitialized values from malformed replies (covered by `status_read_failures_ignored` for the generated driver), one-byte overflow of the reply buffer, unterminated replies accepted (`partial_reply_is_bounded`), unsynchronized property updates from two threads, double global unlock on detach, POSIX-only I/O.

Remaining known limitations: a dropped TCP connection is not detected (polls fail, CONNECTION stays OK — as in the original); rotation has no stall detection beyond the target distance check after the controller reports stopped; the real firmware's shutter latency, rain and XBee behaviour are simulator assumptions from `nexdome_protocol.txt` and the author's `.ino`; network transport covered only against the loopback simulator; no hardware, Linux or Windows validation.

## Scenario-to-test mapping

| Area | Cases |
| --- | --- |
| Simulator protocol self-check (not driver coverage) | `simulator_protocol` |
| Metadata, INFO, interface, no I/O before connection | `metadata_before_connection`, NXD-11 |
| Connection sequence, reset delay, INFO, visible/hidden properties, custom property shapes, park/reversed/shutter state at connection, shutter version reply | `connect_sequence`, `connect_parked_reversed_open`, `reference_trace` |
| Open/identification failures, descriptor balance, recovery | `connection_failures_and_recovery` |
| GOTO BUSY→OK with elapsed motion, wrap, fractional target, repeat; stopped short | `goto_and_wrap`, `goto_stopped_short_alerts` |
| Relative moves, truncated steps, fractional heading | `relative_steps`, NXD-06 |
| Sync | NXD-02 |
| Parked dome refuses moves; park/unpark, park position change, failures | `moves_refused_when_parked`, `park_and_unpark`, NXD-03, NXD-12 |
| Find home and calibration (success, at home, not at home, failures, abort) | `find_home`, `calibration`, NXD-05 |
| Abort while rotating, idle, during park, during shutter motion, failed abort | `abort_rotation`, `abort_park`, `abort_shutter`, `command_failure_messages`, NXD-04 |
| Shutter open/close, repeated requests, not connected, unknown, rain, wireless latency and timeout | `shutter_open_close`, `shutter_states_and_rain`, `no_shutter_connection`, NXD-08, NXD-10, `shutter_without_report_times_out` |
| Reversed flag and shutter link reset | `reversed_flag`, `reset_shutter_communication`, NXD-01, NXD-08 |
| Power status, thresholds, messages | `power_status`, NXD-07 |
| Command and poll read failures, unterminated replies | `command_failure_messages`, `poll_read_failures`, `status_read_failures_ignored`, `partial_reply_is_bounded` |
| BUSY conflicts and queued-handler races | `busy_requests_rejected`, `busy_guard_holds_while_command_runs`, `urgent_abort_cancels_queued_goto`, `queued_request_survives_status_poll`, NXD-01 |
| Disconnect during motion, reconnect, interrupted operations | `disconnect_during_motion`, NXD-09 |
| INIT/SHUTDOWN idempotence and repeated cycles; additional instance | `lifecycle_and_shutdown`, `additional_instance`, NXD-07 |
| Network transport (opt-in `--network`) | `network_nexdome_url`, `network_default_port`, `network_failures` |

Not applicable: hot plug, guider timing (no guider interface), driver-owned persistent settings, `DOME_SPEED`/`DOME_FLAP`/`DOME_HOME`/`DOME_PARK_POSITION` (hidden), hardware acceptance.

## Final test summary

Trace captures count as runs of `reference_trace`; development runs of single cases are included.

- Simulated tests, original driver: 98 run, 59 passed. Breakdown: pre-existing smoke test 1/1; first complete characterization run (including the `simulator_protocol` self-check) 34 run, 26 passed (8 failures analysed in step 3); `connect_sequence` debug run 0/1 and after the test fix 1/1; defect reproducers 12 run, 0 passed (all failed as expected); trace captures 3/3; final harness with promoted reproducers 46 run, 28 passed (18 expected failures, step 10).
- Simulated tests, generated driver: 184 run, 150 passed. Breakdown: invalid concurrent complete run 34 run, 2 passed (global-lock conflict with the parallel trace capture, not driver behaviour); trace capture 1/1; sequential complete run 34 run, 32 passed; `shutter` cases after the fix 6/6; `simulator_protocol` after the test fix 1/1; defect reproducers 12/12; `reference_trace` 1/1; final complete run with promoted reproducers 46/46; ASan + UBSan complete run 46/46; opt-in network cases 3/3.
- Hardware tests: 0 run, 0 passed.
