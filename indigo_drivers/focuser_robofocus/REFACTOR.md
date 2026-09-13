# RoboFocus focuser generated migration

Status: complete, 2026-09-13. Baseline version 0x02000001; generated version 0x03000002. Hardware tests are explicitly out of scope. The user-facing README was not changed.

## Sources and protocol confidence

- Repository guidance: root and `indigo_test/AGENTS.md`, driver-development basics, generator migration guide, serial-simulator contract, common/focuser testing standard, and completed generated focuser migrations.
- Production sources: the hand-written driver, public header and standalone wrapper; the host and Arduino simulators; current integration test; property inventory; build, Xcode and migration records.
- Manufacturer documentation: bundled 45-page RoboFocus Instructions version 3.1/3.2. Its Appendix 1 defines RS-232 at 9600-8N1 and fixed nine-byte packets: eight payload bytes followed by the low byte of their sum. It documents firmware, absolute/relative motion, position sync, maximum travel, four power outputs, backlash, motor configuration, temperature and the stop-on-any-serial-activity rule.
- The manual says valid motion/position values are six decimal digits with a practical 64,000/65K limit, and describes the configuration bytes as raw character values. It says motion emits roughly 10-50 `I`/`O` ticks per second and a final nine-byte position packet. Simulator-backed validation can establish driver behavior against this grammar, but cannot certify firmware variants, serial electrical behavior, physical travel, temperature accuracy or power switching.

## Existing behavior and findings

- The driver exposes absolute and relative position, direction, local reverse-motion, abort, temperature, local min/max limits, four power outlets and a combined duty-cycle/step-delay/step-size/signed-backlash property. Speed is hidden. It permits additional serial instances.
- Connection retries firmware identification up to six times with two-second sleeps, then reads position, power, configuration and backlash. Partial initialization still reports the connection as successful and only marks individual properties ALERT. Maximum travel is not read from the controller.
- `robofocus_command()` uses raw POSIX `select`, `read`, `write` and `close`, assumes one `read(..., 8)` returns a whole frame, has no portable Windows path, ignores write results, stores response arrays on multiple stacks and explicitly does not validate reply checksums or payload grammar.
- Motion is executed by a blocking command helper that consumes tick bytes until the final position reply. The device queue is occupied for the whole move, so queued abort/disconnect cannot run promptly. There is no overall deadline, stalled-motion bound or defined recovery after transport loss. Abort sends a carriage return, although the manual says any serial activity stops motion, and it does not read back the actual stopped position.
- Absolute motion parses the final position at `response + 3` instead of `response + 2`, losing the first digit. Relative motion is implemented as absolute GOTO and has the same parse defect. `FOCUSER_ON_POSITION_SET` is not exposed even though the controller supports both GOTO and sync (`FS`).
- Limits are local only: the driver exposes `FOCUSER_LIMITS` but never reads/writes the documented `FL` maximum-travel setting. Reversal is also intentionally local and changes command direction rather than a controller setting.
- Configuration replies contain raw bytes. The driver constructs `FC` with `sprintf("...%c...")`; a zero duty-cycle inserts NUL into the command and makes string-oriented assumptions unsafe. It uses `X_FOCUSER_CONFIG_BACKLASH_ITEM->number.value` instead of the requested target to select backlash direction, so a sign change can be encoded incorrectly. It accepts unvalidated prefixes, lengths, digits, raw field ranges and checksums.
- The periodic temperature conversion rounds the raw offset before division and therefore drops the documented half-degree resolution. Poll failures are not published. Position is not polled while idle, so handset/external changes are invisible.
- The existing host simulator implements nominal request/reply state but accepts invalid checksums and fields, completes moves instantaneously, emits no documented progress ticks, treats carriage return specially, does not use `serial_motion.h`, and has no command journal, split-frame mode, fault injection, stalled motion, external mutation, reconnect/resource or independent-instance evidence.
- The current integration test is one smoke scenario. It does not independently verify simulator protocol/checksums, command arguments/order/readback, fresh revisions, GOTO/SYNC distinction, motion progress/abort/recovery, boundaries/no-op/overlap, limit/config/power partial failures, malformed or bad-checksum replies, polling/external state, disconnect during motion, reconnect or additional-instance isolation.

## Atomic plan and progress

1. **Baseline and evidence capture — complete.** Read repository and driver guidance, inspect the complete bundled manual including the protocol appendix, inventory behavior/properties/build wiring, build the unchanged driver/simulator/test and run the original smoke case.
2. **Independent simulator contract — complete.** Refactor the host simulator onto `serial_motion.h`; implement strict nine-byte checksum and field validation, elapsed motion with documented ticks and stop semantics, all documented driver-used commands, deterministic events/fault control, alternate and split profiles, external state mutation and protocol-level self-check scenarios. Keep the Arduino sketch as a historical hardware fixture.
3. **Authoritative generator migration — complete.** Create `indigo_focuser_robofocus.driver`; regenerate C/header/main; adopt uniform I/O, one reusable response buffer, strict complete-frame/checksum parsing, transactional open/initialization, generator-owned queues and a nonblocking motion finalizer. Preserve public properties, expose the supported GOTO/SYNC choice, integrate device maximum travel, define abort/disconnect recovery and bump the version.
4. **Complete regression suite — complete.** Replace the smoke test with isolated named simulator-backed cases covering the full applicable focuser standard and driver-specific protocol/settings/failure matrix, with bounded waits and cleanup. Do not test generic framework numeric validation.
5. **Validation and hardening — complete.** Build/regenerate reproducibly; run direct protocol and complete driver scenarios, targeted sanitizers and strict warnings where available; inspect generated queue/urgent-abort/finalizer and rollback semantics; iterate until applicable hardware-free coverage passes.
6. **Repository synchronization — complete.** Add every new persistent non-Visual-Studio file to Xcode, add a Visual Studio project for x64/ARM64 to the Windows solution and server dependencies without adding its files to Xcode, update properties/simulator/test/migration records, preserve the README and unrelated edits, validate project syntax and diffs, remove test artifacts/processes, and record precise simulator/platform/hardware boundaries.

