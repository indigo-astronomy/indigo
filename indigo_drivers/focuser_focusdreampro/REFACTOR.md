# AstroGadget FocusDreamPro refactoring and validation record

Status: migrated to `indigo_generator` on 2026-09-19.

## Current-state audit (2026-09-19)

### Architecture and implementation

- `indigo_focuser_focusdreampro.c` is a hand-written 500-line single-device focuser driver, version `0x03000007`, device name `FocusDreamPro`, driver label `AGadget FocusDreamPro Focuser`.
- Transport is `indigo_uni_open_serial_with_speed()` at 9600 baud. `focusdreampro_command()` discards pending input, writes the command with `indigo_uni_write()` **without any terminator** and reads one `\n`-terminated line with `indigo_uni_read_line()`. All transactions are serialised by a driver-private `pthread_mutex_t`.
- Handlers are dispatched with `indigo_set_timer()` rather than the device handler queue, and the polling callback uses `indigo_reschedule_timer()` with a stored `indigo_timer *`.
- Polling reads `T` (temperature), `I` (moving) and `P` (position) once per cycle, every 0.5 s while moving and every 1 s otherwise, and derives `FOCUSER_POSITION`/`FOCUSER_STEPS` BUSY/OK from the `I` reply.

### Protocol

One-character or `<letter>:<value>` commands, replies are lines. `#` identity (`FD` or `Jolo…`), `T` temperature or `T:false` when no probe is fitted, `I` motion (`I:true`/`I:false`), `P` position, `X:<max>` maximum position, `S:<delay>` speed, `D:<percent>` duty cycle, `M:<position>` absolute move, `R:<position>` sync, `H` halt. Value-setting commands echo the command back. Manufacturer sources: <https://sites.google.com/view/astro-gadget/control-of-focusers/focusdreampro> and <https://github.com/sirJolo/ascom-jolo-focuser>.

### Public properties

`FOCUSER_SPEED` (index 0…5 into a fixed delay table `{ 500, 250, 110, 40, 10, 5 }`), `FOCUSER_STEPS` (0…100000), `FOCUSER_DIRECTION`, `FOCUSER_POSITION` (0…1000000), `FOCUSER_ON_POSITION_SET`, `FOCUSER_LIMITS`, `FOCUSER_ABORT_MOTION`, `FOCUSER_TEMPERATURE` (hidden when the device reports `T:false`) and the driver-defined `X_FOCUSER_DUTY_CYCLE`, the only property saved by `CONFIG.SAVE` and the only connection-dependent one. `FOCUSER_REVERSE_MOTION` is hidden.

### Defects, risks and lifecycle findings

- `PRIVATE_DATA->fdp` and `PRIVATE_DATA->jolo` are written at connect and never read.
- Neither flag is cleared on disconnect, and `FOCUSER_TEMPERATURE_PROPERTY->hidden` is set to `true` when a device reports no probe but is never restored, so reconnecting to a device that does have one leaves the property hidden for the lifetime of the driver. Recorded as `DRV-131`.
- An unrecognised identity reply is accepted: only a failed `#` transaction closes the handle, so any device answering one line connects as a FocusDreamPro. Recorded as `DRV-132`.
- `focuser_position_handler()` publishes `FOCUSER_STEPS` twice, once before the command with no state change.
- `focuser_abort_handler()` leaves `FOCUSER_POSITION` and `FOCUSER_STEPS` in `INDIGO_ALERT_STATE` after a successful abort, so a normal abort is reported as a failure. Recorded as `DRV-133`.
- Nothing rejects a move request while another move is in progress; the second `M:` simply replaces the target.
- The driver holds the mutex across the whole polling cycle, so a property change waits for up to three transactions.

### Platforms, build and packaging

Linux, macOS and Windows; `indigo_focuser_focusdreampro.vcxproj` and `.vcxproj.filters` exist. No `Makefile.inc`.

### Test assets

- `focuser_focusdreampro_simulator/focuser_focusdreampro_simulator.c`, a PTY simulator with **no motion model**: `settle_motion()` snaps the position to the target whenever `I` or `P` is read, so `I` never reports `true` and no BUSY, progress or abort behavior can be observed.
- `indigo_test/integration/test_focuser_focusdreampro_simulator.c`, a single smoke scenario.
- Coverage gaps: everything motion related, the Jolo identity, the missing-temperature device, every failure path, reconnect and the duty cycle readback.

## Baseline (2026-09-19, macOS 15 arm64, universal x86_64+arm64 build)

- `make -C indigo_drivers/focuser_focusdreampro -f ../../Makefile.drv` — succeeded, no warnings.
- `make -C indigo_test build/integration/test_focuser_focusdreampro_simulator` — succeeded.
- `./build/integration/test_focuser_focusdreampro_simulator` — 1 scenario, 1 passed.

