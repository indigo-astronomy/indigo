# WandererAstro rotator migration

## Baseline and audit (2026-09-15)

Source baseline: hand-written driver version 0x03000003, portable uni_io but timer-dispatched handlers protected by a pthread mutex. Mini/MiniV2 use 1142 steps/degree; Lite/LiteV2 use 1199. Public properties: common serial and additional-instance properties, POSITION, RELATIVE_MOVE, ON_POSITION_SET, ABORT_MOTION, DIRECTION, BACKLASH (0–5 degrees), RAW_POSITION, POSITION_OFFSET, and X_SET_ZERO_POSITION.SET_ZERO_POSITION. SYNC is software calibration; mechanical zero writes 1500002. No home, guider, raw-position write or device travel-limit capability.

Read root/driver/test instructions, README, DEVELOPMENT, DRIVER_DEVELOPMENT_BASICS, DRIVER_GENERATOR_MIGRATION, MAKEFILES, SERIAL_DEVICE_SIMULATORS, testing class rules and both bundled manufacturer protocol PDFs. PDFs specify 19200 8N1, A-delimited handshake/completion, signed steps, backlash tenths, reversal, stop and NP undervoltage. They do not document the old absolute-move +1000000 encoding. Existing simulator omits terminal A, reports steps instead of rotated degrees, has no elapsed motion or fault injection, and decodes negative fast moves incorrectly. Existing test is one smoke/compliance case with no failure or race coverage.

Baseline commands on macOS arm64 (universal arm64/x86_64 builds): `make -f ../../Makefile.drv` in driver directory PASS; `make -C indigo_test build/integration/test_rotator_wa_simulator` PASS; `cd indigo_test && build/integration/test_rotator_wa_simulator` 1/1 PASS. Calibration writes to user configuration were denied in sandbox; calibration persistence cannot be validated in this sandbox: uni_io has no configurable folder override and uses the user HOME; writes remain denied. No user configuration modified. Initial git status clean.

Supported platforms: platform independent Linux/macOS/Windows. Existing Makefile.drv, Windows vcxproj and Xcode C/header/main entries. No .driver input. Linux/Windows execution unavailable here; both macOS architectures available for compilation; x86_64 execution will be attempted.

## Hardware decision

No physical hardware tests requested or performed. All validation uses PTYs and public bus APIs; physical motor accuracy, older firmware differences and serial reset timing remain hardware-only. Preserve the existing two-second startup delay as a bounded connection transaction.

## Found defects

- Source audit: completion reader loops forever on silence/read failure while holding the mutex; abort depends on a copied switch and disconnect cannot safely interrupt it. Fix: queued bounded finalizer with monotonic deadline and explicit abort/disconnect cancellation. Regression: delayed, silent, abort and reconnect cases.
- Source audit: atof/atoi accept malformed/NaN fields, NP prefix accepts junk; parsing and unknown-model initialization can publish invalid data. Fix: strict complete numeric/model/frame validation and transactional open. Regression: handshake and completion fault matrix.
- Source audit: absolute move uses undocumented +1000000 encoding, including negative moves. Fix: documented signed step commands for both move kinds. Regression: signed Mini/Lite and wrap/pivot command assertions.
- Source audit: offset branch copies ON_POSITION_SET instead of OFFSET and computes pivot before new calibration is applied. Fix: queued calibration changes update offset and pivot together. Regression: SYNC/offset/pivot moves.
- Source audit: zero and stop report success without verified readback; failed send still enters completion wait. Fix: verify settings/zero/abort using handshake and preserve confirmed measurements on failure. Regression: setting/zero/stop failures and recovery.

- Reproduced during new tests: INFO model/firmware remained stale because generated connection scaffolding does not publish INFO. Fix: explicit INFO updates in connect/disconnect. Regression: all four model cases.
- Reproduced simulator lifecycle defect: EOF ended simulator at PTY close. Fix: tolerate EOF and continue; regression: direct reopen and driver reconnect cases.

- Source audit: urgent abort could precede a queued start, allowing movement after abort. Fix: cancel the identified pending absolute/relative initiators as well as the finalizer. Regression: wa_abort_queued_start (three immediate aborts and no stale revisions).
- Source audit: settings could retain ALERT after reconnect. Fix: initialize control states and relative request values from the fresh session. Regression: wa_setting_failures reconnect/readback.

## Atomic plan

