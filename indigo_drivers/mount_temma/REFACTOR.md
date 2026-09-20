# Mount Temma code-generator migration

Status: simulator-backed migration reopened for coverage completion on 2026-09-13. Baseline driver version: `0x02000008`; first generated version: `0x03000009`; coverage-fix version: `0x0300000A`.

## Instructions and sources reviewed

- `AGENTS.md`, `indigo_drivers/AGENTS.override.md`, `indigo_test/AGENTS.md` and `indigo_test/DRIVER_TESTING_RULES.md`.
- `README.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_docs/SERIAL_DEVICE_SIMULATORS.md`, the Temma driver README, source/header/main files, host-side Temma simulator and existing integration test.

## Current-state audit

`indigo_mount_temma.c` is a hand-written serial driver for Temma-compatible mounts. It exposes a mount and a guider logical device sharing one serial handle and one private-data allocation. The mount supports coordinate polling, GOTO/SYNC, manual RA/DEC motion, abort, parking, tracking/rate selection, side-of-pier switching, correction speed, high-speed mode and zenith sync. The guider supports RA and DEC pulses. A host-side pseudo-terminal simulator implements the Temma protocol and an existing two-case integration test covers basic mount and guider property changes.

The current driver owns its serial configuration with POSIX `termios`, serializes transactions with `pthread_mutex_t`, polls through INDIGO timers, repeats manual-motion commands through a second timer, and blocks in guider handlers and abort polling. These patterns must migrate to portable `indigo_uni_io` transaction helpers and generator-owned handler queues without changing the public property schema. The simulator must be audited against `Temma.pdf`; its command, reply, malformed-reply, lifecycle, overlap, abort and timing coverage must be extended where required.

The hand-written mount properties were `TEMMA_CORRECTION_SPEED`, `TEMMA_HIGH_SPEED` and `TEMMA_ZENITH`. The follow-up audit found that this legacy naming conflicts with the repository-required `X_` custom-property prefix; step 9 therefore intentionally renames them and updates `indigo_docs/PROPERTIES.md`.

The build currently has Makefile, Xcode and integration-test registrations. `MIGRATION_STATUS.md` records API 2, Windows/generator/queues/retesting as incomplete and automated tests as available. Its Comment cell will be preserved exactly. Windows source portability is currently at risk because of direct POSIX and pthread use.

## Hardware-test decision

No physical Temma mount is available for this migration. Hardware tests run/passed are therefore 0/0 and will not be claimed. The deterministic host-side serial simulator will validate the protocol and public-bus behavior; software guider completion timing is not a measurement of physical relay output.

## Atomic plan and results

