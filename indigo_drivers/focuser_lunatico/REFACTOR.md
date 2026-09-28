# focuser_lunatico refactoring record

This record covers `indigo_focuser_lunatico` and the shared Lunatico implementation in
`shared/lunatico_shared.c`. `indigo_rotator_lunatico` includes the same shared file and is
migrated in the same change; its device-specific record is `../rotator_lunatico/REFACTOR.md`.
Read both together.

## Scope and baseline

Migration of `indigo_focuser_lunatico` (Lunatico Astronomia Limpet / Seletek / Armadillo /
Platypus focuser, rotator and powerbox controller) from its hand-written INDIGO 2.0
implementation to `indigo_generator`, with portable `indigo_uni_io` transport, the device
handler queue, and full applicable hardware-free simulator coverage.

Baseline date and source: 2026-09-19, commit `1d370a920`
(`aux_dragonfly/dome_dragonfly: migrated to code generator`), branch `refactoring`. The
working tree carried one unrelated pre-existing modification, `indigo.xcodeproj/project.pbxproj`
(the `shared` group of this driver), which is preserved.

Host: macOS 26.6.2 (`Darwin 25.6.0`), Apple Silicon arm64; repository universal build
(x86_64 + arm64).

Baseline build:

```sh
cd indigo_drivers/focuser_lunatico
make -f ../../Makefile.drv all
cd ../rotator_lunatico
make -f ../../Makefile.drv all
```

Result: both passed. Archive, dynamic library and standalone executable built for x86_64 and
arm64. Note that the bare `make -f ../../Makefile.drv` documented in
`indigo_docs/DRIVER_GENERATOR_MIGRATION.md` does **not** build the driver here: both
`Makefile.inc` files declare a `touch` rule for the driver `.c` as their first target, so the
default goal is that rule and nothing is compiled. The explicit `all` goal is required.

Baseline automated tests (only the two drivers touched by this work were run):

```sh
cd indigo_test
make build/integration/test_focuser_lunatico_simulator build/integration/test_rotator_lunatico_simulator
./build/integration/test_focuser_lunatico_simulator
./build/integration/test_rotator_lunatico_simulator
```

Result: `test_focuser_lunatico_simulator` 11 scenarios run, 11 passed;
`test_rotator_lunatico_simulator` 11 scenarios run, 11 passed. Both exit `0`.

This supersedes the previous version of this file, which recorded the rotator suite as
passing only 7 of 11 scenarios and the port-reconfiguration use-after-free as an open
blocker. Both statements were correct when written and are now stale: commit `fca8eb601`
added the `port_lifecycle_mutex`, `b9f894420` fixed the rotator connect angle, and
`indigo_test/integration/aux_test_isolation.h` runs every scenario in its own forked process,
which is what made the port-reconfiguration scenarios reliable. The rotator suite has its own
build rule and passes in full; it is still missing from `INTEGRATION_TESTS`, so
`make -C indigo_test test` does not run it. That is addressed by this change.

`build/bin/indigo_generator` is present in the build tree, built from the unchanged
`indigo_tools/indigo_generator.c`.

## Hardware-test decision

No hardware testing will be performed. The user has stated that no Lunatico controller is
available. Nothing in this record implies physical validation; every result below is
simulator-backed software behaviour. The `Retested` column of `MIGRATION_STATUS.md` will
record `Sim` only.

## Studied sources

- `indigo_focuser_lunatico.c`, `.h`, `_main.c`, `Makefile.inc`, `README.md` in this directory.
- `shared/lunatico_shared.c` (2425 lines, shared with `rotator_lunatico`) and
  `shared/lunatico_main_shared.c`.
- `../rotator_lunatico/indigo_rotator_lunatico.c`, `.h`, `_main.c`, `Makefile.inc`,
  `README.md`, and `rotator_lunatico_simulator/rotator_lunatico_simulator.c`.
- `indigo_test/integration/test_focuser_lunatico_simulator.c`,
  `test_rotator_lunatico_simulator.c`, `aux_test_isolation.h`,
  `serial_simulator_test_common.h`, `simulator_test_common.h`,
  `indigo_test/simulator_common/aux_simulator_common.h`,
  `serial_simulator_common.h`, `serial_motion.h`, `indigo_test/Makefile`.
- `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md` (shared scope, Focuser,
  Rotator and AUX sections), `indigo_drivers/AGENTS.override.md`, root `AGENTS.md`,
  `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`,
  `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`, `indigo_docs/PROPERTIES.md`.
- `indigo_libs/indigo/indigo_uni_io.h` and `indigo_libs/indigo_uni_io.c`
  (`indigo_uni_open_serial_with_speed()`, `indigo_uni_open_url()`, `indigo_uni_discard()`,
  `indigo_uni_read_section2()`, `indigo_uni_read_available()`, and the fact that a read on a
  UDP handle consumes one whole datagram regardless of the requested length).
- `indigo_tools/indigo_generator.c`, in particular `write_c_attach()` and the
  `INDIGO_DRIVER_INIT` emitter, which establish that a generated serial driver attaches every
  declared device block statically at INIT and calls `<driver>_open(device->master_device)`
  for slave devices.
- Manufacturer protocol reference: <https://lunaticoastro.com/slp_docs/data/commands.json>
  (consulted, not copied into the repository). It documents only a subset of the command set
  this driver uses: `!seletek version#`, and the `step` verbs `goto`, `stop`, `getpos`,
  `setpos`, `ismoving` and `speed`. It does not document `gopr`, `halfstep`, `wiremode`,
  `model`, `movepow`, `stoppow`, `speedrangeus`, `setswlimits`, `delswlimits`, `read temps`,
  `read an` or `write dig`, all of which the driver issues. For those the authoritative
  in-repository evidence is the existing driver itself plus
  `../aux_dragonfly/relio_simulator/relio_simulator.pl`, the author's reference simulator for
  the same SLP protocol family.
- Sibling generated migrations: `aux_dragonfly` / `dome_dragonfly` (the same Lunatico SLP
  protocol, mutually exclusive driver pair, shared implementation file included from the
  `code { }` block of both definitions) and `focuser_primaluce` (multi-device serial driver
  with `focuser` and `rotator` blocks sharing one connection).

## Current-state audit

### Protocol

Framing is Lunatico SLP: a request `!<group> <verb> [<args>]#` is answered by the controller
echoing the request without its trailing `#`, followed by `:<value>#`. `<value>` is `-1` when
the controller rejects the command. Transport is either a 115200 8N1 serial port or UDP on
port 10000, selected by the `lunatico://` scheme in `DEVICE_PORT`.

Commands the driver issues, with the helper that issues them:

