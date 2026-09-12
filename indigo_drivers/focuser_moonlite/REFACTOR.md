# MoonLite focuser generated migration

Status: complete, 2026-09-12. Baseline version 0x0200000A; generated version 0x0300000B. Hardware tests are explicitly out of scope. No generator implementation change was made.

## Sources and protocol confidence

- Repository guidance: root and `indigo_test/AGENTS.md`, driver-development basics, generator migration guide, serial-simulator contract, focuser testing standard, and completed generated focuser migrations with `REFACTOR.md`.
- Production sources: hand-written `indigo_focuser_moonlite.c`, public header and main, host simulator, Arduino sketch, build/project integration and the existing integration smoke test. The user-facing README is reference material only and will remain unchanged.
- Manufacturer documentation: bundled `HighResSteppermotor107.pdf`, an eleven-page MoonLite High Resolution Stepper Motor Options manual, revision 09/13. All pages were text-extracted where possible and visually inspected. It defines 9600-baud ASCII commands framed by `:` and `#`, four-hex-digit position/temperature fields, two-hex-digit settings/status, the temperature-conversion delay and the single- and dual-port command sets.
- The driver uses the documented first-port commands `C`, `GC`, `GD`, `GH`, `GI`, `GN`, `GP`, `GT`, `GV`, `SC`, `SD`, `SF`, `SH`, `SN`, `SP`, `FG`, `FQ`, `+`, and `-`. Position is 0..65535. `SD` accepts 02/04/08/10/20, corresponding to approximately 250/125/63/32/16 steps per second. Temperature and coefficient are signed two's-complement fields; temperature units are half degrees Celsius.
- Expanded display/backlight, motor-scale, home and second-port commands belong to controller variants not exposed by this driver. Their protocol parsing/state will be audited in the host simulator, but they are not INDIGO property capabilities. Real firmware timing, motor direction, mechanical travel and controller variants still require hardware validation.

## Existing behavior and findings

- The driver exposes speed 1..5, direction, relative and absolute motion over 0..65535, abort, temperature, signed compensation, manual/automatic mode, reverse motion, software limits and custom `X_FOCUSER_STEPPING_MODE`. `FOCUSER_ON_POSITION_SET` remains hidden, so writable position always means GOTO and SYNC is unsupported even though the protocol provides `SP`.
- `moonlite_command()` ignores short writes and reports success after timeout, truncation or a reply without `#`. A full malformed reply can write the terminating NUL one byte beyond the caller buffer. Hex values are parsed without exact length, character, trailing-data or range validation.
- The driver uses raw `select`, `read`, `write` and `close`, a custom pthread mutex and timer rather than portable uniform I/O and the generated handler queue.
- Connection may block its handler for roughly ten seconds across six probe attempts. It then sends conversion, stop, full-step, manual-mode and speed commands without checking them, waits synchronously, ignores temperature/position/coefficient read failures and can publish a partially initialized device as connected. It resets device settings instead of reading the supported speed and stepping mode. A mode change publishes the wrong property before scheduling its handler.
- The compensation range attach code assigns `min` twice (`-128`, then `-127`) and never assigns `max`; the documented signed byte range is -128..127.
- Motion combines `SN` and `FG` into one unchecked write, polls position/status from the periodic timer, and has no stalled-motion or overall bound. A failed `GP`/`GI` poll can leave motion BUSY indefinitely. Completion does not verify the measured target. External idle position updates leave a stale target.
- Relative direction uses the legacy controller convention (inward adds unless reverse is enabled), then clamps to the controller range and configured software limits. This convention must be preserved and covered explicitly.
- Abort sends `FQ`, marks motion properties ALERT even on a successful stop, does not publish the measured stopped position and does not cancel identified pending completion work. Start/stop transport uncertainty and recovery are undefined. Disconnect stops best-effort but does not prove the stop or reconcile position.
- Speed, compensation and stepping setters have available getters but do not verify readback. Manual/automatic mode has no documented getter. Reverse motion and limits are local software controls.
- Temperature polling alternates `C` and `GT` on one-second ticks. The `has_temperature_sensor` field is actually conversion-phase state; failures are not consistently published, and the documented 750 ms conversion readiness is not represented precisely.
- The host simulator has a bespoke one-step-per-millisecond thread rather than `serial_motion.h`. It accepts malformed command lengths/hex, silently ignores unknown commands, does not model conversion readiness, and lacks deterministic speed timing, event journal, split frames, protocol/transport faults, stall/external-state controls, startup rollback cases and instance isolation evidence.
- The existing automated suite is one broad smoke case. The unchanged test builds but terminates with `SIGSEGV` (exit 139) on the current macOS arm64 baseline. It does not independently validate protocol grammar, property contract, initialization rollback, readbacks, limits/reverse direction, zero/boundary/no-op motion, overlap, stalled/read/start/stop failures, abort recovery, idle polling, disconnect/reconnect or additional instances.

