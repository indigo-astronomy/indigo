# Celestron / PlaneWave EFA generated migration

Status: complete, 2026-09-09. Baseline version 0x02000011. Preserve the initial user MIGRATION_STATUS edit; its Comment column is manual-only. No generator implementation changes.

## References and decisions

Read repository/test rules, common/focuser standard, generator migration/lifecycle documentation, README, existing C/simulator/INO, prior Lacerta/iOptron migrations and driver REVIEW.md. Primary reference is bundled `PlaneWave EFA Communication Protocols.pdf`: pages 1–2 binary framing/checksum/19200 baud/RTS-CTS and 3799422 nominal travel; pages 3–5 motion, offset SYNC, fan and calibration-state commands; pages 6–7 temperature conversion/no-sensor marker. Page 4 table visually inspected. All messages (including invalid commands) can receive a reply, so matching headers alone do not establish success.

Temperature documentation is internally inconsistent: prose specifies address plus big-endian signed sixteenths, but its sample contains two bytes 5C 01 without address (little-endian 21.75 C is plausible). Support the documented three-byte form and explicit legacy two-byte little-endian form; test both and NC 7F7F. Hardware must settle actual firmware variants. The old simulator's 00 50 01 encoded 1280.0625 C in the old parser, so it is not an independent temperature oracle.

Celestron version/calibration/GOTO extensions (four-byte firmware, 02, 2A/2B/2C) come from the existing driver, not this PlaneWave PDF. Model-specific protocol assumptions are explicit. Preserve PlaneWave's >50000 coarse slew then stop/fine GOTO algorithm and local software limits; do not invent a device minimum-limit command. AUX-bus connection remains unsupported.

## Atomic plan

1. Baseline build/smoke; audit simulator, move to shared serial_motion, add Celestron/calibration, command journal/fault profiles and independent protocol tests. Reproduce malformed-frame and model failures before production fixes.
2. Portable uniform I/O, bounded complete frames/checksum/header/length validation and echo skipping. Keep all platform-specific CTS handling in uniform I/O; use its portable CTS getter and existing RTS setter from the driver. Transactional open/required init rollback. Validate parser/initialization failures with ASan.
3. Authoritative .driver plus generated connection/lifetime queues, delayed motion/calibration finalizers, bounded failure/abort/recovery. Correct measured/target handling, SYNC, limits and temperature/fan readback. Combine queue transition with generator ownership rather than keep duplicate handwritten timer scaffolding. Raise version.
4. Full applicable focuser tests for both models: interface/visibility/metadata; short/long/relative/no-op/clipping/SYNC; coarse positive/negative transitions; fan/temperature; calibration success/failure/abort/limits; invalid frames/ACK/read errors; overlap; disconnect/reconnect and independent instances. Strict O0/O2 arm64/x86_64 warnings including -Wconditional-uninitialized and targeted ASan.
5. Reproducible generation, Xcode/Windows projects, README/PROPERTIES/simulator inventory, CHANGES and scoped review dispositions. Update only MIGRATION_STATUS status columns, preserving Comment. Record final test counts, hardware/platform/flow-control assumptions, diff check and test cleanup.

## Coverage boundaries

No speed, backlash, direction reversal, automatic compensation or arbitrary Celestron SYNC is exposed. Framework input validation/config storage is not duplicated. PTYs cannot prove electrical CTS/RTS timing or real physical calibration/travel; hardware acceptance is small-travel class-standard checks and calibration only when appropriate for attached equipment. Explicit protocol fixtures do not certify unknown firmware variants.


## Steps 1–3 results

Baseline universal build and original smoke passed. The expanded `init_checksum` regression fails on baseline (bad checksum accepted); `init_overlong` with driver ASan reproduces a stack-buffer-overflow writing 201 bytes into the 16-byte response buffer through efa_command/indigo_read. Original ASan compilation also exposed two deprecated sprintf calls. These are fixed in the DSL; baseline logs were retained until final disposition.

The simulator now uses serial_motion independently of queries, both EFA/Celestron profiles, calibration progression/stop/failure, checksum/length/address/echo/split fault fixtures and a private flushed command journal. Two direct binary protocol scenarios cover both models separately from driver tests. Added archive dependency and a separately instrumented production-driver ASan target.

Generated version 0x03000012 replaces legacy timers/mutexes/unbounded movement loops with queue handlers and motion_finalizer/calibration_finalizer. Coarse slew/stop/fine GOTO remains observable in the command trace. Individual transactions are bounded; movement stalls stop after 100 unchanged polls, calibration has a 180-second deadline. Abort cancels the identified finalizers after stop and measured-state confirmation; disconnect owns queue cancellation and best-effort hardware stop. No generator change or MAX_DEVICES override.

Uniform I/O owns the handle and transactional rollback. Added indigo_uni_get_cts() to the shared uniform I/O layer: 1 asserted, 0 clear, -1 invalid/unsupported/error. Unix TIOCMGET and Windows GetCommModemStatus remain entirely in uni_io; the driver calls only portable APIs, including the existing RTS setter. CTS unsupported on a PTY preserves legacy no-line-control fallback; electrical handshake is hardware-only validation. Correct query lengths follow the PDF rather than padding every no-data query with an undocumented zero byte. Calibration-state query sends documented 0x40.

