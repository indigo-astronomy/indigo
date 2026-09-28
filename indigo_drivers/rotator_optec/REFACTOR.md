# Optec Pyxis Rotator Generator Refactoring

## Scope and current-state audit

- Scope: migrate `indigo_rotator_optec` from its hand-written C lifecycle to an `indigo_generator` `.driver` source while preserving the public driver name (`indigo_rotator_optec`) and device name (`Optec Pyxis`). The checked-in `.c`, `.h`, and `_main.c` will become generated outputs.
- Current architecture: one serial rotator device, additional instances enabled, with hand-written attach/enumerate/change/detach code, POSIX file-descriptor I/O, two INDIGO timers, and a private pthread mutex. Serial transactions are nominally serialized by the mutex; absolute moves and homing run blocking read loops in timer callbacks.
- Public behavior: standard `ROTATOR_POSITION` and `ROTATOR_DIRECTION` are visible after connection. `ROTATOR_ON_POSITION_SET` and `ROTATOR_ABORT_MOTION` are hidden because the controller protocol has neither coordinate sync nor a stop command. Custom properties are `X_HOME.HOME`, `X_RATE.RATE` (0-99, default 8), and `X_ROTATE.ROTATE` (-9..9 steps). `X_RATE` is saved through `CONFIG.SAVE`.
- Protocol: Optec Pyxis ASCII serial protocol at 19200 baud, 8N1, no command echo, and one outstanding command at a time. The bundled `17645_manual.pdf`, section 4.3, documents `CCLINK`, `CHOMES`, `CDnxxx`, `CMREAD`, `CGETPA`, `CPAnnn`, `CSLEEP`, `CWAKUP`, `CXxxxn`, and `CTxxxn`. Absolute coordinates are integer degrees 000-359. `CPA`/`CHOMES` emit `!` per step and `F` at completion; documented errors include `ER=1` for failed home, `ER=2` for an already-current target, and `ER=3` for an invalid absolute target. `CX` encodes direction in the tens digit (0/1) and 1-9 steps in the units digit, deliberately invalidates the device's absolute angle, and returns one `!`. `CT` accepts delays 00-99, with 08 as default. `CD` and `CSLEEP` have no reply.
- Supported devices/documentation: README lists Optec Pyxis 2-inch, 3-inch, and LE. The bundled manufacturer PDF is the behavior authority. The repository README is not modified by this refactoring.
- Defects and risks found before migration:
  - `optec_wakeup()` reports success after failed writes and timeouts, so connection and later commands can proceed after transport failure.
  - The driver uses non-portable `int` handles, `select`, `read`, `tcflush`, `close`, and pthread synchronization instead of `indigo_uni_io` and device queues.
  - Absolute move and home callbacks block until completion, preventing prompt disconnect and competing property work; they have only a per-byte timeout and no operation deadline.
  - Absolute position accepts -359 even though `CPA` only accepts 000-359, allowing malformed negative protocol commands.
  - `CPA` completion assigns no confirmed measured value; the public target can remain distinct from value and no `CGETPA` final readback is performed.
  - `X_ROTATE` changes the physical position without updating the controller's absolute-angle model by protocol design; the public absolute position is not explicitly marked unknown/stale.
  - `X_ROTATE` clears only its target, not its published value.
  - Connection initialization tolerates direction/position/rate setup failures and still reports the connection OK.
  - Close ownership uses descriptor value zero as a sentinel and the pthread mutex is never destroyed.
  - There is no generated source of truth and the driver version is `0x02000001`.
- Lifecycle/concurrency: one low-level serial session belongs to one logical device. The generator device queue will replace the private mutex. Open must be transactional; failed initialization closes all acquired resources. Disconnect must cancel pending motion finalization before closing and must not wait for a long-running movement loop.
- Platform/build integration: the top-level build lists `rotator_optec` among untested drivers and `Makefile.drv` builds the archive, shared library, and executable. The Xcode project contains the driver C/header/main, simulator, manual, and integration test, but not yet the new `.driver` or this refactoring record. The current macOS build emits both x86_64 and arm64 slices. Portable generated I/O is required for Linux/macOS/Windows source compatibility; Windows execution is unavailable in this environment.
- Properties documentation: `indigo_docs/PROPERTIES.md` lists the three correctly prefixed custom properties and currently cites the hand-written `.c`; its source note must point to the new `.driver` source of truth.
- Simulator/test inventory: a host PTY simulator and one normal integration test already exist and are wired into `indigo_test/Makefile` and Xcode. The simulator performs blocking movement, accepts malformed negative `CPA`, does not apply `CX` movement or invalidate absolute angle, has no protocol trace assertions or fault injection, and cannot model reconnect/transport failures deterministically. The existing test is one smoke case and may accept stale OK events; it lacks command encoding/order, BUSY transitions, no-op/overlap, wrap/boundary, failure/recovery, reconnect, active disconnect, persistence, and repeated INIT/SHUTDOWN coverage.
- Unsupported/non-applicable class behavior: SYNC and active/idle abort are not implementable because the documented controller protocol provides neither operation. Backlash, standard relative degrees, raw/offset, limits, and hand-controller polling are not exposed. Hardware-only movement, motor torque, home magnet, and physical transport interruption cannot be established by the simulator.

