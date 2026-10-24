# indigo_aux_alto: development record

New driver for the PrimaLuce Lab ALTO cover, task 20261024120152-52cb-primaluce-alto-driver (started 2026-10-24),
the follow-up of the GIOTTO task 20261024092953-7ee4-primaluce-giotto-alto-drivers. The requester chose a driver of
its own because the ALTO and the GIOTTO are separate USB devices and the generator shares one connection between the
devices of a driver.

## Audit of the starting point

- **No INDIGO driver existed.** `primalucelib` had five cover calls taken from the INDI `alto.cpp` (position get/set,
  motor state, stop, calibration step), unit-tested against scripted replies only and used by no driver.
- **Protocol sources:** there is no ALTO document. Sources are the INDI `alto.cpp` and `primalucacommandset.cpp`, the
  common PrimaLuce JSON envelope (`PROTOCOL.md`), and the requester's ALTO1, probed on 2026-10-24 (below).
- **Transport:** the shared `primalucelib` JSON link (USB serial 115200 8N1, or HTTP).
- **Platforms:** generated portable C on `primalucelib`; registered like `indigo_aux_giotto`. Only Linux is available
  to run here.

## Hardware observations (requester's ALTO1, 2026-10-24)

Unit: `MODNAME` "ALTO1", serial `ALTO100490`, `SWAPP` 1.03.11, `SWWEB` 1.01, Silicon Labs CP2102N `10c4:ea60`. Probed
on `/dev/ttyUSB0`, first read-only, then with the requester's full access; raw transcripts are in the session
scratchpad (`alto/hw/`).

| Area | Observation |
| --- | --- |
| Full state | `MOT1` holds `POSITION` (0–100), `ABS_POS_STEP`/`POSITION_STEP`, `STATUS` (`BUSY`, `MST` stop/acc/CstSpeed/dec, `DIR` In/Out), `NTC_T` (number), `CAL_STATUS`, `CAL_MINPOS` 0, `CAL_MAXPOS` 239608, `CAL_HOMEP`, `SPEED`, `TVAL_*` torque values with `TVAL_MIN`/`MAX` and defaults, `HOLDCURR_STATUS`, `ERROR`. Top level: `VIN_12V`, `VIN_USB`, `CALRESTART.MOT1`, `PRESET_1`–`_3` (`NAME`, `M1POS`), LEDs, LAN fields. No light. |
| Combined get | `{"get":{"MOT1":{"POSITION":"","ABS_POS_STEP":"","STATUS":""}}}` and `{"get":{"VIN_12V":"","VIN_USB":"","MOT1":{"NTC_T":""}}}` answer in one reply. |
| Unknown key | Top level `"ERROR: Obj's name not found or invalid command"`; inside `MOT1` `"Error: parameter not found"`. |
| Move | `{"set":{"MOT1":{"POSITION":0}}}` → `"done"`. 0 is closed, 100 open. About 7400 steps/s: a full stroke takes about 34 s with acceleration and deceleration. `POSITION` changes every poll, `ABS_POS_STEP` only about every 0.5 s; the first stopped readback can still carry the previous step count, `POSITION` is already final. 50 lands on step 119804 (half of 239608). |
| Invalid targets | 101, -1, 50.5 and `"x"` are all acknowledged `"done"`; after them the arm started moving inward. The controller does not validate `POSITION`, so only integers 0–100 may be sent. |
| Retarget | A new `POSITION` during a move decelerates and stops at the new target. |
| Stop | `cmd MOT_STOP` → `"done"`, decelerated stop within about 1 s. `cmd MOT_ABORT` → `"done"`, immediate stop. |
| `SPEED` | `set` is acknowledged but `get` stays 0 and the travel time does not change: it is the current speed, not a setting. |
| Presets | 3 presets, `M1POS` in steps. As on the GIOTTO, a position is refused (`"Error: invalid command"`) until the preset has a name. `cmd PRESET n` moves the cover there; an empty preset gives `"Error: empty preset"`, preset 4 `"Error: invalid command"`. |
| LEDs, log level | `DIMLEDS` acknowledged as `{"cmd":{"cmd":"done"}}`; `LOGLEVEL` prints `[mLogs_DefaultSetup]` before `"done"`. |
| Calibration | Not probed: the requester will recalibrate later. The driver follows INDI (`CAL_ALTO` `Init`, `Close_*`/`Open_*`, `MOT_STOP`, `StoreAsClosedPos`, `StoreAsMaxOpenPos`) and marks it unverified. |