Repository rules now explicitly require handler + finalizer for long-running driver operations, a shared private-data receive buffer, portable APIs without OS-specific driver code, and preservation of the manual-only MIGRATION_STATUS Comment column, as directed during this migration. Strict generated O0/O2 arm64/x86_64 compilation, including Xcode conditional-uninitialized, already passes. Final simulator results are recorded below.

## Final user-directed refinements

The bounded 32-byte frame buffer lives in private data and also drains stale input (at most 1024 bytes per transaction). Helpers decode directly from this buffer. Calibration saves completion/progress scalars before its next position query overwrites the buffer. There are no helper-local receive arrays or platform conditionals in the DSL. The shared CTS getter preserves the no-modem-line fallback on PTYs. Both models retain the uncalibrated connection warning; an additional test checks external signed position changes.

The first shared CTS getter latched a PTY modem-status error into last_error, which uni_io uses to reject later reads/writes. The normal and initialization regressions caught this immediately. The getter now returns -1 without changing the data-handle error state; both independent protocol scenarios explicitly probe CTS before exchanging frames, and also check NULL-handle rejection. This tests the unsupported-line fallback without introducing OS-specific code in the driver.

## Reproduction and platform checks

```sh
make -C indigo_libs
build/bin/indigo_generator indigo_drivers/focuser_efa/indigo_focuser_efa.driver
make -C indigo_drivers/focuser_efa -f ../../Makefile.drv
make -C indigo_test build/integration/test_focuser_efa_simulator build/integration/test_focuser_efa_simulator_asan
cd indigo_test
./build/integration/test_focuser_efa_simulator
for efa_filter in init_ poll_ calibration_abort calibration_read_failure disconnect instances; do
  EFA_TEST_FILTER=$efa_filter ./build/integration/test_focuser_efa_simulator_asan || exit $?
done
```

Universal macOS library/driver/test builds pass without warnings. Generated driver syntax passes -Wall -Wextra -Wconversion -Wshorten-64-to-32 -Wconditional-uninitialized -Werror at O0/O2 for arm64 and x86_64; the host simulator passes -Wall -Wextra -Werror. Repeated generation is byte-identical. Xcode plutil validation and Windows project XML/file-reference/solution configuration checks pass. Windows/MSVC and Linux execution remain unverified; Windows support status denotes included source/project integration. Physical CTS/RTS and firmware/calibration behavior need hardware validation.

All 23 targeted ASan scenarios pass after the final shared-buffer/CTS changes, covering 9 initialization failures, 8 malformed polling replies, calibration abort/read failure, 3 disconnect cases and independent instances. The final ordinary suite passes 55/55 scenarios, including the real 180-second calibration timeout.

## Steps 4–5 results

All atomic steps are complete. Final version is 0x03000012, higher than baseline 0x02000011. Full scenario-to-capability mapping is in indigo_test/CHANGES.md. README, property source/capabilities, simulator inventory and scoped DRV-103/DRV-104 dispositions are synchronized. Xcode includes DSL/REFACTOR; Windows project and solution entries cover Debug/Release ARM64/x64. MIGRATION_STATUS records API 3, generated code, queues, Windows project support and simulator retesting, preserving its manual Comment column and the initial unrelated user edit. No generator implementation change.

Final git diff --check passes. All test runs have exited and make -C indigo_test test-clean removed test build artifacts; task-owned temporary protocol extracts, rendered page and diagnostic logs were removed after recording results.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard are now declared with the generator's `reject_change` block. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values instead of an `INDIGO_OK_STATE` update carrying no items, which left the refused value visible in the client.

Covered by the `rejected_change` scenario in `indigo_test/integration/test_focuser_efa_simulator.c`: while `FOCUSER_POSITION` is BUSY, `FOCUSER_STEPS` ends in ALERT with unchanged value and target, and motion is accepted again after the abort.

```sh
cd indigo_test && EFA_TEST_FILTER=rejected_change ./build/integration/test_focuser_efa_simulator
```

## Focuser testing rules alignment (2026-10-05, 3.0.0.22)

The regression test was checked against the extended "Focuser Drivers" chapter of `indigo_test/DRIVER_TESTING_RULES.md` (commit fa5f64839). Version raised from 3.0.0.21 to 3.0.0.22.

### Defects found and fixed

Each was reproduced by the new or extended test against a pre-fix copy of the generated driver built as a separate binary.

