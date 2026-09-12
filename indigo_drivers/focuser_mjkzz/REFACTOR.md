# MJKZZ Rail focuser generated migration

Status: complete, 2026-09-12. Baseline version 0x02000004; generated version 0x03000005. Hardware tests are explicitly out of scope. No generator implementation change was made.

## Sources and protocol confidence

- Repository guidance: root and `indigo_test/AGENTS.md`, driver-development basics, generator migration guide, serial-simulator contract, focuser testing standard, and completed generated focuser migrations with `REFACTOR.md`.
- Production sources: hand-written `indigo_focuser_mjkzz.c`, public header, `mjkzz_def.h`, host simulator, Arduino sketch, build/project integration and the existing one-case integration test. The user-facing README is reference material only and will remain unchanged.
- Manufacturer documentation: bundled `MJKZZ.pdf`, a ten-page MJKZZ Serial Camera Motion Controller manual dated 2016. All pages were text-extracted where possible and visually inspected. It defines the complete eight-byte binary message, address and command acknowledgement bits, checksum, signed big-endian 32-bit value encoding, 9600 baud/one stop bit/no parity, controller registers and commands.
- The manual explicitly defines `SPOS`/`GPOS`, signed incremental `MOVE`, `SSPD`/`GSPD`, and `STOP`. `STOP` stops all actions and returns the actual stopped rail position. Speed accepts 0..255, while 0..3 is recommended because higher values are slower. The current driver deliberately exposes the recommended 0..3 range and uses absolute `SPOS` for both absolute and computed relative targets.
- Real firmware timing, mechanical travel, electrical serial behavior and device variants still require hardware validation. Simulator tests establish conformance to the bundled protocol document and driver behavior, not physical performance.

## Existing behavior and findings

- The driver exposes speed 0..3, direction, relative steps 0..1000, writable absolute position -32768..32767, and abort. Reverse motion and temperature are hidden; `FOCUSER_ON_POSITION_SET` remains hidden, so writable position always means GOTO and SYNC is unsupported.
- Connection opens the serial device, probes `GVER`, then writes hold power 16, low power 2 and microstep mode 0. The bundled manual restricts both power registers to 1..12, so the legacy hold-power value 16 is outside the documented 0.125..1.5 A range. Those three setup replies are ignored. Position and speed read failures are also ignored, so a partially initialized controller may be published as connected with stale defaults.
- `mjkzz_command()` calculates the outgoing checksum but accepts any eight returned bytes. It does not validate the response checksum, response address/high acknowledgement bit, command/high acknowledgement bit, command identity, index, or write length. Invalid-command replies therefore look successful, corrupted values can enter public properties, and a short write is not detected.
- The driver uses raw `close()`, a custom pthread mutex and legacy `indigo_read`/`indigo_write` instead of the portable uniform-I/O layer and generated per-device handler queue.
- Absolute and relative moves send `SPOS` and poll `GPOS` every 0.1 seconds. There is no overall deadline or stalled-position bound. A polling failure leaves position/steps BUSY, stops rescheduling, and retains an ambiguous timer pointer. Disconnect cancels the timer but does not stop an active rail.
- Relative target arithmetic is based on the last published value and does not explicitly clamp or guard overflow. An overlapping position/steps request can replace a target while the previous polling timer remains active. Completion is inferred only from equality with the requested target.
- Abort sends `STOP`, trusts the returned value without validating the response, assigns it as both actual and target, and marks both motion properties ALERT even after a successful stop. A failed stop leaves motion ownership and recovery undefined.
- Initial speed readback is clamped only above 3; negative or malformed values are not rejected. Device-side movement or rotary-switch movement is not periodically polled while idle, although the protocol supports `GPOS`.
- The host simulator has an independent one-step-per-10ms movement thread rather than `serial_motion.h`. It accepts bad checksums, addresses and command/index shapes; returns an ACK for unknown commands; has no command journal, split-frame behavior, protocol/transport fault injection, stall control, external-state mutation, startup rollback testing or deterministic instance isolation evidence.
- The existing automated suite is one broad smoke case. It does not independently validate simulator frames, checksum/ACK behavior, pre/post-connect property contract, initialization rollback/retry, signed/boundary positions, zero move, both relative directions, readback, overlap, idle abort, start/read/stop failures, stalled motion, disconnect during motion, reconnect or additional instances.