Everything was restored: the cover ends fully open (100, step 239608), preset 1 empty with position 0, LEDs on, log
level "errors".

## Design

- One AUX device "PrimaluceLab ALTO", interface `INDIGO_INTERFACE_AUX_DUSTCAP`, generated from `indigo_aux_alto.driver`,
  linked with `primalucelib`.
- **Connect:** `alto_open()` requires a `MODNAME` starting with "ALTO", switches the log output off and reads the full
  state; it releases the port itself on failure. Model, firmware, serial number go to `INFO`; `CALRESTART.MOT1` and
  `MOT1.ERROR` are reported.
- **`AUX_COVER`:** OPEN moves to 100, CLOSE to 0; BUSY while the cover moves, OK at the end. Neither item is on when the
  cover stands between the ends.
- **`X_COVER_POSITION`:** 0–100 %, BUSY with the progress, OK at the target, ALERT when the cover stops elsewhere.
- **`X_COVER_ABORT`:** `MOT_STOP` (decelerated, so the swinging arm is not jerked), ALERT on the cover properties.
- **Motion:** one finalizer polls position and motor state every 0.5 s; it ends on `MST` "stop". A cover that reports
  motion without progress for 5 s, or a move longer than 60 s, is stopped and ends ALERT; a lost readback is retried.
- **Idle polling** every 2 s publishes motion started elsewhere (web page, preset recall from another client).
- **Presets:** `X_PRESETS` (positions of presets 1–3 in %, converted with `CAL_MAXPOS`), `X_PRESET_NAMES`,
  `X_PRESET_RECALL` (moves the cover, through the same finalizer).
