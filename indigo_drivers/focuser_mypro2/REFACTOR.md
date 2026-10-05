# myFocuserPro2 refactoring and validation record

Status: migrated to `indigo_generator` on 2026-09-20.

## Current-state audit (2026-09-19)

### Architecture and implementation

- `indigo_focuser_mypro2.c` is a hand-written 1204-line single-device focuser driver, version `0x0300000B`, device name `myFocuserPro2`, driver label `myFocuserPro2 Focuser`.
- Transport is `indigo_uni_open_serial_with_speed()` at the rate `DEVICE_BAUDRATE` selects (default 9600), or `indigo_uni_open_url()` for an `mfp://`, `tcp://` or `udp://` URL. `mfp_command()` discards pending input, writes the request and reads a `#`-terminated reply with `indigo_uni_read_section()`, then sleeps 50 ms before returning. On a failed transaction over a TCP handle it queues `network_disconnection()`, which disconnects the device and reports an alert.
- All work already runs on the device handler queue through `indigo_execute_handler()`; the abort handler already uses `INDIGO_TASK_PRIORITY_URGENT`. There is no mutex.
- Two independent polling callbacks: `focuser_timer_callback` reschedules itself every 0.5 s while a move runs, and `temperature_timer_callback` runs every 2 s and drives temperature compensation.

### Protocol

`myfocuser2_command_subset.txt` in the driver directory documents the subset used: `:00#` position, `:01#` moving, `:04#` firmware string, `:05xxxxxx#` goto, `:06#` temperature, `:07/:08` maximum position, `:11/:12` coil power, `:13/:14` reverse, `:15x#` speed, `:27#` halt, `:29/:30` step mode, `:31xxxxxx#` sync, `:48#` save to EEPROM, `:71/:72` settle time and `:73`…`:80` backlash in/out with their enables.

### Public properties

`FOCUSER_SPEED` (0…2), `FOCUSER_STEPS`, `FOCUSER_DIRECTION`, `FOCUSER_POSITION` (0…2000000, step 100), `FOCUSER_ON_POSITION_SET`, `FOCUSER_LIMITS` (maximum 1000…2000000, minimum pinned to 0), `FOCUSER_ABORT_MOTION`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_BACKLASH` (0…255), `FOCUSER_TEMPERATURE`, `FOCUSER_COMPENSATION` with its threshold item, `FOCUSER_MODE`, and the driver-defined `X_STEP_MODE` (8 microstepping switches, reduced to 2 for a Gemini board), `X_COILS_MODE` and `X_SETTLE_TIME`. The three driver-defined properties are connection-dependent and are the ones saved by `CONFIG.SAVE`.

`FOCUSER_MODE` is unusual: switching to automatic deletes `FOCUSER_ON_POSITION_SET`, `FOCUSER_SPEED`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_DIRECTION`, `FOCUSER_STEPS`, `FOCUSER_ABORT_MOTION` and `FOCUSER_BACKLASH` and republishes `FOCUSER_POSITION` read-only; switching back to manual restores all of them.

### Defects, risks and lifecycle findings

- `focuser_timer_callback()` reads `moving` from `mfp_is_moving()` even when that call failed, so the value is uninitialised on the error path. Recorded as `DRV-141`.
- `mfp_command()` sleeps 50 ms inside every successful transaction, adding about 1.4 s to the connection sequence and 0.1 s to every polling cycle.
- `focuser_position_callback()` sleeps 0.5 s on the device queue after a sync.
- `compensate_focus()` uses `PRIVATE_DATA->current_position` from a read whose failure it only logs.
- `mfp_get_info()` rewrites the reply in place and returns false unless it finds both a `\n` and a `\r`, so a controller that answers `:04#` with a single line reports no model or firmware.
- `focuser_limit_callback()` sets `INDIGO_OK_STATE` in the *failure* branch of the maximum-position readback. Recorded as `DRV-142`.
- `X_SETTLE_TIME` readback assigns only `number.target`, so the published value never follows the controller. Recorded as `DRV-143`.
- Nothing rejects a move request while another move is in progress.

### Platforms, build and packaging

Linux, macOS and Windows; `indigo_focuser_mypro2.vcxproj` exists. No `Makefile.inc`.

### Test assets

