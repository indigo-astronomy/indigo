# dome_dragonfly refactoring record

This record covers `indigo_dome_dragonfly` (Lunatico Astronomia Dragonfly used as a roll-off
roof controller with a general-purpose relay device). The transport, relay and sensor
implementation is shared with `indigo_aux_dragonfly` through
`../aux_dragonfly/shared/dragonfly_shared.c`; the protocol audit, the shared-code design, the
hardware-test decision and the shared migration plan live in
`../aux_dragonfly/REFACTOR.md`. Read both together. The two drivers are mutually exclusive:
each refuses `INDIGO_DRIVER_INIT` while the other is initialized.

## Scope and baseline

Migration of `indigo_dome_dragonfly` from its hand-written INDIGO 2.0 implementation
(POSIX `select`/`read`/`close` on a raw fd, `pthread` port and relay mutexes,
`indigo_set_timer` timer threads, blocking property handlers, per-logical-device property
arrays with `hidden` masking) to `indigo_generator` with portable `indigo_uni_io` and the
device handler queue, plus the first hardware-free simulator coverage this driver has ever
had.

Baseline date and source: 2026-09-19, commit `371b2d08a`, branch `refactoring`, clean working
tree. Host: macOS 26.6.2 (`Darwin 25.6.0`), Apple Silicon arm64; repository universal build
(x86_64 + arm64).

Baseline build:

```sh
cd indigo_drivers/dome_dragonfly
make -B -f ../../Makefile.drv
```

Result: passed, 0 lines containing `warning`.

Baseline automated tests: **none exist**. `MIGRATION_STATUS.md` records `0 / 0` for this
driver and there is no simulator, no integration test and no ASan target. The
characterization suite required by `indigo_drivers/AGENTS.override.md` therefore has to be
written against the unchanged driver before any production change; that is step 4 of the
shared plan.

## Hardware-test decision

No hardware testing will be performed; no Dragonfly controller and no roof are available.
Nothing in this record implies physical validation.

## Studied sources

In addition to the sources listed in `../aux_dragonfly/REFACTOR.md`:

- `indigo_dome_dragonfly.c`, `.h`, `_main.c`, `Makefile.inc`, `README.md` in this directory.
  The README is the authoritative description of the three supported wirings and of the
  sensor and relay assignment; it is user-facing and is not modified by this work.
- `indigo_libs/indigo_dome_driver.c` (base property visibility, and the fact that the base
  disconnect path resets `DOME_SHUTTER` to `INDIGO_OK_STATE` before deleting it).
- `indigo_test/DRIVER_TESTING_RULES.md`, Dome section.

## Current-state audit

### Hardware model

One Dragonfly controls both the roof and five general-purpose relays:

| Channel | Role |
| --- | --- |
| Relay 1 | 1-button wiring: open/close/stop. 3-button wiring: stop. |
| Relay 2 | 2- and 3-button wirings: open. |
| Relay 3 | 2- and 3-button wirings: close. |
| Relays 4…8 | general purpose, exposed by the `Dragonfly Controller` AUX device |
| Sensor 1 | roof fully open (analog, active above 512) |
| Sensor 2 | roof fully closed (analog, active above 512) |
| Sensors 3…7 | general purpose, exposed by the AUX device |
| Sensor 8 | mount parked (analog, active above `PARK_SENSOR_THRESHOLD`, default 512) |

Wirings, from `README.md`:

- **1 button, push** — relay 1 is pulsed; the controller toggles open → stop → close → stop.
- **2 buttons, push and hold** — relay 2 is switched on to open and stays on until it is
  switched off, relay 3 likewise for closing.
- **3 buttons, push** — relay 2 is pulsed to open, relay 3 to close, relay 1 to stop.

### Architecture and implementation (original)

- `DRIVER_VERSION 0x02000007`, label `Lunatico Dragonfly Dome`. Two logical devices created at
  INIT: `Dome Dragonfly` (index 0, `TYPE_DOME`) and `Dragonfly Controller` (index 1,
  `TYPE_AUX`). Both share one `lunatico_private_data` and reference-count the socket through
  `count_open`. Both publish their own `DEVICE_PORT`, so either can be connected first and
  each carries its own URL.
- The same `device_data[4][2]` matrix, `gp_bits` flags and `create_port_device()` /
  `delete_port_device()` scaffolding as `aux_dragonfly`; only physical index 0 is ever used.