- **Calibration (unverified):** `X_CALIBRATION` START (`Init`), STORE (closed position first, then open position,
  which ends it), CANCEL; `X_CALIBRATION_MOTION` CLOSE_SLOW/CLOSE_FAST/OPEN_SLOW/OPEN_FAST/STOP, accepted only
  during a calibration. `X_CALIBRATION` stays OK between the steps (as the focuser driver's calibration does), so
  that STORE and CANCEL are accepted; the messages say what comes next. Moves are refused during a calibration.
- **Move handlers** schedule `cover_motion_finalizer` themselves and publish BUSY, ALERT and the end; the
  generator's OK prologue and final update are therefore suppressed for `AUX_COVER` and `X_COVER_POSITION`.
- **Settings and state:** `X_LEDS`; `X_STATE` (motor temperature, 12 V and USB supply) every fifth idle poll.
  Hold current and torque values are not exposed: switching the hold current off may let the arm fall.
- **Disconnect:** a running move is stopped first.
- **USB preference:** the CP2102N pattern with `10c4:ea60`; the model check refuses a GIOTTO or a focuser.

## Hardware-test decision

Hardware testing on the requester's ALTO1 with full access: open, close, partial position, abort, presets, LEDs,
reconnect, and the opt-in `test_aux_alto_hw` suite through `run_driver_test.py aux_alto --hw`. Calibration is left
for a later session with the requester watching. Physical hot-plug only if requested.

## Plan

| Step | Change | Verification | State |
| --- | --- | --- | --- |
| 1 | Baseline: `primalucelib` unit tests at master `6a62fa785` | 28/28 | done |
| 2 | `primalucelib`: cover motion poll, environment, presets, acknowledged refusals; preset name and recall shared with the panel | unit tests | done: new case "cover motion, presets and refusals", 29/29; the GIOTTO suite rerun after the shared helpers |
| 3 | Simulator `alto_simulator` (serial_motion based, 5 s full stroke) modelled on the observations | manual exchange | done: the probe sequence gives the same replies |
| 4 | `indigo_aux_alto.driver`, generated outputs | builds | done: no warnings; generated move handlers checked |
| 5 | Integration suite `test_aux_alto_simulator.c` | all pass; failure cases proven by mutation | done: 28/28 after found defect 1; ten mutations each failed their case |
| 6 | Registration: Makefiles, server, Windows, Xcode | builds; project files parse | done |
| 7 | Documentation: `PROPERTIES.md`, `PROTOCOL.md`, `MIGRATION_STATUS.md`, `README.md` (approval) | review | done; `README.md` added with the requester's approval (2026-10-24) |
| 8 | Opt-in hardware suite `test_aux_alto_hw.c` | on the unit | done: recorded `2026-10-24 16:49 3.0.0.1 linux x64 ALTO1 9/9 OK` |
| 9 | Recorded runs | `run_driver_test.py aux_alto` and `--hw` | done: `2026-10-24 16:47 3.0.0.1 linux x64 simulator 28/28 OK`, `2026-10-24 16:49 3.0.0.1 linux x64 ALTO1 9/9 OK` |

## Scenario mapping

| Scenario (DRIVER_TESTING_RULES common and AUX; motion scenarios adapted from the focuser standard) | Simulator case | Hardware case |
| --- | --- | --- |
| Interface bit, metadata, connect sequence, INFO | `metadata` | `alto_publishes_the_property_contract` |
| USB preference | `usb_match_pattern` | not repeated (same pattern as the focuser, checked on hardware) |
| Property contract, connect values, ends shown | `property_contract`, `connect_closed` | `alto_publishes_the_property_contract` |
| Wrong device, silent handshake, calibration flag, no travel | `unsupported_device`, `handshake_timeout`, `needs_calibration`, `uncalibrated` | not applicable |
| INIT/SHUTDOWN, shutdown refused, reconnect, second disconnect | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` | `alto_reinitializes`, `alto_rejects_shutdown_while_connected`, `alto_reconnects` |
| Open/close with progress, partial position, idle polls agree | `close_and_open`, `partial_position` | `alto_closes_and_opens`, `alto_moves_to_partial_positions` |
| Abort mid-move (decelerated), while idle, refused | `abort_motion`, `abort_while_idle`, `abort_refused` | `alto_aborts_a_move` |
| Refused move, stall, lost and persistent readback failures | `move_refused`, `stalled_move`, `motion_readback_failures` | not reproducible without a fault |
| External motion, overlap refused, disconnect during motion | `external_motion_observed`, `overlap_refused`, `disconnect_during_motion` | not done (needs the web page / a second client) |
| Idle poll failure, environment poll and failure | `poll_failure`, `environment_polled` | environment read in the contract case |
| Presets (named rule, steps conversion, recall moves, empty recall, readback after reconnect) | `presets` | `alto_uses_presets` |
| LEDs with readback after reconnect, unanswered request | `leds` | `alto_changes_the_leds` |
| Calibration sequence and cancel | `calibration` (simulator model of the INDI sequence) | not run; recalibration planned with the requester |
| Transport loss | `transport_loss` | not done (no hot-plug requested) |

Mutation checks (each change made to the driver, the named case run, the driver restored): abort sends no stop
(`abort_motion`), no stall check (`stalled_move`), persistent readback failure ignored (`motion_readback_failures`),
disconnect does not stop (`disconnect_during_motion`), external motion not followed (`external_motion_observed`), moves
allowed during calibration (`calibration`), unnamed-preset message removed (`presets`), uncalibrated guard removed
(`uncalibrated`), ends shown between (`partial_position`), refused move treated as started (`move_refused`). All ten
failed as expected.

## Hardware runs

- 2026-10-24, development run of `test_aux_alto_hw` (`ALTO_HW_PORT=/dev/ttyUSB0 make test-aux-alto-hw`), 9/9: the
  cover moved to 80 and 60 %, closed and opened fully, stopped at 88 % when aborted on its way to 20 %, went to a 70 %
  preset, refused a position on an unnamed preset with "Preset #2 needs a name before its position can be set", and kept
  90 % across a reconnect. A probe afterwards found it fully open (step 239608), LEDs on, presets empty, as before.

## Found defects

1. **Calibration STORE refused by the framework** (reproduced by the simulator case `calibration` before any hardware use).
   - Impact: START left `X_CALIBRATION` BUSY, so the framework refused the STORE that has to follow; a calibration could
     never be completed.
   - Root cause: the BUSY guard of `INDIGO_COPY_VALUES_PROCESS_CHANGE` rejects a change to a BUSY property.
   - Fix: the property is OK between the steps, with the next step in the message, as in the focuser driver.
   - Regression test: `calibration` (START and the first STORE end OK, the second STORE completes it).

## Final test summary (3.0.0.1)

- Simulated tests: 28 run, 28 passed: `2026-10-24 16:47 3.0.0.1 linux x64 simulator 28/28 OK`.
- Hardware tests: 9 run, 9 passed on the requester's ALTO1 (firmware 1.03.11): `2026-10-24 16:49 3.0.0.1 linux x64 ALTO1 9/9 OK`.
- Library unit tests: 29 run, 29 passed (`test_primalucelib`); the GIOTTO simulator suite 25/25 after the shared preset helpers.
- Not verified: the calibration on hardware (planned with the requester), Windows and macOS builds, WiFi, physical hot-plug.