- `focuser_mypro2_simulator/focuser_mypro2_simulator.c`, a PTY simulator with **no motion model**: `:01#` reports `I1#` once and snaps the position to the target at the same time, so no move ever takes time.
- `indigo_test/integration/test_focuser_mypro2_simulator.c`, a single smoke scenario.
- Coverage gaps: real motion and progress, abort of a running move, every failure path, the Gemini step-mode reduction, the missing temperature sensor, temperature compensation, reconnect and the network transport.

## Baseline (2026-09-19, macOS 15 arm64, universal x86_64+arm64 build)

- `make -C indigo_drivers/focuser_mypro2 -f ../../Makefile.drv` — succeeded, no warnings.
- `make -C indigo_test build/integration/test_focuser_mypro2_simulator` — succeeded.
- `./build/integration/test_focuser_mypro2_simulator` — 1 scenario, 1 passed, 6.5 s.

## Hardware-test decision

Hardware testing will **not** be performed. No myFocuserPro2 controller is available in this environment, and no hardware validation is claimed anywhere in this record.

## Migration plan and results

1. **Rebuild the simulator on `serial_motion.h`.** — **Done.** A move now takes real time at a rate derived from the motor speed the `:15#` command selects, `:01#` reports `I1#` while it runs, and the profiles `normal`, `gemini`, `no-sensor`, `silent`, `moving-error`, `maxpos-error` and `temperature-drift` are available. The simulator's reply trace also gained the missing newline that previously ran replies and the next request together on one line.
2. **Grow the characterization suite.** — **Done.** One smoke scenario became 16. Thirteen passed against the original driver; `controller_settings`, `moving_query_fails` and `max_position_readback_fails` are the dedicated reproducers of `DRV-144`, `DRV-141` and `DRV-142` and were recorded as expected baseline failures. `abort_overtakes_start` reproduces `DRV-145` intermittently, failing in two of three runs against the original driver and passing in the slower traced run.
3. **Capture the normalized reference trace.** — **Done.** `MYPRO2_TEST_TRACE=1` logs every request and reply. The comparison keeps the ordered request stream and collapses each maximal run of the `:00#`, `:01#` and `:06#` polling commands into one `POLL[...]` token, because the number of polling cycles inside a timed move is not deterministic.
4. **Write `indigo_focuser_mypro2.driver`** and generate the outputs. — **Done.**
5. **Build, re-run and compare.** — **Done.** See *Verification*.
6. **Register and update the status documents.** — **Done.**
7. **Final audit.** — **Done.**

## Intentional differences from the original driver

- **Motion completion is a finalizer.** `focuser_timer_callback` becomes `motion_finalizer`, scheduled only while a move is outstanding. The original also started it once at the end of every connection even though nothing was moving, which cost one `:01#` and one `:00#` per connection; that idle poll is gone. The temperature poll becomes the generated `on_timer` callback and starts immediately instead of one second after the connection.
- **Sync no longer sleeps on the queue.** `:31#` needs a moment before the new coordinate can be read back. The original slept 0.5 s inside the change handler, which blocked the device queue; the readback now runs in a `sync_finalizer` scheduled 0.5 s later, and the property is BUSY until it completes. The command order is unchanged.
- **Move requests while a move runs.** `INDIGO_COPY_VALUES_PROCESS_CHANGE` rejects a `FOCUSER_POSITION` or `FOCUSER_STEPS` change while the property is BUSY. The original replaced the target of a running move instead.
- **Response buffer.** The 100-byte stack buffer in each helper became one buffer in the private data.
- **Range checks removed.** `focuser_position_callback()` and `focuser_steps_callback()` rejected targets outside the item range; the framework validates them before the handler runs.
- **`:04#` parsing.** The original required the reply to contain both a newline and a carriage return and returned failure otherwise, so a firmware answering `F<board> <version>#` on one line reported no model. The separators are now simply normalised to spaces.
- **Trace differences.** Three in the non-polling command stream, all intended: the missing `:05090000#` that the original sent *after* the halt in `abort_overtakes_start` (`DRV-145`), and two blocks of commands from `controller_settings` and `moving_query_fails`, which only the migrated driver runs to the end.

## Found defects

