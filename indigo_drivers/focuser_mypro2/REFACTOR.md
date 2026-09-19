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

## Test coverage map

| Scenario | Profile | Covers |
| --- | --- | --- |
| `metadata` | normal | Interface bit, focuser class properties, the three driver-defined properties, INFO board and firmware, eight step modes, and every value the driver reads back at connect |
| `sync_and_goto` | normal | SYNC as a coordinate update with no move, absolute GOTO with BUSY then OK, and a request for the position already held |
| `relative_move` | normal | Outward and inward relative moves, sign and units |
| `abort_motion` | normal | Abort of a demonstrably running move, settled state, stop point inside travel, stable afterwards, sync and move accepted afterwards |
| `abort_overtakes_start` | normal | `DRV-145`: urgent abort cancelling a queued move |
| `abort_while_idle` | normal | Abort with nothing running, motion properties undisturbed |
| `controller_settings` | normal | `DRV-144`: speed, reversal, backlash, step mode, coil power, settle time and limits written and then read back over a reconnect |
| `focuser_mode` | normal | Automatic mode withdrawing the manual controls and republishing `FOCUSER_POSITION` read only, manual mode restoring them |
| `temperature_compensation` | temperature-drift | Driver-owned compensation: threshold, steps per degree, and the resulting move |
| `gemini_step_modes` | gemini | Board detection reducing `X_STEP_MODE` to full and half step |
| `no_temperature_sensor` | no-sensor | `-127` puts `FOCUSER_TEMPERATURE` idle and no compensation is applied from an invalid reading |
| `silent_controller` | silent | Failed open, no driver property left defined |
| `moving_query_fails` | moving-error | `DRV-141`: a failed motion poll is an alert, not a completed move |
| `max_position_readback_fails` | maxpos-error | `DRV-142`: a failed limits readback is an alert |
| `reconnect` | normal | Property withdrawal and redefinition, motion after reconnect |
| `disconnect_during_motion` | normal | Disconnect while BUSY halts the controller, clean teardown, reconnect finds it stopped short |

Not covered, and why:

- Hardware behavior of any kind; no device is available (see the hardware-test decision above).
- The `mfp://`, `tcp://` and `udp://` transports and the unexpected-disconnection path they enable: the integration target must not open sockets, and the PTY harness cannot exercise the TCP branch.
- `DRV-146` and `DRV-143`, for the reasons recorded with them.
- `CONFIG.SAVE` persistence of the three driver-defined properties: `indigo_save_property()` is framework behavior; the driver-owned part, re-applying the settings and reading them back on the next connect, is covered by `controller_settings`.
- `:03#`, `:28#`, `:40#` and `:42#` from the documented command subset: the driver has never used them.

## Final test summary

- Simulated tests: 32 executed, 32 passed (16 ordinary scenarios and the same 16 under AddressSanitizer). Three of the 16 were recorded as deterministic expected baseline failures against the original driver and one more as an intermittent one; all pass as regression tests against the migrated driver.
- Hardware tests: 0 executed, 0 passed.
