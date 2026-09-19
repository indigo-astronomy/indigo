# aux_dragonfly refactoring record

This record covers `indigo_aux_dragonfly` and the shared Dragonfly transport/relay/sensor
implementation in `shared/dragonfly_shared.c`. `indigo_dome_dragonfly` includes the same
shared file and is migrated in the same change; its device-specific record is
`../dome_dragonfly/REFACTOR.md`. Read both together.

## Scope and baseline

Migration of `indigo_aux_dragonfly` (Lunatico Astronomia Dragonfly relay controller) from its
hand-written INDIGO 2.0 implementation (legacy POSIX `select`/`read`/`close` on a raw fd,
`pthread` port mutex, `indigo_set_timer` timer threads, per-logical-device property arrays)
to `indigo_generator` with portable `indigo_uni_io` and the device handler queue, plus full
applicable hardware-free simulator coverage.

Baseline date and source: 2026-09-19, commit `371b2d08a`
(`ccd_playerone: retry a rejected fan power write instead of reporting success`),
branch `refactoring`, clean working tree.
Host: macOS 26.6.2 (`Darwin 25.6.0`), Apple Silicon arm64; repository universal build
(x86_64 + arm64).

Baseline build:

```sh
cd indigo_drivers/aux_dragonfly
make -B -f ../../Makefile.drv
```

Result: passed, 0 lines containing `warning`. Archive, dynamic library and standalone
executable built for x86_64 and arm64.

Baseline automated tests:

```sh
make -C indigo_test test-aux-dragonfly-simulator
```

Result: 5 scenarios run, 5 passed (`normal`, `pulse`, `wrong_identity`, `relay_error`,
`oversized_reply`; 6.1 s total). `MIGRATION_STATUS.md` records `5 / 0`.

Driver-instrumented ASan baseline:

```sh
cd indigo_test
make build/integration/test_aux_dragonfly_simulator_asan
AUX_TEST_FILTER=oversized ./build/integration/test_aux_dragonfly_simulator_asan
```

Result: 1 scenario run, 1 passed, no sanitizer report. The previously recorded `DRV-087`
stack-buffer-overflow no longer reproduces: it was fixed in commit `fca8eb601` and
`indigo_drivers/REVIEW.md` already records `DRV-087` as Closed. The stale "`DRV-087` remains
open" statement in the previous version of this file was wrong and is superseded by this
record.

`build/bin/indigo_generator` is present in the build tree, built from the unchanged
`indigo_tools/indigo_generator.c`.

## Hardware-test decision

No hardware testing will be performed. The user has stated that no Dragonfly controller is
available. Nothing in this record implies physical validation; every result below is
simulator-backed software behaviour.

## Studied sources

- `indigo_aux_dragonfly.c`, `.h`, `_main.c`, `Makefile.inc`, `README.md` in this directory.
- `shared/dragonfly_shared.c` (shared with `dome_dragonfly`).
- `relio_simulator/relio_simulator.pl` and `relio_simulator/set_sensor.pl` — the author's
  UDP reference simulator; the authoritative in-repository source for reply forms the public
  guide does not spell out (`!seletek echo:0#`, the `!perm <command>:error -500 protected#`
  permission-denied form, `-1` for unknown commands, single-channel `rldgrd`/`snanrd`
  variants, `sndgrd` thresholding at 512).