| Command | Helper | Notes |
| --- | --- | --- |
| `!seletek version#` | `lunatico_get_info()`, `lunatico_check_port_existance()` | reply `:<MOPFF>#`; `M` operating mode (2 = bootloader), `O` model (1 Seletek, 2 Armadillo, 3 Platypus, 4 Dragonfly, 5 Limpet), `P` firmware major, `FF` firmware minor |
| `!step getpos <port>#` | `lunatico_get_position()` | negative position treated as failure |
| `!step setpos <port> <pos>#` | `lunatico_sync_position()` | SYNC, no motion |
| `!step goto <port> <pos> <backlash>#` | `lunatico_goto_position()` | third argument is undocumented publicly; the public JSON documents only two |
| `!step gopr <port> <steps>#` | — | present in the simulator and commented out in the driver; relative moves are issued as absolute `goto` |
| `!step stop <port>#` | `lunatico_stop()` | |
| `!step ismoving <port>#` | `lunatico_is_moving()` | `0` idle, non-zero moving |
| `!step halfstep <port> <0\|1>#` | `lunatico_set_step()` | |
| `!step wiremode <port> <0..3>#` | `lunatico_set_wiring()` | Lunatico/Moonlite × normal/reversed |
| `!step model <port> <0..3>#` | `lunatico_set_motor_type()` | unipolar, bipolar, DC, step-dir |
| `!step movepow <port> <0..1023>#` | `lunatico_set_move_power()` | percent × 10.23 |
| `!step stoppow <port> <0..1023>#` | `lunatico_set_stop_power()` | percent × 10.23 |
| `!step speedrangeus <port> <us> <us>#` | `lunatico_set_speed()` | `us = 1000 / kHz`, rejected outside 50…500000 |
| `!step setswlimits <port> <min> <max>#` | `lunatico_set_limits()` | |
| `!step delswlimits <port>#` | `lunatico_delete_limits()` | |
| `!read temps <sensor>#` | `lunatico_get_temperature()` | internal sensor 0: `((v - 261) * 1.8 - 250) / 10`; external sensor 1: `((v - 192) * 1.7 - 0) / 10` |
| `!read an <port> <pin>#` | `lunatico_read_sensor()` | pins 5…8 |
| `!write dig <port> <pin> <0\|1>#` | `lunatico_enable_power_outlet()` | pins 1…4 |

Port existence is derived from the model digit of `!seletek version#`: Limpet accepts port 0,
Seletek and Armadillo ports 0…1, Platypus ports 0…2, and a controller in bootloader mode
(`M == 2`) is refused.

Both the powerbox outlets and the GPIO sensors are addressed with horizontally flipped pin
numbers (outlet item 1 writes pin 4, sensor item 1 reads pin 8) to compensate for the DB9
connector orientation documented in `README.md`.

### Architecture and implementation (original)

- `indigo_focuser_lunatico.c` and `../rotator_lunatico/indigo_rotator_lunatico.c` are 35-line
  entry points. Each sets `DRIVER_ENTRY_POINT`, `DRIVER_NAME`, `CONFLICTING_DRIVER`,
  `DRIVER_INFO`, `DEFAULT_DEVICE` and `DRIVER_VERSION`, then `#include`s
  `shared/lunatico_shared.c`. The only behavioural difference between the two drivers is
  `DEFAULT_DEVICE` (`TYPE_FOCUSER` versus `TYPE_ROTATOR`), which selects the class of the
  Main port.
- The drivers are mutually exclusive: `INDIGO_DRIVER_INIT` refuses to load when
  `indigo_driver_initialized(CONFLICTING_DRIVER)` is true.
- The driver owns `device_data[MAX_DEVICES=4]`, each holding `port[MAX_PORTS=3]` logical
  devices and one shared `lunatico_private_data`. Only slot 0 is ever used; `MAX_DEVICES` is
  dead capacity.
- `lunatico_private_data` holds the raw fd `handle`, the `count_open` reference count, a
  `udp` flag, a `pthread_mutex_t port_mutex` and `lunatico_port_data port_data[3]`. Each
  `lunatico_port_data` holds that port's device type, focuser and rotator positions and
  targets, backlash, temperature sensor index, previous temperature, four `indigo_timer *`
  and twelve `indigo_property *`.
- Logical devices are created and destroyed at runtime. `INDIGO_DRIVER_INIT` creates port 0
  with `DEFAULT_DEVICE`. `configure_ports()`, dispatched with `INDIGO_ASYNC` from the
  `LUNATICO_MODEL`, `LUNATICO_PORT_EXP_CONFIG` and `LUNATICO_PORT_THIRD_CONFIG` handlers,
  creates or deletes ports 1 and 2 with the requested class. `port_lifecycle_mutex`
  serialises that against itself, INIT and SHUTDOWN.
- The port index of a logical device is stored in the low nibble of `device->gp_bits`; bit 7
  of the same word is the per-device connected flag.
- Transport is hand-written POSIX: `select()` + one-byte `read()` loops in
  `lunatico_command()`, `indigo_write()`, `close()`, guarded by `port_mutex`. The first
  select has a 100 ms timeout and drains pending input; the reply read uses a 3.1 s first-byte
  timeout and a 0.1 s inter-byte timeout, terminating on `#`.
- Every device class runs its own `indigo_set_timer` / `indigo_reschedule_timer` polling
  callbacks: `focuser_timer_callback` and `temperature_timer_callback` for focusers,
  `rotator_timer_callback` for rotators, `sensors_timer_callback` for powerboxes.
- Connection handlers `handle_focuser_connect_property`, `handle_rotator_connect_property`
  and `handle_aux_connect_property` are dispatched through `indigo_set_timer(device, 0, …)`.
- `lunatico_open()` opens the shared handle when `count_open` goes 0 → 1, then calls
  `lunatico_check_port_existance()` for the connecting port and rolls the count back on
  failure. `lunatico_close()` closes it when the count returns to 0.

### Public properties (original)

Common, on every logical device: `DEVICE_PORT`, `DEVICE_PORTS` and `DEVICE_BAUDRATE` are
unhidden on every port device, and `INFO_PROPERTY->count` is 6.

Defined at attach, regardless of connection state:

- `LUNATICO_MODEL` (`LIMPET`, `ARMADILLO`, `PLATYPUS`) — port 0 only, hidden elsewhere.
- `LUNATICO_PORT_EXP_CONFIG`, `LUNATICO_PORT_THIRD_CONFIG` (`FOCUSER`, `ROTATOR`,
  `AUX_POWERBOX`) — port 0 only, hidden elsewhere.
- `AUX_OUTLET_NAMES`, `AUX_SENSOR_NAMES` — powerbox ports only, hidden elsewhere.

Defined on connect by `lunatico_init_device()`, for focuser and rotator devices only:

- `LA_POWER_CONTROL` (`MOVE_POWER`, `STOP_POWER`, percent)
- `LA_TEMPERATURE_SENSOR` (`INTERNAL`, `EXTERNAL`) — focuser devices only
- `LA_MOTOR_WIRING` (`LUNATICO`, `MOONLITE`)
- `LA_MOTOR_TYPE` (`UNIPOLAR`, `BIPOLAR`, `DC`, `STEP_DIR`)
- `LA_STEP_MODE` (`FULL`, `HALF`)

Defined on connect for powerbox devices: `AUX_POWER_OUTLET`, `AUX_GPIO_SENSORS`.

Class properties unhidden by the driver: focuser — `FOCUSER_TEMPERATURE`, `FOCUSER_LIMITS`
(0…100000), `FOCUSER_BACKLASH` (0…200), `FOCUSER_SPEED` (0.002…20 kHz), `FOCUSER_MODE`,
`FOCUSER_COMPENSATION` (-10000…10000), `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`;
rotator — `ROTATOR_STEPS_PER_REVOLUTION` (100…100000, default 3600), `ROTATOR_DIRECTION`,
`ROTATOR_BACKLASH`, `ROTATOR_LIMITS` (-180…360, default -180…180).

### Defects, risks and gaps found by audit

Recorded in full in "Found defects" below. In summary: blocking device I/O on bus callbacks,
a use-after-free on port reconfiguration, non-portable POSIX transport, custom property names
that violate the `X_` rule, a dead `MAX_DEVICES` capacity, a timer-reference misuse that logs
`Attempt to set timer with non-NULL reference`, and a rotator poll that is armed through one
timer field and cancelled through another.

### Build and packaging

Both drivers build through `Makefile.drv` with a `Makefile.inc` that only declares `touch`
dependency rules. `README.md` documents Linux and macOS support; there are no Windows project
files for either driver. The Xcode project already carries the `shared` group for this
driver.

### Existing simulators and tests

`../rotator_lunatico/rotator_lunatico_simulator/rotator_lunatico_simulator.c` is a 67-line
host-side simulator built on `indigo_test/simulator_common/aux_simulator_common.h`. It is
shared by both suites and is built twice, once as a PTY simulator and once with `SIM_UDP=1`.
It models three ports with linear motion, software limits, `speedrangeus`, analog sensors,
digital outputs and a fixed `!read temps` reply of `500`, plus the failure profiles
`silent`, `oversized`, `wrong-model`, `goto-error`, `stop-error` and `read-error`.

Coverage gaps against the class standards, all confirmed absent from both suites: temperature
readback and compensation, `FOCUSER_MODE` manual/automatic permission switching,
`FOCUSER_LIMITS`, `FOCUSER_SPEED`, `FOCUSER_BACKLASH`, `FOCUSER_REVERSE_MOTION`, every
`LA_*` property, abort while idle, disconnect during motion, both connection orders for
shared devices, sibling survival on last close, `ROTATOR_STEPS_PER_REVOLUTION`,
`ROTATOR_LIMITS`, `ROTATOR_DIRECTION`, `ROTATOR_BACKLASH`, powerbox outlet names and sensor
names, and `stop-error`. The simulator also does not record the settings it is told
(`halfstep`, `wiremode`, `model`, `movepow`, `stoppow` are accepted and discarded), so no test
can assert that the driver sent them, and it traces only requests, not replies.

## Migration design

Agreed with the user on 2026-09-19: the dynamic port model is replaced by a static one.

- Two `.driver` definitions, `indigo_focuser_lunatico.driver` here and
  `../rotator_lunatico/indigo_rotator_lunatico.driver`, both named `driver lunatico`. The
  generated private data type is therefore `lunatico_private_data` and the connection helpers
  are `lunatico_open()` / `lunatico_close()` in both, so one shared implementation file can
  serve both generated drivers exactly as `dragonfly_shared.c` serves the Dragonfly pair.
- Each definition declares seven logical devices with explicit `id`s. The first block decides
  the generated entry point name and is the master device that owns `DEVICE_PORT`:

  | `id` | focuser_lunatico | rotator_lunatico | port |
  | --- | --- | --- | --- |
  | `focuser_main` / `rotator_main` | `focuser` | `rotator` | 0 |
  | `focuser_exp` | `focuser` | `focuser` | 1 |
  | `rotator_exp` | `rotator` | `rotator` | 1 |
  | `aux_exp` | `aux` | `aux` | 1 |
  | `focuser_third` | `focuser` | `focuser` | 2 |
  | `rotator_third` | `rotator` | `rotator` | 2 |
  | `aux_third` | `aux` | `aux` | 2 |

  This is the only form the current DSL can express: `write_c_attach()` and the
  `INDIGO_DRIVER_INIT` emitter attach every declared block unconditionally. `LUNATICO_MODEL`,
  `LUNATICO_PORT_EXP_CONFIG` and `LUNATICO_PORT_THIRD_CONFIG` therefore disappear; the user
  connects the logical devices that match the hardware instead of describing the hardware
  first. The generator itself is not modified.
- The port index moves from `device->gp_bits` into each device's `on_attach` block, which
  assigns a `PRIVATE_DATA`-independent constant through a generated-device-local macro. Per
  port state stays in `port_data[MAX_PORTS]` declared in the `data { }` block.
- Two logical devices of the same port must not be connected at once. Each `on_connect`
  claims its port and assigns `connection_result = false` when the port is already claimed or
  when `!seletek version#` says the port does not exist on this controller. `lunatico_open()`
  keeps the transactional contract: it opens the handle and verifies the controller answers,
  or releases the handle and returns false.
- `serial { configurable_speed = true; }` keeps `DEVICE_PORT`, `DEVICE_PORTS` and
  `DEVICE_BAUDRATE` visible on the master, matching the original. The master's `on_attach`
  restores the `115200` default.
- Transport moves to `indigo_uni_io`: `indigo_uni_open_serial_with_speed()` for a serial
  port, `indigo_uni_open_url(url, 10000, INDIGO_UDP_HANDLE, …)` for a `lunatico://` URL,
  `indigo_uni_discard()` before each request, a variadic `lunatico_command()` built on
  `indigo_uni_vprintf()`, and `indigo_uni_read_section2()` with an explicit first-byte and
  inter-byte timeout for the `#`-delimited reply. The UDP path reads one datagram with
  `indigo_uni_read_available()`, as in `dragonfly_shared.c`. The command and response buffers
  move into the private data.
- `port_mutex` is removed. Every device is a slave of the master device, so
  `indigo_execute_handler*` routes all work of all seven devices onto the master's single
  handler queue, which already serialises every transaction of one controller.
- Polling becomes the generated `on_timer` of each device, and each long-running operation
  gets a `_finalizer` following the handler + finalizer pattern, so no handler blocks waiting
  for motion.
