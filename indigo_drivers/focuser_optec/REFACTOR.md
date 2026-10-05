# Optec focuser driver refactoring

This file is the engineering record for migrating `indigo_focuser_optec` to
`indigo_generator`. `README.md` remains the user-facing document and is not
modified by this work.

## Sources reviewed

- The complete legacy driver, public header, standalone entry point, host-side
  simulator and Arduino simulator in this directory.
- `tcf_technical_manual.pdf`, revision 11 (August 2010), including the complete
  serial-protocol section on PDF pages 25–34 (manual pages 19–28).
- INDIGO driver-generator, driver-development, simulator and focuser-test
  guidance, plus generated focuser drivers already carrying a `REFACTOR.md`.
- The original driver build and simulator-backed test. The build succeeded; the
  pre-refactoring integration executable terminated with signal 11 (`exit 139`).

## Existing behavior and findings

### Device and protocol

- The driver represents one Optec TCF-S/TCF-S3 focuser at 19,200 baud, 8N1.
  Serial commands are not terminated. Replies are ASCII records terminated by
  LF followed by CR.
- `FMMODE` establishes manual/PC control and replies `!`. `FFMODE` releases it
  and replies `END`. Automatic A/B modes stream position and temperature until
  `FQUIT1`; the driver uses automatic A only and suppresses that telemetry.
- The driver supports relative inward/outward movement, read-only absolute
  position, temperature, AUTO-A compensation coefficient and sign, manual/AUTO
  mode, and a software reverse switch. Speed and abort are intentionally hidden.
- The protocol exposes no motion-status or abort command. Motion completion must
  therefore be inferred from bounded position polling; abort coverage is not
  applicable and no unsupported stop command will be invented.
- The two-inch TCF-S has a 0–7000 travel range; TCF-S3 has 0–9999. The protocol
  has no model-identification command, so the public portable limit remains
  0–9999 and model-specific end-stop behavior stays controller-owned.
- Movement accepts exactly four decimal digits and is limited to 0–7000 steps
  per request (9999 for TCF-S3), with `*` acknowledgement. Published maximum
  motion rate is 200 steps/s.
- Compensation magnitude is 000–999 and `FLAnnn` contains exactly three digits.
  Sign is read with `FTxxxA` and written with `FZAxxn`; positive is 0 and
  negative is 1. The protocol also documents the equivalent B commands.
- The remaining documented surface is `FCENTR`, `FSLEEP`, `FWAKUP`,
  `FREADA/B`, `FLA/Bnnn`, `FQUITn`, `FDA/Bnnn`, `FHOME`, `FAMODE/FBMODE` and
  sign read/write for A/B. The driver intentionally exposes only capabilities
  represented by standard focuser properties, but the simulator must implement
  this complete protocol surface.
- Documented controller reports are `ER=1` (temperature probe), `ER=2`
  (automatic position outside travel) and `ER=3` (EEPROM). They are transport
  failures from the driver's point of view and must never be accepted as data.

### Legacy implementation defects and risks

- It uses the legacy raw serial API, raw `close()`, `indigo_printf()` /
  `indigo_scanf()`, a custom mutex and timer instead of portable unified I/O and
  generated serialized handlers.
- Connection succeeds after `FMMODE` even if position, temperature, coefficient
  or sign initialization fails. Replies are not checked for complete framing,
  trailing bytes, finite values or protocol ranges.
- Disconnect sends `FFMODE` but does not consume or validate `END`, which can
  hide protocol/session failures.
- Motion completion is polled forever without a timeout or stall bound. Poll
  failures are silently ignored, leaving position and steps BUSY indefinitely.
- Manual-mode entry can block the handler for ten seconds with one-second
  sleeps. Automatic-mode writes have incomplete transport validation.
- Compensation change marks and updates `FOCUSER_MODE` BUSY instead of
  `FOCUSER_COMPENSATION`. Its error prologue also assigns ALERT to
  `FOCUSER_STEPS`. It formats magnitude as `FLA%04d`, producing a seven-byte
  command contrary to the documented six-byte `FLAnnn` grammar. A partial
  magnitude/sign write is not reconciled by readback.