## Atomic plan and progress

1. **Baseline and evidence capture - complete.** Read repository/test standards, build the unchanged 0x0200000A driver and simulator, run the existing test, inspect all eleven PDF pages, and record supported/non-applicable capabilities plus concrete defects. Baseline build passed; the legacy runtime test reproduced exit 139.
2. **Independent simulator contract - complete.** Move the host simulator to `serial_motion.h`; validate exact framing, command lengths and hex fields; model documented state and the 750 ms temperature conversion; add an event journal and deterministic split/malformed/silent/partial/overlong/close/stall/external-state controls. Preserve the Arduino sketch as a historical fixture.
3. **Regression suite - complete.** Replace the smoke case with isolated named scenarios, fresh property revisions, child watchdogs and simulator cleanup. Cover direct protocol behavior, property visibility/ranges, initialization rollback/retry, settings, motion/boundaries/limits/reverse, overlap, abort/recovery, fault paths, idle polling, disconnect/reconnect and additional instances.
4. **Authoritative generator migration - complete.** Create `indigo_focuser_moonlite.driver`, use generated lifecycle/queues and portable uniform I/O with one reusable response buffer, make open/init transactional, strictly validate replies, implement bounded nonblocking motion finalization and explicit abort/disconnect recovery, preserve supported public behavior, fix the compensation range and raise the driver version.
5. **Full validation and hardening - complete.** Regenerate checked-in C/header/main reproducibly, build the driver, run every simulator scenario and targeted sanitizer cases, compile production/simulator code with strict warnings where available, and inspect generated lifecycle/finalizer semantics.
6. **Repository synchronization - complete.** Add all new persistent files to the existing Xcode group, leave README unchanged, update `PROPERTIES.md`, simulator inventory, `indigo_test/CHANGES.md` and only the MoonLite status fields in `MIGRATION_STATUS.md`; verify Xcode syntax, diff hygiene and cleanup while preserving unrelated worktree changes.

## Required coverage and boundaries

The final scenario matrix must cover nominal and split ASCII framing; invalid prefix/terminator/length/hex/unknown commands; signed temperature/coefficient and conversion readiness; full documented simulator command/readback state; connection probe and every required setup/read failure with same-process recovery; pre/post-connect visibility, interface, permissions, ranges and initial readback; speed 1..5, stepping, compensation boundaries and mode commands; absolute zero/no-op/boundary GOTO; inward/outward relative motion with reverse disabled/enabled and limit clamping; BUSY/progress/OK and exact final measured position; overlapping requests; idle/moving abort, failed stop and fresh-motion recovery; stalled and failed position/status polling; externally changed position; disconnect during motion, reconnect, repeated lifecycle and independent additional instances.

SYNC is non-applicable because `FOCUSER_ON_POSITION_SET` is not exposed. Backlash, homing, display/backlight/contrast, motor scaling and the second motor port are not exposed by the INDIGO driver. Generic framework numeric validation will not be duplicated. Hardware tests, Linux/Windows runtime, physical endpoints/direction/motion, real temperature accuracy/conversion timing, controller variants and multiple physical serial controllers remain unverified.

## Steps 2-4 result

The host simulator now uses monotonic elapsed-time `serial_motion` state for each of two independent motor ports. It strictly accepts complete colon/hash frames, exact hexadecimal field widths and only documented values. It implements current/new position, motion status/start/stop, five speed encodings, full/half step, signed temperature/coefficient, the 750 ms conversion window, compensation enable, calibration offset, temperature scale, RGB backlight, contrast, motor scales and home commands. A private event/fault-file contract provides split replies, silent/malformed/partial/unterminated/overlong/closed transport, ignored setters, stalls, external position and signed temperature. The Arduino sketch remains unchanged as a historical fixture.

