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

## Poll versus pending request (3.0.0.2, 2026-10-24, task 20261024163549-1b30-alto-request-versus-poll)

The flaw the repository audit found in the GIOTTO driver (FND-20261024121920-3bd2-repository-audit-021, reviewed source
before this driver existed) was present here too.

2. **An idle poll interfered with a queued move request** (reproduced, then fixed).
   - Impact: a request accepted while an idle poll was in flight (the framework copies it into the property, marks it BUSY
     and queues the handler) was undone by that poll: when the poll saw motion started elsewhere it began following it, so
     the request's handler then refused with "The cover is moving"; when it saw a different position it published that
     position over the request's target. Reproduced: a position of 40 requested during a poll that saw an external move to
     70 was never sent.
   - Root cause: outside a move the idle poll did not check whether a request was pending.
   - Fix: outside a move, a BUSY `AUX_COVER` or `X_COVER_POSITION` means a pending request; the poll then publishes
     nothing, follows no motion and posts no ALERT, and the request's own move takes over.
   - Simulator: `external <position> slow` delays the reply to the poll that sees the move by 400 ms.
   - Regression test: `request_versus_poll` (40 sent once, the cover ends OK at 40 with target 40); it failed on 3.0.0.1.
   - Hardware: not rerun; the race needs the simulator's delayed reply, and the motion the hardware suite covers is unchanged.

## Final test summary (3.0.0.2)

- Simulated tests: 29 run, 29 passed: `2026-10-24 19:36 3.0.0.2 linux x64 simulator 29/29 OK`.
- Hardware tests: 0 run for this change (the last hardware run, 9/9, was on 3.0.0.1).

## Token lookup bound in primalucelib (3.0.0.3, 2026-10-24, task 20261024164718-0e69-primalucelib-token-bounds)

FND-20261024121920-3bd2-repository-audit-017: a lookup in a reply that used all 1024 JSON tokens could read past the
token array of the shared `primalucelib` (reproduced with AddressSanitizer at `primalucelib.c:399`). The library now
bounds every lookup by the number of tokens the parser filled; the regression test is the library unit case "lookups
stay within a full token array". This driver links the library, so its version is raised; nothing else in the driver
changed. The recorded simulator run of this version is in README.md.

## WiFi configuration (3.0.0.4, 2026-10-24, task 20261024194838-6cb6-primaluce-panel-cover-wifi)

The driver could use the cover's WiFi (an `http://host` port) but not configure it: the specification of 3.0.0.1 left
the WiFi configuration out. The cover reports the same network keys as the PrimaLuce focusers (`LANCFG`, `WIFIAP`,
`WIFISTA`), so it gets the focuser's properties under the same names:

- `X_WIFI` (OFF, AP, STA) through `primalucelib_select_wifi()`: OFF and AP switch the access point in the `ap` mode; a
  change between station mode and the access point writes `LANCFG` and restarts the cover, waiting over USB until it
  answers again. Over HTTP the driver says that the cover leaves its network. Read from the full state at connect.
- The mode change is refused while the cover moves or a calibration runs, because the restart would stop either;
  switching the access point alone is served during a move.
- `X_WIFI_AP` and `X_WIFI_STA` write the credentials; the library refuses an SSID over 32 characters (or an empty access
  point SSID), an access point password outside 8 to 63 characters, and a station password over 63, with the reason as
  the message. `"` and `\` are sent escaped (merged from `indigo_focuser_primaluce` 3.0.0.42); the GIOTTO firmware
  decodes them.

Hardware (requester's ALTO1, application firmware 1.03.11, over USB, 2026-10-24; raw probe and driver session):
- `WIFISTA` and `WIFIAP` writes are acknowledged per field (`{"SSID":"done","PWD":"done"}`) and read back; `AP_SET_STATUS`
  `off`/`on` is acknowledged and read back as `WIFIAP.STATUS`.
- The firmware checks nothing: a 3 character access point password and the mode `xyz` were stored and acknowledged
  (both restored at once, before any restart). The library now refuses what WiFi cannot use before sending it.
- `LANCFG` `sta` plus `REBOOT`: acknowledged, ESP32 boot log, `MODNAME` answered again after about 2 s, `LANCFG` `sta`
  with `WIFIAP.STATUS` `off`; back to `ap` the same way. The calibration (`CALRESTART` 0) and the position were kept.
- The access point reports `off` for about 4 s after the controller answers and `on` from about 6 s after `REBOOT`; a
  reconnect within that time shows OFF.

Simulator: stores `LANCFG` and the credentials as sent, applies `LANCFG` at `REBOOT` (boot log, 1 s without answers,
motion stopped), reports `WIFIAP.STATUS` `off` in station mode, and answers `AP_SET_STATUS`.

| Scenario | Test |
| --- | --- |
| WiFi properties and connect values | `property_contract` |
| Credentials written, read back on reconnect, refused before sending | `wifi_credentials` |
| STA and back with one restart each, OFF/AP without restart, cover served after a restart, refused mode | `wifi_modes` |
| Mode change refused while moving and during a calibration, access point switch served during a move | `wifi_restart_waits_for_the_cover` |
| Library: mode selection, restart only on change, credential limits | `test_primalucelib` |

Not verified: the HTTP path of a mode change (no network access to the cover here), whether a changed access point
password takes effect before the next restart, Windows and macOS builds.

## Final test summary (3.0.0.4)

- Simulated tests: 32 run, 32 passed: `2026-10-24 23:14 3.0.0.4 linux x64 simulator 32/32 OK`.
- Hardware tests: the WiFi session above (raw probe and driver through `indigo_prop_tool`); the recorded hardware suite was not rerun.