| Defect | Impact | Fix | Regression test |
| --- | --- | --- | --- |
| Aborted move ended OK | `FOCUSER_POSITION`/`FOCUSER_STEPS` reported a completed move after an abort (both models, also a queued GOTO). | A confirmed abort of a pending, active, uncommanded, uncertain or calibrating operation ends both ALERT at the stopped position (value = target). | `abort_efa`, `abort_celestron`, `abort_queued` |
| Abort while idle sent a stop | `0x24 0` was sent with nothing moving. | Abort with nothing pending ends OK without a command. | `abort_efa`, `abort_celestron` (stop count stays 1) |
| A good poll did not clear a failed poll | After one failed idle position poll both motion properties stayed ALERT until the next move or abort. | A failed idle poll is remembered; the next good poll restores OK. A failed or aborted move is not affected. | `poll_*` |
| Poll took a pending GOTO's target | A GOTO copied while the idle `0x01` reply was outstanding had its target overwritten with the measured position and was published by the poll, so the handler saw a no-op and the focuser never moved. | The poll writes only the measured value while a request is BUSY; target and state are published only when no request is pending. | `request_during_poll` |
| Uncommanded motion was published OK | Hand-control motion and motion running at connect only changed the value. | The idle poll publishes `FOCUSER_POSITION` BUSY with target = measured value while the position changes, OK once it settles; a relative move is refused meanwhile, abort and disconnect stop it. A failed move stays ALERT. | `external_position`, `manual_motion`, `moving_at_connect` |
| Limits could exclude the current position | PlaneWave local limits that excluded the measured position were accepted. | Such a change ends ALERT and keeps the old limits, as do changes during uncommanded motion. | `limits` |
| No-sensor marker reported ALERT | Protocol pages 6–7: 7F7F marks a missing sensor, not a failure. | 7F7F publishes `FOCUSER_TEMPERATURE` IDLE (once) with the last valid value; failed or implausible readings stay ALERT. | `temperature`, `temperature_legacy` |

### Simulator additions

- Fault key `manual <position>`: hand-control motion at 5000 steps/s without a PC command; `external` remains an instantaneous change.
- `slow` action: the matching reply is sent 0.8 s late (queue gate).
- Profiles `start_state` (PlaneWave at 123456, fans on, -3.5 °C), `c_start` (Celestron at 4321, device limits 1000–90000) and `moving` (hand-control motion to 20000 running at start).

### Scenario-to-test mapping (rules chapter → cases)

- Model variants on the bus and on the wire (properties, permissions, ranges, `INFO.DEVICE_MODEL`/`DEVICE_FW_REVISION`), connect sequence per model, SYNC right after connect to the published value, shutdown refused while connected, `X_` properties deleted on disconnect: `normal`, `celestron`, `split`, `echo`, `uncalibrated_*`. Non-default device state at connect: `start_state_efa`, `start_state_celestron`. Reconnect to the other model: `model_change`. Refused handshake and every failed connect-time command: `unknown_identity`, `init_*`.
- Position poll failure keeps the value and the next good poll restores OK: `poll_*`. Temperature sentinel/implausible/recovery and both reply forms: `temperature`, `temperature_legacy`.
- Uncommanded motion: `external_position`, `manual_motion`, `moving_at_connect`.
- Short/relative/no-op/coarse moves, limits and clamping, limit refusals (empty interval, excluding the position, during motion): `movement_*`, `long_move`, `limits`, `overlap`. Overlap refusal: `overlap`, `rejected_change`.
- Abort mid-move (ALERT both, value = target = stopped, stop once, two later equal polls, idle and OFF abort without a command, reconnect OK, fresh move): `abort_efa`, `abort_celestron`. Abort overtaking a queued GOTO and a queued calibration: `abort_queued`, `queued_abort`. Refused stop keeps the move BUSY, retry works: `stop_failure`.
- Rejection at each command of the coarse move (slew start, fine target): `long_move_failure`; start failures and transport loss: `start_failure`, `transport_loss`. Readback/state failure and stall during motion: `motion_read_failure`, `motion_badstate`, `stalled_motion`.
- Calibration (Celestron) success, start/progress/read/limits failures, abort, 180 s timeout: `calibration*`.
- Disconnect during motion sends the stop once, nothing follows, reconnect OK at the stopped position; during a read and during calibration: `disconnect_motion`, `disconnect_read`, `disconnect_calibration`.
- Requests versus polls: `request_during_poll`. Position is the only polled writable value; fans are read at connect and on change only.
- Fans command/readback and failure: `normal`, `fans_failure`. SYNC failure keeps the real position: `sync_failure`. Additional instance: `instances`.

### Rules not applicable

- Speed, reversal, backlash, compensation and modes: not implemented by the protocol subset (speed asserted undefined).
- A controller refusal reason in the client message: replies carry only an ACK byte.

### Test harness fixes found by the first recorded run

The first recorded run ended 62/64. `motion_read_failure` failed because its position-readback fault could be taken by an idle poll still in flight when the GOTO was accepted; the old driver turned that poll failure into an ALERT over the pending request (the defect fixed above), so the case passed by accident. The fault is now injected only after the move command was sent. The second failing case was not printed by the run and did not recur in two further runs of the binary; the most likely candidate is `stalled_motion`, whose fixed 4 s + 8 s wait left little margin over the 100 × 0.1 s stall bound on a loaded host, so the wait was widened to 9 s + 8 s.

Validation: `python3 tools/run_driver_test.py focuser_efa` (recorded in README `## Testing`).