## Hardware-test decision

Hardware testing will **not** be performed. No FocusDreamPro or Jolo controller is available in this environment, and no hardware validation is claimed anywhere in this record.

## Migration plan and results

1. **Rebuild the simulator on `serial_motion.h`.** — **Done.** Motion now takes real time at the step delay the `S:` command selects, `I` reports `true` while it runs, and the profiles `normal`, `jolo`, `no-temperature`, `no-identity`, `temperature-error`, `move-error` and `abort-error` are available. The request framing is unchanged: the controller has no request terminator, so a gap in the incoming bytes still ends a command.
2. **Grow the characterization suite.** — **Done.** One smoke scenario became 17. Fourteen passed against the original driver; `abort_motion`, `abort_overtakes_start` and `temperature_probe_restored` are the dedicated reproducers of `DRV-133`, `DRV-134` and `DRV-131` and were recorded as expected baseline failures. The whole final suite was re-run against the original driver to confirm exactly those three fail and nothing else.
3. **Capture the normalized reference trace.** — **Done.** `FOCUSDREAMPRO_TEST_TRACE=1` logs every request and reply. The comparison keeps the ordered request stream and collapses each maximal run of the `T`/`I`/`P` polling commands into one `POLL[...]` token, because the number of polling cycles inside a timed move is not deterministic. Two runs of the same scenario normalize identically.
4. **Write `indigo_focuser_focusdreampro.driver`** and generate the outputs. — **Done.** 324 lines; the generator emits the property table, the connection handler, change dispatch, detach and the entry point.
5. **Build, re-run and compare.** — **Done.** See *Verification*.
6. **Register and update the status documents.** — **Done.**
7. **Final audit.** — **Done.**

## Intentional differences from the original driver

- **Handler dispatch.** Every handler moves from `indigo_set_timer()` to the generated device handler queue, so the driver-private mutex is gone and all transactions are serialized by the queue. `FOCUSER_ABORT_MOTION` is dispatched at urgent priority.
- **Move requests while a move runs.** `INDIGO_COPY_VALUES_PROCESS_CHANGE` rejects a `FOCUSER_POSITION` or `FOCUSER_STEPS` change while the property is BUSY. The original replaced the target of a running move instead.
- **Unused state removed.** The `fdp` and `jolo` flags were written at connect and never read.
- **INFO model on an unknown controller.** The original left the previous model string in place when the identity reply was neither `FD` nor `Jolo…`; the driver now reports `Unknown`. An unrecognised one-line answer is still accepted, because the README documents only two of an unknown number of compatible firmwares and rejecting the rest would be a regression for them.
- **One `FOCUSER_STEPS` update per position change.** `focuser_position_handler()` published `FOCUSER_STEPS` twice, the first time without having changed anything.
- **Response buffer.** The 16-byte stack buffer in each helper became one 32-byte buffer in the private data, which no longer truncates a longer identity banner.
- **Write errors.** `focusdreampro_command()` checks the result of `indigo_uni_vprintf()` and fails immediately instead of writing blindly and waiting out the read timeout.
- **Trace differences.** Three, all intended: the extra `P` after a successful `H` (see `DRV-133`), the absence of the cancelled `M:` in `abort_overtakes_start` (see `DRV-134`), and the `R:100` of the fresh move that `abort_motion` only reaches now that the scenario no longer fails at the abort.

## Found defects

- `DRV-131` (`focuser_connection_handler`, reproduced). Observable impact: connecting to a controller without a temperature probe set `FOCUSER_TEMPERATURE_PROPERTY->hidden = true` permanently, so every later connection of the same driver instance, including one to a controller that does have a probe, published no temperature. Root cause: the flag was never restored. Fix: `on_connect` sets `hidden = false` before it evaluates the `T` reply. Regression test: `temperature_probe_restored`, which connects to a probe-less controller, disconnects, connects to a second simulator that has a probe and requires `FOCUSER_TEMPERATURE` to be defined and OK; it failed against the original driver and passes now.

- `DRV-133` (`focuser_abort_handler`, reproduced). Observable impact: a *successful* abort left `FOCUSER_POSITION` and `FOCUSER_STEPS` in `INDIGO_ALERT_STATE`, and because the polling callback only converts BUSY to OK they stayed in alert until the next move. A normal user abort was reported as a failure. Root cause: the handler set the alert state unconditionally on the success path. Fix: a successful `H` settles both properties to `INDIGO_OK_STATE` and reads the position back so the stop point is published at once; only a failed `H` reports alert. Regression test: `abort_motion`, which aborts a move that is demonstrably running and requires a fresh OK state, a stop point strictly inside the travel and a stable position two polling cycles later.