## Atomic plan and progress

1. **Baseline and evidence capture - complete.** Read repository/test standards, build the unchanged 0x02000004 driver and simulator, run the existing smoke suite, inspect all ten PDF pages, and record supported/non-applicable capabilities plus concrete defects.
2. **Independent simulator contract - complete.** Move the host simulator to `serial_motion.h`; validate exact eight-byte frames, checksum, address/command ACK semantics and command/index constraints; model the complete documented command/register state; add a deterministic event journal and one-shot split/corrupt/invalid/silent/partial/overlong/close/stall/external-state controls. Preserve the Arduino sketch as a historical fixture.
3. **Regression suite - complete.** Replace the smoke case with isolated named scenarios, fresh property revisions, child watchdogs and simulator reaping. Cover direct protocol behavior, property visibility/ranges, initialization rollback/retry, motion and boundaries, overlap, abort/recovery, fault paths, idle polling, disconnect/reconnect and additional instances.
4. **Authoritative generator migration - complete.** Create `indigo_focuser_mjkzz.driver`, use generated lifecycle/queues and portable uniform I/O with one reusable message buffer, make open/init transactional, strictly validate every response, implement bounded nonblocking motion finalization and explicit abort/disconnect recovery, preserve supported public behavior, correct the documented power setting and raise the driver version.
5. **Full validation and hardening - complete.** Regenerate checked-in C/header/main reproducibly, build the complete driver, run every simulator scenario and targeted sanitizer cases, compile production and simulator code with strict warnings at available architectures/optimization levels, and inspect generated lifecycle/finalizer semantics.
6. **Repository synchronization - complete.** Add all new persistent files to the existing Xcode groups, leave README unchanged, update `PROPERTIES.md`, simulator inventory, `indigo_test/CHANGES.md` and only the MJKZZ status fields in `MIGRATION_STATUS.md`; verify Xcode syntax, diff hygiene and cleanup while preserving unrelated worktree changes.

## Required coverage and boundaries

The final scenario matrix must cover exact nominal and split binary framing; invalid request checksum/address/command/index; response checksum/address/command/index corruption; connection open/probe and every required setup/read failure with same-process recovery; pre/post-connect visibility, interface, permissions, ranges and initial readback; absolute negative/positive/zero/already-selected/boundary GOTO; inward/outward/zero relative moves and target calculation; speed 0..3 with ACK/readback/error recovery; BUSY/progress/final measured position; overlapping requests; idle/moving abort, failed STOP and fresh-motion recovery; stalled and failed position polling; externally changed position; disconnect during motion, reconnect, repeated lifecycle and independent additional instances.

SYNC, reverse motion, temperature, backlash, limits, compensation, mode, camera triggering, focus signal, automated rail capture sequences and configuration registers beyond the three legacy startup writes are non-applicable because the driver does not expose them. Generic framework range validation will not be duplicated. Hardware tests, Linux/Windows runtime, physical endpoints/direction/motion, real controller timing and multi-controller bus wiring remain unverified.

## Step 1 result

The unchanged hand-written driver built as a universal macOS arm64/x86_64 archive, dylib and standalone executable. The existing host simulator and integration test built, and the one smoke scenario exited successfully. The bundled ten-page manual was inspected in full, including the message layout/checksum/ACK rules, command list, register definitions, movement modes, position/speed commands and STOP semantics. No production, simulator or user-facing documentation behavior changed during baseline capture.

## Steps 2-4 result

The simulator now uses monotonic elapsed-time `serial_motion` state rather than a movement thread. It validates the exact request checksum, device/broadcast address, command and index, sets documented acknowledgement bits, and produces valid response checksums. It implements every command listed by the bundled manual: version and status/register access; camera/focus activation acknowledgements; normal/bounded relative motion; absolute position; speed; settle/hold/backlash/shutter-lag/start/end/configuration/count/step-size storage and readback; sequence execution status; and STOP with measured position. Camera electrical signals and the detailed optical capture timeline are represented only at the protocol/state boundary because they are not observable through this focuser driver.