- The `LA_*` custom properties are renamed to the mandatory `X_` prefix:
  `X_STEP_MODE`, `X_POWER_CONTROL`, `X_TEMPERATURE_SENSOR`, `X_MOTOR_WIRING`, `X_MOTOR_TYPE`.
  `AUX_OUTLET_NAMES`, `AUX_SENSOR_NAMES`, `AUX_POWER_OUTLET` and `AUX_GPIO_SENSORS` keep
  their standard AUX names.
- The conflicting-driver guard moves into the driver-scope `on_init { }` block, as in
  `aux_dragonfly`.
- `MAX_DEVICES` is not overridden; the dead four-controller capacity disappears with the
  hand-written device table.

## Atomic migration plan

Steps are executed in order. Each step records its state and evidence immediately after it is
performed.

1. **Record audit, baseline, hardware decision and plan** (this file and
   `../rotator_lunatico/REFACTOR.md`).
   State: **done**. Evidence: baseline build and test results above.
2. **Extend the Lunatico device simulator** to a complete protocol model shared by both
   suites: record every setting the controller is told (`halfstep`, `wiremode`, `model`,
   `movepow`, `stoppow`, `speedrangeus`, limits), add a settable temperature model for
   `!read temps` on both sensors, add reply tracing (`<-`), model the per-model port
   existence of `!seletek version#`, and add the failure profiles both suites need.
   State: **done**, see "Step 2".
3. **Extend `test_focuser_lunatico_simulator.c` to the full focuser + AUX acceptance suite
   against the unchanged driver** (characterization first), covering every gap listed in
   "Existing simulators and tests", and record the normalized reference trace.
   State: **done**, see "Step 3".
4. **Extend `test_rotator_lunatico_simulator.c` to the full rotator acceptance suite against
   the unchanged driver**, and record its normalized reference trace.
   State: **done**, see `../rotator_lunatico/REFACTOR.md` step 4.
5. **Migrate `focuser_lunatico` to the generator**: write `indigo_focuser_lunatico.driver`,
   rewrite `shared/lunatico_shared.c` onto `indigo_uni_io`, the handler queue and the static
   seven-device model, add `shared/lunatico_shared.h`, regenerate, build, re-run the suite
   and compare traces.
   State: **done**, see "Step 5".
6. **Migrate `rotator_lunatico` to the generator** on the same shared file, regenerate,
   build, re-run both suites and compare traces.
   State: **done**, see "Step 6".
7. **Repository integration**: `Makefile.inc` of both drivers, `indigo_test/Makefile`
   including the missing `INTEGRATION_TESTS` entry for the rotator suite, Xcode project
   groups, `indigo_docs/PROPERTIES.md`, `README.md` changes if the user approves them,
   `MIGRATION_STATUS.md`.
   State: **done**, see "Step 7".
8. **Final verification**: regeneration reproducibility, strict build with warnings check,
   ASan runs of both suites, `git diff` audit, driver versioning, test artifact cleanup.
   State: **done**, see "Step 8".

## Step evidence

### Step 2 — Lunatico device simulator

The 67-line `rotator_lunatico_simulator.c` is replaced by a complete protocol model in
`focuser_lunatico_simulator/lunatico_simulator_common.h`, with a thin executable wrapper in
each driver directory, exactly as `dragonfly_simulator_common.h` serves the Dragonfly pair:

- `focuser_lunatico_simulator/focuser_lunatico_simulator.c` (new, `SIM_NAME "focuser_lunatico"`)
- `../rotator_lunatico/rotator_lunatico_simulator/rotator_lunatico_simulator.c` (rewritten as
  a wrapper, `SIM_NAME "rotator_lunatico"`)

What the model adds over the previous version:

- Motion uses the mandated shared helper `indigo_test/simulator_common/serial_motion.h`
  instead of an open-coded integrator, so every port has a real elapsed-time ramp with the
  helper's 0.5 s minimum travel, which guarantees an observable BUSY window.
- Every setting the controller is told is now recorded and range checked instead of accepted
  and discarded: `halfstep` 0…1, `wiremode` 0…3, `model` 0…3, `movepow` / `stoppow` 0…1023,
  `speedrangeus` 50…500000 with both ends equal, `setswlimits` with min ≤ max. A request
  outside those bounds is answered `:-1#`, which turns "the property reached OK" into a real
  assertion about the command the driver produced.
- `speedrangeus` now drives the simulated step rate, so `FOCUSER_SPEED` has an observable
  effect, and `setswlimits` is enforced on `goto`, so `FOCUSER_LIMITS` has one too.
- `!read temps <sensor>#` answers a per-sensor raw value that a scenario can change at run
  time, which is what makes temperature compensation testable. Raw 511 is 20.0 °C on the
  internal sensor and raw 0 is about −72 °C, below the driver's `NO_TEMP_READING`, so an
  absent sensor can be simulated.
- The model digit of `!seletek version#` is selectable, so per-model port existence
  (`limpet`, `armadillo`, `seletek`), bootloader mode and an unparsable version are testable.
- Replies are traced (`<-`), not only requests, following the convention of every other
  simulator in the repository.
- Every received request is appended to `<ready-file>.events`, and a `<ready-file>.control`
  file injects state behind the driver's back (`temperature:`, `temperature-external:`,
  `position:<port>:<steps>`). This follows `rotator_wa_simulator`. The event log is the only
  way to assert the exact command the driver produced: this protocol has no readback for its
  settings or for the digital outputs, so without it the DB9 pin flip and the unit
  conversions would be unverifiable.
- Profiles are `+` separated flags, as in the Dragonfly simulator, so conditions combine.
  The set is `silent`, `oversized`, `wrong-model`, `bootloader`, `bad-version`, `limpet`,
  `armadillo`, `seletek`, `goto-error`, `setpos-error`, `stop-error`, `read-error`,
  `speed-error`, `limits-error`, `step-mode-error`, `wiring-error`, `motor-error`,
  `power-error`, `sensor-error`, `outlet-error`, `no-temp-sensor`.

Build and compatibility check:

```sh
cd indigo_test
make build/integration/focuser_lunatico_simulator build/integration/rotator_lunatico_simulator
./build/integration/test_focuser_lunatico_simulator
./build/integration/test_rotator_lunatico_simulator
```

Result: both simulators build with no warnings, and the **pre-existing** 11-scenario focuser
suite and 11-scenario rotator suite both still pass 11/11 against the rewritten simulator
before any test was added. That is the compatibility gate for the simulator rewrite itself.

Audit against the manufacturer protocol: the public catalogue at
<https://lunaticoastro.com/slp_docs/data/commands.json> documents `!seletek version#` and the
`step` verbs `goto`, `stop`, `getpos`, `setpos`, `ismoving` and `speed` only. Two divergences
between that document and this driver are recorded rather than "fixed", because the driver is
the only evidence available for them and the migration must preserve its behaviour:

1. The document gives `!step goto {port} {position}#` with two arguments; the driver sends a
   third, a backlash compensation. The simulator accepts both arities.