- Periodic position and temperature failures do not publish ALERT and there is
  no explicit recovery behavior.
- The legacy host simulator assumes every command is six bytes, loses fragmented
  input, completes motion on the first position query and implements only part
  of the documented A-side protocol. It has no deterministic profiles, command
  journal or injectable failures, so recovery and framing paths cannot be
  verified.
- The existing automated test is a single broad smoke case. It does not isolate
  lifecycle, protocol grammar, asynchronous states, failures, recovery,
  concurrency or cleanup, and its baseline currently crashes.

## Atomic plan and progress

- [x] 1. Inventory the legacy implementation, bundled documentation, build
  wiring, migration status, property reference, simulator inventory, Xcode
  project and existing tests.
- [x] 2. Build the unmodified driver and run the original simulator-backed test;
  record the successful build and crashing (`exit 139`) test baseline.
- [x] 3. Create the generator source of truth, preserve the public behavior,
  replace legacy transport/lifecycle code with portable unified I/O and bump the
  driver version from 7 to 8.
- [x] 4. Make connection initialization transactional; strictly validate LF/CR
  framing, exact payload grammar, finite/ranged readback and `END` on normal
  session release.
- [x] 5. Implement queued relative motion with BUSY start, bounded polling,
  progress, deterministic completion/stall/transport-failure states and recovery
  without claiming unsupported abort capability.
- [x] 6. Correct compensation state ownership and `FLAnnn` formatting, validate
  both writes and reconcile magnitude/sign by readback, including partial
  failure. Make mode changes bounded and non-blocking beyond individual serial
  transactions.
- [x] 7. Replace the host simulator parser with a fragmentation-safe documented
  protocol model using `serial_motion.h`; implement A/B coefficients, signs and
  delays, modes, quiet control, center, sleep/wake, home, elapsed motion, exact
  LF/CR replies, profiles, event journal and deterministic faults.
- [x] 8. Replace the smoke test with isolated public-bus and direct-protocol
  cases covering metadata/properties, every supported driver capability,
  command/readback grammar, motion/reverse/bounds/stall, compensation and
  partial failure, mode transitions, malformed/partial/overlong/silent/closed
  transport, recovery, disconnect during work, repeated lifecycle, second
  instance isolation, descriptor cleanup and the simulator's documented command
  inventory. Generic framework number validation and hardware-only behavior are
  explicitly out of scope.
- [x] 9. Regenerate `.c`, `.h` and `_main.c`; build the driver and normal plus
  sanitizer test variants, run the complete Optec suite with the host simulator,
  and inspect the generated diff.
- [x] 10. Update `MIGRATION_STATUS.md`, property and simulator references, and
  `indigo_test/CHANGES.md`; add every new persistent file to the Optec/test Xcode
  groups while preserving all unrelated project edits.
- [x] 11. Confirm `README.md` is unchanged, no hardware test was attempted, no
  simulator/test process remains, and temporary render/build artifacts created
  by this work are removed.

## Acceptance matrix

| Requirement | Automated evidence |
|---|---|
| Generator lifecycle and metadata | `capabilities*`, `reconnect`, `descriptor_cleanup` and every isolated case's init/shutdown |
| Strict initialization/readback | `init_handshake_silent`, all `init_position_*`, `init_temperature_malformed`, `init_slope_malformed`, `init_sign_malformed` |
| Relative movement | `relative_motion`, `overlap`, `motion_start_failure`, `motion_poll_failure`, `motion_stall`, `motion_transport_close` |
| Compensation | `controls`, `compensation_partial_failure` and command-journal assertions for `FLA%03d`/`FZAxxn` |
| Manual/AUTO-A mode | `controls`, `mode_failure`, `simulator_automatic_compensation` |
| Periodic telemetry | all `poll_*` cases plus alternate-profile signed temperature and external position recovery |
| Complete simulator protocol | `simulator_protocol`, `simulator_split`, `simulator_automatic_compensation` cover every documented command family |
| Lifecycle/concurrency hygiene | `disconnect_motion`, `reconnect`, `instances`, `descriptor_cleanup`, `overlap` |