- All AUX and dome properties are allocated for **both** logical devices by the shared
  `lunatico_init_properties()` and then masked with `hidden = true` for the device that does
  not own them. The generator replaces this with one property set per device block.
- Dome connection (`handle_dome_connect_property`, timer thread): open socket,
  `!seletek version#`, reject a non-Dragonfly model, copy model and firmware into INFO,
  authenticate, read all sensors once and seed `DOME_SHUTTER` / `roof_state` from sensors 1
  and 2 (`ROOF_OPENED`, `ROOF_CLOSED`, otherwise `ROOF_UNKNOWN` with `DOME_SHUTTER` ALERT),
  then start a 10 s `!seletek echo#` keep-alive timer.
- AUX connection is the `aux_dragonfly` sequence restricted to relays 4…8 and sensors 3…7.
- `DOME_SHUTTER` change (`dome_handle_shutter`, timer thread, blocking):
  1. If already BUSY, republish and return.
  2. Read all sensors. If sensor 8 is above the park threshold, set `parked`, then reconcile
     `roof_state` with sensors 1 and 2: a roof that reached an end position while the driver
     thought otherwise regains control; a roof that left an end position without the driver
     moving it publishes ALERT with "moved by hand" and returns.
  3. If the roof is already in the requested state, or neither switch is on, publish OK and
     return.
  4. If not parked, restore the switch from `roof_state` and publish ALERT
     "Can not move the roof, mount is not parked!".
  5. Otherwise drive the wiring: switch the relay on, publish BUSY with
     "Roof is opening…" / "Roof is closing…" / "Roof is either opening or closing…",
     set `roof_state` and reset `roof_timer_hits`, **sleep** `BUTTON_PULSE_LENGTH` seconds,
     release the relay (1-button always; 3-button only; 2-button keeps it on), and arm the
     roof timer at `READ_SENSORS_DELAY` seconds.
- Roof timer (`dome_timer_callback`, rescheduled every 1 s): increments `roof_timer_hits`;
  when it exceeds `OPEN_CLOSE_TIMEOUT` it switches relays 2 and 3 off, clears both shutter
  switches and publishes ALERT "Open / Close timed out."; otherwise it returns when
  `DOME_SHUTTER` is no longer BUSY, reads the sensors, and on reaching an end position
  switches relays 2 and 3 off and publishes OK with "Roof is open." / "Roof is closed.",
  or ALERT when both end sensors are active.
- `DOME_ABORT_MOTION` change (`dome_handle_abort`, timer thread, blocking): clears the abort
  item and publishes it OK, returns when `DOME_SHUTTER` is not BUSY, otherwise sets
  `DOME_SHUTTER` ALERT, switches relays 2 and 3 off, and for the 1- and 3-button wirings
  pulses relay 1 for `BUTTON_PULSE_LENGTH` seconds; `roof_state` becomes
  `ROOF_STOPPED_WHILE_OPENING` / `ROOF_STOPPED_WHILE_CLOSING` and the message is
  "Roof Stopped." A failed relay write publishes "Can not stop the roof, did you authorize?".
- Dome disconnect cancels the keep-alive and roof timers synchronously, deletes the AUX
  properties (`lunatico_delete_properties()` — note that it deletes the AUX device's
  properties even from the dome device, see the found-defects section) and closes the socket.

### Public properties (original)

`Dome Dragonfly` device, always defined: `LA_DOME_SETTINGS` (4 numbers:
`BUTTON_PULSE_LENGTH` 0…3 s default 0.5, `READ_SENSORS_DELAY` 0…6 s default 2.5,
`OPEN_CLOSE_TIMEOUT` 0…300 s default 60, `PARK_SENSOR_THRESHOLD` 0…1024 default 512) and
`LA_DOME_BUTTON_FUNCTION` (3 switches, one-of-many, default 1-button). Both are persistent.
Inherited and visible: `DOME_SHUTTER` (relabelled "Shutter / Roof"), `DOME_ABORT_MOTION`,
`GEOGRAPHIC_COORDINATES`, `DEVICE_PORT`, `AUTHENTICATION`. Hidden: `DOME_SPEED`,
`DOME_DIRECTION`, `DOME_HORIZONTAL_COORDINATES`, `DOME_STEPS`, `DOME_PARK`,
`DOME_DIMENSION`, `DOME_SLAVING_PARAMETERS`.