- Public manufacturer references (consulted, not copied into the repository):
  the Seletek developer guide (<https://lunaticoastro.com/seletek-developers-guide/>) for SLP
  framing, UDP transport, port/channel ranges and the `!seletek version:<MOPFF>` encoding, and
  the Dragonfly JavaScript API (<https://lunaticoastro.com/df-javascript/>) for relay pulse
  units and sensor semantics. The guide defers the complete command catalogue to a
  spreadsheet available on request, so undocumented reply details are taken from
  `relio_simulator.pl`.
- `aux_dragonfly_simulator/aux_dragonfly_simulator.c`,
  `indigo_test/simulator_common/aux_simulator_common.h`,
  `indigo_test/integration/test_aux_dragonfly_simulator.c`,
  `indigo_test/integration/aux_test_isolation.h`, `indigo_test/Makefile`.
- `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (shared scope, AUX section),
  `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`,
  `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`.
- `indigo_libs/indigo_uni_io.c` (`indigo_uni_open_url()`, `indigo_uni_read_available()`,
  `indigo_uni_discard()`, `indigo_uni_wait_for_data()`, and the fact that `read_data()` on a
  UDP handle consumes one datagram per call regardless of the requested length, which rules
  out `indigo_uni_read_section()` for this protocol), `indigo_libs/indigo_timer.c`
  (one worker thread per handler queue), `indigo_libs/indigo_driver.c`
  (`indigo_execute_handler*` route slave work onto the master device queue).
- Sibling generated migrations: `aux_mgbox` (AUX GPIO outlets, outlet names, pulse lengths),
  `aux_uch` (shared serial connection, variadic command helper),
  `mount_nexstaraux` and `gps_gpsd` (`serial { no_ports = true; }` network transport),
  `dome_skyroof` and `dome_talon6ror` (generated roll-off roof, finalizer pattern).

## Current-state audit

### Protocol (shared by both drivers)

Transport is UDP to port 10000 of the controller (`udp://<host>`); the driver also accepts a
full URL in `DEVICE_PORT`. Every request is one datagram `!<words>#`; every reply is one
datagram that echoes the request without the trailing `#`, followed by `:<payload>#`.

| Command | Reply | Use |
| --- | --- | --- |
| `!seletek version#` | `!seletek version:<MOPFF>#` | identity handshake |
| `!seletek echo#` | `!seletek echo:0#` | keep-alive (dome only) |
| `!aux earnaccess#` | `!aux earnaccess:<level>#` | query access level |
| `!aux earnaccess <password>#` | `!aux earnaccess <password>:<level>#` | raise access level |
| `!relio snanrd 0 0 7#` | `!relio snanrd 0 0 7:<v0>,…,<v7>#` | read all 8 analog sensors |
| `!relio rldgrd 0 0 7#` | `!relio rldgrd 0 0 7:<r0>,…,<r7>#` | read all 8 relays |
| `!relio rlset 0 <ch> <0\|1>#` | `!relio rlset 0 <ch> <v>:<state>#` | set one relay |
| `!relio rlpulse 0 <ch> <ms>#` | `!relio rlpulse 0 <ch> <ms>:0#` | pulse one relay for `<ms>` ms |

`<MOPFF>` decodes as `oper = v / 10000` (0 normal, 1 bootloader, ≥ 2 error),
`model = (v / 1000) % 10` (0 error, 1 Seletek, 2 Armadillo, 3 Platypus, 4 Dragonfly,
5 Limpet), `fwmaj = (v / 100) % 10`, `fwmin = v % 100`. `4529` is Dragonfly firmware 5.29,
the minimum the README requires.

Access levels: 1 read only, 2 read/write, 3 full. When the controller is password protected
and the level is insufficient, a write command answers `!perm <request>:error -500 protected#`
instead of the echoed form, which fails the driver's `sscanf` and is reported as a failed
command. Authentication expires about 30 s after the last command, which is why the dome
device sends `!seletek echo#` every 10 s.

Unknown commands answer `!<request>:-1#`. The driver treats any negative payload as failure.

### Architecture and implementation (original)

- `DRIVER_VERSION 0x02000006`, device name `Dragonfly Controller`, label
  `Lunatico Dragonfly Relay Controller`, INDIGO 2.0 API. Listed in root `STABLE_DRIVERS` and
  registered in `indigo_server`. Marked "no Windows" in `MIGRATION_STATUS.md` because the
  transport is POSIX `select`/`read`/`close`.
- `indigo_aux_dragonfly()` refuses INIT when `indigo_driver_initialized("indigo_dome_dragonfly")`
  is true, so only one of the two drivers can be loaded at a time.
- A `device_data[MAX_PHYSICAL_DEVICES=4][MAX_LOGICAL_DEVICES=2]` matrix plus
  `create_port_device()` / `delete_port_device()` scaffolding survives from the shared
  Lunatico code base, but INIT only ever creates `(0, 0)`. The matrix, the
  `gp_bits` connected flag and logical-device index, and `at_least_one_device_connected()`
  are dead weight for this driver.
- Transport (`lunatico_command()` in `shared/dragonfly_shared.c`): locks
  `PRIVATE_DATA->port_mutex`, drains at most one pending datagram with a 100 ms `select`,
  writes the command with `indigo_write()`, optionally sleeps, then waits 3.1 s for the reply
  datagram and reads up to `max` bytes with `read()`, terminates the buffer and unlocks.
  Callers pass `sizeof(response) - 1`.
- `lunatico_get_info()` parses the version reply; `lunatico_command_get_result()` rebuilds the
  expected echo prefix from the request and `sscanf`s a single `%d` payload;
  `lunatico_analog_read_sensors()` and `lunatico_read_relays()` parse the eight-value forms;
  `lunatico_set_relay()` and `lunatico_pulse_relay()` are thin wrappers;
  `lunatico_keep_alive()` sends `!seletek echo#`; `lunatico_authenticate()` /
  `lunatico_authenticate2()` send `!aux earnaccess …#` and report the earned level with
  `indigo_send_message()`.
- `lunatico_open()` / `lunatico_close()` reference-count `PRIVATE_DATA->count_open` and set or
  clear the `gp_bits` connected flag.
- Connection (`handle_aux_connect_property`, run on a one-shot timer thread): open socket,
  `!seletek version#`, reject anything whose decoded model is not `Dragonfly` (close and
  ALERT), copy model and firmware into INFO, read all relays into `AUX_GPIO_OUTLETS`
  (ALERT on failure but connection still succeeds), define the three connection-dependent
  properties, authenticate with the stored password, start the 1 s sensor timer.
- Sensor timer (`sensors_timer_callback`, timer thread, rescheduled every 1 s): reads all
  eight analog sensors into `AUX_GPIO_SENSORS` and publishes OK, or publishes ALERT.
- `AUX_GPIO_OUTLETS` change (`set_gpio_outlets`, client thread, blocking): reads the current
  relay states, then for every item whose requested value differs from the device state
  either pulses the relay (when the matching pulse length is > 0, the item was switched on and
  no pulse is already running for that relay) and arms a per-relay timer at
  `(length_ms + 20) / 1000` s that clears the item, or sets the relay (when the pulse length
  is 0, or the item was switched off and no pulse is running).
- `AUX_OUTLET_NAMES` / `AUX_SENSOR_NAMES` changes relabel the outlet, pulse-length and sensor
  items and re-define the affected properties while connected; both are saved by
  `CONFIG_SAVE`.
- `AUTHENTICATION` change re-authenticates (empty password → `!aux earnaccess#`,
  non-empty → `!aux earnaccess <password>#`).
- Disconnect cancels the eight relay timers and the sensor timer synchronously, deletes the
  connection-dependent properties and closes the socket. Detach disconnects synchronously.

### Public properties (original)

Always defined: `AUX_OUTLET_NAMES` (8 text items, persistent),
`AUX_SENSOR_NAMES` (8 text items, persistent).
Defined while connected: `AUX_GPIO_OUTLETS` (8 switches, any-of-many),
`AUX_OUTLET_PULSE_LENGTHS` (8 numbers, 0…100000 ms, step 100),
`AUX_GPIO_SENSORS` (8 read-only numbers, 0…1024).
Inherited and unhidden: `DEVICE_PORT` (default `udp://dragonfly`, label `Devce URL` — a typo
in the original), `AUTHENTICATION` (count 1, password only). `INFO` count is raised to 6 so
the model and firmware revision items are published. `DEVICE_PORTS` stays hidden because the
transport is not a serial port.

### Defects, risks and gaps found by audit

See the "Found defects" section; it is the authoritative list and is kept current.

### Build and packaging

`Makefile.inc` adds a dependency of `indigo_aux_dragonfly.c` on `shared/dragonfly_shared.c`
with a `touch` recipe. That recipe collides with the generator rule in `Makefile.drv`, which
also has a recipe for the generated `.c`, so it has to become a dependency of the object file
instead. The Xcode project already references `indigo_aux_dragonfly.[ch]`, `_main.c` and
`shared/dragonfly_shared.c`; the new `.driver` input and the new simulator/test sources must
be added.

### Existing simulators and tests

`aux_dragonfly_simulator/aux_dragonfly_simulator.c` is a 40-line loopback UDP simulator built
on `indigo_test/simulator_common/aux_simulator_common.h`. It answers `version`,
`earnaccess`, `snanrd`, `rldgrd`, `rlset` and `rlpulse`, tracks relay state and pulse
expiry, and offers the profiles `normal`, `wrong-model`, `relay-error`, `short-sensors` and
`oversized`. Gaps: no `echo`, no password/permission model, no unknown-command reply, no
no-reply/timeout profile, no sensor model that can drive a roof, no single-channel command
forms, and no way to exercise `dome_dragonfly` at all.

`indigo_test/integration/test_aux_dragonfly_simulator.c` registers five scenarios
(`normal`, `pulse`, `wrong_identity`, `relay_error`, `oversized_reply`). Gaps: no
property-inventory/visibility assertions, no outlet/sensor rename coverage, no
authentication coverage, no reconnect, no disconnect-during-pulse, no overlapping pulses, no
sensor-read failure, no pulse-length persistence, no `SHUTDOWN`-while-connected rejection.

`dome_dragonfly` has no simulator and no automated test at all
(`MIGRATION_STATUS.md` records `0 / 0`).

## Migration design

- One `indigo_aux_dragonfly.driver` with `driver dragonfly { … aux { … } }`, so the entry
  point stays `indigo_aux_dragonfly`, the generated private data type is
  `dragonfly_private_data` and the connection helpers are `dragonfly_open()` /
  `dragonfly_close()`. `dome_dragonfly` uses the same driver name, so both generated drivers
  expose exactly the same symbols to the shared file and it stays a single implementation.
- `serial { no_ports = true; }` selects the generated `DEVICE_PORT` plumbing and the
  `<driver>_open` / `<driver>_close` wiring without publishing a serial-port list, matching
  `mount_nexstaraux` and `gps_gpsd`.
- `shared/dragonfly_shared.c` is included from the `.driver` `code { }` block of both
  drivers, so it is emitted into the generated low-level code section, after the private data
  and property macros it uses. It is parameterised by `RELAY_FIRST` / `RELAY_COUNT` /
  `SENSOR_FIRST` / `SENSOR_COUNT` in each driver's `define { }` block, which is what lets the
  identical relay/sensor/pulse code serve the AUX driver (relays 1…8, sensors 1…8) and the
  dome driver's AUX device (relays 4…8, sensors 3…7).
- The conflicting-driver guard moves into the driver-scope `on_init { }` block.
- Transport moves to `indigo_uni_io`: `indigo_uni_open_url(url, 10000, INDIGO_UDP_HANDLE, …)`,
  `indigo_uni_discard()`, `indigo_uni_printf()` and a single
  `indigo_uni_wait_for_data()` + `indigo_uni_read_available()` datagram read.
  `indigo_uni_read_section()` cannot be used: on a UDP handle it reads one byte at a time and
  each read consumes a whole datagram, so a delimited read would destroy the reply.
- The `pthread` port mutex is removed. All driver work runs on the master device's handler
  queue, which has one worker thread and, per `indigo_execute_handler*`, also carries the
  slave device's handlers, so every transaction of one physical controller is already
  serialised.
- The per-relay timers collapse into one `relay_pulse_finalizer` scheduled at the earliest
  pending pulse deadline, with deadlines kept in monotonic time.
- `AUX_GPIO_SENSORS` polling becomes the AUX device's generated `on_timer`.

## Atomic migration plan

Steps are executed in order. Each step records its state and evidence immediately after it is
performed.

1. **Record audit, baseline and plan** (this file and `../dome_dragonfly/REFACTOR.md`).
   State: **done**. Evidence: baseline build and test results above.
2. **Extend the Dragonfly device simulator** to a complete protocol model shared by both
   drivers: full command catalogue including `echo`, single-channel forms and unknown
   commands, password/permission model, analog sensor model with open/closed/park sensors,
   roll-off roof kinematics driven by relays 1…3 in all three documented wirings, and the
   failure profiles needed by both suites.
   State: **done**, see "Step 2".
3. **Extend `test_aux_dragonfly_simulator.c` to the full AUX acceptance suite against the
   unchanged driver** (characterization first), and record the normalized reference trace.
   State: **done**, see "Step 3".
4. **Add `test_dome_dragonfly_simulator.c` as the full dome + AUX acceptance suite against the
   unchanged `dome_dragonfly`**, and record its normalized reference trace.
   State: **done**, see `../dome_dragonfly/REFACTOR.md` step 4.
5. **Migrate `aux_dragonfly` to the generator**: write `indigo_aux_dragonfly.driver`, refactor
   `shared/dragonfly_shared.c` onto `indigo_uni_io` and the handler queue, regenerate, build,
   re-run the suite and compare traces.
   State: **done**, see "Step 5".
6. **Migrate `dome_dragonfly` to the generator** on the same shared file, regenerate, build,
   re-run both suites and compare traces.
   State: **done**, see `../dome_dragonfly/REFACTOR.md` step 6.
7. **Repository integration**: `Makefile.inc` of both drivers, `indigo_test/Makefile`,
   Xcode project groups, Windows projects and solution, `indigo_docs/PROPERTIES.md`,
   `MIGRATION_STATUS.md`.
   State: **done**, see "Step 7".
8. **Final verification**: regeneration reproducibility, strict build with warnings check,
   driver-instrumented ASan runs of both suites, re-run of every suite affected by the shared
   harness change, `git diff` audit, test artifact cleanup.
   State: **done**, see "Step 8".

## Step evidence

### Step 2 — Dragonfly device simulator

The device model moved to
`aux_dragonfly_simulator/dragonfly_simulator_common.h` and is included by
`aux_dragonfly_simulator/aux_dragonfly_simulator.c` and by the new
`../dome_dragonfly/dome_dragonfly_simulator/dome_dragonfly_simulator.c`, mirroring the way
the two drivers share `shared/dragonfly_shared.c`.

It now answers the complete command set the two drivers and the author's reference simulator
use — `!seletek version#`, `!seletek echo#`, `!aux earnaccess[ <password>]#`, the eight-value
and single-channel `snanrd`, `sndgrd` and `rldgrd` forms, `rlset`, `rlpulse` and the
`!<request>:-1#` unknown-command reply — and models relay state, pulse expiry, the
password/permission level with the `!perm <request>:error -500 protected#` denial, the
analog sensors, and roll-off roof kinematics for all three wirings of
`../dome_dragonfly/README.md` (1-button edge toggle on relay 1, 2-button hold on relays 2/3,
3-button edge on relays 1/2/3).

Sensor model: channel 1 is 900 when the roof is fully open and 10 otherwise, channel 2 is 900
when fully closed, channels 3…7 are the constants 13…17, channel 8 is 900 when the mount is
parked. 900 and 10 straddle the 512 threshold the protocol and the driver use.

A profile is a `+` separated set of independent flags, so one scenario can combine device
conditions: `slow-roof` (600 s travel), `open`, `half-open`, `two-button`, `three-button`,
`not-parked`, `both-sensors`, `protected`, `wrong-model`, `bad-version`, `short-sensors`,
`sensor-error`, `relay-error`, `relay-read-error`, `pulse-error`, `echo-error`, `no-reply`
and `oversized` (a 200-byte datagram, which is the `DRV-087` regression case).

Build: `make -C indigo_test build/integration/aux_dragonfly_simulator build/integration/dome_dragonfly_simulator`
— passed, no warnings, universal binaries.

Protocol spot check against the author's `relio_simulator/relio_simulator.pl` reply forms
(version, echo, earnaccess, both eight-value reads, rlset, rlpulse expiry, unknown command,
permission denial, the three wirings, the `not-parked`, `oversized`, `no-reply` and
`wrong-model` profiles) — all replies matched the reference forms. One simulator defect was
found and fixed during this check: the denial reply carried the request's `#` inside the
echoed text (`!perm relio rlset 0 3 1#:error…`) instead of `!perm relio rlset 0 3 1:error…`.

### Step 3 — aux_dragonfly characterization suite

`indigo_test/integration/test_aux_dragonfly_simulator.c` grew from 5 to 17 scenarios. The
AUX class checklist of `indigo_test/DRIVER_TESTING_RULES.md` maps onto them as follows.

| Scenario | Profile | Covers |
| --- | --- | --- |
| `identity_and_inventory` | normal | driver metadata, the `INDIGO_INTERFACE_AUX_GPIO` bit, the complete published and hidden property inventory before and after connection, every item of every driver property, sensor ranges, the model and firmware readback |
| `sensors` | normal | all eight analog sensor inputs and the OK state of the poll |
| `relays` | normal | all eight relays on and off, reported from the controller readback |
| `pulse` | normal | a timed pulse that the driver clears when the controller releases the relay |
| `overlapping_pulses` | normal | two pulses of different lengths started back to back, both completing |
| `names` | normal | renaming outlets and sensors relabels the dependent items and republishes the properties |
| `authentication` | protected | a write rejected by the permission level, then `AUTHENTICATION` earning read/write and the same write succeeding |
| `wrong_identity` | wrong-model | connection refused for a controller that is not a Dragonfly |
| `unparsable_identity` | bad-version | connection refused for an identity reply that does not parse |
| `silent_device` | no-reply | connection refused for a controller that does not answer, within the reply timeout |
| `oversized_reply` | oversized | a 200-byte datagram into the 100-byte response buffer; the `DRV-087` regression case |
| `relay_error` | relay-error | a rejected relay write leaves the outlet property ALERT |
| `relay_read_error` | relay-read-error | an unparsable relay readback fails the whole outlet change |
| `sensor_error` | short-sensors | a truncated sensor reply is ALERT, publishes no partial values and does not take the device down |
| `reconnect` | normal | disconnect and reconnect, the connection-dependent properties are deleted and republished, the poll resumes |
| `lifecycle` | normal | SHUTDOWN refused while connected, redundant disconnect tolerated |
| `disconnect_during_pulse` | normal | disconnecting with a long pulse running, then reconnecting and reading the relay back from the controller |

Not applicable for this driver class and device: exposure, guiding, motion and coordinate
scenarios (no such interface), hot-plug and multiple devices (the driver creates one device
at INIT and the controller is addressed by URL), and hardware acceptance (no device
available).

```sh
make -C indigo_test test-aux-dragonfly-simulator
```

Result against the **unchanged** driver: 17 scenarios run, 17 passed, 0 failing scenarios.
There is no expected-failure reproducer: every behaviour the suite asserts is behaviour the
original driver already has.

Two test-side findings while writing the suite, both harness facts rather than driver
defects, recorded here because they constrain what a driver test may assert:
`context.defined_properties` in `simulator_test_common.h` is append-only, so live property
visibility after a disconnect has to be read from the property cache; and
`indigo_update_property()` suppresses the client callback when neither the state nor any item
changed, so a test cannot prove a poll is still running by waiting for another identical
update. `sensor_error` proves it through a subsequent successful relay command instead.

Reference trace. `AUX_TEST_TRACE=1` (added to `run_aux_simulated()` in
`indigo_test/integration/aux_test_isolation.h`) starts every simulator with `--trace`, which
logs the ordered protocol exchange to stderr. Normalization drops the free-running
`!relio snanrd 0 0 7#` poll, whose interleaving with command traffic is inherently
nondeterministic, and collapses consecutive duplicates:

```sh
cd indigo_test
AUX_TEST_TRACE=1 AUX_TEST_FILTER=<scenario> ./build/integration/test_aux_dragonfly_simulator 2>&1 >/dev/null \
  | grep -E '^(->|<-) ' | grep -v 'snanrd 0 0 7' | awk '$0 != prev { print } { prev = $0 }'
```

Traces were captured for `identity_and_inventory`, `relays`, `overlapping_pulses`, `names`,
`authentication`, `relay_error`, `lifecycle` and `reconnect`, and verified to be stable
across repeated runs. They record the manner the migration must preserve, in particular that
every `AUX_GPIO_OUTLETS` change is preceded by one `!relio rldgrd 0 0 7#` readback and that
only channels whose readback differs from the request are written.

### Step 5 — aux_dragonfly migrated to the generator

New `indigo_aux_dragonfly.driver` (one `aux` device, `serial { no_ports = true; }`) plus a
refactored `shared/dragonfly_shared.c` and the new `shared/dragonfly_shared.h`. The header
holds the constants the private data declaration needs and is included from the `define { }`
block of both definitions; the implementation is included from their `code { }` blocks, which
the generator emits after the private data and property macros. `DRIVER_VERSION` is now
`0x03000007` (was `0x02000006`).

What the shared file gained: a `va_list` forwarding command helper family
(`dragonfly_vcommand` does the work, `dragonfly_command`, `dragonfly_command_value` and
`dragonfly_command_ok` forward their `va_list` to it), a pulse finalizer, the relay/sensor window parameterisation by
`RELAY_FIRST` / `RELAY_COUNT` / `SENSOR_FIRST` / `SENSOR_COUNT`, and the transactional
`dragonfly_open()` / `dragonfly_close()` the generator wires into the connection handler.
What it lost: `pthread` port locking, `count_open` reference counting, the `gp_bits`
connected flag and logical-device index, and all POSIX `select` / `read` / `close` calls.

The whole `device_data[MAX_PHYSICAL_DEVICES][MAX_LOGICAL_DEVICES]` matrix,
`create_port_device()` / `delete_port_device()` and `at_least_one_device_connected()` are
gone; the generator owns device creation, `VERIFY_NOT_CONNECTED` and teardown.

The formatted request is materialised into `PRIVATE_DATA->command` and sent with one
`indigo_uni_write()` rather than written straight through `indigo_uni_vprintf()`, for two
reasons. First, one write is one datagram, and `indigo_uni_vprintf()` with a `%` in the
format would allocate and copy through a large buffer on every command. Second, this
protocol derives the expected reply from the request: the controller echoes the request without its trailing `#` and appends
`:<value>#`, and `dragonfly_parse_value()` builds the `sscanf` format from that stored text,
exactly as the original `lunatico_command_get_result()` did. The response buffer lives in the
private data and is reused across transactions.

**Transport choice.** `indigo_uni_discard()` now drains pending input before every request,
as the repository rules require. The reply is read with one
`indigo_uni_wait_for_data()` + `indigo_uni_read_available()` instead of the preferred
`indigo_uni_read_section()` / `indigo_uni_read_section2()`, because the delimited readers
cannot work on a datagram socket. Measured against the simulator with a small probe built on
the production library:

```
read_section2(handle, buffer, 127, "#", "", 2s, 1s) -> 1 '!'
read_available(handle, buffer, 127)                 -> 22 '!seletek version:4529#'
```

`indigo_uni_read_section2()` reads one byte at a time through `read_data()`, which calls
`read(fd, buffer, 1)`; on a `SOCK_DGRAM` socket that consumes the entire datagram and
discards everything past the requested length, so the reader got `!`, lost the other 21
bytes and then timed out waiting for a second byte that can never arrive. On Windows the
same call would fail outright with `WSAEMSGSIZE`. A Dragonfly reply is framed by the datagram
boundary, so no delimiter search is needed; this is the documented case for a custom reader.

Build: `make -B -f ../../Makefile.drv` — passed, 0 warnings, archive, dynamic library and
executable for x86_64 and arm64. Strict check
`clang -fsyntax-only -Wall -Wextra indigo_aux_dragonfly.c` — clean.

Suite: `make -C indigo_test test-aux-dragonfly-simulator` — 17 scenarios run, 17 passed,
**with no change to the test source**, so every behaviour the characterization suite pinned
down is preserved.

Reference trace comparison: all eight captured traces (`identity_and_inventory`, `relays`,
`overlapping_pulses`, `names`, `authentication`, `relay_error`, `lifecycle`, `reconnect`)
are **byte identical** to the pre-migration traces. The ordered protocol exchange, including
the `!relio rldgrd 0 0 7#` readback before every outlet change and the write of only the
channels whose readback differs, is unchanged.

Intentional differences, none of which appear in the protocol trace:

- Generated change dispatch (`INDIGO_COPY_VALUES_PROCESS_CHANGE`) publishes the accepted
  request as BUSY before the handler runs, so `AUX_GPIO_OUTLETS`, `AUX_OUTLET_NAMES`,
  `AUX_SENSOR_NAMES` and `AUTHENTICATION` now show OK → BUSY → OK/ALERT instead of a single
  final update. It also drops a change that arrives while the property is BUSY, which the
  original processed.
- The property handlers no longer run on the bus thread; see defect `DF-01`.
- Connect and disconnect now send the generated `Connected to …` / `Disconnected from …`
  messages.
- `DEVICE_PORT`'s label is `Device URL` (the original had the typo `Devce URL`).
- The eight per-relay timers became one `relay_pulse_finalizer` scheduled at the earliest
  pending pulse deadline in monotonic time, cancelled and rearmed when a new pulse starts.
  `overlapping_pulses` and `pulse` cover it and their traces are unchanged.
- `indigo_uni_discard()` drains every pending datagram instead of at most one; see `DF-02`.

### Step 7 — repository integration

- `Makefile.inc` of both drivers: the shared header and implementation are now prerequisites
  of the **object** file instead of the generated `.c`, whose recipe belongs to the
  generator, and `.DEFAULT_GOAL := all` restores the default goal (see `DF-04`).
- `indigo_test/Makefile`: the `aux_dragonfly_simulator` rule gained the shared simulator
  header; new `dome_dragonfly_simulator`, `test_dome_dragonfly_simulator`,
  `test_dome_dragonfly_simulator_asan` rules and the `test-dome-dragonfly-simulator` target.
- `indigo.xcodeproj/project.pbxproj`: `shared/dragonfly_shared.h`,
  `dome_dragonfly/REFACTOR.md`, a new `dome_dragonfly_simulator` group with its source, and
  `integration/test_dome_dragonfly_simulator.c`. Xcode itself had already picked up the two
  `.driver` files and `dragonfly_simulator_common.h` while the project was open; that part of
  the diff is its own normalisation of existing entries and was left as it stands.
  `plutil -lint` passes and a reference scan finds no dangling ids (6308 objects).
- Windows: `indigo_aux_dragonfly.vcxproj`, `.filters`, `.user` and the `dome_dragonfly`
  equivalents, copied from the `dome_talon6ror` template with new project GUIDs, the shared
  header as `ClInclude` and the shared implementation and the `.driver` as `None`, UTF-8 BOM
  and CRLF preserved (`xml` parse passes); both projects and their four configurations
  registered in `indigo_windows.sln` with a byte-level edit that leaves the rest of the file
  untouched (20 inserted lines, 0 deleted). **No Windows build or test was performed**; the
  transport is now portable but that is not the same as a verified Windows build.
- `indigo_docs/PROPERTIES.md`: both driver sections rewritten to name the `.driver` sources,
  the shared implementation, the channel windows and the `LA_` → `X_` rename.
- `MIGRATION_STATUS.md`: both rows updated; the Comment column is untouched.

### Step 8 — final verification

- **Regeneration reproducibility.** Running `indigo_generator` twice over both definitions
  produces identical output (`md5` unchanged: `63a236277e21db79cf1484b12f99092c` for
  `indigo_aux_dragonfly.c`, `56fddd77dbb9878ec72d9a711a730bbf` for
  `indigo_dome_dragonfly.c`), and the checked-in generated files match a fresh run.
- **Strict build.** `clang -fsyntax-only -Wall -Wextra` on both generated sources: no
  warnings in driver or shared code. `make -B -f ../../Makefile.drv` in both directories: 0
  warnings.
- **Driver-instrumented ASan.** `build/integration/test_aux_dragonfly_simulator_asan`
  (17/17) and `build/integration/test_dome_dragonfly_simulator_asan` (22/22): no sanitizer
  report. The framework library is not instrumented.
- **Affected suites.** Outside the two drivers and their own tests this change touches
  `AUX_TEST_TRACE` support in `integration/aux_test_isolation.h`, which four other suites
  include, and the `DF-06` NULL guard in `indigo_libs/indigo_uni_io.c`.

  Against the library **before** the `DF-06` fix, all four harness users passed:
  `test_aux_cloudwatcher_simulator` 5/5, `test_aux_mgbox_simulator` 32/32,
  `test_focuser_lunatico_simulator` 11/11, `test_rotator_lunatico_simulator` 11/11.

  Against the library **after** the `DF-06` fix: `make -C indigo_test test-unit` 154/154,
  three serial drivers that lean on the delimited readers —
  `test_dome_skyroof_simulator` 11/11, `test_focuser_dmfc_simulator` 1/1,
  `test_aux_uch_simulator` 10/10 — and three of the four harness users,
  `test_aux_cloudwatcher_simulator` 5/5, `test_focuser_lunatico_simulator` 11/11 and
  `test_rotator_lunatico_simulator` 11/11. `test_aux_mgbox_simulator` was **not** rerun
  after the library fix; its run was interrupted at 23 of 32 scenarios with no failure, and
  the user asked not to spend further time on it. The guard cannot change behaviour for a
  non-NULL list, which is what that suite passes.

  Per the "run only affected cases" rule in `indigo_test/DRIVER_TESTING_RULES.md`, the rest
  of the repository suite is not rerun for this change.
- Unavailable and therefore not claimed: Linux and Windows builds and tests, and any
  hardware validation.

## Found defects

Defects of `aux_dragonfly` and of the shared implementation. Dome-specific defects are in
`../dome_dragonfly/REFACTOR.md`. "Reproduced" means a test or a command demonstrated the
failure; "audit" means it was found by reading the source and the fix is not covered by a
dedicated reproducer.

### DF-01 — blocking device I/O on the bus thread (audit, High)

*Impact.* `aux_change_property()` called `set_gpio_outlets()` and `lunatico_authenticate2()`
synchronously, so a relay change or an authentication ran two or three UDP transactions on
the thread that delivers property changes. Each transaction waits up to 3.1 s for a reply, so
an unreachable or slow controller stalled the whole INDIGO bus for that client for up to
about 9 s per request.

*Root cause.* INDIGO 2.0 style dispatch: only `CONNECTION` was moved to a timer thread.

*Fix.* The generator dispatches every non-empty `on_change` through
`INDIGO_COPY_VALUES_PROCESS_CHANGE`, which runs the handler on the device handler queue. No
driver code runs on the bus thread any more.

*Test.* No dedicated reproducer: asserting "the bus was not blocked" needs a second device on
the same bus, which this driver's suite does not have. The structural change is visible in
the generated `aux_change_property()`, and the whole suite exercises the queued path.

### DF-02 — the UDP flush discarded at most one stale datagram (audit, Medium)

*Impact.* `lunatico_command()` drained pending input with a loop that read one datagram and
then `break`ed unconditionally in the UDP branch. If a previous command had timed out and its
reply arrived later, only that one datagram was dropped; a second stale datagram was returned
as the answer to the new command, so every following reply was off by one until the buffer
drained. Since `lunatico_command_get_result()` rebuilds the expected echo from the request,
the mismatched reply fails to parse and the command is reported as failed, which is safe but
persistent.

*Root cause.* The `break` in the UDP branch of the flush loop.

*Fix.* `indigo_uni_discard()` drains every pending datagram before each request.

*Test.* No dedicated reproducer: the simulator answers exactly one datagram per request and
the suite never times out, so a stale datagram cannot be produced through the public
interface. The new drain runs before every command in every scenario.

### DF-03 — a failed relay readback at connect latched `AUX_GPIO_OUTLETS` in ALERT (audit, Low)

*Impact.* The connect path set `AUX_GPIO_OUTLETS_PROPERTY->state = INDIGO_ALERT_STATE` when
`lunatico_read_relays()` failed but never set it back to `INDIGO_OK_STATE` on success, so a
session that followed a failed one kept publishing the property as ALERT until the next relay
change.

*Root cause.* Missing `else` assignment.

*Fix.* `dragonfly_attach_relays()` sets OK on success and ALERT on failure.

*Test.* No dedicated reproducer: the simulator profile is fixed for the lifetime of a
scenario, so a failing connect followed by a succeeding one cannot be scripted. `reconnect`
covers the succeeding-connect path and asserts the property is published OK.

### DF-04 — `make -f ../../Makefile.drv` built nothing in the driver directory (reproduced, Medium)

*Impact.* `Makefile.inc` is included before `Makefile.drv` declares its own rules, and it
contained `indigo_aux_dragonfly.c: shared/dragonfly_shared.c` with a `touch` recipe. GNU make
takes the first target of the first rule as the default goal, so the documented command from
`indigo_docs/DRIVER_GENERATOR_MIGRATION.md` printed
```
make: `indigo_aux_dragonfly.c' is up to date.
```
and built neither the object, the archive, the library nor the executable. The top level
build is unaffected because `Makefile.drvs` passes an explicit target. `dome_dragonfly` had
the same rule and the same effect.

*Root cause.* A rule in an included file capturing the default goal.

*Fix.* The dependency now applies to the object file, and `.DEFAULT_GOAL := all` restores the
intended default goal.

*Test.* Reproduced in the baseline (the command is recorded above) and verified fixed: the
same command now generates, compiles, archives, links and produces the executable.

### DF-05 — relay channel bound was off by one (audit, Low)

*Impact.* `lunatico_set_relay()` and `lunatico_pulse_relay()` rejected `relay < 0 || relay > 8`,
so channel 8 — one past the last channel of a controller with channels 0…7 — would have been
sent to the device. Unreachable from either driver, which never computes an index above 7.

*Fix.* The bound is now `relay >= DRAGONFLY_CHANNELS`.

*Test.* None; the defect is unreachable through the public interface.

### DF-06 — `indigo_uni_read_section2()` crashed on a NULL ignore list (reproduced, High)

*Impact.* `indigo_uni_io.h` documents the `ignore` argument as "optionally ignoring some
characters (or NULL)", but `indigo_uni_read_section2()` ran
`for (const char *s = ignore; *s; s++)` without a NULL check, so a caller that followed the
documented contract crashed the whole process on the first byte read. `terminators` had the
same unguarded loop. This affects every driver that uses the delimited readers, not only the
Dragonfly ones; it was found while measuring whether those readers can be used on a UDP
handle.

*Root cause.* Two unguarded loops over optional arguments.

*Fix.* Both loops now stop at NULL (`s != NULL && *s`), so NULL means "nothing to ignore"
and "no terminator" instead of undefined behaviour. The fix is in
`indigo_libs/indigo_uni_io.c` and was made at the user's explicit request; it cannot change
behaviour for a non-NULL argument.

*Test.* New `indigo_test/unit/test_uni_io.c`, wired into `UNIT_TESTS`.
`read_section_accepts_null_ignore_list` and `read_section_accepts_null_terminator_list` are
the regression tests; `read_section_still_ignores_and_terminates` pins the documented
behaviour for non-NULL lists. Verified as a real reproducer: against the unfixed library the
executable dies with SIGSEGV (`rc=139`); against the fixed library all three pass.
`make -C indigo_test test-unit` — 154/154 passed.

## Final test summary

Counted per registered scenario of the two Dragonfly suites.

**Simulated tests**

| Run | Executed | Passed |
| --- | --- | --- |
| `test_aux_dragonfly_simulator` against the original driver (characterization) | 17 | 17 |
| `test_aux_dragonfly_simulator` against the migrated driver | 17 | 17 |
| `test_aux_dragonfly_simulator_asan` (driver-instrumented) against the migrated driver | 17 | 17 |
| `test_dome_dragonfly_simulator` against the original driver (characterization) | 21 | 21 |
| `test_dome_dragonfly_simulator` against the migrated driver | 22 | 22 |
| `test_dome_dragonfly_simulator_asan` (driver-instrumented) against the migrated driver | 22 | 22 |
| `make test-aux-dragonfly-simulator` after `test-clean`, from a clean build | 17 | 17 |
| `make test-dome-dragonfly-simulator` after `test-clean`, from a clean build | 22 | 22 |
| **Total** | **155** | **155** |

The two suites contain 17 and 22 distinct scenarios, so the current hardware-free coverage is
17 + 22 = 39 scenarios; `MIGRATION_STATUS.md` records `17 / 0` and `22 / 0`.

**Hardware tests: 0 executed, 0 passed.** No Dragonfly controller is available and no
statement in this record implies physical validation.

**Suites affected by the shared harness change.** `integration/aux_test_isolation.h` gained
`AUX_TEST_TRACE` support, and four suites outside this migration include it. All pass:
`test_aux_cloudwatcher_simulator` 5/5, `test_aux_mgbox_simulator` 32/32,
`test_focuser_lunatico_simulator` 11/11, `test_rotator_lunatico_simulator` 11/11.

**Note on the repository-wide target.** `make -C indigo_test test` was started once and
exited 2 with six failures in `test_ccd_baccam_sdk`, all on the `wrong_thread` assertion of
the shared `integration/test_ccd_touptek_sdk.c`. That is unrelated to this change: nothing in
the diff touches `ccd_touptek`, `ccd_baccam` or that test, and the same unmodified binary
passes 29/29 in three standalone runs with these changes applied and in one standalone run
with the whole change stashed, so it is a pre-existing load-sensitive flake. Running the
whole ~140 binary target for a two driver change was broader than
`indigo_test/DRIVER_TESTING_RULES.md` asks for ("run only affected cases after test-only
changes"); the affected-suite list above is the validation this change actually needs, and a
second whole-tree run was started and then abandoned as unnecessary.

The Dragonfly suites are not part of that target anyway; they are opt-in
(`test-aux-dragonfly-simulator`, `test-dome-dragonfly-simulator`) because the protocol uses a
loopback UDP socket.
