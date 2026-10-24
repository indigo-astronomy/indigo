# INDIGO 3.0 refactoring record for `focuser_primaluce`

This record was created together with the 2026-09-18 `reject_change` work. The driver's earlier
migration to `indigo_generator` predates this file and is deliberately not reconstructed here; only
the changes below are recorded, so nothing in this file is inferred history.

## Rejected-change regression coverage (2026-09-18)

A change request that arrives while a motion is already running is now refused with the generator's
`reject_change` block on `FOCUSER_POSITION` and `FOCUSER_STEPS`, with the condition that the other motion property is BUSY and the message
shown in the generated driver. The generated guard marks every item for update, sets
`INDIGO_ALERT_STATE` and publishes the property, so the client receives the actual driver-side values
instead of an update that carries no items.

Both motion properties are published BUSY together at motion start, so `INDIGO_COPY_*_PROCESS_CHANGE` dropped a concurrent request silently: the client received no update at all and kept showing the refused target. The guard runs before that macro and turns the silent drop into an explicit refusal.

## Broken rotator control (2026-09-20)

Writing the rotator part of the test suite showed that the ARCO branch never worked. Four separate
defects were found and fixed; driver version is now `0x0300000C`.

- `ROTATOR_POSITION.on_change` sent `FOCUSER_POSITION_ITEM->number.target`. On a rotator device
  `FOCUSER_CONTEXT` casts the `indigo_rotator_context` to an `indigo_focuser_context`, so that macro
  actually reads the fifth pointer of the rotator context, which is `ROTATOR_ABORT_MOTION`, and then
  reads a switch item through the number member of the union. A request for 45 degrees put
  `"DEG":0` on the wire. The handler now sends `ROTATOR_POSITION_ITEM->number.target`.
- The same handler published `X_CALIBRATE_A` and `FOCUSER_POSITION` from its two error paths instead
  of `ROTATOR_POSITION`, so a refused move never reached the client and an unrelated property was
  republished. Both paths now publish `ROTATOR_POSITION`.
- `rotator_movement_finalizer` read the motor state with `CMD_MOT2_STEP`, a `res/cmd` path, from the
  reply to a `res/get` request. It could never see `stop`, so every rotator move rescheduled the
  finalizer immediately and the driver queried the controller in a tight loop for as long as the
  device stayed connected. It now reads `res/get/MOT2/STATUS/MST`, tolerates a missing field,
  repolls with a 0.2 s delay and reports a failed query as ALERT instead of leaving the property BUSY.
- `GET_MOT2_ERROR` pointed at `MOT1/ERROR`, so a rotator fault was never reported and a focuser fault
  was reported twice.

`X_CALIBRATE_A` additionally stayed BUSY forever: its `on_change` referenced a `_finalizer`, which
makes the generator suppress the final update, and the shared movement finalizer only completes
`ROTATOR_POSITION`. The controller runs the calibration on its own and only acknowledges the
request, so the handler now publishes the acknowledgement as the completion.

## Hardware validation plan (2026-09-20)

Hardware testing **is** performed for this round. The device is a **PrimaluceLab SESTO SENSO 2**,
serial number `SESTOSENSO22522`, application firmware `03.05.08`, web firmware `03.03.00`, reached
over a CP2102N USB-to-UART bridge on `/dev/cu.usbserial-11130`. No ARCO rotator is attached
(`ARCO = 0`, `MOT2.SUBMODEL = "Not enabled"`), so rotator motion cannot be validated on hardware.

Controller state captured from `{"req":{"get":""}}` before any driver change, used as the baseline
for the scenarios below:

- `MOT1.ABS_POS_STEP = 1000`, `MOT1.CAL_MINPOS = 0`, `MOT1.CAL_MAXPOS = 40973`, `MOT1.BKLASH = 0`,
  `MOT1.HOLDCURR_STATUS = 1`, `MOT1.ERROR = ""`, `CALRESTART.MOT1 = 0`.
- `MOT1.FnRUN_ACC/DEC/SPD = 1/1/2`, `FnRUN_CURR_ACC/DEC/SPD/HOLD = 7/7/7/1`, i.e. the `slow` preset.
- The full `get` dump contains **no** `MOT1.SPEED` and **no** `MOT1.ABS_POS`, so the driver must hide
  `FOCUSER_SPEED` and must use the `ABS_POS_STEP` code path on this unit. A targeted
  `{"req":{"get":{"MOT1":{"SPEED":""}}}}` does answer `0`, so only the aggregate dump omits it.
- `EXT_T = -127.00` (no external probe fitted), `VIN_12V = 13.18`, `MOT1.NTC_T = 31.67`,
  `DIMLEDS = "low"`, `WIFIAP.STATUS = "on"`, `WIFIAP.SSID = "SESTOSENSO22522"`.

### Planned hardware scenarios

The focuser draw tube physically moves. Every move is relative to the position the session found and
the session returns the focuser to that position, so no mechanical end stop is approached. The test
needs roughly 3000 steps of free outward travel; `PRIMALUCE_HW_TRAVEL` shortens it.

1. Connect, identity and capability readback.
2. Property contract, including `FOCUSER_SPEED` hidden on this unit.
3. Controller state readback and 10 s polling.
4. Absolute move, with an explicit BUSY-until-arrival check.
5. Relative move outward and inward.
6. Abort during a long move, then a fresh move.
7. Abort while idle.
8. Overlapping motion request refused by `reject_change`.
9. Backlash write and readback, original restored.
10. Hold current toggle, original restored.
11. Run preset applied and `X_CONFIG` readback, original preset restored.
12. LED mode set to each value, original restored.
13. WiFi settings readback only.
14. Rotator connect/disconnect with no ARCO fitted, `ARCO` left disabled, focuser still usable.
15. Reconnect with position preserved.
16. Driver shutdown and reinitialization.

Deliberately not run on hardware: `X_CALIBRATE`, because the documented procedure drives the draw
tube to both mechanical end stops and rewrites `CAL_MINPOS`/`CAL_MAXPOS`. It stays covered by the
simulator scenario `focuser_calibration`. `X_WIFI` mode switching is read-only on hardware because
turning the access point off would drop the unit's own network service.

## Automated test coverage (2026-09-20)

`indigo_test/integration/test_focuser_primaluce_simulator.c` was extended from two smoke tests to the
full focuser and rotator class standards in `indigo_test/DRIVER_TESTING_RULES.md`. Every scenario
runs in its own forked process against a freshly started simulator, so no state leaks between cases.

The simulator `focuser_primaluce_simulator/focuser_primaluce_simulator.c` gained the fixtures the
standard needs and nothing else:

- `--profile <normal|esatto|sestosenso3|no-abs-pos|no-speed|unsupported|old-firmware|needs-calibration|external-motion>`
  selects the controller the scenario connects to.
- `INDIGO_PRIMALUCE_EVENTS` records every accepted request, so a test can assert the JSON the driver
  actually sent instead of only the resulting property state.
- `INDIGO_PRIMALUCE_FAULT` arms a one-shot `silent`, `garbage` or `close` fault on the next request
  containing a given fragment.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Capabilities and readback | `metadata`, `property_contract`, `status_polling` |