`Dragonfly Controller` device: the `aux_dragonfly` property set reduced to five relays
(items `OUTLET_4`…`OUTLET_8`) and five sensors (items `GPIO_SENSOR_NAME_3`…`GPIO_SENSOR_NAME_7`).

### Defects, risks and gaps found by audit

See "Found defects". Notable risks carried into the migration plan: the blocking sleeps in
the shutter and abort handlers hold the only device thread; `lunatico_delete_properties()` is
called from the dome disconnect path; the roof timeout is counted in timer ticks that start
`READ_SENSORS_DELAY` late; and there is no automated coverage whatsoever.

### Build and packaging

`Makefile.inc` makes `indigo_dome_dragonfly.c` depend on the shared file with a `touch`
recipe, which collides with the generator rule and has to become an object-file dependency.
The Xcode project references the existing sources; the `.driver` input, the simulator and the
new test must be added.

## Migration design

- One `indigo_dome_dragonfly.driver` with `driver dragonfly { … dome { … } aux { … } }`, so
  the entry point stays `indigo_dome_dragonfly`, the dome is the master device and the AUX
  device is its slave. The shared file sees exactly the same `dragonfly_private_data`,
  `dragonfly_open()` and `dragonfly_close()` symbols as in `aux_dragonfly`.
- `RELAY_FIRST 3`, `RELAY_COUNT 5`, `SENSOR_FIRST 2`, `SENSOR_COUNT 5` in `define { }` select
  the dome driver's AUX channel window in the shared relay/sensor code.
- The dome's `on_timer` becomes the keep-alive loop; the roof state machine becomes
  `dome_shutter_handler` (generated from `DOME_SHUTTER.on_change`) plus a named
  `dome_shutter_finalizer`, as required by the finalizer rules in `AGENTS.md`.
- The roof timeout becomes a monotonic deadline taken when motion starts, instead of a tick
  counter that begins `READ_SENSORS_DELAY` late.
- `DOME_ABORT_MOTION` is dispatched by the generator at urgent priority and cancels the
  pending `dome_shutter_handler` and `dome_shutter_finalizer`.
- The AUX device's `DEVICE_PORT` becomes hidden and the connection follows the dome device's
  `DEVICE_PORT`, which is the generated multi-device contract
  (`<driver>_open(device->master_device)` guarded by `PRIVATE_DATA->count`).
- `LA_DOME_SETTINGS` and `LA_DOME_BUTTON_FUNCTION` are renamed to `X_DOME_SETTINGS` and
  `X_DOME_BUTTON_FUNCTION` to satisfy the mandatory `X_` prefix for driver-specific custom
  properties in the root `AGENTS.md`. Item names and semantics are unchanged.

## Atomic migration plan

The shared, numbered plan is in `../aux_dragonfly/REFACTOR.md`. Steps owned by this driver:

4. **Add `test_dome_dragonfly_simulator.c` as the full dome + AUX acceptance suite against the
   unchanged driver**, plus its simulator target, and record the normalized reference trace.
   State: **done**, see "Step 4".
6. **Migrate `dome_dragonfly` to the generator** on the refactored shared file, regenerate,
   build, re-run the suite and compare traces.
   State: **done**, see "Step 6".

## Step evidence

### Step 4 — dome_dragonfly characterization suite

This driver had no automated coverage at all. `indigo_test/integration/test_dome_dragonfly_simulator.c`
is new and registers 21 scenarios over the shared Dragonfly simulator
(`dome_dragonfly_simulator/dome_dragonfly_simulator.c`, wired into `indigo_test/Makefile`
together with an ASan target):

