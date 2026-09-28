# INDIGO 3.0 refactoring record for `aux_upb`

This record was created together with the 2026-09-18 `reject_change` work. The driver's earlier
migration to `indigo_generator` predates this file and is deliberately not reconstructed here; only
the change below is recorded, so nothing in this file is inferred history.

## Rejected-change regression coverage (2026-09-18)

A change request that arrives while a motion is already running is now refused with the generator's
`reject_change` block on `FOCUSER_POSITION` and `FOCUSER_STEPS`, with the condition that the other motion property is BUSY and the message
shown in the generated driver. The generated guard marks every item for update, sets
`INDIGO_ALERT_STATE` and publishes the property, so the client receives the actual driver-side values
instead of an update that carries no items.

Both motion properties are published BUSY together at motion start, so `INDIGO_COPY_*_PROCESS_CHANGE` dropped a concurrent request silently: the client received no update at all and kept showing the refused target. The guard runs before that macro and turns the silent drop into an explicit refusal.

Driver version is now `0x0300001A`. Regression coverage is the existing suite in
`indigo_test/integration/test_aux_upb_simulator.c`, which was re-run after the change.

```sh
cd indigo_test && ./build/integration/test_aux_upb_simulator
```

- Simulated tests run: 4; passed: 4.
- Hardware tests run: 0; passed: 0.

## Protocol coverage build-out (2026-09-21)

The suite was a four case smoke test - heater duty cycles, one focuser move, and two
initialization failures - against a driver that exposes fifteen powerbox properties and nine
focuser properties over two logical devices. It is now a protocol suite of 36 cases covering both
box generations.

Simulator (`aux_upb_simulator.c`) extensions, all audited against
`Ultimate_Powerbox_Serial_Command_Table.pdf` and its v2 counterpart: configurable voltage, current,
power, weather and overcurrent fields of the `PA` frame, a persistent auto dew state, and fault
injection per command (`--fault`, `--fault-once`, `--fault-after N`) with `invalid`, `short`,
`silent` and `close` modes, so a fault can be aimed at the connect or at a later poll.

Test coverage added: property inventory and interface bits for both models, v1 versus v2 outlet
inventory, sensor seeding from `PA` and the counters from `PC`, per outlet power switching with
readback, outlet state lights including overcurrent, per outlet current, heater duty cycles with
their state and current, manual and automatic dew control including the state adopted from the
device, per port USB switching on v2 and the hub switch on v1, the variable voltage outlet, outlet
renaming, the momentary reboot and save-as-default switches, reconnect, repeated disconnect,
refused shutdown while connected, unknown identity, firmware read failure, a vanished port, a
truncated status frame at connect and during polling, and for the shared focuser: property
inventory, absolute goto including a no-op, relative steps in both directions, limit clamping,
abort during motion and while idle, sync, speed/backlash/reverse round trip across a reconnect,
a refused overlapping request, initialization rollback and the shared connection surviving the
sibling.

### Defects found

| Defect | Impact | Root cause | Fix | Test |
| --- | --- | --- | --- | --- |
| UPB-01 | Connecting a v1 box crashes the process with a null pointer dereference inside libusb. | The smart hub is enumerated with `libusb_get_device_list()` on the default context, but nothing in the driver ever initialized libusb, so the default context was null. In a server process another driver happened to initialize it first, which hid the defect. | `indigo_start_usb_event_handler()` before the enumeration. | `upb1_model_reduces_the_outlet_inventory`, `usb_hub_switches_on_the_v1_box` |
| UPB-02 | A switched outlet, USB port, heater level or dew mode silently reverts and the device is commanded back to its old state. | The polling timer copies the device state into the properties without checking whether a change request has already copied the client's values and published BUSY. The queued handler then sends the stale values to the device. | `upb_adopt()` re-checks the property state before the poll adopts a writable property. | `each_power_outlet_switches_on_its_own`, `each_usb_port_switches_on_its_own`, `heaters_hold_independent_duty_cycles` |

Driver version is now `0x0300001C`.

```sh
cd indigo_test && ./build/integration/test_aux_upb_simulator
```

## Hardware acceptance (2026-09-21)

`indigo_test/hardware/test_aux_upb_hw.c` runs the AUX acceptance checklist against a physically
connected box. It switches the power outlets, so it is opt-in and never part of an automatic run;
every outlet, heater, dew mode and USB control it touches is captured at the start of the session
and restored before it disconnects.