## Baseline (before production changes)

- Date/environment: 2026-09-13, macOS Darwin 25.6.0 on arm64 Apple hardware; repository working tree already contained unrelated edits to `indigo.xcodeproj/project.pbxproj` and an untracked `indigo_drivers/aux_dsusb/REFACTOR.md`, which will be preserved.
- Driver build: `make -B -C indigo_drivers/rotator_optec -f ../../Makefile.drv all` — passed; clang built x86_64 + arm64 archive, dylib, and executable.
- Integration build: `make -B -C indigo_test build/integration/test_rotator_optec_simulator` — passed; simulator and test built for x86_64 + arm64.
- Existing integration runtime: `(cd indigo_test && ./build/integration/test_rotator_optec_simulator)` — passed 1/1 (`optec_rotator_passes_serial_compliance_checks`).
- Pre-existing limitation: the passing smoke test does not establish the protocol, asynchronous, failure, lifecycle, or recovery behavior listed above.

## Hardware-test decision

No hardware test will be performed: no Optec Pyxis 2-inch, 3-inch, or LE device was supplied or identified as available. Hardware tests run/passed remain 0/0. The final result will claim simulator validation only; physical direction, step timing, home-magnet behavior, active cable removal, and post-failure hardware recovery remain unverified.

## Atomic migration plan

1. **Complete — establish audit and baseline.** Read repository/driver/test rules, manufacturer protocol pages 13-16, current sources, integration, and project wiring; the dual-architecture build and existing 1/1 simulator case passed with the exact evidence recorded above. The audit found that the existing signed `CX` encoding is consistent with the manual, while negative `CPA` is not.
2. **Complete — extract and normalize the generator source.** Reverse-extracted `indigo_rotator_optec.driver` with `build/bin/indigo_generator -c indigo_drivers/rotator_optec/indigo_rotator_optec.driver`, inspected the skeleton, and replaced hand-written lifecycle/I/O with generator-owned serial lifecycle plus `indigo_uni_io` helpers. Version advanced from `0x02000001` to generated 3.0.2 and copyright now includes 2026. Generation plus the dual-architecture `Makefile.drv` build passed without warnings.
3. **Complete — make motion bounded and asynchronous.** Implemented absolute/home start plus `motion_finalizer`, bounded progress reads, a rate-derived operation deadline, final `CGETPA` confirmation, cross-property BUSY rejection, and generated disconnect cancellation. A 1 ms availability probe keeps the bounded poll portable to Windows COM handles. Generated handlers were inspected: start handlers publish BUSY themselves and textual `_finalizer` references correctly suppress generator completion updates. Simulator cases cover BUSY/completion, overlap, protocol error, malformed final readback, timeout, active disconnect, and recovery.
4. **Complete — align settings and small-step behavior with the manual.** Absolute range is 0-359 and only `CPA%03d` is emitted. `CT` retains 00-99 and `CX` emits direction/step pairs such as `03` and `13`. Momentary small-step values reset to zero; successful `CX` marks absolute position ALERT/unknown and blocks `CPA` until a successful home. Failed setting writes preserve the prior accepted value. Reconnect reapplies the accepted rate; simulator traces verified the exact commands and repeated `CTxx05` application.
5. **Complete — upgrade the deterministic simulator.** Replaced the blocking loop with monotonic elapsed-time stepping, strict six-character command parsing/ranges, asynchronous `!`/`F`, documented error support, timestamped command traces, nth-command reply/drop injection, one-shot motion stalls, and a PTY keepalive for reconnect. The simulator compiles with strict warnings and is exercised by all ten integration cases. Physical `CX` displacement is intentionally not exposed as a valid absolute coordinate, matching the manual.
6. **Complete — expand rotator integration coverage.** Replaced the single stale-state smoke case with ten independent registered cases using property/state revisions: property contract/reconnect; absolute BUSY/no-op/overlap/boundaries/readback; controls/home/protocol/reconnect settings; connection failure/recovery; setting failure/accepted-value recovery; motion error/recovery; final-readback failure/recovery; home error/recovery; motion timeout/recovery; and disconnect-during-motion plus reinitialization. Normal execution passed 10/10, as did an ASan+UBSan build/run (leak detection is unsupported on this macOS runtime and was disabled).
7. **Complete — register persistent files and documentation.** Added `.driver`, `REFACTOR.md`, and the new Windows project to the existing Xcode driver group without disturbing concurrent project edits; `plutil` validation passed. Added `indigo_rotator_optec.vcxproj` for x64/ARM64 and registered it in `indigo_windows.sln` and `indigo_server.vcxproj`; XML validation passed, while an actual Windows build is unavailable. Updated the properties source note and the migration row to 3.0/API/Windows/generator/async/Sim with 10/0 tests, preserving the Comment column. Existing Makefile test wiring already used the exact driver test name and required no change.
8. **Complete — final verification and cleanup.** SHA-1 checks before/after the final unchanged-generator run were identical for generated C/H/main (`397b56…`, `ec6e4f…`, `3f5757…`). Generated version is `0x03000002`. The final universal x86_64/arm64 driver build passed with `-Wall -Wextra -Werror`; simulator and integration sources also passed strict compilation (only unused shared-harness helpers were excluded). The final normal suite and final native arm64 ASan+UBSan suite each passed 10/10; macOS rejected `detect_leaks=1`, so the successful sanitizer run used `detect_leaks=0`. `plutil`, `xmllint`, and `git diff --check` passed. `make -C indigo_test test-clean`, the driver clean target, and explicit removal of generated PDF previews and sanitizer binaries removed task artifacts. Native Linux/Windows builds and physical hardware remain unavailable and are not claimed.