| Scenario | Profile | Covers |
| --- | --- | --- |
| `dome_identity_and_inventory` | normal | driver metadata, dome interface bit, the complete published/hidden property inventory before and after connection, the relabelled shutter, the settings and wiring items, the identity readback |
| `aux_identity_and_inventory` | normal | the relay device's inventory, exactly five outlets (`OUTLET_4`…`OUTLET_8`) and five sensors (`GPIO_SENSOR_NAME_3`…`GPIO_SENSOR_NAME_7`) |
| `one_button_open_and_close` | normal | the default wiring, open and close to the end sensors |
| `two_button_open` | two-button | push-and-hold wiring |
| `three_button_open` | three-button | push wiring with separate open/close/stop relays |
| `abort_while_opening` | slow-roof | abort of a travelling roof, the stop button, shutter ALERT, abort OK, and that the shutter stays ALERT |
| `open_timeout` | slow-roof | the open/close timeout, both shutter switches cleared |
| `refuses_to_move_when_not_parked` | not-parked | the mount park interlock and the restored shutter switch |
| `initial_state_open` | open | shutter seeded from the end sensors at connection |
| `initial_state_unknown` | half-open | a roof between the end positions is ALERT with no switch set |
| `initial_state_both_sensors` | open+both-sensors | contradictory end sensors are ALERT |
| `already_open` | open | requesting the state the roof is already in completes without moving it |
| `shutter_relay_error` | relay-error | a rejected relay command fails the operation |
| `aux_relays` | normal | all five general purpose relays on and off |
| `aux_sensors` | normal | all five general purpose sensors |
| `aux_pulse` | normal | a timed pulse that clears itself |
| `shared_connection_dome_first` | normal | the AUX device joins the dome's session, the dome survives the AUX disconnect and still moves the roof |
| `shared_connection_aux_first` | normal | the reverse order, and the AUX device survives the dome disconnect |
| `lifecycle` | normal | SHUTDOWN refused while connected, redundant disconnect tolerated |
| `wrong_identity` | wrong-model | connection refused for a non-Dragonfly controller |
| `silent_device` | no-reply | connection refused for a controller that does not answer |

```sh
make -C indigo_test test-dome-dragonfly-simulator
```

Result against the **unchanged** driver: 21 scenarios run, 21 passed, 0 failing scenarios,
41.6 s. There is no expected-failure reproducer.

Reference trace, same mechanism and normalization as `../aux_dragonfly/REFACTOR.md` step 3,
except that the periodic `!seletek echo#` keep-alive is dropped instead of the sensor poll,
because on the dome device the sensor reads are part of the roof state machine and are
deterministic while the keep-alive interval is not:

```sh
cd indigo_test
AUX_TEST_TRACE=1 AUX_TEST_FILTER=<scenario> ./build/integration/test_dome_dragonfly_simulator 2>&1 >/dev/null \
  | grep -E '^(->|<-) ' | grep -v 'seletek echo' | awk '$0 != prev { print } { prev = $0 }'
```

Traces captured for `one_button_open_and_close`, `two_button_open`, `three_button_open`,
`abort_while_opening`, `refuses_to_move_when_not_parked`, `shutter_relay_error`,
`aux_relays` and `aux_pulse`, and verified stable across repeated runs. They pin down the
manner the migration must preserve, notably:

- Connection reads the sensors once (`!relio snanrd 0 0 7#`) after `!seletek version#`, and
  sends no `!aux earnaccess#` while the stored password is empty.
- Every shutter request reads the sensors once before deciding, and sends nothing at all when
  the mount is not parked.
- 1-button: `rlset 0 0 1` then `rlset 0 0 0` after the button pulse.
  3-button: the same edge pair on the open/close relay. 2-button: `rlset 0 1 1` only, with no
  release until the end sensor is reached.
- Reaching an end position always sends `rlset 0 1 0` followed by `rlset 0 2 0`, in that
  order, regardless of the wiring.
- Abort sends `rlset 0 1 0`, `rlset 0 2 0`, then `rlset 0 0 1` and `rlset 0 0 0` around the
  button pulse for the 1- and 3-button wirings.

### Step 6 — dome_dragonfly migrated to the generator

New `indigo_dome_dragonfly.driver` with a `dome` master device and an `aux` slave device over
`serial { no_ports = true; }`, on the refactored `../aux_dragonfly/shared/dragonfly_shared.c`
and `dragonfly_shared.h`. `DRIVER_VERSION` is now `0x03000008` (was `0x02000007`).

Structure:

- The shared relay and sensor code is reused unchanged through `RELAY_FIRST 3`,
  `RELAY_COUNT 5`, `SENSOR_FIRST 2`, `SENSOR_COUNT 5`, so the relay device of this driver and
  the whole of `aux_dragonfly` run the same implementation.
- Every property now belongs to exactly one device block, which removes the old pattern of
  allocating the full property set for both logical devices and masking it with
  `hidden = true`.
- The dome is the master: it owns `DEVICE_PORT`, and the relay device connects through
  `dragonfly_open(device->master_device)` under the generated `PRIVATE_DATA->count`
  reference counting. The relay device's own `DEVICE_PORT` is therefore hidden; see the
  intentional differences below.