| Model dependent capabilities | `esatto_profile`, `sestosenso3_profile`, `no_abs_pos_profile`, `no_speed_profile`, `old_firmware`, `needs_calibration` |
| Initialization failures | `unsupported_device`, `handshake_timeout` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` |
| Motion, units and sign | `absolute_move`, `relative_move`, `relative_move_clamped` |
| Stop and overlapping requests | `abort_motion`, `abort_while_idle`, `overlap_rejected` |
| Command failures | `move_command_failures`, `settings_reported_failures` |
| Externally changed position | `external_motion_observed` |
| Disconnect during motion | `disconnect_during_motion` |
| Modes and controls | `controller_settings`, `run_preset`, `focuser_calibration` |
| Transport loss | `transport_loss` |
| Rotator identity and lifecycle | `rotator_metadata`, `rotator_lifecycle` |
| Shared connection and ownership | `shared_connection` |
| Rotator motion, stop and calibration | `rotator_move`, `rotator_abort`, `rotator_calibration`, `rotator_move_failure` |

### Notes and gaps

- `ROTATOR_ON_POSITION_SET` is published since the sync support (see "Controller features").
- `FOCUSER_MODE`, `FOCUSER_COMPENSATION` and `FOCUSER_REVERSE_MOTION` stay hidden; the controller has
  no temperature compensation and no reverse switch of its own.
- `FOCUSER_STEPS.on_change` contained the statement
  `FOCUSER_POSITION_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state;`, which does nothing. The
  hardware round below showed what it was meant to be and replaced it; see defect D1.
- The focuser movement finalizer rescheduled itself with `indigo_execute_handler`, so a focuser move
  was polled without any delay for as long as it ran. The hardware round below gave it the same
  0.2 s spacing as the rotator finalizer.
- The driver keeps no persistent settings of its own, so there is no `CONFIG` roundtrip to test.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run in that round; it is covered by the
  hardware round below.

## Hardware validation results (2026-09-20)

`indigo_test/hardware/test_focuser_primaluce_hw.c` implements the focuser hardware acceptance
checklist against the physical controller. It is opt-in and excluded from `make -C indigo_test test`.

```sh
make -C indigo_test build/hardware/test_focuser_primaluce_hw
cd indigo_test && PRIMALUCE_HW_PORT=/dev/cu.usbserial-11130 ./build/hardware/test_focuser_primaluce_hw --run
```

Every scenario works relative to the position the session finds and the session restores that
position, the backlash, the hold current, the run configuration and the LED mode before it
disconnects. The controller was at position 1000 before and after every run.

| Planned scenario | Test case | Result |
| --- | --- | --- |
| Connect, identity and capabilities | `primaluce_reports_identity_and_capabilities` | pass |
| Property contract | `primaluce_publishes_the_property_contract` | pass, `FOCUSER_SPEED` hidden as expected |
| Controller state readback | `primaluce_reads_controller_state` | pass, `X_STATE` has no USB rail item on this model |
| Status polling | `primaluce_polls_controller_state` | pass |
| Absolute move, busy until arrival | `primaluce_moves_to_an_absolute_position` | pass |
| Relative move both directions | `primaluce_moves_by_relative_steps` | pass |
| Overlapping request refused | `primaluce_refuses_an_overlapping_request` | pass, all four combinations |
| Abort and fresh move | `primaluce_aborts_a_move_and_recovers` | pass, stopped at 1620 of a 1000 to 4000 move |
| Abort while idle | `primaluce_accepts_an_abort_while_idle` | pass |
| Backlash write and readback | `primaluce_sets_backlash` | pass, survives a reconnect |
| Hold current | `primaluce_sets_hold_current` | pass |
| Run preset and `X_CONFIG` | `primaluce_applies_a_run_preset` | pass |
| LED modes | `primaluce_sets_led_modes` | pass |
| WiFi readback | `primaluce_reads_wifi_settings` | pass |
| Focuser calibration | `primaluce_calibrates_the_focuser` | skipped, see below |
| Rotator logical device | `primaluce_connects_the_rotator` | pass, no ARCO fitted |
| Reconnect | `primaluce_reconnects` | pass |
| Driver shutdown and reinitialization | `primaluce_reinitializes` | pass |

`primaluce_calibrates_the_focuser` is gated behind `PRIMALUCE_HW_CALIBRATE=1` and was not run: the
documented procedure drives the draw tube to both mechanical end stops and rewrites
`CAL_MINPOS`/`CAL_MAXPOS`. The simulator scenario `focuser_calibration` covers the command sequence.

## Found defects

Four defects were reproduced on the SESTO SENSO 2 and three more were found by reading the code
around them. All seven are fixed; the driver version is now `0x0300000D`.

### D1 A relative move reported completion before the focuser moved (reproduced)

`FOCUSER_STEPS.on_change` handed the move to `focuser_position_handler` and left the generated
epilogue to publish the property, so `FOCUSER_STEPS` went BUSY and back to OK within milliseconds
while the draw tube was still travelling. `FOCUSER_POSITION` was never published BUSY at all,
because only the absolute-move change branch sets that state. A client driving the focuser with
relative steps saw every move finish instantly and read the position it had before the move.

The handler now publishes both motion properties as BUSY and leaves completion to
`focuser_movement_finalizer`. Covered by `primaluce_moves_by_relative_steps`, which additionally
asserts that `FOCUSER_POSITION` reports the motion, and by the simulator scenario `relative_move`.

### D2 An interrupted move published the position the focuser started from (reproduced)

`FOCUSER_ABORT_MOTION.on_change` set both motion properties to ALERT and published them before it
sent `MOT_STOP`, so the ALERT carried the position from before the move. Aborting a move from 1000
to 4000 after 0.5 s published position 1000 although the draw tube had reached about 1300.

A running motion is always owned by `focuser_movement_finalizer`, so the abort handler now only
stops the motor and lets that finalizer observe the stop, read the position the draw tube actually
reached and publish it. The two `indigo_cancel_pending_handler()` calls were removed with it: the
initiating handlers are queued on the same device queue as the abort, so they have always run by
the time the abort handler starts. `primaluce_aborts_a_move_and_recovers` asserts the published
position by reconnecting and comparing against the value the controller reports itself.

### D3 Overlapping motion requests were accepted (reproduced)

The `reject_change` guards inspected only the other motion property, which left three of the four
combinations unprotected:

- a second absolute move during a running absolute move was dropped silently by
  `INDIGO_COPY_NUMBER_PROCESS_CHANGE` with no update to the client at all;
- because of D1, `FOCUSER_POSITION` was never BUSY during a relative move, so both a relative and an
  absolute request were accepted while one was running, and two moves overlapped on the controller.

Both guards now inspect both motion properties. `primaluce_refuses_an_overlapping_request` exercises
all four combinations and asserts the refusal message each time.

### D4 A refusal disarmed the guard of a running relative move (reproduced)

After D1 was fixed, `FOCUSER_STEPS.on_change` still derived its own state from
`FOCUSER_POSITION_PROPERTY->state` after `focuser_position_handler()` returned. A refused
`FOCUSER_POSITION` request is published from the bus thread and sets that property to ALERT, so a
refusal that landed inside that window left `FOCUSER_STEPS` non-BUSY without publishing anything,
and the next relative request was accepted on top of the running move. The recorded trace shows
`FOCUSER_STEPS` published BUSY with the new target while the first move was still running.

The handler now sets both properties to BUSY itself and never reads the other property's state back.
A command the controller refuses is completed for both properties by the new `focuser_motion_failed`
helper.

### D5 Null dereference when the controller omits `MST` (source audit)

`focuser_movement_finalizer` passed the result of `get_string(device, GET_MOT1_MST)` straight to
`strcmp()`. Any reply without `MOT1.STATUS.MST`, including an error reply, crashed the driver. The
rotator finalizer already guarded this. The focuser one now treats a missing `MST` as stopped and
lets the settle loop decide between OK and ALERT.

### D6 A failed status query left the motion properties busy forever (source audit)

The finalizer published nothing and rescheduled nothing when its position query failed, and it did
the same when the extra SESTO SENSO 3 `MST` query failed, so a transport error during a move left
`FOCUSER_POSITION` and `FOCUSER_STEPS` BUSY until the device was disconnected, with the
`reject_change` guards refusing every further move. Both paths now end the operation with ALERT.

### D7 The `ABS_POS` capability flag was shared by both motors (source audit)

`PRIVATE_DATA->has_abs_pos` was written by the focuser from `MOT1` and overwritten by the rotator
from `MOT2`. On a controller whose two motors do not report the same field, connecting the rotator
silently switched the focuser to a position field its motor does not have, and the focuser then read
position 0. The rotator now has its own `rotator_has_abs_pos` flag. The attached unit reports
neither `MOT1.ABS_POS` nor `MOT2.ABS_POS`, so the defect could not be reproduced on it;
`primaluce_connects_the_rotator` exercises the sequence and confirms the focuser still moves
correctly after a rotator session.

### D8 Station mode never joined an existing network (reproduced, 2026-10-03)

`X_WIFI.STA` sent `{"req":{"cmd":{"STA_SET_STATUS":"on"}}}`, which a SESTO SENSO 2 with firmware
`SWAPP 03.05.08` answers with `{"res":{"cmd":{"ERROR":"SYS: Invalid command"}}}`.
`primaluce_command` accepted any JSON answer, so the switch reported OK while nothing changed. The
firmware selects the mode with `{"req":{"set":{"LANCFG":"sta"}}}` (`"ap"` for the access point),
which the attached unit acknowledges and reads back. The firmware applies a new `LANCFG` only
after `{"req":{"cmd":{"REBOOT":""}}}`, which it acknowledges before printing its ESP32 boot log on
the serial line. `X_WIFI_STA`, which carries the network name and password, was hidden, so a client
could not set the network to join at all. The driver now shows `X_WIFI_STA`, sets `LANCFG` and
restarts the controller when the mode changes, waits for it to answer again, reads `LANCFG` back on
connect to show `STA`, and fails a request answered with `ERROR`. The simulator answers `LANCFG` and
`REBOOT` (with a boot log) and gains a `reject` fault; `wifi_station_mode` covers all of it. On
hardware the unit joined the network after the restart and reported `LANCONN` `gotip`.

## Network connection (2026-10-03)

The controller's web server (port 80) answers the same JSON requests as
`GET /ajax.php?jreq=<URL-encoded request>`, one request per connection. It replies with
`Content-Length` but does not close the connection, so the driver ends the read on the content
length rather than on end of file. A device port of `http://host[:port]` (or `tcp://`) selects this
transport. Verified against the attached SESTO SENSO 2 in station mode: connect, readback of model,
firmware, serial number, position, temperature, voltage and WiFi mode, and a 100-step move out and
back, with each request taking about 20 ms. The full hardware suite passes over WiFi
(`--port http://<address>`, 18/18) as well as over USB (18/18). The
simulator has no HTTP transport, so the network path has hardware coverage only.

## Controller features (2026-10-03)

- Travel range: connect reads `MOT1.CAL_MINPOS` and `CAL_MAXPOS` and uses them as the range of
  `FOCUSER_POSITION` and `FOCUSER_STEPS`, and a relative move longer than the remaining travel
  stops at either calibrated end, not only at zero. Simulator `calibrated_travel`; on hardware the
  range reads 0 to 40973.
- `X_LEDS` gains `MIDDLE`, sent and read back as `DIMLEDS` `"middle"`. Simulator `led_middle`;
  verified on hardware.
- Rotator sync: `ROTATOR_ON_POSITION_SET` is published, and `SYNC` sends
  `{"req":{"cmd":{"MOT2":{"SYNC_POS":{"DEG":<angle>}}}}}`, which redefines the angle without a
  move. A refused sync reads the angle back and ends ALERT. Simulator `rotator_sync`; not verified
  on hardware because no ARCO is attached (the controller answers
  `"Error: invalid command; ARCO is not connected"`).
- Rotator angle: the controller reports the mechanical angle as `MOT2.ABS_POS_DEG` and the synced
  angle as `MOT2.POSITION_DEG` (`MOVE_ABS` targets the synced angle). The driver reads
  `POSITION_DEG` where the firmware reports it and falls back to `ABS_POS_DEG`, so a sync is
  visible in `ROTATOR_POSITION`. Simulator `rotator_sync` (move after a sync); not verified on
  hardware.
- ARCO calibration is started with `{"req":{"set":{"MOT2":{"CAL_STATUS":"exec"}}}}` (it was sent as
  `cmd` and completed at once). `X_CALIBRATE_A` now stays busy until `MOT2.CAL_STATUS` reads
  `"stop"`, and `ROTATOR_ABORT_MOTION` ends a running calibration with `CAL_STATUS` `"stop"`.
  Simulator `rotator_calibration`; not verified on hardware.
- Stored positions: SESTO SENSO keeps nine named focuser positions as `PRESET_1` to `PRESET_9`
  (`NAME`, `M1POS`). `X_PRESETS` writes a changed position with
  `{"req":{"set":{"PRESET_<n>":{"M1POS":<steps>}}}}`, `X_PRESET_NAMES` a changed name, and
  `X_PRESET_GOTO` moves to a stored position through the absolute move. The controller refuses a
  zero position ("Error: invalid command"), so a stored position cannot be cleared again; the
  driver keeps the stored value and ends ALERT. A model that reports no `PRESET_1` (ESATTO) hides
  the three properties. Simulator `stored_positions`; on hardware the readback and the write of a
  position and a name were checked directly, the hardware suite asserts the properties.
