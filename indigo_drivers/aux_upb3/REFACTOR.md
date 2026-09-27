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

Simulated tests run: 29; passed: 29. Hardware tests run: 0; passed: 0.

## Dew control adoption at connect, TGT-D03 (3.0.0.8, 2026-09-27)

Finding TGT-D03 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`, on branch `refactoring_targets`.

- **Defect (reproduced):** the `PD` branch of `on_connect` set MANUAL only when MANUAL was already
  shown and AUTOMATIC only when AUTOMATIC was already shown, so it never changed the switch. Both
  items start ON, so the first connect of a driver lifecycle happened to show the reported mode, but
  a later connect to a box in the other mode (dew mode changed outside the driver, or another box on
  the port) kept showing the previous mode.
- **Fix:** `on_connect` selects the item the `PD` reply reports (`PD:000` MANUAL, otherwise
  AUTOMATIC) without looking at the item shown.
- **Regression test:** `dew_state_is_adopted_at_connect` connects to a box started with `--autodew`,
  then to one in manual mode and back, and checks the switch each time; it failed on 3.0.0.7 (MANUAL
  stayed off after the connect to the manual box) and passes on 3.0.0.8.
- **Not changed, the poll part of TGT-D03:** the poll sends `IS` (outlet currents and overcurrent
  flags), `ES`, `VR` and `PC`; none of these replies carries the outlet, USB or dew switch states,
  which only `PA`, `AJ`, `UA` and `PD` report. The poll therefore cannot refresh those switches
  without querying more commands per poll, which changes the protocol traffic and was deliberately
  not done here; a change made outside the driver is shown only after the next connect. Because the
  poll never writes these switches, no queued request can be overwritten by it and the switch target
  pattern is not needed for them.
- **Left open:** both AUX_DEW_CONTROL items are initialised ON in a one-of-many property, so when
  the `PD` query fails at connect the property is defined with both items on (TGT-D18).

Recorded run: `TZ=Europe/Bratislava python3 tools/run_driver_test.py aux_upb3`, 30/30 on linux x64.

## Final test summary

- Simulated tests run: 30; passed: 30.
- Hardware tests run: 0; passed: 0. No Ultimate Powerbox v3 was available.