- The roof state machine is `dome_shutter_handler` (generated from `DOME_SHUTTER.on_change`)
  plus the named `dome_shutter_finalizer`, and the keep-alive is the dome device's
  `on_timer`. The `device_data[4][2]` matrix, `create_port_device()` / `delete_port_device()`,
  `at_least_one_device_connected()`, the `gp_bits` flags, both `pthread` mutexes and every
  POSIX `select` / `read` / `close` call are gone.

Build: `make -B -f ../../Makefile.drv` — passed, 0 warnings.
Strict check `clang -fsyntax-only -Wall -Wextra indigo_dome_dragonfly.c` — clean.

Suite: `make -C indigo_test test-dome-dragonfly-simulator` — 22 scenarios run, 22 passed
(21 from the characterization suite plus the new `abort_before_start`), 38.7 s.

Reference trace comparison: `one_button_open_and_close`, `two_button_open`,
`three_button_open`, `abort_while_opening`, `refuses_to_move_when_not_parked`,
`shutter_relay_error`, `aux_relays` and `aux_pulse` are **byte identical** to the
pre-migration traces. The `aux_relays` and `aux_pulse` comparison additionally filters the
free-running one second sensor poll of the relay device, whose interleaving with command
traffic is nondeterministic on both drivers.

Two test changes were needed, both because the generated dispatch publishes state the
original never published, not because driver behaviour regressed:

- `already_open`: the original answered a request for the state the roof is already in with
  an `indigo_update_property()` that the bus suppressed, because neither the state nor any
  item had changed — the client was never told the request had been processed. The generated
  dispatch publishes BUSY on acceptance and the handler then publishes OK, so the scenario
  now asserts an OK update that is newer than the request and that the shutter then stays OK,
  which is a stronger assertion than the original could satisfy.
- `abort_while_opening`: because acceptance already publishes BUSY, an abort sent as soon as
  BUSY appears is dispatched at urgent priority and overtakes the still queued shutter start.
  That is the documented generator behaviour and the handler cancels the queued start, so the
  roof is never pushed. The scenario now waits for the second BUSY update, the one the
  handler publishes once the button has actually been pushed, and the overtaking case became
  its own scenario `abort_before_start`.

Intentional differences:

- **`LA_DOME_SETTINGS` → `X_DOME_SETTINGS`** and **`LA_DOME_BUTTON_FUNCTION` →
  `X_DOME_BUTTON_FUNCTION`**. The root `AGENTS.md` requires the `X_` prefix for
  driver-specific properties and both recently migrated dome drivers (`dome_beaver`,
  `dome_talon6ror`) follow it. Item names, labels, ranges, defaults and semantics are
  unchanged. This is the one user-visible incompatibility of the migration: a saved
  configuration written by an older build no longer restores these two properties, and a
  client that addresses them by name has to be updated. `indigo_docs/PROPERTIES.md` records
  the old names.
- **The relay device no longer has its own `DEVICE_PORT`.** In the original both logical
  devices published one and whichever connected first decided the URL. The generated
  multi-device contract opens through the master, so the dome device's `DEVICE_PORT` is the
  single source of truth. Both connection orders are covered by
  `shared_connection_dome_first` and `shared_connection_aux_first`.
- **The keep-alive runs immediately on connect** and every 10 s afterwards, instead of first
  at 10 s, because the generator starts a device's `on_timer` at the end of a successful
  connection. One extra `!seletek echo#` right after the identity handshake.
- **The open/close timeout is a monotonic deadline** taken when the motion starts; see
  `DF-11`.
- **`DOME_ABORT_MOTION` is published OK when the stop has completed**, not before the stop is
  attempted. The generated urgent dispatch publishes BUSY on acceptance, so the client still
  sees the request being processed.
- Messages: the failed 1-button open now says "Can not move the roof, did you authorize?"
  (`DF-10`) and the contradictory-sensor message spells "quantum" (`DF-13`).
- The generated connect/disconnect messages are new, as for `aux_dragonfly`.

The `indigo_usleep()` that holds a momentary button for `BUTTON_PULSE_LENGTH` was
**deliberately preserved** in `dome_start_motion()` and in the abort handler. It is not a
wait for motion to complete but the emulation of a button press, bounded by the property's
own 0…3 s range; splitting it into a queued release would make a cancelled or delayed
release leave the relay latched, which on the push-and-release wirings means a stuck button
on real hardware. The cost is that an abort issued during the press waits for it, exactly as
in the original.