2. The document names `!step speed {port} {speed}#`; the driver uses
   `!step speedrangeus {port} {us} {us}#`. The simulator implements what the driver sends.

The remaining verbs the driver issues (`gopr`, `halfstep`, `wiremode`, `model`, `movepow`,
`stoppow`, `speedrangeus`, `setswlimits`, `delswlimits`, `read temps`, `read an`,
`write dig`) are absent from the public catalogue; their request form and the `:0#` / `:-1#`
reply convention are taken from the driver and from
`../aux_dragonfly/relio_simulator/relio_simulator.pl`.

### Step 3 — focuser_lunatico characterization suite

`indigo_test/integration/test_focuser_lunatico_simulator.c` grows from 11 to **49 scenarios**,
all passing against the **unchanged** driver. Shared helpers moved into the new
`indigo_test/integration/lunatico_test_common.h` (transport URL, control channel, event-log
assertions, the temperature conversions and request helpers that wait for a fresh revision).

```sh
cd indigo_test
./build/integration/test_focuser_lunatico_simulator
```

Result: 49 scenarios run, 49 passed, 0 failing scenarios.

Scenario-to-requirement mapping:

| Requirement (Focuser and AUX class standards) | Scenario |
| --- | --- |
| Interface bit, class property completeness, identity from `!seletek version#`, transport and driver-specific property inventory, full connect configuration sequence | `identity_and_inventory` |
| Absolute GOTO with BUSY window, measured progress, exact command, on each of the three ports | `main_absolute_move`, `exp_absolute_move`, `third_absolute_move` |
| Zero/no-op GOTO issues no command | `no_op_goto` |
| SYNC updates the counter without a move | `sync_without_moving` |
| Relative steps in both directions, sign from `FOCUSER_DIRECTION`, no backlash on relative moves | `relative_steps` |
| Relative move clamped at the position minimum | `relative_steps_clamp` |
| Abort in motion, measured stopped position, fresh move afterwards | `abort_motion` |
| Abort while idle | `abort_while_idle` |
| Speed unit conversion kHz → µs at both ends of the range | `speed` |
| Software limits written, enforced by the controller, and deleted for the full range | `limits` |
| Driver-side rejection of inverted limits | `inverted_limits` |
| Backlash passed to absolute GOTO | `backlash` |
| Reverse motion and motor wiring combined into one `wiremode` value, all four combinations | `wiring` |
| Motor type, step mode and coil current, including the ×10.23 scaling | `motor_settings` |
| Temperature readback and sensor selection | `temperature` |
| Disconnected sensor reported IDLE, not as a failure | `absent_temperature_sensor` |
| Compensation moves the focuser by steps/°C in automatic mode | `temperature_compensation` |
| No compensation in manual mode | `manual_mode_does_not_compensate` |
| Mode switches the manual controls and the position permission | `focuser_mode` |
| Start failure on absolute and relative moves, not overwritten by the poll | `goto_failure`, `relative_move_failure` |
| SYNC, stop and readback failures | `sync_failure`, `stop_failure`, `read_failure` |
| Partial setting write failures, one per controller setting | `step_mode_failure`, `wiring_failure`, `motor_type_failure`, `power_control_failure`, `speed_failure`, `limits_failure` |
| Overlapping request against the poll armed by connect | `goto_overlapping_connect_poll` |
| Identity refusal: wrong model, bootloader mode, unparsable version, no reply, unterminated reply | `wrong_model`, `bootloader`, `bad_version`, `silent`, `oversized` |
| Port that does not exist on this controller model | `limpet_has_no_exp_port`, `armadillo_has_no_third_port` |
| Disconnect/reconnect, driver-specific properties deleted and redefined | `reconnect` |
| Disconnect during motion, no traffic after close | `disconnect_during_motion` |
| Shared connection, both orders, sibling survives, last close owns the transport | `shared_connection` |
| SHUTDOWN refused while a device is connected | `shutdown_refused_while_connected` |
| Powerbox outlets and analog inputs including the DB9 pin flip | `powerbox` |
| Outlet and sensor renaming | `powerbox_names` |
| Outlet write and sensor read failures | `powerbox_outlet_failure`, `powerbox_sensor_failure` |
| A rotator on a secondary port of the focuser entry point | `rotator_on_exp` |

Deliberately not covered, with justification:

- **Hardware-only**: real transport loss, physical DB9 wiring, actual motor behaviour under
  the four wiring modes and the two step modes, and the UDP transport against a real
  controller. The UDP *code path* is covered by the opt-in
  `make -C indigo_test test-rotator-lunatico-udp` build of the rotator suite.
- **Framework-owned**: numeric range, step and finite-value validation of incoming requests,
  configuration storage, and the BUSY guard that drops a change arriving on a busy property.
  Per the shared scope in `indigo_test/DRIVER_TESTING_RULES.md` these are not driver tests.
- **Not observable through this protocol**: the four coil-current, step-mode, wiring and
  motor-type settings have no readback command, so the assertions prove the exact request
  through the event log and the accept/reject reply, not a device-side state query.

Harness note: `run_aux_simulated()` in `indigo_test/integration/aux_test_isolation.h` gained a
`known_defect` flag on `aux_simulated_case` and a `--known-defects` mode, following
`test_guider_cgusbst4_simulator.c`. Scenarios marked that way are skipped by default and run
only in that mode, where failing is the expected result. The five existing users of the
harness were rebuilt unchanged.

### Step 4 — rotator_lunatico characterization suite

See `../rotator_lunatico/REFACTOR.md` step 4. Summary: 26 scenarios, 25 passing against the
unchanged driver plus one recorded expected baseline failure, the `LU-06` reproducer.

### Reference traces

`AUX_TEST_TRACE=1` starts every simulator with `--trace`, which logs the ordered protocol
exchange to stderr. Normalization drops the free-running polls whose interleaving with
command traffic is inherently nondeterministic (`!step getpos#`, `!step ismoving#`,
`!read temps#`, `!read an#`) and collapses consecutive duplicates. Those four commands are
not lost from the contract: their exact form and arguments are asserted directly through the
simulator's event log in the scenarios above.

```sh
cd indigo_test
AUX_TEST_TRACE=1 AUX_TEST_FILTER=<scenario> ./build/integration/test_<class>_lunatico_simulator 2>&1 >/dev/null \
  | grep -E '^(->|<-) ' \
  | grep -vE '(read temps|read an|step ismoving|step getpos)' \
  | awk '$0 != prev { print } { prev = $0 }'
```

Traces were captured from the unchanged drivers for the focuser scenarios
`identity_and_inventory`, `main_absolute_move`, `sync_without_moving`, `relative_steps`,
`abort_motion`, `limits`, `backlash`, `wiring`, `motor_settings`,
`temperature_compensation`, `powerbox` and `reconnect` (454 lines total), and for the rotator
scenarios `identity_and_inventory`, `main_goto`, `goto_wraps`, `sync_without_moving`,
`abort_motion`, `steps_per_revolution`, `limits`, `direction`, `backlash`, `exp_focuser`,
`third_powerbox` and `reconnect`. Each set was captured twice and verified byte identical
across runs before being accepted as the comparison baseline.