## Validation result

- Generator output was refreshed from `indigo_focuser_optec.driver`; the generated
  source contains no `MAX_DEVICES` override and the driver version is 8.
- The driver archive, shared library and standalone executable build succeeded on
  macOS for both arm64 and x86_64.
- All 32 isolated simulator-backed scenarios passed in the normal build.
- All 32 scenarios passed with the generated driver compiled directly under
  AddressSanitizer. macOS reported that LeakSanitizer is unsupported, so leak
  detection could not be enabled; descriptor balance is covered separately.
- `plutil -lint indigo.xcodeproj/project.pbxproj` and the scoped
  `git diff --check` passed.
- No hardware test was run. Linux and Windows compilation and physical TCF-S /
  TCF-S3 behavior remain unverified.

Hardware validation is intentionally not part of this task. Platform portability
will be established by generated code and portable APIs; only the local macOS
simulator-backed build is executable here.

## Focuser testing rules alignment (3.0.0.10)

The suite was checked against the "Focuser Drivers" chapter of
`indigo_test/DRIVER_TESTING_RULES.md` (compliance scenarios and the Focuser
Driver Test Standard) and extended where a rule applies to the TCF-S.

### Found defects

| Defect | Impact | Fix | Regression test |
|---|---|---|---|
| A controller without a working temperature probe answers `FTMPRO` with the documented `ER=1` report, and the connect required a valid temperature. | A TCF-S without its probe (the most common fault the manual lists) could not be connected at all. | `ER=1` is treated as the no-sensor report: connect completes and `FOCUSER_TEMPERATURE` is IDLE; any other invalid reading stays ALERT with the last valid value. | `no_probe` (fails against 3.0.0.9: connect refused) |
| Position changes the driver did not command (hand controller, a move still running at connect after a disconnect during motion) were published OK with every poll. | Clients saw a moving focuser as settled and could start a relative move from a stale position. | A poll that reads another position than the previous one publishes `FOCUSER_POSITION` and `FOCUSER_STEPS` BUSY with target equal to the measured value; the next unchanged poll ends both OK. `FOCUSER_STEPS` is only reset when the driver itself set it BUSY, so a pending move request is never overwritten. | `external_motion`, `disconnect_motion` (both fail against 3.0.0.9: no BUSY) |
| A compensation request during a move was refused, but the readback `FREADA`/`FTxxxA` was still sent. | Device traffic in the middle of a move for a request already refused. | The request ends ALERT with the previous value and target restored and no command. | `settings_during_motion` (fails against 3.0.0.9: readback commands counted) |
| `FOCUSER_REVERSE_MOTION` was accepted during a move. | A motion-geometry control changed while a move was running. | The request ends ALERT and keeps the previous item; the driver keeps the accepted value in its private data. | `settings_during_motion` (fails against 3.0.0.9: OK instead of ALERT) |

The pre-fix results were obtained with a separate binary built against a copy of
the 3.0.0.9 sources; the shared tree was not reverted.

### Simulator additions

- Profile `no-probe`: `FTMPRO` answers `ER=1`, the documented report for a
  missing or failed probe.
- Fault `handmove <position>`: the focuser moves at 200 steps/s without a serial
  command, as with the hand controller.
- Fault action `slow`: the reply is delayed by 400 ms, so a poll can be held in
  flight while a request arrives.

### Rule mapping