- `DRV-141` (`focuser_timer_callback`, reproduced). Observable impact: when `:01#` failed the driver read an uninitialised local `moving`, and with the value it happened to see it converted the failed poll into `INDIGO_OK_STATE`, overwriting the alert it had just set. A move whose progress could not be read was reported as finished. Root cause: `bool moving;` was declared without an initialiser and only assigned on the success path of `mfp_is_moving()`. Fix: `motion_finalizer` treats a failed `:01#` or `:00#` as an alert, publishes it and stops polling. Regression test: `moving_query_fails` over the `moving-error` profile, which requires the alert and requires the next move request to be accepted and reported the same way.

- `DRV-142` (`focuser_limit_callback`, reproduced). Observable impact: a failed `:08#` readback set `INDIGO_OK_STATE`, so a maximum position the controller never confirmed was reported as applied. Root cause: the success and failure branches were swapped. Fix: both a failed `:07#` and a failed `:08#` set `INDIGO_ALERT_STATE`, and the published value is the last one the controller confirmed. Regression test: `max_position_readback_fails` over the `maxpos-error` profile.

- `DRV-144` (`focuser_limit_callback`, reproduced). Observable impact: a *successful* limits change left `FOCUSER_LIMITS` in `INDIGO_BUSY_STATE` forever, because the handler only ever assigned a state in its two failure branches. Root cause: the same swapped branches; nothing set OK on success. Fix: the generated handler sets `INDIGO_OK_STATE` before the block and the block only downgrades it. Regression test: the limits part of `controller_settings`.

- `DRV-145` (`focuser_abort_callback`, reproduced intermittently). Observable impact: an abort issued while a move request was still queued cancelled only the polling callback, so the driver halted the controller and then started the move it had been told to abandon. The original trace shows `:27#` followed by `:05090000#`. Root cause: the abort cancelled `focuser_timer_callback` only. Fix: the abort also cancels the pending `focuser_position_handler`, `focuser_steps_handler` and `sync_finalizer`. Regression test: `abort_overtakes_start`, which aborts immediately after requesting a 90000 step move and requires the focuser to stay put; it fails against the original driver when the abort wins the race, which it did in two of three untraced runs.

- `DRV-146` (`focuser_steps_callback`, source audit only, fixed). Observable impact: a relative inward move larger than the current position computed `current - steps` in `uint32_t`, so it wrapped to a value near 2^32, was clamped to `FOCUSER_POSITION_ITEM->number.max` and sent the focuser to the far end of its travel instead of to the near limit. Root cause: unsigned arithmetic on a difference that can be negative. Fix: the target is computed in `long long` and then clamped. No regression test: reproducing it needs a move to the maximum position, which the simulator would have to run through at its modelled step rate; the clamp itself is covered by `relative_move`.

- `DRV-143` (`foccuser_x_settle_time_callback`, source audit only, fixed). Observable impact: the settle time read back from the controller was written to `number.target` only, so a value the controller adjusted was never shown to clients. Root cause: a missing assignment to `number.value`. Fix: `mypro2_update_settle_time()` assigns both. No regression test: the simulator accepts every value the driver sends, so the readback never differs from the request.

- `DRV-147` (connection handler, source audit only, fixed). Observable impact: `X_STEP_MODE_PROPERTY->count` was reduced to 2 for a Gemini board and never restored, so a later connection of the same driver instance to a non-Gemini board published only two of the eight microstepping modes. Root cause: the reduction was applied without a matching restore. Fix: `on_connect` resets the count to 8 before it evaluates the board name, the same way `focuser_dsd` restores its model-dependent capabilities.

## Verification (2026-09-20, macOS 15 arm64)

- `../../build/bin/indigo_generator indigo_focuser_mypro2.driver` run twice: the second run reproduces the first output byte for byte.
- `make -C indigo_drivers/focuser_mypro2 -f ../../Makefile.drv` — universal x86_64 + arm64, no errors, no warnings.
- `./build/integration/test_focuser_mypro2_simulator` — 16 scenarios, 16 passed. The same suite against the original driver fails the three deterministic defect reproducers, and `abort_overtakes_start` in addition when the abort wins its race.
- Normalized trace comparison — the non-polling request stream differs in three places, each explained above; the polling runs differ only where the migrated driver no longer performs an idle motion poll.
- AddressSanitizer: the generated driver compiled with `-fsanitize=address -O1` and linked into the same suite — 16 scenarios, 16 passed, no sanitizer report. The framework library is still not instrumented.
- No `MAX_DEVICES` override, no build products in the diff, driver version raised from `0x0300000B` to `0x0300000C`.
- Linux and Windows builds were not run in this environment; only the macOS universal build is validated.