They record the manner the migration must preserve, in particular:

- the connect order `!seletek version#` (twice: once for the port-existence check, once for
  the identity), `delswlimits`, `movepow`, `stoppow`, `model`, `halfstep`, `getpos`,
  `speedrangeus`, `wiremode`, then the limits decision;
- that the rotator additionally re-syncs the counter with `!step setpos <port> <steps>#`
  right after reading the angle, while the focuser does not;
- that `FOCUSER_LIMITS` and `ROTATOR_LIMITS` choose between `setswlimits` and `delswlimits`
  rather than always writing a range;
- that a relative focuser move sends `goto` with a zero backlash argument while an absolute
  move sends the configured one.

### Step 5 — focuser_lunatico migrated to the generator

New `indigo_focuser_lunatico.driver` (1033 lines), a new `shared/lunatico_shared.h` for the
constants and the per-port state type that the private data declaration needs, and a rewritten
`shared/lunatico_shared.c`. `Makefile.inc` is deleted; its only content was the two `touch`
rules of defect LU-07, and the generator's own dependency in `Makefile.drv` replaces them.

Seven logical devices are declared, as agreed: `focuser_main`, then `focuser_exp`,
`rotator_exp`, `aux_exp`, `focuser_third`, `rotator_third` and `aux_third`.

**A generator limitation shaped the property layout.** Driver-defined properties are stored in
the generated driver's shared private data, one field per property id, and the macro
`<ID>_PROPERTY` is emitted once per declaration. Declaring the same property id in several
device blocks therefore emits a duplicate `#define` and a duplicate struct member, and the
generated C does not compile. This was verified with a two-block probe definition before the
layout was chosen. Each logical device consequently needs its own property handle. The five
per-port settings are therefore declared once per stepper device with a port-suffixed handle
(`X_FOCUSER_STEP_MODE_MAIN`, `..._EXP`, `..._THIRD`, `X_ROTATOR_STEP_MODE_EXP`, `..._THIRD`
and so on), and the powerbox properties likewise (`AUX_POWER_OUTLET_EXP`, `..._THIRD`). The
generator itself was not modified.

The published property names are qualified by class, `X_FOCUSER_*` and `X_ROTATOR_*`, which
also satisfies the mandatory `X_` prefix (defect LU-04). The duplicated declarations stay
purely declarative: every `on_change` is a one-line call into a shared helper that takes the
property as an argument, so there is exactly one implementation of each setting.

Transport, concurrency and lifecycle:

- `lunatico_open()` opens a serial port with `indigo_uni_open_serial_with_speed()` or, for a
  `lunatico://` URL, a UDP handle with `indigo_uni_open_url(url, 10000, INDIGO_UDP_HANDLE, …)`,
  then verifies the controller answers `!seletek version#` and closes the handle again if it
  does not. That is the transactional contract the root instructions require.
- `lunatico_vcommand()` formats the request with `vsnprintf` into the private data buffer,
  discards stale input with `indigo_uni_discard()`, and reads the reply with
  `indigo_uni_read_section2(handle, …, "#", NULL, 3.1 s, 0.1 s)`, which keeps the terminator
  in the buffer so the reply can be matched against the echoed request. The UDP path uses a
  single `indigo_uni_wait_for_data()` + `indigo_uni_read_available()` because a read on a UDP
  handle consumes the whole datagram. The command and response buffers live in the private
  data and are reused.
- The `pthread` port mutex and the `port_lifecycle_mutex` are gone. All seven devices are
  slaves of the Main device, so `indigo_execute_handler*` routes every handler of every device
  onto the master's single queue, which already serializes the controller.
- The four `indigo_timer` fields per port are gone. Motion completion is
  `focuser_motion_finalizer` / `rotator_motion_finalizer`, each a bounded progress check that
  reschedules itself while the controller reports motion; temperature and GPIO sensor polling
  are the generated `on_timer` of the focuser and powerbox devices.
- A port is claimed by the first of its three logical devices to connect
  (`lunatico_claim_port()`), which also performs the per-port existence check against the
  model the controller reports. `on_disconnect` releases it.
- `MAX_DEVICES` is not overridden and the dead four-controller device table is gone.

Build and generation:

```sh
cd indigo_drivers/focuser_lunatico
../../build/bin/indigo_generator indigo_focuser_lunatico.driver
make -B -f ../../Makefile.drv
```

Result: generated and built for x86_64 and arm64, 0 lines containing `warning` or `error`.
`make -f ../../Makefile.drv` without an explicit goal now builds the driver, which closes
defect LU-07.

### Step 6 — rotator_lunatico migrated to the generator

New `../rotator_lunatico/indigo_rotator_lunatico.driver` (972 lines) over the same shared
implementation, with `rotator_main` in place of `focuser_main`; `Makefile.inc` deleted. Both
definitions use `driver lunatico`, so both generated drivers expose `lunatico_private_data`,
`lunatico_open()` and `lunatico_close()` to the shared file and it stays a single
implementation, exactly as `dragonfly_shared.c` serves the Dragonfly pair. See
`../rotator_lunatico/REFACTOR.md` step 6.

### Steps 5 and 6 — suite and trace comparison

Both suites were run against the migrated drivers, then the drivers were reverted to
`1d370a920` and the **same** suites were run against the original drivers again, so the
adaptations made after migration are proven not to hide a regression. The only source change
needed for the pre-migration run was mapping the property-name constants in
`indigo_test/integration/lunatico_test_common.h` back to the old `LA_*` names.

| | migrated driver | original driver |
| --- | --- | --- |
| `test_focuser_lunatico_simulator` | 50 / 50 passed | 49 / 50 passed, `sync_to_zero_after_connect` fails (LU-09) |
| `test_rotator_lunatico_simulator` | 27 / 27 passed | 26 / 27 passed, `limits_change_keeps_angle` fails (LU-08) |

The two failures against the original driver are the dedicated reproducers of the two defects
the trace comparison uncovered; both are recorded below and both pass after the migration.

Six scenarios needed a stronger gate after migration, and the same gate is valid before it.
The generated change branch publishes `INDIGO_BUSY_STATE` and returns before the queued
handler issues the command, so a scenario may no longer treat "the property went BUSY" as
"the command was sent". Those scenarios now wait for the command in the simulator's event log
(`lunatico_wait_for_command()`) instead, and the abort scenarios additionally wait for the
motion to be measurably under way before aborting. This is the intended consequence of moving
device I/O off the bus thread (defect LU-01), not a behaviour regression.