The simulator additionally provides split request/reply frames, an event journal, one-shot reply corruption and transport failure, stalled motion and signed external position mutation. Its power and microstep validation follows the manufacturer ranges. The legacy hold-power write of 16 was corrected to the documented maximum 12; low power 2 and quarter-step mode 0 are retained. `mjkzz_def.h` now contains the manufacturer-defined movement and microstep enums shared by driver, simulator and tests.

The generated driver uses uniform I/O and a reusable private response frame. Its binary reader has explicit first/inter-byte bounds and rejects trailing bytes. Every response must match the request address, acknowledged command, index and checksum before its signed big-endian value is used. Open plus every required startup write/read is transactional, and failed initialization closes the handle before a tested same-process retry.

Absolute and relative moves run on generated handlers. `motion_finalizer` polls measured position without blocking the queue, publishes BUSY progress, terminates on the measured target and stops after bounded read failure or five seconds without progress. Targets use 64-bit intermediate arithmetic and clamp to the preserved -32768..32767 public range. Cross-property overlap is rejected. Successful abort cancels the identified start/finalizer work, uses the documented STOP readback, clears motion BUSY and allows an immediate new move; failed stop leaves an explicit uncertain/ALERT state recoverable by a later successful abort. Disconnect cancels pending work and best-effort stops active or uncertain motion. Idle polling publishes controller-side movement.

## Final validation and Step 6 result

- Final ordinary simulator suite: **36/36 scenarios passed** on macOS arm64. Three direct simulator cases cover the full documented command set, split requests/replies and invalid checksum/address/command/index/broadcast behavior. Isolated driver cases cover the complete applicable capability, absolute/relative motion, speed, abort, overlap, transactional initialization, corrupt-frame, polling, external-state, stall, transport, disconnect/reconnect and additional-instance matrix.
- Targeted ASan: **22/22 scenarios passed** (`init_`, `poll_`, `failure`, `start_transport_loss`, `abort_motion`, `disconnect_motion`, `instances`). Generated production C and the test harness were instrumented; the prebuilt INDIGO shared library and simulator executable were not. LeakSanitizer was disabled at that prebuilt framework boundary. No ASan finding occurred.
- Generated C passed `-Wall -Wextra -Wconversion -Wshorten-64-to-32 -Wconditional-uninitialized -Wunreachable-code -Wcomma -Werror` at O0 and O2 for arm64 and x86_64 compilation. The simulator passed strict C11 `-Wall -Wextra -Werror`. The production universal archive, dylib and executable build passed.
- A repeated generator invocation produced byte-identical C, header and main files. Generated handler/finalizer dispatch, connection rollback, disconnect cancellation and the absence of a `MAX_DEVICES` override were inspected. `git diff --check` and Xcode `plutil` validation passed.
- `REFACTOR.md` and `indigo_focuser_mjkzz.driver` are referenced by the existing MJKZZ Xcode group; the simulator, shared protocol header and integration test were already referenced. No dedicated Windows project entry exists. README is unchanged. Property, simulator, automated-test and migration inventories agree.

Hardware behavior, real firmware variants, physical travel/direction, motor current and microstep effects, camera electrical isolation/trigger timing, capture-sequence timing, serial bus with multiple addressed controllers, Linux/Windows compilation/runtime and x86_64 runtime remain unverified. The x86_64 result above is compilation only. No hardware test was attempted.

Reproduction from repository root:

```sh
build/bin/indigo_generator indigo_drivers/focuser_mjkzz/indigo_focuser_mjkzz.driver
make -C indigo_drivers/focuser_mjkzz -f ../../Makefile.drv
make -C indigo_test build/integration/test_focuser_mjkzz_simulator build/integration/test_focuser_mjkzz_simulator_asan
cd indigo_test
./build/integration/test_focuser_mjkzz_simulator
for mjkzz_filter in init_ poll_ failure start_transport_loss abort_motion disconnect_motion instances; do
  MJKZZ_TEST_FILTER="$mjkzz_filter" ASAN_OPTIONS=detect_leaks=0 ./build/integration/test_focuser_mjkzz_simulator_asan || exit 1
done
```