- `LOGLEVEL`: connect sends `{"req":{"cmd":{"LOGLEVEL":"no output"}}}`, so diagnostic output
  cannot interleave with replies. Verified on hardware.
- `FOCUSER_ABORT_MOTION` sends `MOT1` `MOT_ABORT`, which stops the motor at once, and falls back to
  `MOT_STOP` when the controller refuses it. Simulator `abort_motion`, `move_command_failures`;
  the hardware suite aborts a move with it.
- The response buffer grows from 4096 to 8192 bytes: the full state of the attached unit is about
  3.9 kB over USB.
- A motor without its 12 V supply answers a move or stop with `"12V_PowerSupply_Error"`; the
  driver now ends the operation ALERT with the message "The motor has no 12 V power supply".
  Simulator `motor_without_power` (fault `nopower`); not reproduced on hardware.

## Generator fix (2026-10-03)

`indigo_generator` counted braces inside string and character literals and comments, so a code
block containing a string such as `"{\"res\":{"` failed with "Failed to parse C code". The code
lexer now tracks strings, character literals, line and block comments. Regenerating all 116
`.driver` files in the tree with the fixed generator changed no output.

### Intentional behaviour deviations

- `focuser_movement_finalizer` now reschedules itself with a 0.2 s delay instead of immediately, the
  same spacing the rotator finalizer uses. Polling as fast as the link allows republished both
  motion properties from the device queue hundreds of times per second, which raced with a refusal
  published from the bus thread: `indigo_update_property()` resets `property->do_update` at the end
  of a publication, so the refusal's own publication and its message could be swallowed. The
  progress publication on every poll is preserved, so a client still sees the position advance.

## Notes and gaps

- `FOCUSER_POSITION` and `FOCUSER_STEPS` take their range from the calibrated travel
  (`MOT1.CAL_MINPOS` and `CAL_MAXPOS`, 0 to 40973 on the attached unit); an uncalibrated controller
  keeps the range 0 to 1000000. See "Controller features" above.
- `X_STATE` has two items on SESTO SENSO and three on ESATTO. The attached unit answers
  `"Error: invalid command"` to `VIN_USB`, which confirms the model-dependent item count.
- `EXT_T` reads -127 on the attached unit because no external probe is fitted. Since 3.0.0.20 the
  driver publishes `FOCUSER_TEMPERATURE` IDLE for it instead of the value (see below).
- `X_WIFI` mode switching is not exercised on hardware: turning the access point off would drop the
  unit's own network service. Only the readback is asserted.
- The driver exposes no guider interface, so the guiding-pulse accuracy measurement does not apply.
- Windows and Linux builds of this driver were not exercised; only macOS arm64 was available.

```sh
make -C indigo_test build/integration/test_focuser_primaluce_simulator
cd indigo_test && ./build/integration/test_focuser_primaluce_simulator
make -C indigo_test build/hardware/test_focuser_primaluce_hw
cd indigo_test && PRIMALUCE_HW_PORT=/dev/cu.usbserial-11130 ./build/hardware/test_focuser_primaluce_hw --run
```

- 2026-10-03 round: simulated tests run: 35; passed: 35.
- 2026-10-03 round: hardware tests run: 18; passed: 18.

## Focuser testing rules alignment (3.0.0.20, 2026-10-05)

The simulator suite was checked against the "Focuser Drivers" chapter of
`indigo_test/DRIVER_TESTING_RULES.md` and extended where a rule applies. No hardware test was run
in this round; the latest hardware record stays at 3.0.0.19.

### Found defects

| # | Defect | Fix | Regression test (fails against 3.0.0.19) |
|---|---|---|---|
| D9 | An aborted, stalled or failed move ended ALERT with the requested target, not at the stopped position. | Every move that ends short of its target publishes the measured position as value (the target keeps the request since 3.0.0.23, D21). | `abort_motion`, `stalled_move` |
| D10 | Disconnecting during a move let the motor run on: the port closed without a stop. | `on_disconnect` sends `MOT_ABORT` (fallback `MOT_STOP`) while a move, an uncommanded motion or a focuser calibration runs. | `disconnect_during_motion` |
| D11 | `FOCUSER_ABORT_MOTION.ABORT_MOTION` stayed ON after the abort, and an abort while idle sent `MOT_ABORT`. | The item is released in every path; an idle abort sends nothing and ends OK, or ALERT when the controller did not answer the last request. | `abort_while_idle`, `abort_refused` |
| D12 | An abort that overtook a queued move was followed by that move. | The abort marks the pending move, which is then never sent and ends ALERT. | `abort_overtakes_queued_move` |
| D13 | The position was only read during the driver's own moves: uncommanded motion (hand keypad, a move running at connect) and a failed idle read were never published. | The position is polled every 2 s while no move of the driver runs (0.5 s while it changes); uncommanded motion is BUSY with the target following the measurement and OK once it stops; a failed or incomplete read is ALERT with the last position and the next good read restores OK. A poll whose reply arrives after a request was accepted leaves the request alone. | `external_motion_observed`, `hand_keypad_motion`, `position_poll_failure`, `request_versus_poll` |
| D14 | `EXT_T` -127 (no probe, observed on the attached SESTO SENSO 2) was published as a temperature, and a failed temperature poll published nothing. | -127 is published IDLE; a failed poll publishes `FOCUSER_TEMPERATURE` and `X_STATE` ALERT with the last values; the next poll restores OK. | `no_probe`, `status_polling` |
| D15 | A single lost readback during a move ended it ALERT while the motor kept running; a stalled motor (MST `move`, position unchanged) stayed BUSY forever. | One or two lost readbacks are retried; a third stops the motor and ends ALERT. A motor that does not advance for 25 polls (5 s) is stopped and ends ALERT. | `move_readback_failures`, `stalled_move` |
| D16 | An absolute move kept `FOCUSER_STEPS` OK, a refused move kept the requested value as position, and the controller's refusal reason never reached the client. | Both properties are BUSY for every move; a refused move ends both ALERT with the position read last as value, with "Move refused: <reason>". | `absolute_move`, `move_command_failures` |
| D17 | Backlash and focuser calibration were accepted during a move. | `reject_change` refuses both without a command while a move runs. | `settings_during_motion` |
| D18 | A rejected backlash, speed, hold current, LED or WiFi write left the requested value published; a failed calibration start left `START` ON. | The value or item the controller keeps is restored; the calibration items are released. | `settings_reported_failures`, `focuser_calibration` |
| D19 | A WiFi mode change restarts the controller even while the rotator shares the connection. | Refused with ALERT while the rotator is connected. | `controller_restart_refused_with_sibling` |
| D20 | `FOCUSER_ABORT_MOTION` did not stop a focuser calibration running between START and END. | The abort stops the motor and ends `X_CALIBRATE` ALERT. | `focuser_calibration` |

The pre-fix results come from a separate binary built against a copy of the 3.0.0.19 sources; the
shared tree was not reverted.

### Simulator additions

- Profile `no-probe`: `EXT_T` is `-127.00`, as the attached unit reports without a probe.
- Fault `handmove <position>`: the motor moves without a request, as with the hand keypad.
- Fault action `stall`: a move is acknowledged, MST stays `move` and the position does not change.
- Fault action `slow`: the reply is delayed by 400 ms; an `always_` prefix keeps any fault armed.

### Rule mapping

| Rule | Test |
|---|---|
| Model variants, ranges, hidden controls, conservative fallbacks | `esatto_profile`, `sestosenso3_profile`, `no_abs_pos_profile`, `no_speed_profile`, `old_firmware`, `calibrated_travel` |
| Connect sequence `MODNAME`, `SWVERS`, `LOGLEVEL`, full state; identity and its reset | `metadata`, `reconnect` |
| Refused connect leaves nothing defined, port released, next connect works | `unsupported_device`, `handshake_timeout` |
| Poll failures and recovery, no-probe sentinel, uncommanded motion | `position_poll_failure`, `status_polling`, `no_probe`, `external_motion_observed`, `hand_keypad_motion` |
| Absolute/relative moves, both properties BUSY, GOTO to the current position, clamping | `absolute_move`, `relative_move`, `relative_move_clamped`, `calibrated_travel` |
| Overlapping move refused, running move keeps its target and single command | `overlap_rejected` |
| Abort mid-move ending ALERT at the stopped position with the requested target, idle abort, refused stop and retry, abort overtaking a queued move | `abort_motion`, `abort_while_idle`, `abort_refused`, `abort_overtakes_queued_move`, `test_focuser_primaluce_motion` |
| Move rejection, unacknowledged and malformed start, refusal reason, power message | `move_command_failures`, `motor_without_power` |
| Stall, lost and persistent readback during a move | `stalled_move`, `move_readback_failures` |
| Disconnect and transport loss during motion | `disconnect_during_motion`, `transport_loss_during_motion`, `transport_loss` |
| Settings writes, readback after reconnect, failures keep the device value, settings during a move | `controller_settings`, `led_middle`, `wifi_station_mode`, `run_preset`, `settings_reported_failures`, `settings_during_motion` |
| Calibration sequence, failure, abort | `focuser_calibration` |
| Poll in flight while a move is accepted | `request_versus_poll` |
| Shutdown refused while connected; shared transport with the rotator, controller-wide restart refused while it is connected | `shutdown_rejected_while_connected`, `shared_connection`, `controller_restart_refused_with_sibling` |

### Rules not applicable

- SYNC on the focuser: the controller has no focuser sync command (only the ARCO has `SYNC_POS`).
- `FOCUSER_LIMITS`, reverse motion, compensation and mode: the controller exposes none of them;
  the travel range comes from the calibration (`CAL_MINPOS`/`CAL_MAXPOS`).
- Homing and zeroing: not offered by the controller.
- Rotator scenarios belong to the rotator class rules and were not part of this round.

### Validation

`python3 tools/run_driver_test.py focuser_primaluce` runs `test_focuser_primaluce_simulator`
(52 cases) and `test_focuser_primaluce_motion` (1 case).

