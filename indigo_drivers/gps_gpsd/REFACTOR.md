# gps_gpsd refactoring record

## Scope and baseline

This record covers migration of `indigo_gps_gpsd` from its hand-written INDIGO 2.0 implementation to `indigo_generator`, a host-side gpsd protocol simulator, simulator-backed characterization and regression coverage, and production fixes proven by that coverage.

Baseline date and source: 2026-09-16, commit `d59866457` (`focuser_astroasis: migrated to code generator`). The working tree already contained unrelated, uncommitted `ccd_uvc` work in `MIGRATION_STATUS.md`, `indigo.xcodeproj/project.pbxproj`, `indigo_docs/PROPERTIES.md`, `indigo_test/Makefile`, `indigo_drivers/ccd_uvc/` and `indigo_test/`. This work must preserve those edits and add only its own entries. Host: macOS 26.6.2 (`Darwin 25.6.0`), Apple Silicon arm64.

Baseline build commands and results:

```sh
cd indigo_drivers/gps_gpsd
make -B -f ../../Makefile.drv
```

Result: failed before compiling the driver. Forcing all targets makes the generic `Makefile.drv` SDK rule try to regenerate `externals/libgps/Makefile` by running `./configure`, which does not exist for this vendored libgps (it ships a hand-written `Makefile`). This is a pre-existing build-system limitation, not a driver compile failure.

```sh
cd indigo_drivers/gps_gpsd
touch indigo_gps_gpsd.c
make -f ../../Makefile.drv
```

Result: passed without compiler or linker warnings. The driver object, archive, dynamic library and executable were built for x86_64 and arm64 against the already built universal `externals/libgps/libgps.a` (gpsd 3.20 client library, API 9.0). Build products in `externals/libgps` are untracked.

Baseline automated tests: none. No simulator, integration test or hardware test exists; `MIGRATION_STATUS.md` records `0 / 0`.

## Hardware-test decision

No hardware or real-gpsd testing will be performed. The user requested a gpsd simulator for testing; no GPS receiver is available and `gpsd`, `gpsfake` and `gpspipe` are not installed on this host. All evidence is simulator-backed software behaviour and does not imply validation against a real gpsd release or receiver.

## Test transport decision

libgps connects only over TCP (`gps_sock_open()` → `netlib_connectsock(AF_UNSPEC, host, port, "tcp")`); the bundled client has no Unix-socket path for the driver's `gpsd://host:port` addressing. The simulator therefore listens on `127.0.0.1` on an ephemeral port. `indigo_test/AGENTS.md` says not to open network sockets in the normal integration target; the repository already runs a loopback TCP simulator there (`mount_starbook_simulator` / `test_mount_starbook_simulator`), and the user explicitly requested a gpsd simulator. This test follows that precedent: loopback only, ephemeral port, launched and terminated by the test, no external network access. The deviation is recorded here and reported to the user.

## Studied sources