## Found defects

Defects specific to this driver. The transport, relay and build defects shared with
`aux_dragonfly` are in `../aux_dragonfly/REFACTOR.md` (`DF-01`…`DF-06`); `DF-01`, `DF-02`,
`DF-04` and `DF-05` apply to this driver as well.

### DF-10 — a failed open reported a close failure (audit, Low)

*Impact.* In the one-button wiring, the branch that opens a closed roof published
"Can not close the roof, did you authorize?" when the relay write failed, so a user
debugging a permission problem was told the wrong operation had failed.

*Root cause.* Copy/paste from the close branch.

*Fix.* One shared failure message, "Can not move the roof, did you authorize?".

*Test.* `shutter_relay_error` covers the failure path; the harness does not cache messages,
so the text itself is not asserted.

### DF-11 — the open/close timeout was longer than configured and could overwrite an abort (audit, Medium)

*Impact.* The roof timer counted its own ticks and was first armed `READ_SENSORS_DELAY`
seconds after the push, so a `OPEN_CLOSE_TIMEOUT` of 60 s actually expired after about
62.5 s with the default settings. The tick also evaluated the timeout **before** checking
whether `DOME_SHUTTER` was still BUSY, so a tick that arrived after an abort had already
settled the property could overwrite "Roof Stopped." with "Open / Close timed out." and turn
the direction relays off again.

*Root cause.* Tick counting instead of a deadline, and the order of the two guards.

*Fix.* `dome_start_motion()` stores `indigo_monotonic_time() + OPEN_CLOSE_TIMEOUT` and
`dome_shutter_finalizer()` returns immediately when the shutter is no longer BUSY, before it
looks at the deadline.

*Test.* `open_timeout` drives a roof that never reaches its end sensor with a 3 s timeout and
asserts BUSY → ALERT with both shutter switches cleared. `abort_while_opening` asserts that
the shutter stays ALERT for two seconds after the abort.

### DF-12 — a shutter request could be left unanswered in the two and three button wirings (audit, Low)

*Impact.* When neither the open nor the close condition matched, the two/three button branch
fell out of the handler without publishing anything. It is unreachable in the original
because the earlier "already in the requested state" check catches the same conditions, but
under the generated dispatch, which publishes BUSY on acceptance, it would leave
`DOME_SHUTTER` BUSY forever.

*Fix.* The branch publishes `INDIGO_OK_STATE` and returns.

*Test.* None; the branch is unreachable through the public interface, and the fix exists to
keep it that way under the new dispatch.

### DF-13 — the dome disconnect deleted the relay device's properties (audit, Low)

*Impact.* `handle_dome_connect_property()` called `lunatico_delete_properties()`, which
deletes `AUX_GPIO_OUTLETS`, `AUX_OUTLET_PULSE_LENGTHS` and `AUX_GPIO_SENSORS` through the
dome device. Those property pointers belong to the dome device's own slot of the shared
`device_data[]` array and are `hidden` there, so `indigo_delete_property()` ignored them and
nothing was published; the call was dead code that hid how the property ownership actually
worked.

*Fix.* Each device block owns its own properties and the generator deletes exactly those on
that device's disconnect.

*Test.* `shared_connection_dome_first` and `shared_connection_aux_first` assert that
disconnecting one logical device leaves the other working.

Also fixed in passing: the message "Roof shows qantum properties" now spells "quantum".

## Final test summary

**Simulated tests**

| Run | Executed | Passed |
| --- | --- | --- |
| `test_dome_dragonfly_simulator` against the original driver (characterization) | 21 | 21 |
| `test_dome_dragonfly_simulator` against the migrated driver | 22 | 22 |
| `test_dome_dragonfly_simulator_asan` (driver-instrumented) against the migrated driver | 22 | 22 |
| `make test-dome-dragonfly-simulator` after `test-clean`, from a clean build | 22 | 22 |
| **Total** | **87** | **87** |

The suite contains 22 distinct scenarios; `MIGRATION_STATUS.md` records `22 / 0`. The
combined totals for both Dragonfly drivers, and the whole-repository run, are in
`../aux_dragonfly/REFACTOR.md`.

**Hardware tests: 0 executed, 0 passed.** No Dragonfly controller and no roof are available,
so the roof kinematics, the three wirings, the mount park interlock, the button pulse timing
and the relay wiring itself are validated only against the simulator model of
`README.md`. Nothing in this record implies physical validation.
