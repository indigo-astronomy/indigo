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