1. Baseline and audit: COMPLETE; commands and protocol evidence above.
2. Migrate to .driver with strict protocol helpers, transactional lifecycle, serialized operations and bounded motion finalizer. COMPLETE: generation and universal build passed; generated handlers inspected. Abort references suppress generated completion, so its state/update are explicitly owned. Generator already cancels all handlers on disconnect. Generate and compile; inspect generated state ownership.
3. Audit/extend simulator using serial_motion.h, model selection, trace/control and deterministic faults. COMPLETE: strict clang -Wall -Wextra -Werror build passed. Direct PTY protocol audit PASS: documented framing/degree completion, elapsed-time motion, stop, zero and reopen. EOF on macOS PTY must keep simulator alive; fixed and verified. Strict simulator build and direct protocol checks.
4. Extend named driver integration cases across all applicable rotator scenarios, lifecycle, framing, failures and concurrency. COMPLETE: 34/34 final cases passed separately on arm64, x86_64 and ASan/UBSan (102/102 acceptance executions). Original compliance requests now assert fresh revisions. Atomic rename publishes fault controls to avoid simulator reading a partially written file. Historical defined-property inventory is not a deletion cache; deletion assertions use find_cached_property. Run suite, fix regressions, record scenario mapping.
5. Register new input/log in Xcode, reconcile properties reference and migration status. COMPLETE: preserved user Xcode registration of .driver/REFACTOR; plutil lint PASS. Added Windows project None entries, updated properties source mapping (property inventory unchanged), migration row 34 / 0 and simulator-only retest. Comment preserved exactly. Check project syntax, counts and unchanged Comment.
6. Final strict builds, sanitizer suite, both available architectures, reproducibility and diff/cleanup audit. COMPLETE: universal driver build, strict checks and all three acceptance suites PASS; generated C/header/main SHA-256 unchanged after regeneration. Project XML/plist and diff checks PASS; no generator changes, MAX_DEVICES overrides, README edits or added build products. Test cleanup completed as recorded below.

## Scenario-to-test mapping

| Driver-owned area | Registered tests and assertions |
| --- | --- |
| Public inventory and lifecycle | wa_rotator_passes_serial_compliance_checks; wa_metadata_open_failure: metadata/interface, serial and rotator property/items, before/after connection, repeated INIT/SHUTDOWN, invalid open rollback and fresh operation. |
| Model and signed step mapping | wa_mini_moves, wa_miniv2_moves, wa_lite_moves, wa_litev2_moves: exact model, 1142/1199 steps per degree, positive/negative absolute and relative commands and measured readback. |
| Wrap/pivot and calibration | wa_wrap_sync_offset_zero: both wrap directions, external position read at GOTO, software SYNC without a move, pivot after SYNC and offset, verified mechanical zero and calibration property updates. |
| Controls/no-op | wa_noop_and_settings: absolute/relative zero emits no movement command, backlash degrees to tenths at 0/0.5/5, both direction settings, device setting readback after reconnect. |
| Overlap/abort/queue races | wa_busy_abort_recovery, wa_abort_queued_start: both motion properties BUSY, rejected overlapping moves/settings/zero/offset, measured position retained until confirmation, active/idle abort, cancelled queued starts, no stale completion, fresh subsequent move. |
| Disconnect and independent instances | wa_disconnect_during_motion, wa_independent_instances: pending finalizer cancellation, custom-property deletion, no further status requests on closed session, reconnect and fresh motion; second Lite instance has independent commands/handle and first Mini survives its disconnect. |
| Handshake parsing/rollback | wa_bad_model, wa_bad_position, wa_bad_backlash, wa_bad_reverse, wa_bad_firmware, wa_missing_field, wa_extra_field, wa_handshake_silence, wa_handshake_truncated, wa_handshake_overlong: invalid identity/numbers/ranges/field count/framing/silence rejected; no connected custom property and successful retry/move. |
| Completion framing and failures | wa_no_power, wa_completion_invalid, wa_completion_nan, wa_completion_missing, wa_completion_extra, wa_completion_truncated, wa_completion_overlong, wa_split_frames: NP/invalid payloads never become position; CRLF/A framing and fragmented replies, ALERT and successful subsequent GOTO. |
| Silence and transport loss | wa_timeout_recovery: real 32-second deadline for a one-degree move with no completion, bounded ALERT then recovery. wa_transport_loss: PTY master removal during motion, failed abort/write, bounded disconnect and new PTY reconnect/move. |
| Setting and stop failures | wa_setting_failures: ignored setting/zero, malformed pre-GOTO readback, confirmed accepted values preserved, retry and settings reset/readback after reconnect. wa_stop_failure: ignored stop/absent status returns ALERT; successful second stop and move. |