Device: Pegasus Ultimate Powerbox v1, firmware 1.4, on `/dev/cu.usbserial-PA36T4RB`, macOS arm64.
Reported 12.3 V and 0.10 A. Run with all four outlets free, as confirmed by the operator.

```sh
cd indigo_test && UPB_HW_PORT=/dev/cu.usbserial-PA36T4RB make test-aux-upb-hw
```

Covered: identity and the model dependent inventory, the property contract, the sensor readings and
their polling, all four power outlets switched and restored with readback across a status poll, the
outlet state light and current, both heater outlets, dew control, the USB hub control, outlet
renaming, the shared focuser logical device with the powerbox connection surviving its close,
reconnect and driver reinitialization.

The hardware run fed one observation back into the simulator: a box with no environmental probe
reports zeros for the whole weather group, which the simulator could not produce and the suite
therefore never covered. `--no-probe` and `--outlet-current` were added and
`a_box_without_a_probe_reports_zero_weather` now covers it, bringing the suite to 37 cases.

The simulator still cannot reproduce one thing the hardware showed: the v1 box publishes
`AUX_USB_PORT` because it contains an internal Microchip hub that the driver finds over libusb.
That hub is a USB device rather than a serial one, so the simulator's v1 mode has no way to present
it and that path stays hardware-only.

Not covered on this box: the environmental probe reads 0 C and 0 %RH because none is attached, so
the weather values are only checked for plausibility rather than against a reference; the variable
voltage outlet and the per port USB switching are v2 features this box does not have; the focuser
has no motor attached, so only its contract, position readback and an idle abort were exercised, not
motion; physical hot-plug was not part of the run.

## v1 smart hub port change lost to the poll (2026-09-25)

### Observation

On the Pegasus UPB v1.7 bench box (`/dev/cu.usbserial-PA36T4RB`, macOS arm64, `indigo_server -vv`,
2026-09-25 00:55), 11 `AUX_USB_PORT` requests for port #2 sent through `indigo_agent_alpaca`
every ~4.5 s produced only 7 `Turning port #2 on/off` log lines. The lost requests arrived while
the status poll's `PA`/`PC` exchange was in progress (request 00:55:53.645, poll `PA` at
00:55:53.462), and the value read back afterwards contradicted the request. ASCOM ConformU reported
the same on the USB port switches as "GetSwitch returned True after SetSwitch(False)" and "Set/Read
differ by 90-100% of SwitchStep".

### Audit

`aux_timer_callback` reads each downstream port of the internal Microchip USB2517 hub
(`0x0424:0x2517`) with `LIBUSB_REQUEST_GET_STATUS` and assigns `AUX_USB_PORT` from it without
checking the property state, while the v2 branch of the same poll guards the assignment with
`upb_adopt()` (UPB-02). `INDIGO_COPY_VALUES_PROCESS_CHANGE` copies the requested value and
publishes BUSY on the client thread; the poll then overwrites it with the old hub state, and
`aux_usb_port_handler`, which only sends `SET_FEATURE`/`CLEAR_FEATURE(PORT_POWER)` for ports whose
requested value differs from the hub, sends nothing and publishes OK with the old value.

A second, related problem in the same block: one flag, `updateUSBPorts`, publishes both
`AUX_USB_PORT` and `AUX_USB_PORT_STATE` in OK. A port status light that changes while a request is
pending would therefore publish `AUX_USB_PORT` OK before its handler has run.

`AUX_USB_PORT_STATE` is a read-only light the handler never reads, so it keeps following the hub
while a request is pending; only the writable `AUX_USB_PORT` is guarded.

Why the earlier suite missed it: the v1 smart hub is a USB device, which the serial simulator
cannot present, so the v1 cases ran against whatever libusb found on the host. On this Mac that
was the real UPB hub - the v1 simulator cases opened it and read its ports.

