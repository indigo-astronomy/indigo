# INDIGO 3.0 refactoring record for `aux_upb3`

The driver's migration to `indigo_generator` predates this file and is deliberately not
reconstructed here. Only the work below is recorded, so nothing in this file is inferred history.

## Protocol coverage build-out (2026-09-21)

The suite was two smoke cases - one per logical device - against a driver that exposes nine power
outlets, three heaters, eight USB ports, adjustable buck and boost outputs, a relay, the weather
and consumption sensors and a focuser, all over one shared serial connection. It is now a protocol
suite of 29 cases.

The simulator already implemented the command set faithfully, so it only gained the test hooks the
suite needs: fault injection per command (`--fault`, `--fault-once`, `--fault-after N`) with
`invalid`, `short`, `silent` and `close` modes, and start-up states for the outlets, the USB ports
and automatic dew control.

Test coverage added: the property inventory and interface bits, the reported firmware, the outlet
and USB state adopted at connect for both a box left on and one left off, per outlet switching
across all nine outputs with readback, per port USB switching, independent heater duty cycles, dew
control, the adjustable outputs, outlet renaming, the momentary save-as-default and reboot
switches, the weather and consumption sensors, reconnect, repeated disconnect, refused shutdown
while connected, unknown and silent identity, a vanished port, a status fault while polling,
transport loss, and for the shared focuser: property inventory, absolute goto, relative steps in
both directions, abort during motion, sync, speed and reverse settings, polled temperature and the
powerbox connection surviving the focuser being closed.

### Defects found

None. Every case that failed while the suite was being written was a defect in the test rather than
in the driver; they are listed here because each one is a trap the next suite can fall into:

- A request that arrives while its property is still BUSY is dropped by the change dispatch, so a
  case that switches nine outlets in a row has to let each one settle first.
- A relative move is refused while `FOCUSER_POSITION` is BUSY and dropped while `FOCUSER_STEPS` is,
  so both have to settle before one is requested.
- The position follows the device through the status poll, so it needs a bounded wait rather than
  an immediate sample after the motion property reports completion.

```sh
cd indigo_test && ./build/integration/test_aux_upb3_simulator
```

- Simulated tests run: 29; passed: 29.
- Hardware tests run: 0; passed: 0. No Ultimate Powerbox v3 was available.