## Calibration END refused during the run (3.0.0.21, 2026-10-05)

Found on the SESTO SENSO 2 hardware. The idle position poll added in 3.0.0.20 took the calibration
run (`GoOutToFindMaxPos`) for an uncommanded move and published `FOCUSER_POSITION` BUSY, and
`X_CALIBRATE` refuses every request while the position is BUSY, so END was refused with "The
focuser is moving" and the draw tube kept running outward until it was stopped by hand. The poll now
follows the calibration run without publishing it as a move, and `X_CALIBRATE` accepts a request
while a calibration runs. The simulator models the calibration run (it moves outward from
`GoOutToFindMaxPos` until `StoreAsMaxPos`), and `focuser_calibration` checks that the position
advances without turning BUSY and that END is accepted; the case fails against 3.0.0.20.

The hardware calibration case (`PRIMALUCE_HW_CALIBRATE=1`) reads MOT1 `CAL_MINPOS`, `CAL_MAXPOS`,
`CAL_DIR` and `COMPENSATION_POS_STEP` with raw JSON before it runs, starts the calibration at the
stored minimum so the numbering stays the same, and afterwards stops the motor and writes the
stored limits, direction and offset back with readback. It never writes `ABS_POS`: the controller
implements that write as an offset in `COMPENSATION_POS_STEP`, after which every move stops short of
its target by that offset. An earlier version of the case pressed END at once and left the unit with
`CAL_MAXPOS = 8`; restoring it by writing `ABS_POS` introduced a 2215 step offset that made every
move end short. The unit was recalibrated and the offset cleared by hand.

## Refused request hid a queued move (3.0.0.22, 2026-10-05)

Found on the SESTO SENSO 2 over WiFi, intermittently. An accepted `FOCUSER_POSITION` request is
BUSY only until a refused second request publishes ALERT over it from the bus thread. When that
happened while an idle position poll was in flight, the poll no longer saw a move pending, wrote the
read position into the item and the queued handler sent that position instead of the requested
target (a GOTO to 8000 went out as `MOVE_ABS` 5000 and ended OK at once). Over USB the window is too
short to hit; over HTTP the poll holds the device queue long enough. A `pending` flag is now set when
a `FOCUSER_POSITION` or `FOCUSER_STEPS` request is accepted and cleared when its handler runs; the
refusal condition, the idle poll and the abort check it instead of the property state.
`refusal_keeps_queued_move` (slow poll reply, accepted GOTO, refused second GOTO) fails against
3.0.0.21.

## Target kept after a move ends (3.0.0.23, 2026-10-10)

| # | Defect | Fix | Regression test |
|---|---|---|---|
| D21 | An aborted, stalled, refused or short move overwrote the requested `FOCUSER_POSITION` target with the reached position, and every idle poll (2 s) rewrote it with the measured one. | `focuser_motion_failed`, `focuser_movement_ended` and the idle poll set the value only; the target follows the position only for uncommanded motion and the calibration run. | `abort_motion`, `abort_overtakes_queued_move`, `stalled_move`, `move_readback_failures`, `move_command_failures`, `test_focuser_primaluce_motion` |

## Both motion properties busy from the accepted request (3.0.0.24, 2026-10-10)

| # | Defect | Fix | Regression test |
|---|---|---|---|
| D22 | An accepted move published only its own property BUSY until its handler ran, so a request of the other motion property refused in that window ended that property ALERT in the middle of the move (found by `primaluce_refuses_an_overlapping_request` on the SESTO SENSO 2, whose overlap case also still expected the refusals to end ALERT instead of the BUSY message only). | The `on_change_request` of `FOCUSER_POSITION` and `FOCUSER_STEPS` publishes the other motion property BUSY too, so the refusal is a BUSY message and neither property publishes ALERT. The hardware overlap case expects the message with both properties BUSY and the target of the running move kept. | `refusal_while_move_queued`; hardware `primaluce_refuses_an_overlapping_request` |

## Final test summary

- Simulated tests: 54 run, 54 passed (recorded run of 3.0.0.22, macOS arm64).
- Hardware tests: 18 run, 18 passed on the SESTO SENSO 2 over USB serial and 18 run, 18 passed over
  WiFi (3.0.0.22, macOS arm64, with `PRIMALUCE_HW_CALIBRATE=1`; the stored calibration was read and
  restored in both runs).

## ARCO abort stays ALERT (2026-10-11, 3.0.0.25)

The rotator abort set `ROTATOR_POSITION` ALERT but left the movement finalizer running; once the motor reported
`stop`, the finalizer published OK when the angle equalled the target, so an abort that stopped the rotator at its
target ended OK. The finalizer now judges arrival only while the move is BUSY; after an abort it publishes the angle
the rotator stopped at and keeps ALERT. `rotator_abort` asserts ALERT and the requested target two seconds after the
abort, also for a request to the current angle; the exact case of a running move stopped at its target is not
reproducible with the simulator and is covered by the code change only.

Simulated tests: 55 run, 55 passed. Hardware tests: 0 run, 0 passed.

## Protocol library extraction (2026-10-22, task 20261022212521-61cf-primalucelib)

More PrimaLuce drivers are planned, so the JSON protocol code moves out of this driver into the
shared library `indigo_drivers/shared/primalucelib/`. This is a pure extraction: no behaviour,
property or wire-traffic change is intended, and the driver version stays at 3.0.0.26.

### Audit of the code that moves

- Transport: USB serial at 115200 baud through `indigo_uni_open_serial_with_speed2()` and HTTP
  `GET /ajax.php?jreq=<URL-encoded request>` with one connection per request and the body ended by
  `Content-Length` (see "Network connection" above).
- Framing: a request is written as is; a reply is one line ended by `\n` (a `\r` before it is
  dropped). Lines that start with `[` are diagnostic output and are skipped.
- Parsing: jsmn (strict, parent links) over an 8192-byte response with 1024 tokens; string tokens
  are NUL-terminated in place. A reply that starts with `"` or does not parse fails the request.
- Errors: an `ERROR` key under `res/cmd`, `res/set` or `res/get` fails the request and keeps its
  text for the motion refusal message; a request without any answer sets `link_failed`, which the
  idle abort reports.
- About 100 request strings and reply paths, among them irregular spacing (`{"req":{"get": ""}}`,
  `"VIN_12V": ""`, `{"req":{"cmd": {"MOT1": {"CAL_FOCUSER":...}}}}`) that the simulator matches and
  that therefore must stay byte-identical.
- Risks: the order and manner of the requests is a compatibility contract
  (`indigo_drivers/AGENTS.md`). The library therefore takes the existing code verbatim, and each
  driver call site maps to exactly one library call that sends the same request.

### Baseline (original driver, Linux x86_64, gcc 13.3.0, revision 80d073bcb)

```sh
make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all
make -C indigo_test build/integration/test_focuser_primaluce_simulator build/integration/test_focuser_primaluce_motion
cd indigo_test && ./build/integration/test_focuser_primaluce_simulator; ./build/integration/test_focuser_primaluce_motion
```

44 of 54 simulator cases pass; `test_focuser_primaluce_motion` fails its single case. None of the
failures is caused by this driver, and the earlier records were all made on macOS arm64:

- `handshake_timeout`, `repeated_init_shutdown`, `reconnect`, `disconnect_during_motion`,
  `controller_settings`, `led_middle`, `stored_positions`, `wifi_station_mode`,
  `rotator_lifecycle` and the motion case fail at their second open of the simulator port with
  `Failed to open /dev/pts/N (Device or resource busy)`. `indigo_uni_io` opens serial ports with
  `TIOCEXCL` and never clears it; on Linux a PTY slave keeps the exclusive flag after its last
  descriptor is closed while the master stays open, so a reconnect is refused. A standalone
  reproducer (open, `TIOCEXCL`, close, reopen) gets EBUSY, and gets a working reopen when
  `TIOCNXCL` is issued before the close. This is a framework issue outside this driver.
- `status_polling` is timing-flaky on Linux (2 of 3 isolated reruns failed): it allows 15 s for the
  ALERT that follows a silent environment reply, which arrives at the next environment poll
  (every 10 s) plus the read timeout.

Both are recorded as central findings; this task does not change them. The extraction is accepted
when the same cases pass and fail for the same reasons, and when the wire traffic is unchanged.

### Hardware decision

No hardware test in this round (requester decision: non-interactive simulator run). The
extraction does not change any request, which the unit test proves byte for byte, so the
hardware records of 3.0.0.22 remain the latest hardware evidence. No hardware validation is
claimed.

### Plan

| Step | Change | Verification | State |
| --- | --- | --- | --- |
| E1 | Trace capture: the simulator suite copies each case's event log to `INDIGO_SIMULATOR_TRACE_DIR` | Suite result unchanged | done: same 10 failures, 54 traces written |
| E2 | Reference traces of the original driver, two runs to measure nondeterminism | Traces stored outside the tree | done: both runs identical in all 54 cases (distinct requests, command order, collapsed sequence) |
| E3 | `primalucelib.{h,c}`, `jsmn.h` moved, `PROTOCOL.md` | Standalone compile | done: clean with `-Wall -Wextra`; `git mv` keeps `jsmn.h` unchanged |
| E4 | Unit test `test_primalucelib` (golden requests, parsing, error forms, framing, device helpers) | Unit test passes | done: 19/19; a one-space change in one request macro fails the golden case |
| E5 | Driver uses the library; regenerate | Build; generated diff reviewed | done: generator-owned code identical outside the `//+`/`//-` user blocks, `.h`/`_main.c` unchanged, regeneration reproducible, archive exports only `indigo_focuser_primaluce` |
| E6 | Post-change suites and trace comparison | Same pass/fail set, same request sets per case | done: same 10 failures; all 54 traces byte-identical to both reference runs (865 requests) |
| E7 | Xcode, Visual Studio and test Makefile registration | Review | done |
| E8 | Recorded run `python3 tools/run_driver_test.py focuser_primaluce` | Record written | done: 55/55 OK, linux x64, 3.0.0.26 |

### Library contents

