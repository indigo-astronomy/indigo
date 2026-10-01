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
  the `PD` query fails at connect the property is defined with both items on (TGT-D18, fixed in
  3.0.0.9, see below).

Recorded run: `TZ=Europe/Bratislava python3 tools/run_driver_test.py aux_upb3`, 30/30 on linux x64.

## One dew control mode when `PD` fails, TGT-D18 (3.0.0.9, 2026-09-27)

Finding TGT-D18 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`, on branch `refactoring_targets`.

- **Defect (reproduced):** both items of the one-of-many AUX_DEW_CONTROL were initialised ON. When
  the `PD` query failed at connect nothing was adopted, so the property was defined with MANUAL and
  AUTOMATIC both on.
- **Fix:** AUTOMATIC is initialised OFF, so the property starts with MANUAL only. MANUAL is the mode
  of a box whose automatic dew control has not been switched on (`PD:000`, also the simulator's
  default). The `on_connect` adoption of TGT-D03 is unchanged, and on a later connect whose `PD`
  fails the mode shown before stays, still exactly one item. Regenerated with the unchanged
  generator; the only generated changes are that item and the version. Regeneration reproducible.
- **Regression test:** `dew_control_has_one_mode_when_pd_fails` connects to a simulator started
  with the existing `--fault PD silent` and checks that exactly one item, MANUAL, is on. 3.0.0.8
  failed 3/3 (both items on), 3.0.0.9 passed 3/3; `dew_state_is_adopted_at_connect` still passes.
- **Noticed, not changed:** any `PD` reply other than `PD:000`, including a malformed one, selects
  AUTOMATIC; the build still prints the pre-existing `-Wformat-truncation` warnings for the outlet
  labels copied from the outlet names.

Recorded run: `TZ=Europe/Bratislava python3 tools/run_driver_test.py aux_upb3`,
2026-09-27 14:57 3.0.0.9 linux x64 simulator 31/31 OK.

## TGT-D03 poll part: the switches follow the box (3.0.0.10, 2026-09-27)

- **Defect:** the poll read only `IS`, `ES`, `VR` and `PC`, which carry no outlet, USB or dew
  control state, so AUX_POWER_OUTLET, AUX_HEATER_OUTLET, AUX_VARIABLE_POWER_OUTLET, AUX_USB_PORT
  and AUX_DEW_CONTROL kept the state read at connect. A change made outside the driver, or a
  request the box did not execute, was never shown.
- **Fix:** the connect and the poll share `upb3_read_power()` (`PA`), `upb3_read_adjustable()`
  (`AJ`), `upb3_read_usb()` (`UA`) and `upb3_read_dew()` (`PD`). Each adopts the reported state
  only into a property that is not BUSY, checked right before the write with no I/O in between,
  and the poll publishes a changed property only if it is still not BUSY, so a request copied
  while a reply is read keeps its values and its handler sends it. An outlet switched by the box
  also updates its state light unless that shows an overcurrent. The poll writes no number
  targets; the connect still sets the voltage targets as before. A `PD` reply that does not start
  with `PD:` is now ignored instead of selecting AUTOMATIC. The poll sends four more commands every
  2 s.
- **Simulator:** `--external-after N` changes outlet 2, the relay, heater 1, the buck voltage, USB
  port 2 and the dew control from the N+1st `PA` on without a command; `--slow-file PATH` sends the
  reply of the next command named in PATH, composed at once, 0.5 s late.
- **Regression tests:** `switches_follow_the_box_while_polling` (fails on 3.0.0.9: outlet 2 stayed
  on) and `usb_request_survives_poll_read` (a USB port request copied while the poll waits for the
  late `UA` reply stays off; with the BUSY check removed it fails, the port is switched back on).

## Simulator: focuser speed option (2026-10-01)

No driver change, version stays 3.0.0.10. ASCOM ConformU, run against `indigo_agent_alpaca` with this driver on `aux_upb3_simulator`, moves an absolute focuser by a tenth of its range and allows 60 s for the move. The driver offers 0 to 9 999 999 steps and the simulator moved one step per millisecond, so the move of 999 999 steps could not finish and 1 of the 28 focuser tests failed (`../agent_alpaca/REFACTOR.md`, section 11.4).

The simulator has a new option `--focuser-rate <steps>`, the number of focuser steps per millisecond tick. Its default of 1 keeps the motion every test of this driver relies on, including the abort in the middle of a 20 000 step move; the ConformU harness starts the simulator with 500. With it the ConformU focuser test passes 28/28.

## Final test summary

- Simulated tests run: 33; passed: 33.
- Hardware tests run: 0; passed: 0. No Ultimate Powerbox v3 was available.