| Rule | Test |
|---|---|
| Relative-only negative contract (read-only position, no `FOCUSER_ON_POSITION_SET`, `FOCUSER_LIMITS`, `FOCUSER_SPEED`, `FOCUSER_ABORT_MOTION`), ranges | `capabilities*` |
| Connect command sequence `FMMODE`, `FPOSRO`, `FTMPRO`, `FREADA`, `FTxxxA`; model placeholder | `capabilities` |
| Non-default start state (position, signed temperature, negative coefficient) | `capabilities_alternate` |
| Refused connect: ALERT, no focuser property left, port released, next connect works | `init_*` |
| Failed/malformed/partial/overlong position poll: ALERT with last value, recovery, move works | `poll_position_*` |
| Failed or implausible temperature: ALERT with last value, recovery, move works; no-sensor report IDLE once | `poll_temperature_malformed`, `temperature_implausible`, `no_probe` |
| Uncommanded motion BUSY then OK, target equals measurement, no command, later move starts from it | `external_motion` |
| Replies split across reads | `capabilities_split`, `simulator_split` |
| Inward/outward, reverse, zero steps, clamping at both ends sent as a move to the end | `relative_motion` |
| Both motion properties BUSY also for a move shorter than one poll period | `short_move` |
| Move request while a move is BUSY: no command, running move ends at its target | `overlap` (the same-property request is dropped by the framework BUSY guard) |
| Start rejected (no acknowledgement): ALERT on both properties, position unchanged, next move works | `motion_start_rejected` |
| Malformed acknowledgement, failed readback, stall | `motion_start_failure`, `motion_poll_failure`, `motion_stall` |
| Transport loss during a move: ALERT, later requests ALERT without BUSY, no new handshake, disconnect completes | `motion_transport_close` |
| Disconnect during motion: no poll after close; reconnect publishes the still running move BUSY then OK at the real position; fresh move | `disconnect_motion` |
| Compensation, mode and reverse during a move: ALERT, previous value kept, no command | `settings_during_motion` |
| Setting write, rejected/partial write with readback, mode switch and restore | `controls`, `compensation_partial_failure`, `mode_failure` |
| Poll in flight when a move is accepted: one move command, `FOCUSER_STEPS` BUSY until the move ends OK | `request_versus_poll` |
| `INDIGO_DRIVER_SHUTDOWN` refused while connected, connection keeps working | `shutdown_while_connected` |
| Additional instance publishes its own device, the first survives its disconnect | `instances` |

### Rules not applicable

- Abort, aborted move ending ALERT, abort while idle, stop on stall and stop on
  disconnect during motion: the TCF-S protocol has no stop command, so
  `FOCUSER_ABORT_MOTION` is hidden and the controller always completes a move.
  A disconnect during motion therefore releases serial control with `FFMODE`
  and the next connect reports the still running move as uncommanded motion.
- Absolute GOTO, SYNC, `FOCUSER_LIMITS`, speed, backlash, homing and
  calibration: not exposed by the protocol or not represented by the driver.
- Model and firmware identity: the protocol has no identification command, so
  TCF-S (0–7000) and TCF-S3 (0–9999) cannot be told apart and the model keeps the
  placeholder.
- Driver-owned compensation: compensation runs inside the controller in AUTO-A
  mode; the driver only writes the coefficient and switches the mode.
- Shared controllers: one focuser per port.

### Validation

- `make -C indigo_test build/integration/test_focuser_optec_simulator`, then one
  recorded run with `python3 tools/run_driver_test.py focuser_optec`: 40 cases.

## Failed moves stay ALERT (3.0.0.11, 2026-10-05)

A rule of the focuser chapter was missed in 3.0.0.10: a stalled move or a failed status read
during a move "is not turned OK by a later idle poll", and a fresh move works.

| Defect | Fix | Regression test (fails against 3.0.0.10) |
|---|---|---|
| After a start failure, a stall or a failed readback during a move the driver waited for two equal position polls and then published both motion properties OK, so the failure disappeared; until then the requested target stayed published. | The failure sets the target to the position read last; the two equal polls only re-enable moves and set value and target to the settled position, while both properties stay ALERT through later idle polls. The next move, an uncommanded motion or a zero-step request clears it. | `motion_start_failure`, `motion_start_rejected`, `motion_poll_failure`, `motion_stall` |

The pre-fix result comes from a separate binary built against a copy of the 3.0.0.10 sources.

## Final test summary

- Simulated tests: 40 run, 40 passed (recorded run of 3.0.0.11, macOS arm64).
- Hardware tests: 0 run, 0 passed.