Measured hub replies used to model it (read-only `GET_STATUS`, 2026-09-25, this box): ports 1-6
`0x00000100` (powered, nothing attached), port 7 `0x00000103` (the box's own serial bridge).

### Decisions

- Hardware testing: yes, on the same UPB v1.7. Only `AUX_USB_PORT` port #2 is switched; nothing is
  attached to any downstream hub port (all six read `0x0100`), and no power outlet is switched. The
  ASI294MC Pro powered from an outlet is not affected.
- The v1 path is modelled hardware-free with a fake smart hub in the test binary, using the same
  compile-time libusb replacement as `test_ccd_ssag_usb`, so the existing suite stops depending on
  the host's USB bus.

### Plan and results

1. **Done.** Fake smart hub in `indigo_test/integration/test_aux_upb_simulator.c`. The suite now
   links `build/integration/aux_upb_smart_hub_driver.o`, the driver source compiled with
   `libusb_get_device_list`, `libusb_free_device_list`, `libusb_get_device_descriptor`,
   `libusb_open`, `libusb_close` and `libusb_control_transfer` renamed to the fake
   (`AUX_UPB_SMART_HUB_REPLACEMENTS` in `indigo_test/Makefile`). The hub answers `GET_STATUS` with
   the measured words, applies `SET_FEATURE`/`CLEAR_FEATURE(PORT_POWER)`, counts them per port and
   can park the next status read until the test releases it. It is absent by default, so the
   existing v1 cases no longer reach the host's USB bus. Three cases were added:
   `v1_without_a_smart_hub_hides_the_usb_ports`, `v1_usb_ports_switch_through_the_smart_hub`
   (each port switched with exactly one request, the others untouched, the connection, overcurrent
   and idle lights from the status word, readback after a later poll) and the reproducer below.
2. **Done, failed as expected.** `v1_usb_port_change_survives_a_concurrent_poll` parks the poll
   in its first port read, switches port #2 off, changes port #1's status meanwhile and releases the
   poll. Against the original driver (version 28): `test_aux_upb_simulator.c:694: expected 1,
   got 0`, meaning no `CLEAR_FEATURE` reached the hub. The other 39 cases passed except
   `focuser_overlapping_request_is_rejected`; see "Unrelated intermittent failure" below.
3. **Done, failed as expected.** Hardware reproducer `upb_usb_port_changes_survive_the_poll` in
   `indigo_test/hardware/test_aux_upb_hw.c`: 16 requests on port #2, spaced 0.9-3.1 s apart so they
   step through the poll period. Each one must be published with the requested value and still
   hold before the next request. Original driver, 2026-09-25 12:12:
   `16 requests, 2 lost, 2 reverted` (requests 1 and 16 published with the old value, only 14
   `Turning port #2` lines), FAIL. The session restore now skips outlets and USB ports that are
   already in their initial state, and it restores the USB ports too. The only commands this
   filtered run sent besides the port requests were the unchanged heater (`P5:0`, `P6:0`), dew
   (`PD:0`) and hub (`PU:1`) values. No power outlet was switched.
4. **Done.** `indigo_aux_upb.driver`: in the v1 hub loop the `AUX_USB_PORT` assignment is guarded
   with `upb_adopt(AUX_USB_PORT_PROPERTY)`, the same guard the v2 branch uses. The light is assigned
   separately and published through its own `updateUSBPortState` flag, so a light change no
   longer publishes the pending `AUX_USB_PORT` as OK. v2 publishes as before, because
   `updateUSBPorts` still publishes both. Version 29 (`0x0300001D`). Regenerated with
   `../../build/bin/indigo_generator indigo_aux_upb.driver`. A second regeneration in a clean
   directory is byte-identical for `.c`, `.h` and `_main.c`. The generator's five
   `FOCUSER_*->hidden set to false` notices are pre-existing (the version 28 `.driver` prints the
   same five) and do not change the output. `make -f ../../Makefile.drv all` produces no compiler
   warnings, and `-Wall -Wextra` on the driver and both test sources is clean.
5. **Done.** Results below.

6. **Done.** Merged `origin/refactoring`. Its generator change TOOLS-014 (commit `d19397b8d`,
   which resets BUSY properties on disconnect) had also taken version 29 for this driver, so the
   combined driver is version 30 (`0x0300001E`). Regenerating with the rebuilt generator reproduces
   the merged `indigo_aux_upb.c` exactly except for `DRIVER_VERSION`. The results recorded in the
   README are the runs on version 30: simulator 40/40, and hardware `16 requests, 0 lost, 0 reverted`.

### Defect

| Defect | Impact | Root cause | Fix | Test |
| --- | --- | --- | --- | --- |
| UPB-03 | On a v1 box a USB port request that arrives while the status poll runs is lost: the driver publishes OK with the old value and never switches the port (4 of 11 via ConformU, 2 of 16 in the hardware reproducer). | The v1 smart hub branch of `aux_timer_callback` assigned `AUX_USB_PORT` from `GET_STATUS` without the UPB-02 guard, and one flag published both the switch and its light in OK. | `upb_adopt(AUX_USB_PORT_PROPERTY)` before the assignment, and a separate publish flag for `AUX_USB_PORT_STATE`. | `v1_usb_port_change_survives_a_concurrent_poll` (simulator + fake hub), `upb_usb_port_changes_survive_the_poll` (hardware) |

Reproduced both hardware-free and on hardware. The hardware observation (ConformU and the
`indigo_server -vv` log of 2026-09-25 00:55) is what the fake hub's held status read models.

### Verification

```sh
make -C indigo_drivers/aux_upb -f ../../Makefile.drv all
cd indigo_test && make build/integration/test_aux_upb_simulator && ./build/integration/test_aux_upb_simulator
cd indigo_test && make build/hardware/test_aux_upb_hw && UPB_HW_PORT=/dev/cu.usbserial-PA36T4RB INDIGO_TEST_CASE_FILTER=upb_usb_port_changes ./build/hardware/test_aux_upb_hw --run
```

- Simulator suite, fixed driver, macOS arm64: 40/40 passed in each of three consecutive full runs.
- Hardware, fixed driver, Pegasus UPB v1 (USB product "UPB v1.7", firmware 1.4) on
  `/dev/cu.usbserial-PA36T4RB`, 2026-09-25 12:13:
  `16 requests, 0 lost, 0 reverted`, 16 `Turning port #2` lines, PASS. Hub ports 1-6 read
  `0x0100` again afterwards, and the ASI294MC Pro was still enumerated on USB.
- Only the new scenario was run on hardware (`INDIGO_TEST_CASE_FILTER`). The other 14 hardware
  cases switch every power outlet, and one outlet powers the ASI294MC Pro, so they were not repeated.

### Unrelated intermittent failure

`focuser_overlapping_request_is_rejected` failed once in the first full run ("Refused
FOCUSER_STEPS was never published in ALERT, state is 1"), on a binary whose driver was still the
unmodified version 28. It passed 4/4 in isolation both with the original binary (driver archive,
real libusb) and with the fake-hub binary, and it passed in two full runs of the original binary
and all three full runs after the fix. Under ASan + UBSan it fails far more often: 1 of 40 in the
full run, 2 of 3 filtered runs with the fixed driver, and 3 of 3 filtered runs with the **original**
driver and the **original** test source. So it is a pre-existing timing dependency in that case or in
the focuser reject path, not something UPB-03 or the fake hub introduced. It concerns the focuser's
motion timing, not the USB path, and is not addressed here.

## Switch requests read from their targets (2026-09-27, 3.0.0.32)

Findings TGT-038 to TGT-042 and TGT-D02 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`.

### Defects

- TGT-038 to TGT-042: the status poll checks AUX_POWER_OUTLET, AUX_USB_PORT (v2 status frame and v1
  smart hub loop), AUX_DEW_CONTROL and X_AUX_HUB for BUSY with `upb_adopt()` and then writes the state
  the box reports into the values. A request copied on the bus thread between that check and the write
  was overwritten, and the handler, which read `sw.value`, sent the box its current state and reported
  OK. The handlers also ignored a missing reply and published the request as OK.
- TGT-D02: an X_AUX_HUB change adopted by the v1 poll set `updateAutoHeater` instead of `updateHub`, so
  the hub change was never published and AUX_DEW_CONTROL was published OK instead.

### Fix

- The poll records the outlet, USB port, hub and dew states the box (or the v1 smart hub) reports in
  private data on every poll and adopts them into the values only while the property is not BUSY, as
  before. It sets `updateHub` for a hub change and publishes AUX_DEW_CONTROL, X_AUX_HUB and AUX_USB_PORT
  only when they are still not BUSY at the end of the poll.
- AUX_DEW_CONTROL and X_AUX_HUB (one-of-many) send the target read with `indigo_get_switch_target()`,
  apply it with `indigo_apply_switch_targets()` once the box answered and otherwise show the state last
  reported with ALERT.
- AUX_POWER_OUTLET and AUX_USB_PORT (any-of-many) copy the values into the targets in
  `on_change_request` while not BUSY, because the poll writes values only and the items a request does
  not carry would otherwise keep a stale target (the mount_lx200 TGT-025 pattern). The handler sends
  each target in the original order (`P1`..`P4`, `U1`..`U6`, or the v1 hub ports 1..6 compared with the
  status read before switching) and writes an accepted one into the value; an item the box did not
  answer for, or a hub port that could not be read or switched and the ports after it, shows the state
  last reported with ALERT.
- The simulator gets `--hub-off` (a box whose hub was left disabled); the fake smart hub in the test can
  refuse port power requests.

### Verification (Linux x64)

- Unchanged 3.0.0.31 suite: 40/40 passed before the change.
- The check-to-write windows have no I/O or log line inside them, so no permanent case can hit them.
  A temporary instrumented copy of the generated driver (debug line right after each poll check,
  request sent from that log line by a temporary case; neither committed) lost every request on
  3.0.0.31 in 3/3 runs: OUTLET_1 OFF published ON and the outlet stayed powered, v2 PORT_2 OFF published
  ON, no CLEAR_FEATURE for v1 hub port 2, `PD:0` sent for AUTOMATIC, `PU:1` sent for DISABLED. On
  3.0.0.32 every request was sent and published as requested in 3/3 runs, and after the end-of-poll
  checks the first OK after each request was the handler's.
- New cases, each failing on 3.0.0.31 and passing on 3.0.0.32:
  `v1_hub_change_reported_by_the_box_is_published` (TGT-D02),
  `power_outlet_failure_shows_the_box_state` (TGT-038, also a request for another outlet after the
  failure does not resend the refused one), `usb_port_failure_shows_the_box_state` (TGT-039),
  `v1_usb_port_failure_shows_the_hub_state` (TGT-040), `dew_control_failure_shows_the_box_state`
  (TGT-041), `v1_hub_failure_shows_the_box_state` (TGT-042).
- Regeneration from the `.driver` is reproducible. Recorded run through `tools/run_driver_test.py
  aux_upb`: 46/46.
- Not verified on hardware.

### Left open

- TGT-D15: the poll never sets `updatePowerOutlet`, so an outlet change the driver did not make is
  adopted into AUX_POWER_OUTLET but not published (AUX_POWER_OUTLET_STATE does follow it). Found by
  reading, not reproducible with the simulator, not changed here. Fixed in 3.0.0.33, see below.

## Outlet changes reported by the box are published (2026-09-27, 3.0.0.33)

Finding TGT-D15 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`.

### Defect

The poll adopts the outlet states the box reports into AUX_POWER_OUTLET while it is not BUSY, but set
only `updatePowerOutletState`; `updatePowerOutlet` was declared and never set. An outlet that changed
without a request from the driver (a late or repeated request, another tool on the port before the
driver) was written into the values but never published, so clients kept seeing the old switch state
while AUX_POWER_OUTLET_STATE followed the box. Reproduced with the simulator.

### Fix

- The adoption loop also sets `updatePowerOutlet` for a changed outlet. The poll publishes
  AUX_POWER_OUTLET OK only when it is still not BUSY at the end of the poll (`upb_adopt()` re-checked
  right before the write, like AUX_DEW_CONTROL, X_AUX_HUB and AUX_USB_PORT), so a request copied after
  the outlet check keeps its values, targets and BUSY state for its handler. The poll still writes no
  targets; `on_change_request` copies the values into them while not BUSY, as before. No serial command
  changed.
- The simulator gets the test control `--outlet-after N OUTLET 0|1`: from the status frame after the
  first N `PA` replies the outlet reports the given state without any command.

### Verification (Linux x64)

- New case `power_outlet_change_reported_by_the_box_is_published` (outlet 2 switched off by the
  simulator after the connect and the first poll; the state light goes IDLE, then the switch must be
  published off with the property OK and outlet 1 still on). 3.0.0.32 failed 4/4 (the light went IDLE,
  `OUTLET_2` stayed on), 3.0.0.33 passed 3/3 in isolation.
- Regeneration from the `.driver` is reproducible. Recorded run through `tools/run_driver_test.py
  aux_upb`: 47/47.
- Not verified on hardware.

## One dew control mode when the connect status fails, TGT-D22 (2026-09-27, 3.0.0.34)

Finding TGT-D22 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`.

### Defect

Both items of the one-of-many AUX_DEW_CONTROL were initialised ON. The dew control mode is read only
from the `Autodew` field of the `PA` status frame. A truncated frame refuses the connection, but a `PA`
that is not answered at all (or does not start with `UPB`) skips the whole parse and the connect still
succeeds, so the property was defined with MANUAL and AUTOMATIC both on. The poll compares only the
AUTOMATIC item with the reported mode, so on a box in automatic dew mode nothing changed and both items
stayed on for the whole session. Reproduced with the simulator.

### Fix

AUTOMATIC is initialised OFF, so the property starts with MANUAL only. MANUAL is the mode of a box whose
automatic dew control has not been switched on (`PD:0`, the simulator's default), and it matches the
zero-initialised `automatic_dew` that a failed dew control request falls back to. The adoption at connect
and in the poll is unchanged; a poll that reports automatic mode now switches the property over. Regenerated
with the unchanged generator; the only generated changes are that item and the version. Regeneration
reproducible.

### Verification (Linux x64)

- New case `dew_control_has_one_mode_when_the_connect_status_fails` starts the simulator with the existing
  `--autodew --fault-once PA silent`, checks that exactly one item is on after the connect, and that the
  poll then adopts AUTOMATIC with MANUAL off and the property OK. 3.0.0.33 failed 3/3 (MANUAL and AUTOMATIC
  both on), 3.0.0.34 passed 3/3 in isolation.
- Recorded run through `TZ=Europe/Bratislava python3 tools/run_driver_test.py aux_upb`:
  2026-09-27 15:53 3.0.0.34 linux x64 simulator 48/48 OK.
- Not verified on hardware.
- Noticed, not changed: a connect whose `PA` goes unanswered also leaves the outlet, USB port and heater
  values at their initial values until the first poll, and it still sends `PU:1`; the build prints the
  pre-existing `-Wformat-truncation` warnings for the outlet labels copied from the outlet names.

## Heater publication and variable voltage in the poll (TGT-D28, 3.0.0.35, 2026-09-27)

- **Defects:** (a) the poll adopted the heaters from the status frame only while AUX_HEATER_OUTLET was
  not BUSY, but published it OK without that check after the `PC` round trip, so a heater request copied
  in between was shown OK before its handler sent it (display only, the handler sends the right values).
  (b) X_AUX_VARIABLE_POWER_OUTLET was read from `PS` at connect only, so a voltage changed outside the
  driver, or by a request the box did not execute, was never shown (the aux_upb3 TGT-D03 case).
- **Fix:** AUX_HEATER_OUTLET is published only while it is still not BUSY, like the other switches. On a
  v2 box the poll also sends `PS` and adopts the variable voltage into the value (not the target) when
  the property is not BUSY, publishing it the same way. The poll sends one more command every 2 s.
- **Simulator:** `--outlet-after N OUTLET VALUE` now covers the heaters (5-7, raw 0-255) and the variable
  voltage (8); `--slow-file PATH OUTLET VALUE` changes an outlet in the status frame read while PATH
  exists and answers the `PC` of that poll 0.5 s late; `--command-log PATH` appends every command.
- **Regression tests:** `variable_power_outlet_change_reported_by_the_box_is_published` (fails on
  version 34: the voltage stayed at 12 V) and `heater_request_survives_the_poll_publish` (version 34
  published OK before the box got `P6:127`). The poll's early OK is not visible as a second update,
  because the handler publishes the same values and the bus suppresses the repeat; the test checks the
  command log when the first OK arrives.
- **Noticed, not changed (TGT-D29):** `focuser_overlapping_request_is_rejected` fails intermittently
  with version 34 as well (2 of 3 runs; 2 of 9 with version 35): the focuser poll sets FOCUSER_POSITION and FOCUSER_STEPS OK
  when `SI` reports no motion, also while a position request is queued and not yet sent, so the
  following FOCUSER_STEPS request is accepted instead of refused.

## Final test summary

- Simulated tests run: 50; passed: 50 on macOS arm64 with version 35, except the intermittent
  `focuser_overlapping_request_is_rejected` (TGT-D29, also failing with version 34).
- Earlier: 48 run, 48 passed (recorded run of `test_aux_upb_simulator` through
  `tools/run_driver_test.py`, driver version 34, Linux x64). The previous macOS arm64 runs of the 40
  earlier cases and the ASan + UBSan note above refer to driver version 30.
- Hardware tests run: 1; passed: 1 (`upb_usb_port_changes_survive_the_poll`, Pegasus UPB v1 firmware 1.4,
  driver version 30). The last full hardware run, 14 run and 14 passed, was on driver version 28 and
  did not include this case. Versions 32 to 34 were not run on hardware.