`indigo_drivers/shared/primalucelib/` holds `primalucelib.h` (keys, values, exact request strings,
reply paths, types and the API), `primalucelib.c`, the relocated `jsmn.h`, `PROTOCOL.md` and the
static library builds: `Makefile` (`build/lib/libprimalucelib.a`) and `primalucelib.vcxproj`
(`primalucelib.lib`, in `indigo_windows.sln`). The driver includes the header and links the library:
its `Makefile.inc` routes the archive through `SDK_LIB`/`MAKE_SDK` (rebuilt and relinked when a library
source changes); its `.vcxproj` has a `ProjectReference`; in Xcode the library source is compiled once
into the `indigo` and `indigo_m1` targets, which hold all drivers. The driver archive contains only
the driver (`nm` shows only `indigo_focuser_primaluce`); the library exports its `primalucelib_`
functions and `PRIMALUCE_REPLY_` tables and keeps jsmn internal. The driver calls the library for
every request; it keeps only device policy (property states, polling cadence, stall and abort logic).
The library also offers the GIOTTO, ALTO and further SESTO SENSO 3 commands known from the INDI
drivers for future drivers; they are unit-tested against INDI-shaped replies only.

### Found defects (this round)

None of them is in the driver; the driver version therefore stays 3.0.0.26.

