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
| D9 | An aborted, stalled or failed move ended ALERT with the requested target, not at the stopped position. | Every move that ends short of its target sets target to the measured position. | `abort_motion`, `stalled_move` |
| D10 | Disconnecting during a move let the motor run on: the port closed without a stop. | `on_disconnect` sends `MOT_ABORT` (fallback `MOT_STOP`) while a move, an uncommanded motion or a focuser calibration runs. | `disconnect_during_motion` |
| D11 | `FOCUSER_ABORT_MOTION.ABORT_MOTION` stayed ON after the abort, and an abort while idle sent `MOT_ABORT`. | The item is released in every path; an idle abort sends nothing and ends OK, or ALERT when the controller did not answer the last request. | `abort_while_idle`, `abort_refused` |
| D12 | An abort that overtook a queued move was followed by that move. | The abort marks the pending move, which is then never sent and ends ALERT. | `abort_overtakes_queued_move` |
| D13 | The position was only read during the driver's own moves: uncommanded motion (hand keypad, a move running at connect) and a failed idle read were never published. | The position is polled every 2 s while no move of the driver runs (0.5 s while it changes); uncommanded motion is BUSY with the target following the measurement and OK once it stops; a failed or incomplete read is ALERT with the last position and the next good read restores OK. A poll whose reply arrives after a request was accepted leaves the request alone. | `external_motion_observed`, `hand_keypad_motion`, `position_poll_failure`, `request_versus_poll` |
| D14 | `EXT_T` -127 (no probe, observed on the attached SESTO SENSO 2) was published as a temperature, and a failed temperature poll published nothing. | -127 is published IDLE; a failed poll publishes `FOCUSER_TEMPERATURE` and `X_STATE` ALERT with the last values; the next poll restores OK. | `no_probe`, `status_polling` |
| D15 | A single lost readback during a move ended it ALERT while the motor kept running; a stalled motor (MST `move`, position unchanged) stayed BUSY forever. | One or two lost readbacks are retried; a third stops the motor and ends ALERT. A motor that does not advance for 25 polls (5 s) is stopped and ends ALERT. | `move_readback_failures`, `stalled_move` |
| D16 | An absolute move kept `FOCUSER_STEPS` OK, a refused move kept the requested value as position, and the controller's refusal reason never reached the client. | Both properties are BUSY for every move; a refused move ends both ALERT at the position read last, with "Move refused: <reason>". | `absolute_move`, `move_command_failures` |
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
| Abort mid-move ending ALERT at the stopped position, idle abort, refused stop and retry, abort overtaking a queued move | `abort_motion`, `abort_while_idle`, `abort_refused`, `abort_overtakes_queued_move`, `test_focuser_primaluce_motion` |
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

## Final test summary

- Simulated tests: 54 run, 54 passed (recorded run of 3.0.0.22, macOS arm64).
- Hardware tests: 18 run, 18 passed on the SESTO SENSO 2 over USB serial and 18 run, 18 passed over
  WiFi (3.0.0.22, macOS arm64, with `PRIMALUCE_HW_CALIBRATE=1`; the stored calibration was read and
  restored in both runs).