No home/calibration motor workflow, guider, raw-position write, travel-limit commands or SDK/hotplug interfaces are implemented, so those class scenarios do not apply. The protocol has only completion feedback, not a moving-status field or progress stream; no speculative position polling is sent during movement. Idle hand-controller polling was not provided by the old driver and neither bundled protocol specifies a hand controller. SYNC/offset storage is handled by shared rotator calibration APIs; generic disk configuration is outside driver acceptance. Calls and updated values are exercised, but actual disk persistence is unverified due to the sandbox denial. The documented final A is accepted; legacy LF replies without final A remain compatible, while LF termination, payload and field count are required.

## Final verification evidence

- `make -f ../../Makefile.drv` in `indigo_drivers/rotator_wa`: universal arm64/x86_64 archive, dylib and executable PASS; version increased from 0x03000003 to 0x03000004.
- `make -C indigo_test build/integration/test_rotator_wa_simulator`: universal simulator and integration executable PASS.
- `cd indigo_test && build/integration/test_rotator_wa_simulator`: final 34/34 PASS, exit 0.
- `cd indigo_test && arch -x86_64 build/integration/test_rotator_wa_simulator`: final 34/34 PASS, exit 0 (Rosetta x86_64 driver execution).
- Strict driver: `clang -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -Iindigo_libs -Ibuild/include -Wall -Wextra -Werror -Wno-unknown-pragmas -fsyntax-only indigo_drivers/rotator_wa/indigo_rotator_wa.c` PASS. Simulator strict check uses `-Wall -Wextra -Werror -Wno-unused-function`; integration also excludes existing shared-harness unused parameters. No production diagnostic suppressed except generated pragmas.
- ASan/UBSan: separately compile driver object, simulator and integration source with clang `-arch arm64 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`, the strict flags above, normal repository include paths and existing libindigo/libindigocat. Integration links the instrumented driver object and sets ROTATOR_WA_SIMULATOR_EXECUTABLE to the instrumented simulator in a temporary directory. Final 34/34 PASS, exit 0, no sanitizer reports. Shared framework libraries were not rebuilt with sanitizers; LeakSanitizer is unavailable on this macOS setup.
- Regenerated with `build/bin/indigo_generator indigo_drivers/rotator_wa/indigo_rotator_wa.driver`: SHA-256 of all three outputs unchanged.
- Direct PTY protocol audit PASS: exact status framing, elapsed motion before target, stopped intermediate position, mechanical zero, degree-valued completion and PTY reopen. One audit run, six protocol assertions; separate from registered driver-case totals.
- `plutil -lint indigo.xcodeproj/project.pbxproj`, Python XML parsing of vcxproj and `git diff --check`: PASS. Migration Comment matches its pre-change value exactly.
- `make -C indigo_test test-clean`: removed test binaries/dSYM artifacts. Removed task-created temporary sanitizer builds/scripts/logs and abandoned simulator ready directory. Completed test/simulator processes exited; the earlier blocked diagnostic process was terminated with user-approved scope.

Intermediate diagnostic suites were retained as failure evidence until the final runs: 31 cases (29 passed; historical-inventory deletion assertion and stop injection failed), 33 sanitizer cases (32 passed; stop injection failed), 33 x86_64 cases (33 passed), and 34 native cases (33 passed; stop injection failed). Those four failures are resolved by using the deletion cache and atomic fault publication; all affected cases passed in the final three suites. An earlier interrupted diagnostic suite had buffered output and cannot provide reliable case totals; it is explicitly excluded from completed-run totals. Three one-case smoke executions and three selected regression executions passed.

Linux and Windows builds/execution and physical hardware remain unverified. Windows support denotes portable source/project integration, not a tested Windows build. Older firmware and physical timing/precision remain hardware gaps. Generic calibration disk storage was denied by the sandbox, as recorded above.

## Final test summary

Final simulated acceptance tests run: 102; passed: 102 (34 cases on each of arm64, x86_64 and ASan/UBSan).
Total completed registered simulated test executions, including baseline and diagnostic runs: 239; passed: 235; failed: 4 (intermediate failures resolved). Interrupted diagnostic totals are unavailable and excluded. Direct protocol audit runs: 1; passed: 1.
Hardware tests run: 0; passed: 0.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard in `indigo_rotator_wa.driver` are now declared with the generator's `reject_change` block for `ROTATOR_POSITION_OFFSET`, `ROTATOR_POSITION`, `ROTATOR_RELATIVE_MOVE`, `ROTATOR_DIRECTION`, `ROTATOR_BACKLASH` and `X_SET_ZERO_POSITION`. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values.

The hand-written `wa_idle()` helper only sent a message: the property kept its previous state and the client kept the refused value. The two motion properties refused each other silently, with no update at all.

Covered by `wa_rejected_change` in `indigo_test/integration/test_rotator_wa_simulator.c`.

```sh
cd indigo_test && ./build/integration/test_rotator_wa_simulator wa_rejected_change
```