1. **Establish baseline and audit protocol/simulator coverage — completed.** `make -B -C indigo_drivers/mount_temma -f ../../Makefile.drv all` built the unmodified universal driver successfully. `make -B -C indigo_test build/integration/test_mount_temma_simulator` built the simulator and test. An initial run from repository root failed because the test's default simulator path is relative to `indigo_test/`; this is a test invocation issue, not a driver failure. Running `build/integration/test_mount_temma_simulator` from `indigo_test/` passed both baseline cases (2/2). The host simulator covers version, coordinates, slew state, LST/latitude, rate, correction, high-speed, stop, pier and zenith commands used by the driver, but it currently reports an immediate completed GOTO and has no fault injection. `Temma.pdf` is present (eight pages) but no usable text extractor is installed; command compatibility was therefore audited against the driver and simulator source, not inferred from an unreadable PDF stream.
2. **Create generator source without changing behavior — completed.** Reverse extraction was run exactly as `build/bin/indigo_generator -c indigo_drivers/mount_temma/indigo_mount_temma.driver`, creating the new definition from the hand-written source without passing the existing C file as the output argument. It recovered the mount/guider topology, custom properties and inherited property list. The first generated build correctly exposed one extraction defect: custom properties retained the legacy `CCD_ADVANCED_GROUP` symbol without its definition. The definition restores that symbol, supplies the Temma names, portable serial open/close and bounded line transaction helper, and establishes mount connection/polling lifecycle. Regeneration followed by `make -B -C indigo_drivers/mount_temma -f ../../Makefile.drv all` passed. The generated header no longer exposes private name macros, so the test now uses the public generated names; its rebuilt baseline passed 2/2. Queue-owned motion, abort and guider behavior were completed in step 4.
3. **Replace non-portable I/O and mutex ownership — completed.** The generated source now owns an `indigo_uni_handle`, opens the serial port with `19200-8E1`, uses a reusable private response buffer with `indigo_uni_discard()`, `indigo_uni_vprintf()` and `indigo_uni_read_section2()`, and closes through `indigo_uni_close()`. The hand-written termios/select/read loop and `pthread_mutex_t` are gone; the generator's unconditional pthread include remains but no mutex object or mutex API is emitted.
4. **Move operations to handler queues — completed.** Polling is generator queue based; GOTO completion uses `mount_goto_finalizer`, abort cancels GOTO and manual-motion handlers, RA/DEC manual motion is rescheduled on the device queue, and independent RA/DEC guider pulses use time-priority finalizers. No driver entry point waits for motion completion.
5. **Initial simulator and integration expansion — completed but insufficient.** The simulator holds GOTO BUSY for a finite interval, supports one malformed-position mode and keeps its slave PTY alive across reconnects. The resulting eight scenarios pass, but a follow-up audit found that they do not prove fresh property updates, final RA/DEC motion states, exact commands, error replies and recovery, shared mount/guider ownership, disconnect races or guide-pulse timing. The malformed-position case proves rejection only, not recovery, and the zenith property is enumerated but not exercised.
6. **Integrate generated sources and Windows support — completed with platform limitation.** The `.driver`, refactoring record and host simulator are registered in the existing Xcode mount groups. Added `indigo_mount_temma.vcxproj` and filters for Debug/Release x64 and ARM64, compiling the generated C and exporting through the generated header, and registered the project plus all four configurations in `indigo_windows.sln`. `xmllint --noout` accepted both project files. `MIGRATION_STATUS.md` status columns now record API 3, Windows support, generator, queues and simulator retesting; its Comment cell is unchanged. Windows compilation/execution is unavailable here and is not claimed.
7. **Verify and close — completed with documented limits.** Final generated-driver build and the rebuilt host-side integration suite passed 8/8; `plutil -lint indigo.xcodeproj/project.pbxproj` and `git diff --check` passed. Sanitizer, Linux and native Windows builds/runs are unavailable and are not claimed.
8. **Re-audit coverage against the class standards and Temma protocol — completed.** All eight pages of `Temma.pdf` were rendered and inspected. The audit identified missing ACK/error validation, stale-OK waits, incomplete public-property assertions, missing manual-motion final states, incomplete park/location/options coverage, missing shared-device lifecycle/races and absent guide-pulse timing statistics. It also found that the legacy `TEMMA_` custom-property names conflict with the repository-required `X_` prefix.
9. **Fix driver protocol/state behavior and property naming — completed.** Driver version is `0x0300000A`. Replies are stripped of CR/LF and command acknowledgement now requires the exact `R1` response; position, slew-status and correction-rate replies are structurally and numerically validated. Coordinate and park commands preserve the sign for declinations between -1° and 0°. Abort and failed abort publish terminal coordinate and RA/DEC motion states, combined mount/guider direction masks preserve the other axis, failed/replaced/zero guide requests clear the applicable mask, and disconnect cancels pending work and sends the applicable motion/GOTO stop before shared-handle release. High-speed manual-motion state no longer leaks into a later guider-only pulse. The custom properties are now `X_TEMMA_CORRECTION_SPEED`, `X_TEMMA_HIGH_SPEED` and `X_TEMMA_ZENITH`; `indigo_docs/PROPERTIES.md` was updated. Regeneration and both the normal and `-Wall -Wextra -Werror` universal driver builds passed.
10. **Make simulator evidence deterministic — completed.** The simulator now records monotonic timestamped command bytes through `--trace-file`, including binary motion masks, and provides one-shot `--fault-reply` and `--drop-reply` injection with exact or prefix matching. Tests use the trace to assert ASCII commands, binary RA/DEC combinations, rate selection, abort/stop and guider ON/OFF timing endpoints. The simulator builds normally and with `-Wall -Wextra`; the only strict warning is the pre-existing unused `serial_simulator_write_all()` helper in shared `serial_simulator_common.h`.
11. **Complete automated mount and guider coverage — completed.** The suite now has 13 independent hardware-free cases using post-request property revisions/state history and is registered in the normal `test-integration` target. It covers the property contract and disconnect battery; exact control commands; signed and sub-degree-negative SYNC, GOTO progress/readback, busy overlap, post-slew tracking, abort and recovery; all manual directions, rates, simultaneous axes, reversal and stop; location, pier and park; ACK failure, malformed reply, timeout, invalid open and recovery; guider directions, zero, replacement, concurrent axes, command failure/mask cleanup and recovery; both shared-device orders, sibling survival, disconnect with GOTO/pulse pending and verified stop; and actual simulator-process loss during active and idle work followed by reconnect. Repeated normal full runs passed 13/13, including a final-tree run, and the final-tree ASan+UBSan-instrumented driver/simulator/test run passed 13/13 (`detect_leaks=0`, because macOS reports leak detection unsupported).
12. **Final verification and reconciliation — completed.** A checksum-before/after regeneration check confirmed byte-identical generated C/H/main output. The universal normal build and `-Wall -Wextra -Werror` driver build passed; the final normal simulator suite and final ASan+UBSan suite each passed 13/13, and `test_generator_architecture` passed all cases. `plutil -lint` accepted the Xcode project, `xmllint --noout` accepted both Visual Studio project files, the Windows solution contains the project and eight configuration mappings, and `git diff --check` passed. `MIGRATION_STATUS.md` now records 13/0 tests without changing its Comment cell. `make -C indigo_test test-clean` removed the generated test binaries and dSYM artifacts. Native Linux/Windows and physical-device validation remain unavailable and are not claimed.

