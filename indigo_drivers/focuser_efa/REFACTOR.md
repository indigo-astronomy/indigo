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