- `indigo_gps_gpsd.c`, `.h`, `_main.c`, `Makefile.inc`, `README.md` in this directory.
- Vendored gpsd 3.20 client library in `externals/libgps`: `gps.h`, `libgps_core.c` (`gps_open`/`gps_close`/`gps_read`/`gps_stream`), `libgps_sock.c` (TCP session, `?WATCH` command, `gps_waiting` via `pselect`, non-blocking line reader), `netlib.c` (blocking connect, then `O_NONBLOCK`), `libgps_json.c` (TPV/SKY/DEVICES/WATCH/VERSION/ERROR unpacking and `set` mask semantics), `gpsutils.c`, `gpsd_config.h`.
- gpsd JSON protocol documentation (`gpsd_json(5)`, https://gpsd.io/gpsd_json.html) and gpsd `NEWS` for the 3.20 `alt`/`altHAE`/`altMSL` change.
- `indigo_libs/indigo_gps_driver.c`, generator sources and `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `../gps_nmea/indigo_gps_nmea.driver`, `indigo_test/DRIVER_TESTING_RULES.md` (GPS standard), `indigo_test/integration/serial_simulator_test_common.h`, `../mount_starbook/mount_starbook_simulator`, `../mount_ioptron/mount_ioptron_simulator` (control/event file convention).

## Current-state audit

### Architecture and implementation

- Version `0x02000004`, label `GPS Sevice Daemon (GPSD) Client` (misspelling is part of the published driver description), device `GPSD Client`, author Thomas Stibor. One static device, no hot plug, no additional instances. Registered in `indigo_server/indigo_server.c`; no Windows project exists (`MIGRATION_STATUS.md` comment: libgps for Windows is TODO).
- `DEVICE_PORT` is visible with label `GPS daemon host`, item label `Hostname (host:port)` and default `gpsd://localhost:2947`; `DEVICE_PORTS` and `DEVICE_BAUDRATE` are hidden. The optional `gpsd://` prefix is stripped; without a colon the port is `2947`; host ≥ 128 bytes or port ≥ 15 bytes is rejected.
- Connection runs on a zero-delay timer: `gps_open(host, port)` (blocking TCP connect; libgps then makes the socket non-blocking), `gps_stream(WATCH_ENABLE | WATCH_JSON)` (result ignored, sends `?WATCH={"enable":true,"json":true};`), resets coordinates to 0, fix lights to IDLE, status/coordinates/UTC to BUSY and UTC text to `0000-00-00T00:00:00.00`, starts the reader on another zero-delay timer, publishes CONNECTION OK. Open failure publishes CONNECTION ALERT/disconnected.
- Disconnect runs on a timer thread: `gps_stream(WATCH_DISABLE)`, `gps_close()` (frees libgps private data and closes the socket), CONNECTION OK. SHUTDOWN refuses with `INDIGO_BUSY` while connected.
- The reader is an unbounded `while (IS_CONNECTED)` loop occupying a timer worker thread for the whole session. Each iteration calls `gps_waiting(200 ms)`; without data it marks GPS_STATUS BUSY (not published) and repeats. `gps_read() == -1` marks GPS_STATUS ALERT (not published), sleeps one second and repeats. Otherwise (including `gps_read() == 0`, a partial line) it processes the libgps `set` mask and publishes GPS_STATUS, GEOGRAPHIC_COORDINATES, UTC_TIME and, when enabled, GPS_ADVANCED_STATUS.

### Published data mapping

- Per processed message: UTC/coordinates/status start BUSY, advanced status OK, fix lights IDLE.
- `TIME_SET`: UTC from `fix.time.tv_sec` via `indigo_timetoisogm()` (sub-second part dropped), UTC OK.
- `LATLON_SET`: latitude/longitude, coordinates OK. `ALTITUDE_SET`: elevation from deprecated `fix.altitude` (JSON `alt`), coordinates OK.
- `MODE_SET`: mode 1 → NO_FIX light ALERT, 2 → 2D light BUSY, 3 → 3D light OK; status OK unless mode 0.
- DOP: PDOP/HDOP/VDOP copied when finite, regardless of `DOP_SET` (never set by this libgps). `SATELLITE_SET`: satellites used/visible; advanced state OK only if `DOP_SET` (always true path through the preceding OK assignment).
- libgps semantics that shape behaviour: TPV replaces the whole `set` mask (`STATUS_SET` plus field bits); SKY only adds `SATELLITE_SET`, so the last TPV bits and values are republished on each SKY; VERSION/DEVICES/WATCH/ERROR clear only the union bits.

### Properties

No custom properties. Inherited GPS properties used: `GPS_ADVANCED` (visible), `GPS_ADVANCED_STATUS`, `GEOGRAPHIC_COORDINATES` (count 3), `GPS_STATUS`, `UTC_TIME` (count 1), plus common `DEVICE_PORT` customization. No `X_` renaming is required.

### Defects and risks found by source audit

Identifiers are used in the found-defects section; each is either reproduced by a dedicated reproducer against the original driver or explicitly marked audit-only.

- **GPSD-01** (audit-only; see baseline) Reader lifetime is not owned by the connection: after disconnect the loop can still be inside `gps_waiting()` for up to 200 ms; a reconnect in that window starts a second reader, and both loops use the same `gps_data_t` concurrently.
- **GPSD-02** Disconnect calls `gps_close()` (which frees libgps private data) while the reader may be about to call or be inside `gps_waiting()`/`gps_read()`: use after free / calls after close.
- **GPSD-03** (audit-only; see baseline) SHUTDOWN immediately after disconnect frees the device while the old reader is still running and will evaluate `IS_CONNECTED` on freed memory.
- **GPSD-04** Loss of the gpsd connection is never reported: `gps_read() == -1` sets an unpublished ALERT, sleeps one second and retries forever on a dead socket while CONNECTION remains OK.
- **GPSD-05** Elevation is taken only from the deprecated `alt` field. A gpsd sending only `altHAE`/`altMSL` (the fields documented since 3.20) sets `ALTITUDE_SET` but leaves `fix.altitude` NaN, so NaN elevation is published as OK.
- **GPSD-06** GPS_ADVANCED_STATUS values (satellites, DOP) from a previous session remain published after reconnect until a new SKY arrives.
- **GPSD-07** After a partial JSON line, libgps keeps buffered bytes so `gps_waiting()` returns true immediately while `gps_read()` returns 0; the reader loop spins and republishes stale properties until the rest of the line arrives.
- Audit-only risks: the blocking TCP connect in `gps_open()` can occupy the connection handler for the operating system connect timeout on an unreachable host; `gps_stream()` failures are ignored (a failed send to a just-closed peer is not reliably reproducible); receiver silence has no freshness deadline (the GPS test standard forbids inventing one); a WATCH-disable request sent after gpsd closed the connection could raise SIGPIPE in the standalone driver executable (`indigo_server` ignores SIGPIPE; a single send after the peer's FIN normally does not raise it).

## Atomic plan

1. **Done — instructions, sources and baseline.** Read governing instructions, driver/libgps/build/project files, gpsd protocol documentation, generator semantics and the generated `gps_nmea` driver. Baseline build evidence is recorded above.
2. **Done — gpsd simulator.** Added `gps_gpsd_simulator/gps_gpsd_simulator.c` (single-threaded `select` loop, no threads). It sends VERSION (`release`/`rev` 3.20, `proto_major` 3, `proto_minor` 14) on accept; parses `;`/newline-terminated `?`-requests; answers `?WATCH={"enable":true,...}` with DEVICES then WATCH and starts TPV/SKY, `?WATCH={"enable":false,...}` with WATCH, `?WATCH`, `?VERSION`, `?DEVICES`, `?POLL`, and ERROR `Unrecognized request` otherwise. TPV follows the fix mode (time when valid; lat/lon for modes 2–3; altitude for mode 3 as `alt`, `altHAE`/`altMSL`/`geoidSep` or both). SKY sends DOP and the satellites array, optionally `nSat`/`uSat`. Control commands (`.control`) set mode/time/position/altitude/styles/satellites/DOP/period, emit TPV/SKY, send raw or split lines, close clients, refuse connections and suppress the banner; `.events` logs connections, requests, WATCH state, emissions and closes. Audit against the documentation and the bundled parser: every field the simulator emits is either in the libgps 3.20 TPV/SKY/DEVICE/WATCH tables or covered by their ignore rules, except `nSat`/`uSat`, which are deliberately emitted only in the `modern` SKY style to characterize a library limitation (below). A standalone `nc` smoke test showed the documented message sequence.
3. **Done — harness and original-driver characterization suite.** Added `indigo_test/integration/test_gps_gpsd_simulator.c` with Makefile targets `test-gps-gpsd-simulator` and `test-gps-gpsd-simulator-sanitize`. The production driver is compiled separately with seams around `gps_open`, `gps_close`, `gps_stream`, `gps_waiting` and `gps_read` that call the real vendored libgps and record call counts, open sessions, overlapping reader calls, `gps_close` during a reader call, calls after close and a trace. A thread-safe observer with `force_property_updates` tracks definitions, updates, messages and updates of undefined properties. 12 preservation cases pass against the unchanged driver. Two planned reproducers (GPSD-01, GPSD-03) passed on the original driver on macOS and were reclassified as regression cases; five reproducers (GPSD-02, GPSD-04..07) run in isolated child processes with `--known-defects` and fail as expected.
4. **Done — original reference trace.** `indigo_test/fixtures/gps_gpsd/original_reference_trace.txt` records libgps calls (normalized simulator port, returned status, `set` mask classes) and ordered publications of CONNECTION, GPS_STATUS (with lights), GEOGRAPHIC_COORDINATES, UTC_TIME, GPS_ADVANCED and GPS_ADVANCED_STATUS for connect, legacy-altitude 3D fix, SKY, advanced status, no fix, 2D fix, mode 0 without time, 3D fix with both altitude styles, an ERROR object and disconnect. Nine consecutive runs produced identical traces.
5. **Done — create `.driver` and regenerate.** Added `indigo_gps_gpsd.driver`: version 2.0.4 (`0x02000004`) → 5 (`0x03000005`), unchanged label/device name/author, `serial { no_ports = true; }` (visible `DEVICE_PORT`, no port enumeration or baud rate), transactional `gpsd_open`/`gpsd_close` with the original host/port parsing and libgps call order, `on_connect` resets and an `on_timer` reader. Generated `.c/.h/_main.c`; `make -f ../../Makefile.drv` passes for x86_64 + arm64 without warnings. The generated connection handler and `on_timer` both run on the device queue, so the reader cannot start before the connection handler has defined the GPS properties and set CONNECTION OK. No generator change.
6. **Done — post-migration characterization and trace comparison.** Only the expected version constant changed in the suite. All 12 preservation cases passed on the first run, including `reference_trace` against `original_reference_trace.txt`; five further explicit captures were byte-identical to the original fixture, so no separate generated fixture is needed and the suite keeps comparing against the original trace.
7. **Done — defect fixes.** All five reproducers passed on the generated driver. They were promoted into the ordinary suite, the isolated `--known-defects` mode was removed, and I/O-safety invariants (no overlapping reader calls, no `gps_close()` during a reader call, no reader call active at teardown) were enabled for every case. Three consecutive full runs passed 17/17. A mutation check (scratch copy of the generated source that publishes on `gps_read() == 0`) made `GPSD-07 partial_line_does_not_spin` fail with 4,428 publications.
8. **Done — repository integration and documentation.** The `.driver`, `REFACTOR.md` and simulator source were already registered in the Xcode `gps_gpsd` group by the user; `test_gps_gpsd_simulator.c` and `fixtures/gps_gpsd/original_reference_trace.txt` were added (`plutil -lint` OK, existing `ccd_uvc`/`focuser_astroasis` entries unchanged). `indigo_test/Makefile`: simulator, separately compiled driver object with libgps seams, test and sanitizer targets, `INTEGRATION_TESTS` entry. `indigo_docs/PROPERTIES.md`: property usage and `.driver` source. `MIGRATION_STATUS.md`: API 3, generator/async `✅ Yes`, retested `✅ Sim`, tests `17 / 0`; Windows stays `❌ No`, Comment unchanged. No Windows project exists; README unchanged.
9. **Done — final verification.** Evidence below.

## Original-driver baseline evidence

Environment: macOS 26.6.2 arm64; ordinary test build universal x86_64/arm64, sanitizer build arm64 only; the vendored `libgps.a` is linked uninstrumented.

- `make -C indigo_test test-gps-gpsd-simulator`: 12 run, 12 passed.
- `make -C indigo_test test-gps-gpsd-simulator-sanitize` (ASan + UBSan, `detect_leaks=0`): 12 run, 12 passed, no sanitizer report.
- `indigo_test/build/integration/test_gps_gpsd_simulator --known-defects`: 4 run, 4 failed as expected:
  - GPSD-02: `close_during_io` 1 — `gps_close()` ran while the reader was inside `gps_waiting()`.
  - GPSD-04: CONNECTION stayed OK/connected after the simulator closed the gpsd connection.
  - GPSD-05: elevation `nan` for a TPV carrying only `altHAE`/`altMSL`.
  - GPSD-06: satellites in use 7 (previous session) after reconnect.
  - GPSD-07 (added while designing the reader, before any production change): 3 of 3 runs failed; one TPV line split 100 ms apart produced 43,504 `UTC_TIME` publications.
- Not reproduced on this platform: GPSD-01 (second reader after quick reconnect) and GPSD-03 (reader still running after SHUTDOWN). Closing the libgps socket wakes the original reader's `pselect()` on macOS, so it observes the BUSY connection state and exits before the race window. `reconnect_single_reader` and `shutdown_after_disconnect_stops_reader` therefore pass on the original driver and are kept as regression cases, not as reproductions.

## Library limitation (not changed)

The vendored gpsd 3.20 client parses SKY with a strict attribute table that has no ignore rule. Later gpsd releases add `nSat` and `uSat`; libgps then stops parsing that SKY object at the first unknown attribute, so the DOP values emitted before it are applied but the satellite list is not, and `SATELLITE_SET` is not set. `gps_unpack()` still reports success. `advanced_satellites_and_dop` characterizes this: with the `modern` SKY style DOP updates while satellite counts keep their previous values. Fixing it requires updating the vendored library, which is outside this task.

## Post-migration evidence

- Reproducible generation: two further generator runs left the checked-in outputs unchanged: `.c` `133bd2bb1016b1714f48b31d9be49d93c838ed78`, `.h` `75fbf84ea6edd5c9a10873b0bf79d8edf1f95332`, `_main.c` `b7c6fb29cf9eab004f26b0740f89ba06fa4950ec`.
- `make -f ../../Makefile.drv` in this directory: passed, x86_64 + arm64, zero warnings. (`make -B` still fails on the pre-existing libgps `./configure` rule described in the baseline.)
- Strict syntax checks (`-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wshadow -Wformat=2`): generated driver zero warnings; simulator zero warnings after adding a printf format attribute to its event logger.
- `make -C indigo_test test-gps-gpsd-simulator`: 17 run, 17 passed in each of five full runs after promotion (the last after all edits).
- `make -C indigo_test test-gps-gpsd-simulator-sanitize` (arm64 ASan + UBSan, `detect_leaks=0`, vendored libgps uninstrumented): 17 run, 17 passed after all edits, no sanitizer report.
- `git diff --check` on changed text files and a formatting audit of the `.driver`, simulator and test: no trailing whitespace, tab indentation, no blank lines inside function bodies. Test artifacts removed with `make -C indigo_test test-clean`.
- Unavailable: Linux builds (where closing a socket does not wake `select()`, the case GPSD-01/03 target), Windows (libgps not ported), a real gpsd and GPS receiver.

## Reference trace comparison

The generated driver reproduces `original_reference_trace.txt` exactly: the same libgps call sequence (`gps_open`, `gps_stream(WATCH_ENABLE|WATCH_JSON)`, one `gps_read` per message, `gps_stream(WATCH_DISABLE)`, `gps_close`) and the same ordered GPS_STATUS, GEOGRAPHIC_COORDINATES, UTC_TIME, GPS_ADVANCED and GPS_ADVANCED_STATUS publications with identical values and states for every step. Behaviour changes are confined to paths the trace deliberately does not include and that are covered by the regression cases below.

## Intentional behaviour differences

- **Reader ownership:** the unbounded loop on a timer worker thread is replaced by an `on_timer` handler on the device queue. It polls with `gps_waiting(..., 0)`, processes at most 32 complete messages per run, reschedules immediately after a full batch or after 0.1 s when idle. The original blocked in `gps_waiting(200 ms)`; worst-case added latency is 0.1 s, and connection changes and teardown now wait for the reader instead of racing it (GPSD-01..03).
- **Partial lines:** `gps_read() == 0` no longer republishes stale properties; the reader waits for the rest of the line (GPSD-07).
- **Transport loss:** `gps_read() == -1` closes the session through the generated connection handler (WATCH disable, `gps_close`, property deletion) and publishes CONNECTION ALERT/disconnected with `Connection to gpsd lost` instead of retrying a dead socket every second (GPSD-04).
- **Elevation:** `alt` is still used when present (unchanged values for older gpsd); otherwise `altMSL`, otherwise `altHAE`. A non-finite altitude no longer marks coordinates OK by itself (GPSD-05).
- **Session reset:** satellites and DOP are reset to 0 and GPS_ADVANCED_STATUS to BUSY on connect, matching the existing reset of coordinates, lights and UTC (GPSD-06).
- **Messages:** generated connection messages (`Connected to GPSD Client on <port>`, `Failed to connect ...`, `Disconnected from ...`) are sent in addition to the unchanged driver log lines.
- **Generic C strings:** host/port copies use bounded `snprintf` with the same length limits instead of `strcpy`/`strncpy`.

## Found defects

| ID | Status | Observable impact | Root cause | Fix | Regression test |
| --- | --- | --- | --- | --- | --- |
| GPSD-01 | Audit-only (not reproduced on macOS) | Two readers could process the same libgps session after a quick disconnect/reconnect. | Reader loop lifetime not owned by the connection. | Queue-owned `on_timer` reader cancelled by the generated connection handler. | `reconnect_single_reader` plus the overlap invariant in every case |
| GPSD-02 | Reproduced | `gps_close()` frees libgps state while the reader is inside `gps_waiting()` (use after free / calls after close). | Disconnect ran on another timer thread than the reader. | Reader and connection handler serialized on the device queue; disconnect cancels and waits for the reader. | `GPSD-02 disconnect_closes_idle_session`, close-during-I/O invariant |
| GPSD-03 | Audit-only (not reproduced on macOS) | SHUTDOWN right after disconnect could free the device under a running reader. | Unowned reader thread. | Device queue owns the reader; detach deletes the queue after pending work. | `shutdown_after_disconnect_stops_reader`, active-I/O invariant |
| GPSD-04 | Reproduced | gpsd shutdown/restart leaves the device "connected" with frozen data and a one-second retry loop. | `gps_read() == -1` only set an unpublished ALERT and slept. | Close the session, publish CONNECTION ALERT with a message; reconnect works. | `GPSD-04 gpsd_loss_reported` |
| GPSD-05 | Reproduced | NaN elevation published as OK with gpsd sending only `altHAE`/`altMSL`. | Driver used deprecated `fix.altitude` only. | Fallback to `altMSL`, then `altHAE`; require a finite value. | `GPSD-05 modern_altitude_elevation` |
| GPSD-06 | Reproduced | Satellites/DOP from a previous session shown after reconnect. | Advanced status not reset on connect. | Reset values and state in `on_connect`. | `GPSD-06 reconnect_resets_advanced_status` |
| GPSD-07 | Reproduced | A TCP-split message caused ~43,000 stale property publications in 100 ms (CPU spin, client flood). | `gps_read() == 0` treated as a processed message while `gps_waiting()` stayed true. | Stop the batch on a partial read and poll again after 0.1 s. | `GPSD-07 partial_line_does_not_spin` |

## Scenario-to-test mapping

GPS class standard (`indigo_test/DRIVER_TESTING_RULES.md`):

| Area | Cases |
| --- | --- |
| Metadata, interface, DEVICE_PORT customization, hidden common properties, connected inventory and counts, advanced toggle define/delete | `metadata_and_properties` |
| Host/port parsing (prefix, default port, too long host/port), WATCH enable/disable request order, open failure and recovery | `port_parsing_and_open_failures`, `connection_refused_and_recovery` |
| Input and framing: split messages, consecutive bursts, ERROR/unknown class, malformed JSON, non-JSON lines | `message_framing_and_malformed_input`, `GPSD-07` |
| Coordinates and time: 3D fix values and hemispheres, legacy/modern/both altitude, UTC mapping and second truncation, no time | `fix_lifecycle_and_mapping`, `GPSD-05`, `reference_trace` |
| Fix lifecycle: 3D → 2D → no fix → mode 0 → reacquisition, SKY republishing last TPV state, periodic streaming | `fix_lifecycle_and_mapping`, `periodic_stream`, `reference_trace` |
| Advanced data: satellites used/visible, DOP retention when absent, disabled advanced status not published, newer SKY `nSat`/`uSat` library limitation | `advanced_satellites_and_dop` |
| Reader lifetime and errors: disconnect while waiting, transport loss, reconnect with fresh data and reset state, quick reconnect, SHUTDOWN while connected and immediately after disconnect | `disconnect_reconnect_fresh_data`, `shutdown_while_connected`, `reconnect_single_reader`, `shutdown_after_disconnect_stops_reader`, `GPSD-02`, `GPSD-04`, `GPSD-06` |
| Ordered compatibility contract | `reference_trace` |

Not applicable: constellation/talker selection and receiver commands (gpsd is only watched, never configured), accuracy item (not published), guider timing (no guider), hardware acceptance (no receiver or gpsd). Receiver silence has no freshness deadline in the original driver and none was invented.

## Final test summary

- Simulated tests, original driver: 12 preservation cases run, 12 passed (ordinary); 12 run, 12 passed (ASan/UBSan); 5 defect reproducers run, 0 passed (all failed as expected).
- Simulated tests, final generated driver: 17 registered cases; final ordinary run 17 run, 17 passed; final ASan/UBSan run 17 run, 17 passed.
- Hardware tests: 0 run, 0 passed.