## Scenario-to-test mapping

- `optec_property_contract_and_reconnect`: pre/post-connection visibility, interface, items, ranges, unsupported properties, disconnect deletion, and reconnect.
- `optec_absolute_motion_noop_and_overlap`: accepted target versus measured value, BUSY observation, cross-property overlap rejection, confirmed final readback, same-position no-command behavior, and 0/359 boundaries.
- `optec_controls_home_and_protocol`: rate, reversal, exact `CPA`/`CT`/`CD`/positive and negative `CX` encodings, momentary reset, invalidated absolute coordinate and blocked absolute move, home recovery, and rate application after reconnect.
- `optec_connection_failure_recovers`: malformed initialization read, transactional rollback, public disconnected/ALERT state, and successful retry.
- `optec_setting_failure_recovers`: malformed ACK, preservation of the accepted rate, and successful retry.
- `optec_motion_error_recovers`: documented-style `CPA` error, unchanged measured position, and subsequent successful move.
- `optec_final_readback_failure_recovers`: malformed completion readback, no target-as-value publication, and subsequent confirmed move.
- `optec_home_error_recovers`: documented `ER=1`, unchanged measured position, and successful subsequent home.
- `optec_motion_timeout_recovers`: absent motion progress/completion, bounded ALERT completion, unchanged measured position, and successful retry.
- `optec_disconnect_during_motion_and_reinitialize`: non-hanging active disconnect, shutdown, fresh INIT/connect, and successful movement.
- Abort, SYNC, backlash, standard relative-degrees, raw/offset, limits, and hand-controller polling are non-applicable because the driver does not expose them and the protocol cannot implement them. `CD` and `CSLEEP` have no reply, so device-side acceptance cannot be confirmed; write/close errors are the only observable transport failures. Generic configuration-file serialization is framework-owned; the driver-specific evidence verifies that the accepted `X_RATE` value is reapplied on reconnect without touching user configuration files.

## Final test summary

Simulated tests run: **10**. Simulated tests passed: **10**. Hardware tests run: **0**. Hardware tests passed: **0**. The ten distinct simulator cases were repeated under normal instrumentation and ASan+UBSan; totals count distinct cases rather than repeated executions. The pre-migration 1/1 smoke baseline is recorded separately above.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard in `indigo_rotator_optec.driver` are now declared with the generator's `reject_change` block for `X_HOME`, `X_RATE`, `X_ROTATE`, `ROTATOR_DIRECTION` and `ROTATOR_POSITION`. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values.

The hand-written `optec_operation_idle()` helper set `INDIGO_ALERT_STATE` and published the message but never marked the items, so the refused value stayed visible in the client. `ROTATOR_POSITION` carries a second guard for the invalidated absolute position; both are now declared blocks tested in the original order.

Covered by `optec_rejected_change` in `indigo_test/integration/test_rotator_optec_simulator.c`.

```sh
cd indigo_test && ./build/integration/test_rotator_optec_simulator
```

## Regenerated for the shared refusal and connect macros (2026-09-21)

No behaviour change and no version bump. `indigo_generator` stopped emitting the eight-line refusal
block and the five-line CONNECTION admission block inline and now emits `INDIGO_REJECT_CHANGE_IF()`
and `INDIGO_PROCESS_CONNECT()` / `INDIGO_PROCESS_QUEUED_CONNECT()` instead, so this driver was
regenerated along with the other 114 generator inputs. The macros expand to exactly the statements
that were written out before; for a sample of four drivers the preprocessed translation unit is
byte-identical apart from the new `indigo_reject_change()` declaration and shifted `assert()` line
numbers. Background and the defect that motivated the refusal macro are in `indigo_drivers/REVIEW.md`
(DRV-213, DRV-214) and `indigo_libs/REVIEW.md` (LIB-015, LIB-016).

Verification: the complete simulator suite was re-run after regeneration —
`indigo_test/build/integration/test_rotator_optec_simulator`, macOS arm64, 11/11 passed on 2026-09-21 12:43.