## Focuser testing rules alignment (2026-10-05)

The regression suite was checked against the "Focuser Drivers" chapter of `indigo_test/DRIVER_TESTING_RULES.md` (commit fa5f64839) and extended where a relevant rule was not verified. The simulator gained a request journal (`INDIGO_MYPRO2_EVENTS`), a one-shot fault file (`INDIGO_MYPRO2_FAULT`: `silent`, `ignore`, `garbage`, `value <reply>`, `overlong`, `truncated`, `split`, `slow`, `close`, `external <target>`, `stall`, `model <board>`) and the `custom` (every setting off the driver defaults) and `moving` (a move running at connect) profiles. The permanent `moving-error` and `maxpos-error` profiles were replaced by one-shot faults, because the connect sequence now reads `:01#` and `:08#` and a permanently failing query refuses the connection. A Gemini board starts in half step, the only modes it has. The suite runs with a private `HOME` and plans its forked cases.

Driver version 3.0.0.13 → 3.0.0.14.

### Defects found and fixed

- `MFP-1` Aborted move ended OK. User decision: an aborted move ends `FOCUSER_POSITION` and `FOCUSER_STEPS` ALERT at the stopped position with value equal to target. The abort handler now publishes ALERT with both set to the readback after `:27#`. Test: `abort_motion`, `abort_overtakes_start`.
- `MFP-2` Abort while idle or with the item OFF sent `:27#` and republished the motion properties. Now answered OK without a command. Test: `abort_while_idle`.
- `MFP-3` A failed `:01#`/`:00#` during motion ended ALERT but left the motor running. The finalizer now sends `:27#` before publishing ALERT. Test: `moving_query_fails` (also checks a later idle poll does not turn the failed move OK).
- `MFP-4` A motor reporting a move without progress was polled forever. Six polls without progress (3 s) stop it and end ALERT. Test: `stalled_move`.
- `MFP-5` `FOCUSER_POSITION` and `FOCUSER_STEPS` were capped at 2000000 regardless of the controller's maximum, so a GOTO or relative move past the travel sent the unreachable target and finished OK at another position. Both maxima now follow `:08#` at connect and every confirmed limits change (republished, since ranges travel with a definition); the framework clamps the GOTO on the wire. Test: `metadata`, `custom_state`, `limits_and_clamping`, `controller_settings`.
- `MFP-6` A `FOCUSER_LIMITS` maximum below the current position was written. Now refused ALERT without `:07#`, old value kept; a readback different from the request is an ALERT too. Test: `limits_and_clamping`.
- `MFP-7` A SYNC to the published value was dropped (the target-equals-position shortcut ran before the SYNC branch), and an unapplied `:31#` ended OK at whatever the controller reported. SYNC now always sends `:31#`, and a readback different from the requested value ends ALERT at the real position. Test: `sync_contract`.
- `MFP-8` A second motion request during a move was silently dropped by the BUSY guard (or accepted through the other property's `INDIGO_COPY_*` branch while only one was BUSY). `reject_change` on both motion properties refuses it ALERT with the held value and target; progress polls keep the published state so the refusal stays visible, and `motion_active` keeps the guard while a refused property shows ALERT. Test: `overlap_refused`.
- `MFP-9` Backlash, limits, reverse, compensation, mode and step mode changes during a move were applied (and reverse/backlash/step-mode commands sent). Now refused without a command. Test: `settings_refused_during_motion`.
- `MFP-10` Connect-time query failures were ignored, so a controller that answered only `:00#` was reported connected with driver defaults. Every query of the connect sequence (position, motion state, backlash, maximum, reverse, coils, step mode, settle time) is now mandatory: a failure refuses the connection, closes the port, and the next connect succeeds. `:04#` stays optional and its failure keeps the placeholders, which are now reset on every connect so a failed identification no longer shows the previous board. Test: `connect_refused`, `identity_and_model_change`.
- `MFP-11` The driver never polled the position while idle, so a hand-controller move or a move running at connect was not published. The temperature timer now also reads `:01#`/`:00#` while no move is running: a running move is followed BUSY then OK with target equal to the measured value, a changed position is published, a failed or malformed poll publishes ALERT with the last valid value and the next good poll restores OK. A request accepted while the poll was in flight owns the motion properties (the poll re-checks before publishing). Test: `external_motion`, `moving_at_connect`, `poll_failure_recovery`, `request_survives_poll`.
- `MFP-12` A reply that timed out before `#` or filled the buffer was parsed (`sscanf` does not check the terminator). Replies must now end in `#`. Test: `poll_failure_recovery` (`truncated`, `overlong`).
- `MFP-13` A relative move whose `:00#` start read failed was sent from the cached position. It now ends ALERT on both properties without `:05#`. Test: `relative_start_read_fails`.
- `MFP-14` Unacknowledged setting writes were reported OK: reverse was never read back, backlash only on failure, and a step mode, coils mode or settle time readback that differed from the request was published OK; a failed readback left the requested switch item shown. Every setting is now read back, a mismatch or failed readback ends ALERT showing the value the controller last confirmed. Test: `settings_failures`.
- `MFP-15` A temperature reading outside the DS18B20 range (other than the `-127` sentinel) was published and fed compensation. Now ALERT with the last valid value. Test: `temperature_failures`.
- `MFP-16` (source audit, no test) A compensation move whose position read or `:05#` failed still advanced the temperature reference, so the correction was lost. The reference now advances only when the move was started. Not reproducible hardware-free: the PTY write never fails and the position read cannot be failed deterministically between the temperature read and the move.

All new cases were run against the pre-fix driver (built from a copy of the 3.0.0.13 sources): 23 of 35 fail there; the passing ones are unchanged-behaviour cases (`silent_controller`, `reconnect`, `disconnect_during_motion`, `focuser_mode`, compensation, sensor and Gemini cases, `additional_instance`, `shutdown_rejected_while_connected`, `sync_and_goto`, `relative_move`).

### Rules not applicable

- Relative-only profile, model/firmware variants beyond the Gemini step-mode reduction, and the absence of temperature-dependent properties: the controller is absolute, all boards share the protocol subset, and `FOCUSER_TEMPERATURE`/`FOCUSER_COMPENSATION`/`FOCUSER_MODE` are always defined (a missing probe answers `-127`, covered by `no_temperature_sensor`).
- `FOCUSER_LIMITS` minimum and an empty interval: the minimum is pinned to 0 by the controller.
- Automatic mode device command and controller-computed compensation parameters: the driver computes compensation itself and does not use `:22#`–`:26#`.
- Homing, zeroing, calibration and other driver momentary switches: not implemented (`:28#` is unused).
- Controller refusal reasons: the protocol has no refusal replies; setting commands are never acknowledged, so readbacks decide.
- Shared controllers/hub: single-device driver; `ADDITIONAL_INSTANCES` is covered by `additional_instance`.
- Legacy unprefixed property names: the driver-specific properties were always `X_`-prefixed.
- Reversal implemented by the driver: the controller owns it (`:14#`/`:13#`).

### Open

- A refused or unconfirmed stop (`:27#` has no reply; the readback after it fails) ends `FOCUSER_ABORT_MOTION` ALERT and the next abort resends the stop (`stop_pending`), but no case injects it: the `:00#` after the stop cannot be failed without also hitting the concurrent motion poll.
- On disconnect the EEPROM save `:48#` follows the stop; it is a settings save, not motion, and was kept.
- The `mfp://` network transport remains untested (no sockets in the integration target).

## Test coverage map

| Scenario | Profile | Covers |
| --- | --- | --- |
| `metadata` | normal | Driver info, `X_` properties absent before connect, interface bit, class properties, INFO board and firmware, connect sequence, every value read back at connect, position/steps maxima from `:08#` |
| `custom_state` | custom | Connect publishes the controller's non-default position, maximum and ranges, backlash, settle time, step mode, coils, reverse, temperature and identity |
| `identity_and_model_change` | normal | Failed `:04#` keeps placeholders and connects; Gemini then non-Gemini reconnect defines exactly that board's step modes (`DRV-147`); placeholders reset after a later failure |
| `connect_refused` | normal | Handshake garbage, failed `:08#`, malformed `:29#` refused with nothing defined, then a working connect |
| `silent_controller` | silent | Failed open, no driver property left defined |
| `shutdown_rejected_while_connected` | normal | `INDIGO_DRIVER_SHUTDOWN` is BUSY while connected and the connection keeps working |
| `reconnect` | normal | Property withdrawal and redefinition, motion after reconnect |
| `additional_instance` | normal + custom | Second instance on its own port publishes its own controller, both instances move independently |
| `sync_and_goto` | normal | SYNC without a move, GOTO with both properties BUSY then OK and the command on the wire, GOTO to the held position without a command |
| `sync_contract` | normal | SYNC right after connect to the published value reaches the controller; an unapplied SYNC ends ALERT at the real position; retry accepted |
| `relative_move` | normal | Outward and inward relative moves, sign and units on the wire |
| `zero_and_short_moves` | normal | Zero step and GOTO to the current position end OK without a command; a 5-step move holds both properties BUSY |
| `limits_and_clamping` | normal | Limits change updates both ranges; GOTO beyond the limit clamped on the wire; relative moves past either end sent to that end; limit below the position refused without a command |
| `abort_motion` | normal | Mid-move abort: one stop, both properties ALERT, value = target = stopped position, two fresh polls at the same position, fresh move works |
| `abort_overtakes_start` | normal | `DRV-145`: urgent abort ends a queued move ALERT with one stop |
| `abort_while_idle` | normal | Idle abort and abort with the item OFF end OK without `:27#`, position untouched |
| `overlap_refused` | normal | Steps/GOTO during a move refused ALERT with values restored, running move ends at its target with one move command |
| `settings_refused_during_motion` | normal | Backlash, limits, compensation, reverse, mode, step mode refused during a move without a command; accepted again when idle |
| `external_motion` | normal | Hand-controller move published BUSY then OK, target = measured, no command; the next relative move starts from it |
| `moving_at_connect` | moving | Move running at connect published BUSY then OK without a command |
| `moving_query_fails` | normal | `DRV-141`: failed motion poll ends ALERT, stops the motor, never OK, not cleared by idle polls; next move works |
| `stalled_move` | normal | Motor without progress stopped and ALERT, never OK; fresh move works |
| `relative_start_read_fails` | normal | Failed start read ends both properties ALERT without `:05#`; next move works |
| `poll_failure_recovery` | normal | Silent, garbage, overlong and truncated idle polls ALERT with the last value, next poll OK; split reply reassembled |
| `request_survives_poll` | normal | GOTO accepted while an idle poll reply is outstanding stays BUSY, never OK early, one move command |
| `disconnect_during_motion` | normal | Disconnect sends one stop (then the EEPROM save) before the port closes, nothing afterwards; reconnect OK at the real position; fresh move works |
| `transport_loss` | normal | Port lost while idle: poll ALERT, later requests ALERT, disconnect completes |
| `controller_settings` | normal | Speed, reverse, backlash, step mode, coils, settle time and limits on the wire and read back over a reconnect (`DRV-144`); compensation sends no command |
| `settings_failures` | normal | Failed or unapplied step mode, coils, settle time, reverse, backlash and limits (`DRV-142`) end ALERT with the confirmed value; immediate retries succeed |
| `focuser_mode` | normal | Automatic mode withdraws manual controls and republishes `FOCUSER_POSITION` read-only, manual restores them |
| `temperature_compensation` | temperature-drift | One compensation move of coefficient × ΔT, BUSY then OK |
| `manual_mode_no_compensation` | temperature-drift | The same drift in manual mode moves nothing |
| `no_temperature_sensor` | no-sensor | `-127` publishes IDLE once, no compensation from it |
| `temperature_failures` | normal | Failed, implausible and malformed readings ALERT with the last valid value, next reading OK, motion usable |
| `gemini_step_modes` | gemini | Board detection reducing `X_STEP_MODE` to full and half step |

Not covered, and why:

- Hardware behavior of any kind; no device is available (see the hardware-test decision above).
- The `mfp://`, `tcp://` and `udp://` transports and the unexpected-disconnection path they enable: the integration target must not open sockets.
- `DRV-143`, `MFP-16` and a refused stop, for the reasons recorded with them.
- `CONFIG.SAVE` persistence of the three driver-defined properties: framework behavior; re-applying and reading them back is covered by `controller_settings`.
- `:03#`, `:28#`, `:40#` and `:42#` from the documented command subset: the driver has never used them.

## Final test summary

- Simulated tests: 2026-09-20 migration: 32 executed, 32 passed (16 scenarios, plain and under AddressSanitizer). 2026-10-05 rules alignment: 35 scenarios in the recorded run (see README `## Testing`); the same 35 against the pre-fix 3.0.0.13 driver failed 23.
- Hardware tests: 0 executed, 0 passed.