The generated driver uses uniform I/O with a single reusable private response buffer and requires an exact, terminated payload with no trailing data. Firmware version, position, coefficient, speed, stepping mode and converted temperature are mandatory transactional initialization readbacks; any failure closes the acquired handle before a tested same-process retry. Existing device settings are read instead of forcibly replacing speed and stepping mode. Speed, stepping and signed compensation writes are verified through their documented getters. Manual/automatic mode remains write-only because the protocol defines no getter.

Absolute and relative starts write `SN`, confirm `GN`, then issue `FG`. Relative arithmetic uses a 64-bit intermediate, preserves the legacy MoonLite inward/reverse convention and clamps to the controller and accepted software limits. The named `motion_finalizer` publishes measured progress without blocking the handler queue, validates both `GP` and `GI`, confirms a stopped-position snapshot, and stops after about five seconds without progress. A race found during sanitizer/repeated execution—movement completing between the `GP` and `GI` queries—is handled by one confirming `GP` read when status first becomes stopped. Abort cancels the identified start/finalizer work, stops, reads the actual position, clears BUSY on success and leaves an explicit recoverable uncertain/ALERT state on failure. Disconnect cancels pending finalization and best-effort stops active or uncertain movement. Idle polling synchronizes externally changed position, and temperature conversion is scheduled as bounded queue work rather than a misleading sensor flag.

The documented compensation range is corrected to -128..127. Software limits now reject an inverted minimum/maximum and explicitly accept their target values. The legacy erroneous mode-property update disappears with generated dispatch. `FOCUSER_ON_POSITION_SET` stays hidden, so GOTO behavior and the unsupported SYNC boundary are preserved.

## Final validation and Step 6 result

- Final ordinary simulator suite: **38/38 scenarios passed** on macOS arm64. Three direct protocol cases cover nominal/split framing, conversion readiness, strict rejection, both motor ports, settings and elapsed motion. Isolated driver cases cover the complete applicable property, readback, movement, endpoint, limit/reverse, overlap, abort, initialization, framing fault, polling, external state, stall, stop, transport, reconnect and additional-instance matrix.
- Targeted ASan: **25/25 selected scenarios passed** (`init_`, `poll_`, `_reject`, `failure`, `abort`, `disconnect`, `instances`). Generated production C and the test harness were instrumented; the prebuilt INDIGO shared library and separate simulator executable were not. LeakSanitizer was disabled at that prebuilt framework boundary. No ASan finding occurred.
- Generated C passed `-Wall -Wextra -Wconversion -Wshorten-64-to-32 -Wconditional-uninitialized -Wunreachable-code -Wcomma -Werror` at O0 and O2 for arm64 and x86_64 compilation. The simulator passed strict C11 `-Wall -Wextra -Werror`. The universal production archive, dylib and executable build passed.
- A repeated generator invocation produced byte-identical C, header and main files. Generated handler/finalizer dispatch, transactional rollback, disconnect cancellation and the absence of a `MAX_DEVICES` override were inspected. `git diff --check` and Xcode `plutil` validation passed.
- `REFACTOR.md` and `indigo_focuser_moonlite.driver` are referenced by the existing MoonLite Xcode group; the simulator, PDF, generated files and integration test are also referenced. No dedicated Windows project entry exists. README is unchanged. Property, simulator, automated-test and migration inventories agree.

Hardware behavior, real firmware variants, physical travel/direction, motor speed/step effects, temperature calibration, dual-controller electrical operation, Linux/Windows compilation/runtime and x86_64 runtime remain unverified. The x86_64 result above is compilation only. No hardware test was attempted.

Reproduction from repository root:

```sh
build/bin/indigo_generator indigo_drivers/focuser_moonlite/indigo_focuser_moonlite.driver
make -C indigo_drivers/focuser_moonlite -f ../../Makefile.drv
make -C indigo_test build/integration/test_focuser_moonlite_simulator build/integration/test_focuser_moonlite_simulator_asan
cd indigo_test
./build/integration/test_focuser_moonlite_simulator
for moonlite_filter in init_ poll_ _reject failure abort disconnect instances; do
  MOONLITE_TEST_FILTER="$moonlite_filter" ASAN_OPTIONS=detect_leaks=0 ./build/integration/test_focuser_moonlite_simulator_asan || exit 1
done
```