## Required coverage and boundaries

The completed matrix must map named cases to: metadata and pre/post-connect visibility; strict frame checksum and command grammar; initialization success and each decision-point failure; initial position/maximum/config/backlash/power/temperature readback; absolute GOTO and SYNC; inward/outward relative motion, local reversal, zero/no-op and min/max boundaries; BUSY/progress/final position, overlap rejection, idle and active abort, malformed progress/final replies, start/stop/read loss and fresh-move recovery; external position/temperature; local minimum and controller maximum limits; every power channel; binary duty/delay/step-size settings; positive/negative/zero backlash; partial setting failure/reconciliation; disconnect during motion, reconnect and independent additional instances.

Controller speed selection and temperature-compensation algorithms are non-applicable because this driver does not expose them. Hardware tests will not be run. Simulator results cannot verify actual firmware timing/quirks, electrical serial behavior, physical direction/travel/end stops, motor configuration effects, temperature accuracy, remote power loads, Windows/Linux runtime or real multiple-controller operation.

## Baseline result

The unchanged hand-written 0x02000001 driver built as a universal macOS archive, dynamic library and standalone executable. The existing host simulator and integration test built, and its sole smoke scenario passed. No production behavior or user documentation was changed during baseline capture.

## Implementation result

The `.driver` definition is now authoritative and generates the checked-in C, public header and standalone wrapper. The driver uses a binary-logged uniform-I/O handle and a single private nine-byte response buffer. Every request and response is read as a complete fixed frame, and response prefixes, decimal fields, raw-byte fields and checksums are validated before accepted values are changed. Open plus the required position/maximum/power/configuration/backlash/temperature initialization is transactional; any failure closes the handle and a retry in the same process is supported.

GOTO starts on the device queue and returns immediately. `motion_finalizer` drains bounded batches of documented `I`/`O` progress bytes, publishes measured progress, checks the final `FD` packet and detects roughly three seconds without progress. The queue therefore remains available to the generator's urgent abort handler. Abort cancels a queued start or pending finalizer, sends the documented carriage-return stop activity, consumes the stopped position, and uses an explicit position query to recover if the final motion packet was malformed or already consumed. Disconnect cancels pending work and best-effort stops uncertain motion.

The standard GOTO/SYNC selector is visible; SYNC uses `FS` without motor motion. Maximum limits are read from and written to `FL`, while the minimum remains a local bound because the protocol exposes no minimum-travel setting. Reverse motion remains the existing local direction transform. Temperature preserves half-degree raw resolution. Power and motor configuration use explicit binary payload construction, including a zero duty byte. Backlash direction now uses the requested target sign and the documented 0..255 magnitude.

The host simulator validates incoming request checksums and field grammar, uses monotonic `serial_motion` state, emits one progress byte per simulated step, implements stop-on-any-serial-activity, and models every command used by the driver. It provides normal, split, alternate and one-shot-stall profiles, a binary command journal, response faults, and external position/temperature mutation. The Arduino sketch remains unchanged and is documented as a historical hardware fixture.

## Validation result

- Final ordinary simulator suite: **18/18 isolated scenarios passed** on macOS arm64. Coverage includes a transport-independent direct protocol case; normal/alternate/split capabilities; GOTO/SYNC, relative/reversed/no-op/limited movement with BUSY progress; queued overlap and idle/active abort; all power/configuration/backlash controls; polling/final checksum errors; stall and recovery; three transactional initialization failures with descriptor rollback and retry; external state, reconnect, disconnect during motion and an independent additional instance.
- AddressSanitizer: **18/18 scenarios passed** with the generated production source and test harness instrumented; leak detection was disabled at the prebuilt framework boundary. No ASan finding occurred.
- Generated production C passed `-Wall -Wextra -Wconversion -Wshorten-64-to-32 -Wconditional-uninitialized -Wunreachable-code -Wcomma -Werror` at O0 and O2. The simulator passed C11 `-Wall -Wextra -Werror`. A strict standalone test compilation is blocked by warnings in the shared test harness; the repository's normal universal test build is warning-clean for this test source.
- A repeated generator invocation produced byte-identical C, header and main files. The generated connection queue, urgent abort dispatch, `_finalizer` completion ownership, transactional rollback and absence of a `MAX_DEVICES` override were inspected. The driver archive, dynamic library and executable built for macOS arm64/x86_64.
- `REFACTOR.md` and the `.driver` source are in the existing RoboFocus Xcode group; the simulator and integration test references remain present. The new Visual Studio x64/ARM64 project follows the `focuser_ioptron` template and is wired into `indigo_windows.sln` and the server project dependencies. Per the explicit repository rule for these files, the `.vcxproj` and `.vcxproj.filters` are not added to Xcode. Property, simulator, test-coverage and migration records are synchronized.

No hardware test was attempted. Real controller firmware variants, physical direction/travel/end stops, progress timing, temperature accuracy, motor-configuration effects, switched loads, Linux/Windows compilation and runtime, and real multiple-controller operation remain unverified.

Reproduction from the repository root:

```sh
build/bin/indigo_generator indigo_drivers/focuser_robofocus/indigo_focuser_robofocus.driver
make -C indigo_drivers/focuser_robofocus -f ../../Makefile.drv
make -C indigo_test build/integration/test_focuser_robofocus_simulator
cd indigo_test
./build/integration/test_focuser_robofocus_simulator
```