| ID | Defect | Fix | Regression test |
| --- | --- | --- | --- |
| FND-000049 | Reproduced: on Linux every reconnect to the simulator failed with EBUSY (9 simulator cases and the motion case). `indigo_uni_io` opened serial ports with `TIOCEXCL` and closed them without `TIOCNXCL`; a Linux pseudo-terminal keeps the flag after its last descriptor is closed while the master stays open. | `indigo_uni_close()` and the `open_tty()` error path clear the flag before closing. | `indigo_test/unit/test_uni_io.c` `serial_port_reopens_after_close` (fails before, passes after); the 10 cases pass. |
| FND-000050 | Reproduced: `status_polling` allowed 15 s for an ALERT that arrives up to 15 s after the fault is armed (10 s environment poll plus the 5 s read timeout); it failed 2 of 3 Linux reruns. | The window is 20 s and the cadence is documented in the case. | `status_polling` 3 of 3 and in the recorded run. |
| FND-000051 | Reproduced: `handshake_timeout` asserted that the device was not connected while the refused attempt could still be BUSY with CONNECTED on; `connect_focuser()` and the unanswered read both take 5 s. Failed in all 5 Linux runs. | The case waits for CONNECTION ALERT before the check. | `handshake_timeout` 3 of 3 and in the recorded run. |
| FND-000052 | Source audit, not fixed: WiFi SSIDs and passwords are inserted into the JSON without escaping, so a `"` or `\` produces an invalid request. The extraction preserved the behaviour. | Open finding; needs its own change with a version bump. | none yet |

### Validation

```sh
make -C indigo_test build/unit/test_primalucelib && (cd indigo_test && ./build/unit/test_primalucelib)
make -C indigo_test test-unit
python3 tools/run_driver_test.py focuser_primaluce
```

Linux x86_64 only; macOS and Windows builds were not run in this environment. No hardware test.

## Final test summary

- Simulated tests: 55 run, 55 passed (recorded run of 3.0.0.26, linux x64, with the library), plus 19 of
  19 library unit cases. Before the FND-000049/50/51 fixes the same driver passed 44 of 55, exactly
  as the original driver did, with byte-identical wire traffic in all 54 simulator cases.
- Hardware tests: 0 run, 0 passed in this round; the latest hardware record stays 3.0.0.25 (linux
  arm64) and 3.0.0.22 (mac arm64, USB and WiFi).

## SESTO SENSO 3 support (3.0.0.27, 2026-10-23, task 20261022235021-bcb6-primaluce-sestosenso3)

On the owner's SESTO SENSO 3 (application firmware 1.0.9, web 03.06.01, CP2102N on USB, delivered
uncalibrated) `FOCUSER_POSITION` alternated between OK and ALERT and moves failed. The work followed
the owner's document "PrimaluceLab - Communication Protocol - SS3 v1.0" (06 Oct 2025, not committed)
and supervised hardware sessions: small moves and calibrations, approved by the owner.

### Hardware observations

- Every `CAL_DIR` write is acknowledged and then restarts the ESP32 (boot log on the serial line,
  about 9.5 s without answers); requests sent meanwhile are answered late, after the restart.
- `LOGLEVEL` is preceded by a `[mLogs_DefaultSetup]` line; `DIMLEDS` is acknowledged as
  `{"res":{"cmd":{"cmd":"done"}}}`.
- An uncalibrated controller (`CAL_MAXPOS` 0, `CALRESTART.MOT1` 1) acknowledges `GOTO` with "done"
  and does not move. A controller with its motor bridges off (`STATUS.HIZ` 1, seen after the motor
  ran into the mechanical end with the fault flags set) does the same.
- Semi-automatic calibration: `Init`, `StoreAsMinPos` (zeroes the coordinate),
  `GoOutToFindMaxPos` (runs until `StopMotor`), `StoreAsMaxPos`. The ordinary stop does not end the
  run (protocol document); `stop_calib` leaves the procedure and the focuser uncalibrated.
- `CAL_MAXPOS` keeps the previous value (0, or the previous calibration) for up to about 0.65 s
  after `StoreAsMaxPos` is acknowledged.

### Found defects

| ID | Defect | Fix | Regression test |
| --- | --- | --- | --- |
| FND-000053 | Reproduced on hardware: after a late reply every following reply answered the previous request, so the driver never recovered (OK/ALERT oscillation, failed moves). | The library discards pending input before each request and drops replies without the request verb, up to `PRIMALUCE_MAX_STALE_REPLIES`. | `test_primalucelib` `a_duplicate_reply_is_discarded`, `a_late_reply_is_skipped`, `only_stale_replies_fail_the_request` (the first two fail against the previous library); `sestosenso3_calibration_restart`. |
| FND-000054 | Reproduced on hardware: an uncalibrated SESTO SENSO 3 accepted moves that never ran. | Moves are refused with "The focuser is not calibrated" while the reported range is empty. | `sestosenso3_uncalibrated`, `sestosenso3_calibration_restart`. |
| FND-000055 | Reproduced on hardware: START failed, because the `CAL_DIR` write inside the SESTO SENSO 2 sequence restarts the controller. | SESTO SENSO 3: `CAL_DIR` is read and written first, only when it changes, followed by the restart wait (BUSY with "The focuser restarts to change the calibration direction") and `Init`, `StoreAsMinPos`, `GoOutToFindMaxPos`; END sends `StopMotor` before `StoreAsMaxPos`. | `sestosenso3_calibration`, `sestosenso3_calibration_restart`. |
| FND-000056 | Reproduced on hardware (twice): after END the driver read the range at once, got 0 or the previous maximum and refused moves as uncalibrated or published the position out of range. | END reads the range before `StoreAsMaxPos` and polls (up to 5 s) until the maximum changes; the motion properties are redefined with the new range; no range ends the calibration ALERT. | `sestosenso3_calibration` (with the previous poll it publishes 100000 and fails). |
| FND-000057 | Reproduced on hardware: X_CALIBRATE requests were accepted while a step ran (the handler sets OK when it starts), queued calibrations ran after each other, the last one cleared the calibration state and END was refused, so the run drove into the mechanical end and the owner had to unplug the focuser. | Requests are refused while a step runs; during the run only END is accepted; END without a run is refused. | `sestosenso3_calibration_restart`, `sestosenso3_calibration`, `calibration_end_without_run`. |
| FND-000058 | Protocol document and simulator: abort during a SESTO SENSO 3 calibration sent the ordinary stop, which does not end the run. | Abort sends `StopMotor` and `stop_calib` and publishes the focuser uncalibrated. | `sestosenso3_calibration_restart`; not exercised on hardware. |
| FND-000059 | Reproduced on hardware: a move the controller acknowledged and never ran ended ALERT without a reason. | The ALERT says "The focuser stopped before reaching the target". | `move_not_started`. |

### Simulator additions

`--profile sestosenso3` models the observations above: the log line before `LOGLEVEL`, the
`DIMLEDS` acknowledgement, the restart after `CAL_DIR` (2 s here, requests answered late), the
semi-automatic calibration with `StopMotor`, the delayed `CAL_MAXPOS`, `stop_calib` and the ignored
ordinary stop during the run. `--profile sestosenso3-uncalibrated` is the delivered state, and the
`ignore` fault acknowledges a move without running it. `CAL_MINPOS`, `CAL_MAXPOS` and `CAL_DIR` are
simulator state; a SESTO SENSO 2 `StoreAsMaxPos` stores the position at once.

### Limitations

- The SESTO SENSO 3 START handler holds the device queue during the restart wait (up to 10 s of
  polling, before any motion starts) and END polls the new range for up to 5 s; a request queued
  meanwhile, including an abort, runs afterwards. No motion is running during either wait.
- Abort during a SESTO SENSO 3 calibration and the `HIZ` case are verified with the simulator only.
- The SESTO SENSO 3 SC automatic calibration, `RECOVER_DELAY` and the stored positions 10 to 12
  are not supported (task non-goals).

### Validation

Driver hardware sessions on the SESTO SENSO 3 (linux x64, supervised): moves after the library fix;
an inverted-direction calibration (maximum 17197, which showed FND-000056 before its fix); a
normal-direction calibration after all fixes, with the restart, END accepted during the run and the
new maximum 30975 published at END without reconnecting; moves after it. The final recorded run is
in the test summary below.

## Final test summary (3.0.0.27)

- Simulated tests: 60 run, 60 passed: `2026-10-23 02:17 3.0.0.27 linux x64 simulator 60/60 OK`
  (59 simulator cases, 5 of them new, plus the motion case). Library unit cases: 23 of 23.
- Hardware: the supervised SESTO SENSO 3 sessions are listed under Validation above. They were
  manual sessions, not a `tools/run_driver_test.py --hw` run, so they produce no hardware record.
  The latest hardware records stay 3.0.0.25 (linux arm64) and 3.0.0.22 (mac arm64).

## Calibration steps as finalizers (3.0.0.28, 2026-10-23, task 20261023011844-cc6a-primaluce-calibration-finalizers)

The 3.0.0.27 calibration held the device queue during its waits. START slept a second between the
steps, and a SESTO SENSO 3 that changed direction waited up to 10 s for the restart. END polled the
new range for up to 5 s. Requests queued meanwhile, including an abort, ran only afterwards
(FND-000060).

The waits are now finalizers on the device queue, and each one does a single bounded transaction:

- `focuser_calibration_start_finalizer` sends one start step and schedules the next one 1 s later:
  `Init`, `CAL_DIR` (SESTO SENSO 2), `StoreAsMinPos`, `GoOutToFindMaxPos`. After the last step
  X_CALIBRATE turns OK and only END is accepted.
- `focuser_calibration_restart_finalizer` polls a restarting SESTO SENSO 3 with one `MODNAME` request
  every 0.5 s, at most `PRIMALUCE_RESTART_POLLS` times, and then starts the steps. If the controller
  does not answer, X_CALIBRATE ends ALERT "The focuser did not answer after the restart".
- `focuser_calibration_end_finalizer` reads the range every 0.25 s, at most 20 times, until the
  maximum differs from the one before `StoreAsMaxPos`. It then republishes the motion properties.

X_CALIBRATE stays BUSY until the run starts or END completes. An abort during the start cancels the
pending step or restart poll, and a SESTO SENSO 3 that already entered the procedure gets
`stop_calib`. X_CALIBRATE ends ALERT "Calibration aborted". Disconnect cancels the finalizers through
the generated `indigo_cancel_pending_handlers()`. Because the abort handler names these finalizers,
the generator drops its prologue and epilogue, and the handler publishes its completion itself.

Regression tests:
- `sestosenso3_calibration`: a LED change during the range poll completes while X_CALIBRATE is BUSY.
- `sestosenso3_calibration_restart`: the same during the restart.
- `sestosenso3_abort_during_restart`: an abort during the restart cancels the calibration; no step
  is sent, the range is kept, and the next START in the new direction starts without a restart.

The three cases fail against the 3.0.0.27 driver: the LED change completes only after X_CALIBRATE
leaves BUSY, and the abort waits behind the restart. This removes the blocking limitation listed for
3.0.0.27.

### Restart gate (found on hardware during this change)

On the SESTO SENSO 3 the first finalizer version still delayed requests during the restart. The
idle position poll and every other request waited the serial read timeout of 5 s on the silent
controller: an abort ran only after the restart, and the timed-out polls briefly published
FOCUSER_POSITION and X_STATE ALERT. The library now keeps a restarting state:

- `primalucelib_begin_restart()` makes every request fail at once with "The controller is
  restarting". A move shows it as "Move refused: …", and other settings end ALERT.
- The idle polls skip while the controller restarts.
- The restart finalizer uses `primalucelib_probe_restart()`, a `MODNAME` request with a 0.3 s read
  timeout. An abort during the restart therefore acts at once, while the probe continues until the
  controller answers and reopens the link.

The next hardware attempt failed the calibration after the restart. The controller answers all the
probes it received while restarting (12) at once, so Init met more stale replies than
`PRIMALUCE_MAX_STALE_REPLIES`. After a successful probe the library now reads late replies away
until the line has been quiet for 0.3 s.

The simulator restart now lasts 5 s and replays up to 16 queued requests, which reproduces that
failure without the drain. Regression tests:
- `test_primalucelib` `a_restarting_link_refuses_requests_until_probed`;
- `sestosenso3_calibration_restart`: the LED change during the restart ends ALERT within 1 s without
  a request, and the calibration then starts;
- `sestosenso3_abort_during_restart`: the abort ends the calibration within 1 s, during the restart,
  and the position never turns ALERT.

Hardware (supervised, SESTO SENSO 3):
- START with a direction change: the LED change was refused within 0.3 s, 11 late replies were
  dropped, then Init, StoreAsMinPos and GoOutToFindMaxPos followed. END published 6977 after
  polling past the stale 18250.
- An abort during a restart reported "Calibration aborted" in the same millisecond.
- A move and an abort afterwards worked.

## Final test summary (3.0.0.28)

- Simulated tests: 61 run, 61 passed: `2026-10-23 04:02 3.0.0.28 linux x64 simulator 61/61 OK`.
  Library unit cases: 24 of 24.
- Hardware: the supervised SESTO SENSO 3 session above was manual, so it produces no hardware
  record.

## Audit blocker repair (2026-10-23)

Task: 20261023065605-60d4-repository-audit. Two scoped defects: the shared archive recipe hard-codes ar flags despite the macOS libtool configuration, and shared JSON lookup accepts a shorter received key as a requested longer key. The latter can confuse ABS_POS and ABS_POS_STEP units/availability. Public property schema, command bytes, queue/finalizer lifecycle and shared-open ownership remain unchanged.

Baseline: audit clean archive build failed on macOS; original shared-library 24 cases passed using an explicitly constructed archive, and the actual JSON reproducer returned ABS_POS=123 for missing ABS_POS_STEP. Exact commands/results are retained in the task verification evidence. Existing simulator and protocol golden-request suites characterize supported behavior. This is a bounded defect repair, not a driver migration or transport refactor.

Hardware decision: no hardware access; noninteractive serial-simulator validation only. No physical device, WiFi credentials or saved configuration will be used.

Atomic plan: forward configured archiver flags and verify clean builds; add failing reply-prefix regression; require exact keys; bump driver 28 to 29 and regenerate using the unchanged generator; run full library/driver simulator checks and independent review. No generator source changes.

Executed repair: the archive recipe forwards ARFLAGS and preserves atomic publication; shared parsed-reply lookup now requires equal key lengths. Added the failing-then-passing parsed serial-peer regression. Driver version increased from 28 to 29; isolated regeneration matches all three checked-in outputs. No property changes or generator modifications.

## Final test summary (3.0.0.29)

- `python3 tools/run_driver_test.py focuser_primaluce`: exit 0; simulated tests 61 run, 61 passed. Recorded: `2026-10-23 09:41 3.0.0.29 mac arm64 simulator 61/61 OK`. README Testing and TEST_SUMMARY were updated by the runner.
- Library: 25 run, 25 passed; ASan/UBSan library/regression build: 25 run, 25 passed. Clean macOS/libtool and ar-compatible builds pass; forced archiver failure preserves the previous archive and removes temporary files. Linux/Windows execution remains unverified.
- Hardware: 0 run, 0 passed in this repair task. Historical manual hardware observations above do not validate this version.
- Independent source review: core_audit PASS; exact baseline/repair commands, logs and limits are retained in `.factory/tasks/archive/20261023065605-60d4-repository-audit/verification.md`.

## ARCO on the SESTO SENSO 3 (3.0.0.30, 2026-10-23, task 20261023080442-7930-primaluce-arco-sestosenso3)

The requester's SESTO SENSO 3 with an ARCO (firmware 1.0.9) had three defects, all found on hardware:

| ID | Defect | Fix | Regression test |
| --- | --- | --- | --- |
| FND-20261023080442-7930-primaluce-arco-sestosenso3-001 | An absolute move turned the ARCO but ended ALERT at once. The SESTO SENSO 3 acknowledges `MOVE_ABS` and `SYNC_POS` under `MOT2.DEG`, while the library expected `MOT2.STEP` and `MOT2.SYNC_POS` (SESTO SENSO 2), so sync also failed. | The library accepts either key for both. | `sestosenso3_rotator_move` and `sestosenso3_rotator_sync`; both fail without the fix. |
| FND-20261023080442-7930-primaluce-arco-sestosenso3-002 | A move to 15° ended ALERT at 14.99994°: the driver compared the target and the angle for equality. | The target is reached within `ROTATOR_POSITION_TOLERANCE` (0.1°, the requester's choice). | `sestosenso3_rotator_move`; fails at 44.9999 with the exact comparison. |
| FND-20261023080442-7930-primaluce-arco-sestosenso3-003 | After quick repeated syncs the controller printed debug text in front of the reply on the same line, so the reply failed to parse. | The library parses a reply line from its first `{`. | `test_primalucelib` `debug_text_before_a_reply_is_skipped`; `sestosenso3_rotator_sync`. |

Simulator: the SESTO SENSO 3 profile acknowledges ARCO moves and syncs under `DEG`, the sync behind
debug text, and reports the angle with five decimals and the step rounding (the target − 0.00006°).

Hardware (supervised): absolute moves end OK, an abort stops the ARCO and ends ALERT, a sync takes
effect without moving (three syncs in a row, one behind debug text), and moves after the sync use the
synced angles.

## Final test summary (3.0.0.30)

- Simulated tests: 63 run, 63 passed on the tree merged with the audit repair:
  `2026-10-23 11:07 3.0.0.30 linux x64 simulator 63/63 OK`. Library unit cases: 26 of 26.
- Hardware: the supervised SESTO SENSO 3 ARCO session above was manual and ran before the merge
  (local build that the merge numbers 3.0.0.30, without the audit's exact key matching), so it
  produces no hardware record.

## Lost coordinate after a calibration (3.0.0.31, 2026-10-23, task 20261023091538-dfa4-primaluce-calibration-position-check)

On the requester's SESTO SENSO 3 (firmware 1.0.9) the controller sometimes reports `ABS_POS_STEP`
1000000 with `STATUS.HIZ` 1: after a restart, after an aborted calibration, after `Init` or
`StoreAsMinPos`. The value lies far outside the calibrated range and is apparently the
coordinate of a controller that lost its position. The protocol document does not describe it. Once
the requester saw a finished calibration end OK with that value as the position (target 13995,
value 1000000). That run was not reproduced, but the logs of five sessions contain the reading
(FND-20261023091538-dfa4-primaluce-calibration-position-check-001).

END now reads the position back with the new range. If it lies outside the range, X_CALIBRATE ends
ALERT with "The focuser reports a position outside the calibrated range, run the calibration
again". The controller's range is still published, and a new calibration can be started at once.
This is the behaviour the requester asked for.

Simulator: the `lostposition` fault makes the next SESTO SENSO 3 `StoreAsMaxPos` leave the
coordinate at 1000000. Regression test: `sestosenso3_calibration_lost_position`, which fails
without the check.

Not verified on hardware: the reading could not be provoked on demand. Moves from that state were
not tried.

## Final test summary (3.0.0.31)

- Simulated tests: 64 run, 64 passed: `2026-10-23 12:18 3.0.0.31 linux x64 simulator 64/64 OK`.
  Library unit cases: 26 of 26.
- Hardware: none for this change (see above).

## Calibration reason for moves that do not run (3.0.0.32, 2026-10-23, task 20261023101445-b9b5-primaluce-needs-calibration-message)

The requester's ARCO on a SESTO SENSO 3 (firmware 1.0.9) was flagged for calibration
(`CALRESTART.MOT2` 1, `MOT2.ERROR` "CalRestart; CalRestart_MaxPosEqualMinPos; "). It acknowledged
`MOVE_ABS` with `DEG: done`, did not turn (HIZ 1), and the move ended ALERT without a message. The
driver reported the flag at connect only (FND-20261023101445-b9b5-primaluce-needs-calibration-message-001).

The driver now keeps the `CALRESTART` flag of both motors from connect. A move that ends short of
its target says "The rotator needs calibration" or "The focuser needs calibration" when its motor
is flagged, and "… stopped before reaching the target" otherwise. A successful ARCO or focuser
calibration clears the flag. Moves are not refused up front, because a SESTO SENSO 2 with the flag
set still moves (`needs_calibration`).

Simulator: the `sestosenso3-arco-uncalibrated` profile flags the ARCO, acknowledges its moves
without turning it, and clears the flag with an ARCO calibration.

Regression tests (both fail without the fix):
- `sestosenso3_rotator_needs_calibration`: ALERT with the reason; after an ARCO calibration the move
  works.
- `needs_calibration`, extended: a focuser move acknowledged and not run names the calibration.

Hardware (supervised): rotator moves that did not run ended ALERT with "The rotator needs
calibration", and the ARCO calibration was started.

## Final test summary (3.0.0.32)

- Simulated tests: 65 run, 65 passed: `2026-10-23 13:36 3.0.0.32 linux x64 simulator 65/65 OK`.
  Library unit cases: 26 of 26.
- Hardware: the supervised rotator check above was manual, so it produces no hardware record.

## Rotator angle and value during a move (3.0.0.33, 2026-10-23, task 20261023111747-3b69-primaluce-rotator-progress)

During an ARCO move the movement finalizer read the angle every 0.2 s but published ROTATOR_POSITION
only at the end, so a client saw the start angle until the rotator arrived (FND-20261023111747-3b69-primaluce-rotator-progress-001, requester's report
on the SESTO SENSO 3). The finalizer now publishes the angle on every poll while the move is BUSY,
like the focuser.

Regression test: `rotator_move` and `sestosenso3_rotator_move` also move from 10° to 200° and
require an angle strictly between them while BUSY. Both fail without the fix.

Hardware (supervised): during a move from 10° to 17° the published angle counted 10.0, 10.4, 11.1,
… before it ended OK.

The first BUSY publication of a move also showed the requested angle or position as its value for one
update, because both position properties were dispatched with `INDIGO_COPY_VALUES_PROCESS_CHANGE`
(FND-20261023111747-3b69-primaluce-rotator-progress-002). FOCUSER_POSITION and ROTATOR_POSITION now use `preserve_values = true`, the generator's copy
of the targets only (`INDIGO_COPY_TARGETS_PROCESS_CHANGE`), as the ZWO rotator does: the request
goes to the target, and the value keeps the measured position. Regression tests: `absolute_move`,
`rotator_move` and `sestosenso3_rotator_move` check the value of the first BUSY update (logged by the
test client); without the change it shows 19000 or 200, the request. Hardware: the first BUSY update
of a rotator move showed the angle the ARCO had reported (0) with the target 20.556.

## Final test summary (3.0.0.33)

- Simulated tests: 65 run, 65 passed: `2026-10-23 15:55 3.0.0.33 linux x64 simulator 65/65 OK`.
  Library unit cases: 26 of 26.
- Hardware: the supervised checks above were manual, so they produce no hardware record.

## Review against the SESTO SENSO 3 protocol document (3.0.0.34, 2026-10-23, task 20261023132326-3447-primaluce-field-errors)

The driver and the library were checked against all 11 pages of "PrimaluceLab - Communication
Protocol - SS3 v1.0". The document covers only the SESTO SENSO 3 specific commands and refers to the
SESTO SENSO 2 document for the rest.

**Follows the document:**
- The request and reply envelope, and `"name":""` reads.
- One request at a time, waiting for its reply.
- `GOTO` with integer steps, and `"done"` acknowledgements.
- The semi-automatic sequence from `Init` through `GoOutToFindMaxPos`, `StopMotor` and `StoreAsMaxPos`. `StoreAsMaxPos` ends the procedure.
- `stop_calib` leaves the procedure and the focuser uncalibrated.

**Fixed here (FND-20261023132326-3447-primaluce-field-errors-001):** the document reports a refusal in the parameter's own field, for example
`"PRESET_1": "Error: invalid parameter"`, but the library only recognised `ERROR` keys:
- The `CAL_FOCUSER` steps and the `CAL_DIR` write were taken as done without checking the reply.
- Refused moves and syncs ended ALERT without the reason.

`primalucelib_reply_is_acknowledged()` accepts `"done"` and keeps any other text as the reason. It
is used by the calibration steps, the direction write, the `MoveIn`/`MoveOut` steps and the ARCO
sync. Refused focuser and rotator moves and syncs now show "Move refused: …" or "Sync refused: …".
INDI also requires `"done"` for every command on both models.

**Deviation, kept:** the driver does not search for the closed end with `GoInToFindMinPos`. It stores
the current position as the minimum, as the SESTO SENSO 2 flow does, so the tube has to be fully in
before START.

**Not implemented:**
- The manual procedure (`Init-Manual`, `StoreAsMaxPos-Manual`).
- The `MoveIn`/`MoveOut` fine steps.
- The SC automatic calibration (`start_auto_cal`, `CAL_STATUS`).
- `RECOVER_DELAY`.

**Simulator:** the `fielderror` fault answers `CAL_FOCUSER`, `CAL_DIR`, `GOTO`, `MOVE_ABS` and
`SYNC_POS` with the error in the field.

**Regression tests (both fail without the fix):**
- `test_primalucelib` `field_errors_refuse_calibration_requests`.
- `sestosenso3_field_errors`, which covers the calibration step, the focuser move, the rotator move and the sync.

## Final test summary (3.0.0.34)

- Simulated tests: 66 run, 66 passed: `2026-10-23 16:31 3.0.0.34 linux x64 simulator 66/66 OK`.
  Library unit cases: 27 of 27.
- Hardware: none for this change; the refusal cannot be provoked on demand.

## ARCO kept enabled (3.0.0.35, 2026-10-23, task 20261023193406-58c7-primaluce-arco-keep-enabled)

On the requester's SESTO SENSO 3 the ARCO needed calibration after almost every connect (FND-20261023193406-58c7-primaluce-arco-keep-enabled-001).

**Cause:** the rotator disconnect switched the ARCO port off with `{"set":{"ARCO":0}}`, and the next
connect switched it on again with `"ARCO":1`. This dates from the original ARCO support. The unit
keeps the ARCO calibration while the port is off, and switching the port on clears it. In the log,
`"ARCO":0` with `CALRESTART.MOT2` 0 became `"ARCO":1` with `CALRESTART.MOT2` 1. INDI only ever
enables the ARCO.

**Fix:** the connect reads the port switch from the full state (`"ARCO"`) and switches the port on
only when it is off. The disconnect leaves it on. After an INDIGO disconnect the ARCO therefore
stays enabled.

**Simulator:** the ARCO port starts off. On the SESTO SENSO 3 profile, switching it off sets the
ARCO calibration flag.

**Tests:**
- `rotator_lifecycle` now expects one switch-on and no switch-off, a reconnect without another
  switch-on, and no `"ARCO":0`. Its earlier expectation, a switch-off on every disconnect,
  described the behaviour this task removes.
- `sestosenso3_arco_stays_calibrated` moves after a rotator reconnect without the calibration flag.
- Both fail with the old disconnect.

**Hardware (requester's unit, cycles driven with `indigo_prop_tool`):**
- At the first connect the port was off, left that way by the old driver. The single switch-on set
  the flag.
- The ARCO was calibrated (with the requester watching), and then three cycles ran: a rotator
  disconnect and reconnect, a focuser and rotator disconnect that closed the serial port and
  reconnected, and another rotator cycle.
- Each reconnect read `"ARCO":1` and `CALRESTART.MOT2` 0. No `"ARCO":0` was sent, no further
  switch-on was sent, and no calibration notice appeared.

## Final test summary (3.0.0.35)

- Simulated tests: 67 run, 67 passed: `2026-10-23 22:43 3.0.0.35 linux x64 simulator 67/67 OK`.
  Library unit cases: 27 of 27.
- Hardware: the scripted connect/disconnect cycles above; not a `--hw` record.

## Calibration hint (3.0.0.36, 2026-10-23, task 20261023202204-7851-primaluce-calibration-hint)

Item 2 of the protocol review: the semi-automatic calibration in the document first finds the closed
end (`GoInToFindMinPos`, or the `MoveIn`/`MoveOut` steps), then sends `StopMotor` and
`StoreAsMinPos`. The driver sends `StoreAsMinPos` right after `Init`, so the position at START
becomes the minimum. This is the SESTO SENSO 2 flow. If the tube is not fully in, the stored range
is shorter than the travel, and the inner part of the travel becomes unreachable.

The requester chose to keep the flow and make the precondition visible, option (a). The X_CALIBRATE
label is now "Calibrate focuser (fully in first)", which still starts with the label the README
refers to. The first successful start step publishes "The current position is stored as the
innermost one". This applies to both models and to both START items.

Regression test: `focuser_calibration` checks the label and the message. It fails without either
of them.

## Final test summary (3.0.0.36)

- Simulated tests: 67 run, 67 passed: `2026-10-23 23:25 3.0.0.36 linux x64 simulator 67/67 OK`.
- Hardware: none for this change; it affects text only.

## Calibration complete messages (3.0.0.37, 2026-10-23, task 20261023205001-3f61-primaluce-calibration-complete-message)

A successful calibration used to turn X_CALIBRATE OK without saying what it stored. The requester
asked for a completion message for the focuser and for the ARCO:

- A focuser END that stores a valid range publishes "Calibration complete, range <min> to <max>",
  with the range the controller reported (both models).
- A finished ARCO calibration publishes "ARCO calibration complete". Its range is in motor steps
  (±1536000 on the requester's unit), so the message gives none.
- Failed calibrations keep their existing reasons.

Regression tests: `focuser_calibration`, `sestosenso3_calibration` (which checks the exact range)
and `rotator_calibration` check the messages. All three fail without them.

## Final test summary (3.0.0.37)

- Simulated tests: 67 run, 67 passed: `2026-10-23 23:54 3.0.0.37 linux x64 simulator 67/67 OK`.
- Hardware: none for this change; it adds messages only.

## Relative rotator move (3.0.0.38, 2026-10-24, task 20261023211312-b017-primaluce-rotator-relative-move)

The ARCO has no relative command. Neither the INDI command set (`MOVE_ABS` in DEG/ARCSEC/STEP,
`SYNC_POS`, `MOT_STOP`) nor the session logs show one. As in the ZWO rotator, ROTATOR_RELATIVE_MOVE
is now shown and computed from the current angle:

- The rotator moves to the angle it has plus the offset, using the existing `MOVE_ABS`.
  ROTATOR_POSITION takes the target, publishes the progress, and ends the move with the usual
  reasons, such as the needs-calibration message.
- `rotator_relative_move_ended()` ends ROTATOR_RELATIVE_MOVE with the same state, also after an
  abort.
- A target outside the ROTATOR_POSITION range (-180..360) is refused without a command. A refused
  move gives the controller's reason.
- A rotator that stands at its last target within `ROTATOR_POSITION_TOLERANCE` moves from that
  target. Without this, the SESTO SENSO 3 rounding (29.99994 for 30) would add up over several
  relative moves. The SESTO SENSO 3 case caught it: -20.00006 instead of -20.
- Both moves share the BUSY guard: a relative move is refused while either one is BUSY.

Regression tests: `rotator_relative_move` and `sestosenso3_rotator_relative_move`:
- a move of +30 and then -50 (to -20);
- a refused -170, which would end at -190;
- an abort during a +170 move, which ends both properties ALERT.

## Final test summary (3.0.0.38)

- Simulated tests: 69 run, 69 passed: `2026-10-24 00:18 3.0.0.38 linux x64 simulator 69/69 OK`.
- Hardware (requester's SESTO SENSO 3 with ARCO, moves driven with `indigo_prop_tool` while the
  requester watched):
  - +10 from 0 went to 10 (read back 9.99492);
  - -20 went to -10, from the reached target;
  - -175 (target -185) was refused without a command;
  - +90 from -10 was aborted at -4.75, and both properties ended ALERT after `MOT_STOP`.
  The ARCO turns at about 2°/s. A fresh connect found the ARCO still enabled and calibrated,
  confirming the previous task on the unit. This was not a `--hw` run, so there is no hardware record.

## ESATTO calibration and home sensor (3.0.0.39, 2026-10-24, task 20261023231027-08cd-esatto-calibration-home)

Checked against the PrimaLuce Lab "Communication Protocol" v4.0 (05 Dec 2022, ESATTO, SESTOSENSO2 and
ARCO models) and the requester's ESATTO2 log (application firmware 3.05.28, with an ARCO):

- **No calibration on an ESATTO.** v4.0 lists `CAL_FOCUSER` as SESTOSENSO2 only, and the ESATTO2 answered
  all 44 `CAL_FOCUSER` `Init` requests of the session with `"Error: invalid command"`. The ESATTO is
  calibrated in the factory and uses hall sensors at both ends (`HSENDET`, `CAL_MIN_LO`/`HI`,
  `CAL_MAX_LO`/`HI`). X_CALIBRATE is now hidden on an ESATTO and shown on a SESTO SENSO. The
  `esatto_profile` expectation that the calibration stays available on an ESATTO was wrong and is
  corrected; the document and the hardware log above are the reason.
- **Needs calibration on an ESATTO.** A `CALRESTART` flag gives "<model> needs calibration, which an
  ESATTO cannot run from the driver", so the message no longer points to the hidden property. The
  controller's `ERROR` text is still sent as an alert at connect.
- **Home sensor reset.** An inward move from 500 to 400 stopped, reported `NOPCMD` and reset the
  coordinate to 0; moves of ±100 around 5000 ran normally. A move that ends at 0 short of a target above
  0 on an ESATTO now ends ALERT with "The focuser reached its home sensor and reset its position to 0"
  instead of "The focuser stopped before reaching the target". The document does not describe the
  reset; the simulator models it on `CAL_HOMEP` (1000), which is an assumption.
- **Relative ARCO move.** v4.0 documents a relative `MOVE` for both motors, so "the ARCO has no relative
  command" in the previous section holds only for the SESTO SENSO 3 document. The relative rotator move
  keeps using `MOVE_ABS` on every model, as `MOVE` is not in the SESTO SENSO 3 document and has not been
  tried; the comment, `PROTOCOL.md` and `PROPERTIES.md` are corrected.
- Already correct for an ESATTO: `MOVE_ABS` instead of the deprecated `GOTO`, no `SPEED`, the SESTO
  SENSO 2-only run presets, motor settings and hold current hidden, `VIN_USB` shown, `-127` external
  temperature treated as no reading.

Simulator: the `esatto` profile refuses `CAL_FOCUSER` and re-homes an inward move to a target below
1000; a new `esatto-needs-calibration` profile reports `CALRESTART` with a sensor error.

Regression tests, each confirmed to fail without the driver change:
- `esatto_profile`: no X_CALIBRATE and no `CAL_FOCUSER` request;
- `esatto_home_reset`: 17500 to 400 ends ALERT at 0 with the home-sensor reason, then 500 runs normally;
- `esatto_needs_calibration`: the sensor error and the ESATTO needs-calibration message at connect.

The simulator's ESATTO profile now also acknowledges the ARCO move under `DEG`, as the unit does.

Hardware check (requester's ESATTO2 with ARCO on `/dev/ttyUSB0`, application firmware 3.05.28, driven with
`indigo_prop_tool` while the requester watched):
- Connect: no X_CALIBRATE; the focuser stood at 1000.
- 1000 to 500: the readback went 1270, 0 with `NOPCMD` 1, 1000, then 0; the move ended ALERT at 0 with
  "The focuser reached its home sensor and reset its position to 0". This also fits the `CAL_HOMEP`
  model of the simulator.
- 0 to 400 outward ended OK at 400 (one readback of -370 on the way).
- 400 to 60000 ran at about 12000 steps/s and finished before the abort arrived; the abort then found the
  motor stopped, and the move ended OK at 60000.
- 60000 to 300000, aborted after about 4 s: `MOT_ABORT` done, FOCUSER_POSITION ALERT at 107855.
- Rotator connect: the controller's "CalRestart; CalRestart_MaxPosEqualMinPos; " and "ARCO needs
  calibration"; the ARCO was enabled already, so no `ARCO` write was sent.
- A 10° move of the uncalibrated ARCO: `MOVE_ABS` acknowledged under `DEG`, `HIZ` 1 and no movement;
  ROTATOR_POSITION ended ALERT with "The rotator needs calibration".
- Disconnect sent no `"ARCO":0`.

- ARCO calibration (requested by the requester): `{"set":{"MOT2":{"CAL_STATUS":"exec"}}}` acknowledged
  `"done"`, `CAL_STATUS` read `exec` once a second and `stop` after about 66 s; X_CALIBRATE_A ended OK
  with "ARCO calibration complete".
- A 10° move then turned the ARCO at about 3°/s and ended OK at 10.
- A rotator disconnect and reconnect reported `CALRESTART` `MOT2` 0 and no notice, at 10°.

This was not a `--hw` run, so there is no hardware record.

## Final test summary (3.0.0.39)

- Simulated tests: 71 run, 71 passed: `2026-10-24 02:22 3.0.0.39 linux x64 simulator 71/71 OK`.
- Hardware: the requester's ESATTO2 with ARCO, see the checks above.

## USB match pattern with VID/PID (3.0.0.40, 2026-10-24, task 20261023235957-e03c-primaluce-usb-pattern)

The requester asked for the USB VID/PID in the serial port preference, as in the ZWO mount drivers
(`mount_asi`, `mount_lx200`), which register `0x03C3:0x4001`.

- The requester's SESTO SENSO 3 and ESATTO2 both appear as `10c4:ea60`, "CP2102N USB to UART Bridge
  Controller": the Silicon Labs default identity, not a PrimaLuce one. A VID/PID pattern alone would also
  pick any other CP210x bridge (CP2102, CP2104, ...), so the VID/PID is added to the existing "CP2102N"
  product pattern. All fields of a pattern must match (`indigo_usbserial_match_index()`), so the preference
  is now narrower than before: a CP2102N name behind another VID/PID no longer matches.
- Windows: the port list fills only the friendly name and no VID/PID (`indigo_usbserial_utils.c`), so the
  pattern does not match there, as before for the product string unless the friendly name contains it. The
  port is chosen by hand on Windows. This is a framework limitation and is not changed here.

Regression test `usb_match_pattern`, confirmed to fail without the change: the attached focuser registers one
pattern; a CP2102N name behind `0403:6001` and a CP2104 behind `10c4:ea60` do not match, and a CP2102N behind
`10c4:ea60` does.

## Final test summary (3.0.0.40)

- Simulated tests: 72 run, 72 passed: `2026-10-24 03:01 3.0.0.40 linux x64 simulator 72/72 OK`.
- Hardware (follow-up, task 20261024002000-64d7-primaluce-usb-pattern-hw): with the requester's SESTO SENSO 3 on
  `/dev/ttyUSB0` (`10c4:ea60`, "CP2102N USB to UART Bridge Controller") and an Arduino Micro on `/dev/ttyACM0`
  (`2341:8037`), `indigo_server -vvv indigo_focuser_primaluce` logged
  "indigo_select_matching_usbserial_device(): Selected new port for 'PrimaluceLab Focuser': auto:///dev/ttyUSB0",
  and connecting through that port identified the SESTOSENSO3. The ESATTO2 identity is the same (earlier logs).
  This was not a `--hw` run.