One scenario needed a sequencing change for a documented driver rule: temperature
compensation deliberately restarts from the reading that follows the switch to automatic
mode, and the generated `on_timer` runs once immediately at connect, so
`temperature_compensation` now waits for one seeding `!read temps 0#` after selecting
automatic mode before injecting the temperature change.

Reference trace comparison, 24 traces captured before and after with the command and
normalization recorded above:

- 22 of 24 are **byte identical**, including `identity_and_inventory` for both drivers. The
  connect order is preserved exactly: `!seletek version#` twice (once from the open, once
  from the per-port existence check), then `delswlimits`, `movepow`, `stoppow`, `model`,
  `halfstep`, `getpos`, for the rotator `setpos`, then `speedrangeus`, `wiremode` and the
  limits decision.
- `focuser/abort_motion.txt` gains one `!step setpos 0 0#`. That is defect LU-09: the
  original swallowed the scenario's first SYNC entirely.
- `rotator/limits.txt` changes the argument of the re-sync `!step setpos` after a limits
  change. That is defect LU-08: the original re-synced to a step value computed from the old
  mapping.

Both differences are the two defect fixes and nothing else; no unexplained difference remains.

### Step 7 — repository integration

- `indigo_test/Makefile`: the simulator is now built once per driver
  (`focuser_lunatico_simulator`, `rotator_lunatico_simulator` and the `SIM_UDP=1`
  `rotator_lunatico_udp_simulator`) from a shared `LUNATICO_SIMULATOR_SOURCES` prerequisite
  list; `test_rotator_lunatico_simulator` was added to `INTEGRATION_TESTS`, where it had been
  missing, so `make -C indigo_test test` now runs it; both suites gained an opt-in
  `*_asan` target.
- `indigo.xcodeproj/project.pbxproj`: the two `.driver` definitions, `shared/lunatico_shared.h`,
  the two simulator groups with their sources, `indigo_test/integration/lunatico_test_common.h`,
  `rotator_lunatico/REFACTOR.md` and the two Windows projects were added; the two deleted
  `Makefile.inc` entries were removed. `plutil -lint` reports OK.
- Windows: `indigo_focuser_lunatico.vcxproj`, `.filters` and `.user` and the rotator
  equivalents were added following `aux_dragonfly`, and both projects were registered in
  `indigo_windows.sln` with their build configurations. The drivers contain no
  platform-dependent code any more, but **no Windows build was performed**; this is structural
  integration only.
- `indigo_docs/PROPERTIES.md`: the shared section was rewritten for the new property set,
  the seven-device model and the driver-specific semantics.
- `MIGRATION_STATUS.md`: both rows advanced to 3️⃣ with Windows, generator and async queues
  marked, `Retested` left at `Sim`, and the counts corrected to `50 / 0` and `27 / 0`. The
  Comment column is unchanged.
- `README.md` of both drivers is **not** updated: it documents the port configuration that
  this change removes, and the root instructions forbid changing a `README.md` without the
  user's explicit approval. This is flagged for the user as the one remaining documentation
  gap.

### Step 8 — final verification

- Generation reproducibility: re-running `indigo_generator` on both definitions reproduces
  the checked-in `.c` byte for byte.
- Strict rebuild: `make -B -f ../../Makefile.drv` in both driver directories, 0 lines
  containing `warning` or `error`, universal x86_64 + arm64.
- Sanitizer: `build/integration/test_focuser_lunatico_simulator_asan` and
  `..._rotator_..._asan` compile the driver into the test with
  `-fsanitize=address -fno-omit-frame-pointer`. Both run all scenarios with **no sanitizer
  report** and 0 failing scenarios. This is also the evidence that defect LU-02, the
  `heap-use-after-free` on port reconfiguration, is gone: the dynamic port lifecycle that
  caused it no longer exists.
- UDP transport: `make -C indigo_test test-rotator-lunatico-udp` runs the whole rotator suite
  over a UDP simulator, 27 / 27 passed.
- Platforms: macOS arm64 host, universal build. Linux and Windows were **not** built or run;
  recorded as an unverified platform.

## Found defects

Defects discovered during this refactoring. Each records observable impact, root cause, the
production fix and the regression test that proves it. Fixes and tests are recorded as the
corresponding plan steps complete.

### LU-01 — blocking device I/O on bus callbacks (audit, High)

Impact: a `FOCUSER_POSITION`, `ROTATOR_POSITION`, `AUX_POWER_OUTLET`, `FOCUSER_LIMITS`,
`FOCUSER_SPEED` or `LA_*` change is executed synchronously inside `*_change_property`, which
runs on the bus thread. `set_power_outlets()` alone issues four round trips, and each round
trip can block for up to 3.1 s on the reply timeout, so one unresponsive controller stalls
the whole bus for over twelve seconds.

Root cause: the driver predates the handler queue and does its device I/O inline.

Fix: every change handler is now a generated `on_change` dispatched onto the master device's
handler queue, and no handler waits for motion.

Status: **fixed**. Evidence: the generated change branches all use
`INDIGO_COPY_VALUES_PROCESS_CHANGE`, and the behavioural consequence is directly visible in
the suites, which had to stop treating "the property went BUSY" as "the command was sent"
(six scenarios, listed under step 5/6).

### LU-02 — use-after-free on port reconfiguration (audit, High)

Impact: changing `LUNATICO_PORT_EXP_CONFIG` or `LUNATICO_PORT_THIRD_CONFIG` on a live driver
runs `delete_port_device()` and then `create_port_device()`; a later
`indigo_init_number_property()` reuses memory already freed by `indigo_focuser_detach()`. ASan
reports `heap-use-after-free` at `indigo_bus.c:1139`.

Root cause: `delete_port_device()` frees the `indigo_device` immediately after
`indigo_detach_device()` without waiting for the bus to finish with it.

Fix: the whole dynamic port lifecycle is removed by the static seven-device model, so no
`indigo_device` is ever freed while the driver is loaded.

Status: **fixed**. Evidence: both suites run clean under AddressSanitizer (step 8), and the
properties that drove the reconfiguration no longer exist.

### LU-03 — non-portable POSIX transport (audit, Medium)

Impact: the driver cannot build on Windows. `lunatico_command()` uses `select()`, `read()`,
`close()` and a raw `int` fd directly.

Root cause: predates `indigo_uni_io`.

Fix: the transport is rewritten onto `indigo_uni_io`, and Windows project files were added
for both drivers.

Status: **fixed**, with the limitation that no Windows build was performed here. Evidence:
the full suites, including the `silent`, `oversized` and `bad_version` profiles and the UDP
transport run, pass against the rewritten transport.

### LU-04 — custom property names without the `X_` prefix (audit, Low)

