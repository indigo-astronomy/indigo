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

## Final test summary

- Simulated tests run: 40; passed: 40 (latest full run of `test_aux_upb_simulator`, driver version
  29). Sanitizer run (ASan + UBSan, arm64, leak detection off) of the same 40 cases: 40
  run, 39 passed, no sanitizer report; the one failure is the pre-existing
  `focuser_overlapping_request_is_rejected` timing failure described above.
- Hardware tests run: 1; passed: 1 (`upb_usb_port_changes_survive_the_poll`, Pegasus UPB v1 firmware 1.4,
  driver version 29). The last full hardware run, 14 run and 14 passed, was on driver version 28 and
  did not include this case.