- `DRV-134` (`focuser_abort_handler`, reproduced). Observable impact: an abort issued while a move request was still queued did not cancel that request, so the driver halted the controller and then immediately started the move it had just been told to abandon. The trace of the original driver shows `H` followed by `M:200000`. Root cause: the handlers ran on independent one-shot timers and nothing cancelled them. Fix: the abort handler cancels the pending `focuser_position_handler` and `focuser_steps_handler` before it sends `H`. Regression test: `abort_overtakes_start`, which aborts immediately after requesting a 200000 step move and requires the focuser to stay put across two polling cycles.

- `DRV-132` (identity check, source audit only, **not fixed**). Any controller that answers the `#` command with one line is accepted, whatever the banner says. This is deliberate: the README names the AstroGadget and Astrojolo firmwares, but the command set is shared and the set of compatible banners is not documented, so rejecting unknown ones would be a regression for them. The `no-identity` profile covers the case that is unambiguously wrong, a controller that does not answer at all.

## Verification (2026-09-19, macOS 15 arm64)

- `../../build/bin/indigo_generator indigo_focuser_focusdreampro.driver` run twice: the second run reproduces the first output byte for byte.
- `make -C indigo_drivers/focuser_focusdreampro -f ../../Makefile.drv` — universal x86_64 + arm64, no errors, no warnings.
- `./build/integration/test_focuser_focusdreampro_simulator` — 17 scenarios, 17 passed. The same suite against the original driver fails exactly the three defect reproducers.
- Normalized trace comparison — three differences, each explained above; the rest of the ordered request stream is identical over 175 normalized lines.
- AddressSanitizer: the generated driver compiled with `-fsanitize=address -O1` and linked into the same suite — 17 scenarios, 17 passed, no sanitizer report. The framework library is still not instrumented.
- No `MAX_DEVICES` override, no build products in the diff, driver version raised from `0x03000007` to `0x03000008`.
- Linux and Windows builds were not run in this environment; only the macOS universal build is validated.

## Test coverage map

| Scenario | Profile | Covers |
| --- | --- | --- |
| `metadata` | normal | Interface bit, focuser class properties, `X_FOCUSER_DUTY_CYCLE`, speed range, hidden `FOCUSER_REVERSE_MOTION`, INFO model, first temperature reading |
| `sync_and_goto` | normal | `FOCUSER_ON_POSITION_SET` SYNC as a coordinate update with no move, absolute GOTO with BUSY then OK and the measured final position |
| `relative_move` | normal | Outward and inward relative moves, sign and units, zero step no-op |
| `limits_clamp` | normal | `FOCUSER_LIMITS` boundary applied to an absolute target |
| `abort_motion` | normal | `DRV-133`: abort of a running move, settled state, stop point inside travel, stable afterwards, fresh move accepted |
| `abort_while_idle` | normal | Abort with nothing running, motion properties undisturbed |
| `abort_overtakes_start` | normal | `DRV-134`: urgent abort cancelling a queued move |
| `speed_and_duty_cycle` | normal | All six speed indices mapped to the delay table, duty cycle readback, movement at the fastest speed |
| `jolo_identity` | jolo | Astrojolo banner and model string, same command set |
| `no_temperature_probe` | no-temperature | `T:false` hides `FOCUSER_TEMPERATURE` and does not block motion |
| `temperature_probe_restored` | no-temperature | `DRV-131`: the probe-less controller must not hide the property for the next connection |
| `silent_controller` | no-identity | Failed open, alert connection, no driver property left defined |
| `temperature_error` | temperature-error | Malformed `T` reply reports alert and does not block motion |
| `move_rejected` | move-error | Rejected `M:`/`R:` report alert and move nothing |
| `abort_rejected` | abort-error | Rejected `H` reports alert |
| `reconnect` | normal | Property withdrawal and redefinition, duty cycle re-applied on connect, motion after reconnect |
| `disconnect_during_motion` | normal | Disconnect while BUSY, clean teardown, reconnect and fresh motion |

Not covered, and why:

- Hardware behavior of any kind; no device is available (see the hardware-test decision above).
- `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_MODE` and `FOCUSER_REVERSE_MOTION`: the controller has none of them and the driver leaves them hidden, which `metadata` asserts for reversal.
- `CONFIG.SAVE` persistence of `X_FOCUSER_DUTY_CYCLE`: `indigo_save_property()` is framework behavior; the driver-owned part, re-applying the value with `D:` on the next connect, is covered by `reconnect`.

## Final test summary

- Simulated tests: 34 executed, 34 passed (17 ordinary scenarios and the same 17 under AddressSanitizer). Three of the 17 were recorded as expected baseline failures against the original driver and pass as regression tests against the migrated driver.
- Hardware tests: 0 executed, 0 passed.