Impact: `LA_STEP_MODE`, `LA_POWER_CONTROL`, `LA_TEMPERATURE_SENSOR`, `LA_MOTOR_WIRING`,
`LA_MOTOR_TYPE`, `LUNATICO_MODEL`, `LUNATICO_PORT_EXP_CONFIG` and
`LUNATICO_PORT_THIRD_CONFIG` violate the mandatory `X_` prefix for driver-specific
properties, so they are indistinguishable from standard INDIGO properties by name.

Root cause: predates the naming rule.

Fix: the five surviving properties are renamed to `X_FOCUSER_*` and `X_ROTATOR_*`; the three
port configuration properties are removed with the dynamic port model. The class qualifier is
required anyway, because each logical device needs its own property handle.

Status: **fixed**. Evidence: the property inventory assertions of `identity_and_inventory` in
both suites, and `indigo_docs/PROPERTIES.md`.

### LU-05 — timer reference reused while still armed (reproduced, Low)

Impact: `Attempt to set timer with non-NULL reference` is logged by the framework on focuser
paths. Observed in the baseline run of `test_focuser_lunatico_simulator`, scenarios
`abort_motion` and `read_failure`.

Root cause: `focuser_timer_callback` is armed through `&PORT_DATA.focuser_timer` from several
places without the previous reference being cleared or cancelled first.

Reproduced as a log line, not as a functional failure. A dedicated scenario,
`goto_overlapping_connect_poll`, issues a GOTO while that poll is still pending: the warning
appears every time against the original driver, but the move still completes, because the
already pending poll picks it up. The scenario is kept to pin that observable outcome.

Fix: explicit `indigo_timer` fields disappear; polling is the generated `on_timer` and
completion a named `_finalizer`, both owned by the handler queue, so the hazard class is gone
by construction.

Status: **fixed**. Evidence: `goto_overlapping_connect_poll` passes after migration and the
warning no longer appears in the suite output.

### LU-06 — the rotator poll is armed and cancelled through different fields (audit, Medium)

Impact: `ROTATOR_ABORT_MOTION` cancels `PORT_DATA.rotator_timer`, but the goto branch of
`ROTATOR_POSITION` arms the same `rotator_timer_callback` through `PORT_DATA.focuser_timer`.
An abort therefore cannot stop the poll that a goto started, and `rotator_timer_callback`
clears `PORT_DATA.focuser_timer` on completion even though connect armed it through
`PORT_DATA.rotator_timer`.

Root cause: copy-paste from the focuser implementation; both fields exist in the same
`lunatico_port_data`.

Not reproducible through public bus APIs, and this is recorded honestly rather than claimed:
the orphaned poll writes to an already closed handle, so nothing reaches the controller and
no property update survives the class properties being deleted on disconnect. It is visible
only in the driver's own log, where `lunatico_is_moving(0) failed` and `NO response` appear
after a clean disconnect. An earlier attempt to assert it through `ROTATOR_POSITION` was
withdrawn because it failed for an unrelated reason, that the property is uncached on
disconnect, both before and after the migration.

Fix: removed with the timer fields. The generated disconnect calls
`indigo_cancel_pending_handlers(device)` before tearing the device down, so no callback of a
disconnected device can survive.

Status: **fixed by construction**. Evidence: the generated connection handler, and the
`disconnect_during_motion` scenario of the rotator suite, which asserts that nothing reaches
the controller after the close and that the next connection starts from a freshly read
angle.

### LU-07 — `make -f ../../Makefile.drv` builds nothing (reproduced, Low)

Impact: the command documented in `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` prints
`indigo_focuser_lunatico.c' is up to date` and compiles nothing, because the first target of
the driver's `Makefile.inc` is a `touch` rule and therefore the default goal.

Root cause: `Makefile.inc` is included after the default goal would otherwise be established.

Fix: both `Makefile.inc` files are deleted; the generator dependency in `Makefile.drv`
replaces what they declared.

Status: **fixed**, reproduced before and verified after: `make -f ../../Makefile.drv` with no
explicit goal now compiles and links the driver.

### LU-08 — a rotator limits change moved the reported angle (reproduced, High)

Impact: changing `ROTATOR_LIMITS.MIN_POSITION` from -180 deg to -90 deg while the rotator
reports 0 deg made it report 90 deg, without any motion and without the user asking for one.
Measured directly against the original driver: the reported angle went from `0.000` to
`90.000` across the limits change.

Root cause: both limits and the steps per revolution define the degree-to-step mapping, so the
driver re-syncs the controller's position counter after either changes. The original
`lunatico_sync_to_current()` wrote a step value that did not correspond to the displayed angle
under the new mapping, so the next conversion of the unchanged counter produced a different
angle. In the captured trace it wrote `!step setpos 0 1800#` where the new mapping requires
900.

Fix: `lunatico_rotator_resync()` converts the currently displayed angle with the new mapping,
so the counter and the angle stay consistent.

Regression test: `limits_change_keeps_angle` in `test_rotator_lunatico_simulator.c`. It
asserts `!step setpos 0 900#` and that the reported angle stays 0 deg. It **fails against the
original driver** and passes after the migration.

### LU-09 — the first focuser SYNC was silently swallowed (reproduced, High)

Impact: the first `FOCUSER_POSITION` request for position 0 after connecting was answered
`INDIGO_OK_STATE` with no command sent at all, while the focuser stayed where it was. The
client saw a successful SYNC that never happened.

Root cause: the `FOCUSER_POSITION` handler skips a request that equals
`PORT_DATA.f_current_position`, and `handle_focuser_connect_property()` set
`FOCUSER_POSITION_ITEM->number.value` from the controller but never set
`PORT_DATA.f_current_position`, which therefore stayed at its zero-initialised 0. This is the
focuser counterpart of `DRV-210`, which was fixed for the rotator in commit `b9f894420` and
left unfixed here.

Fix: `lunatico_focuser_connect()` seeds `PORT_STATE.focuser_position` and
`PORT_STATE.focuser_target` from the position it reads at connect.

Regression test: `sync_to_zero_after_connect` in `test_focuser_lunatico_simulator.c`. It
asserts `!step setpos 0 0#` reaches the controller and the reported position becomes 0. It
**fails against the original driver** and passes after the migration.

## Final test summary

- Simulated tests: **77 executed, 77 passed** — `test_focuser_lunatico_simulator` 50 of 50 and
  `test_rotator_lunatico_simulator` 27 of 27 against the migrated drivers.
- The same 77 scenarios were also run against the **original** drivers, where 75 passed and
  the two dedicated defect reproducers `sync_to_zero_after_connect` (LU-09) and
  `limits_change_keeps_angle` (LU-08) failed, as recorded above.
- Additional runs of the same scenarios: 77 of 77 under AddressSanitizer with no sanitizer
  report, and 27 of 27 over the UDP transport (`make -C indigo_test test-rotator-lunatico-udp`).
- Hardware tests: **0 executed, 0 passed.** No Lunatico controller was available and no
  hardware validation is claimed.
