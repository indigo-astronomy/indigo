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

Not covered on this box: the environmental probe reads 0 C and 0 %RH because none is attached, so
the weather values are only checked for plausibility rather than against a reference; the variable
voltage outlet and the per port USB switching are v2 features this box does not have; the focuser
has no motor attached, so only its contract, position readback and an idle abort were exercised, not
motion; physical hot-plug was not part of the run.

## Final test summary

- Simulated tests run: 36; passed: 36. Sanitizer run (ASan + UBSan, arm64): 36 run, 36 passed.
- Hardware tests run: 14; passed: 14. Pegasus Ultimate Powerbox v1, firmware 1.4.