## Baseline evidence

Environment: macOS universal x86_64/arm64 build; native arm64 execution. The existing baseline commands and results are recorded in step 1. Baseline simulated tests run/passed: 2/2 after the corrected working directory. Hardware tests run/passed: 0/0.

## Guider timing evidence

The simulator trace measures the monotonic interval from a direction bit appearing in an `M` relay mask to that same bit disappearing. It is software/protocol-edge timing, not physical relay-output accuracy. Each row has 12 retained samples (all four directions, three repeats each) after one discarded warm-up pulse per direction. The final normal full run reported:

| Workload | Requested | Actual mean | Signed error min / mean / median / p95 / p99 / max | Stddev | Max absolute | Mean error |
| --- | ---: | ---: | --- | ---: | ---: | ---: |
| Idle | 20 ms | 23.862 ms | 0.810 / 3.862 / 3.744 / 5.268 / 5.268 / 5.268 ms | 1.457 ms | 5.268 ms | 19.309% |
| Idle | 100 ms | 103.542 ms | 0.822 / 3.542 / 2.746 / 6.544 / 6.544 / 6.544 ms | 1.569 ms | 6.544 ms | 3.542% |
| Idle | 500 ms | 501.998 ms | 0.655 / 1.998 / 1.707 / 5.243 / 5.243 / 5.243 ms | 1.203 ms | 5.243 ms | 0.400% |
| Mount tracking | 20 ms | 23.885 ms | 0.943 / 3.885 / 4.329 / 5.470 / 5.470 / 5.470 ms | 1.333 ms | 5.470 ms | 19.426% |
| Mount tracking | 100 ms | 102.839 ms | 0.480 / 2.839 / 2.689 / 5.212 / 5.212 / 5.212 ms | 1.728 ms | 5.212 ms | 2.839% |
| Mount tracking | 500 ms | 502.954 ms | 0.208 / 2.954 / 2.439 / 5.373 / 5.373 / 5.373 ms | 1.715 ms | 5.373 ms | 0.591% |

Hardware-only gaps remain physical relay timing, mechanical motion/pointing/tracking, real firmware variants and physical serial interruption. Linux and native Windows build/runtime validation are unavailable in this environment; the portable API and Windows project are present, but those platforms are not claimed as executed.

## Final test summary

Simulated tests run: **13**. Simulated tests passed: **13**. Hardware tests run: **0**. Hardware tests passed: **0**. The complete simulator suite was repeated under normal instrumentation and ASan+UBSan; the totals count distinct driver integration cases, not repeated executions. A physical Temma mount remains required to validate relay output, mechanics, pointing/tracking and firmware-specific behavior.

## Overlapping guide pulses (2026-09-20)

`GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare `accept_while_busy = true` and zero both axis
items in `on_change_request`, replacing the earlier workaround that forced the property state back
to `INDIGO_OK_STATE` so the BUSY-guarded dispatch macro would let the request through. Zeroing both
items also closes a gap the old workaround left open: a reversing request kept the superseded
direction set, so `temma_update_motion()` could be handed both bits of one axis. The driver now
uses the same pattern as every other INDIGO driver that exposes a guider.

`temma_guider_directions_replacement_axes_and_zero` gained two duration-measuring cases: a 2000 ms
pulse replaced after 500 ms by a 600 ms pulse in the same direction, and the same pulse replaced by
a 300 ms pulse in the opposite direction. The existing assertions only waited for a state
transition, which passes even when the driver keeps the superseded pulse running to its own
deadline.
