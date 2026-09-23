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

# Temma protocol repairs (2026-09-23)

## Scope and hardware-test decision

Three defects reported against this driver after a comparison with cocoaTemma, the upstream INDI
driver and the public INDIGO `master`: two introduced by the generator migration
(`1db9fb0ffa2b484acdb82d377e6d733917522029`, "mount_temma: migrated to code generator") and one
older than it. No physical Temma mount is available, so hardware tests run/passed stay 0/0 and no
hardware validation is claimed. Everything below is validated against the host-side simulator,
which was corrected first so that it stops confirming the driver's own mistakes.

Driver version `0x0300000D` to `0x0300000E`.

## Source conflict worth recording

The bundled `Temma.pdf` describes the last pair of the right ascension field in `E`, `D` and `P`
as "Seconds (0 - 59)". The units repair below implements **hundredths of a minute**, on the
instruction of the repository owner and consistent with cocoaTemma, with the worked example
`123450` = 12h 34m 30s and with the fact that the field really carries values of 60 to 99, which
seconds cannot. The manual's own wording is the only source that disagrees, and it is recorded
here rather than silently overruled.

## Found defects

| ID | Status | Observation and root cause | Fix and regression |
| --- | --- | --- | --- |
| TM001 | FIXED | The migration introduced `temma_command_ack()`, which waits for a reply and accepts only `R1`, and used it for every command. Temma has no universal acknowledgement. For `D` and `P`, `R0` is the success and `R1` is the right ascension **error**, so the driver reported every accepted sync and GOTO as a failure and would have reported an error as a success. `T`, `I`, `Z`, `LA`, `LB`, `LL`, `LK`, `M<byte>`, `MA`, `PS` and `PT` are answered by nothing at all, so every one of them paid the full read timeout; on the guider that timeout lands inside the pulse. `STN-ON`, `STN-OFF` and `STN-COD` answer `stn-on` or `stn-off`, and `v1` and `v2` answer their own version line. | The transport layer is split by what the mount actually answers: `temma_no_reply_command()`, `temma_position_command()` which requires `R0` and names the documented `R1` .. `R3` errors, `temma_set_standby()` which requires the standby state back, and `temma_set_high_speed()` which requires the version line. Simulator: every no-reply command is now silent, the standby and version commands answer their own text, and `D`/`P` answer `R0`, `R1` or `R3`. Regression `temma_position_reply_codes_and_trailer` requires a GOTO answered `R1` to be refused and the one answered `R0` to run to completion. |
| TM002 | FIXED | The migrated `temma_update_position()` accepted only `E` or `W` at index 13 and only `0` or `1` at index 14, and rejected the whole reply otherwise. The protocol also uses `F` at index 13, the temporary marker the mount reports for the first readings after an automatic introduction, and index 14 is the handbox letter, which is what the cocoaTemma emulator sends as `H`. A valid reply was therefore thrown away, and with it the position. | Index 13 accepts `E`, `W` and `F`, the declination sign accepts the space the protocol uses when the declination is exactly zero, and index 14 is not validated at all. An `F` keeps the last known side of the mount instead of replacing it with a value that is not a side; the GOTO state is followed through `s` as before. Simulator: the `E` reply ends with `H`, and `--introduction <count>` reports `F` for that many readings after a GOTO. Regression: the same case, which runs a GOTO through four `F` readings and requires the side to survive. |
| TM003 | FIXED | Older than the migration and present in the public `master` as well: the right ascension of `E`, `D` and `P` was read and written as `HHMMSS` in seconds. The field is `HHMMcc`, where `cc` is hundredths of a minute, so `123450` is 12h 34m 30s and not 12h 34m 50s. Every position was wrong by up to 39 seconds of right ascension, which is ten arc minutes on the sky, in both directions. The migration made it worse by rejecting a last pair above 59, although 60 to 99 are the values the mount really sends. | Reading divides by 6000 and no longer bounds the last pair. Writing goes through one `temma_format_position()` used by sync, GOTO and the park position, which rounds to whole hundredths and whole tenths of an arc minute first, so a value that rounds up carries into the next minute, the next hour and across 24 hours instead of being sent as 60 minutes or 24 hours. `T` keeps ordinary seconds and `I` keeps tenths of an arc minute, and both were given the same carry-safe rounding. Simulator: `format_ra()` and `parse_radec()` use hundredths. Regression `temma_position_units_are_hundredths_of_a_minute` requires the exact command `D123450+45300` for 12.575 hours, requires a last pair of 99 to be accepted in both directions, and requires 23.999999 hours to be sent as `000000`. |

## Simulator corrections

The simulator answered `R1` to every command, which is precisely what made the driver's universal
acknowledgement look correct. It now answers what the mount answers: nothing for the no-reply
commands, `stn-on` / `stn-off` for the standby commands, a version line for `v1` and `v2`, and
`R0` / `R1` / `R3` for `D` and `P`. `STN-COD` was missing and is implemented. `format_ra()` and
`parse_radec()` use hundredths of a minute, `parse_radec()` rejects a non-numeric field instead of
accepting whatever `sscanf` leaves behind, the `E` reply ends with the handbox letter, and
`--introduction <count>` models the `F` readings after an automatic introduction.

## Test changes

`temma_timeout_and_open_failures_recover` dropped the reply of `PS` to make the abort fail. `PS`
has no reply, so that is no longer a failure of anything; the case now drops the reply of `v`,
which the connection reads first, so the timeout lands in one deterministic place, and the abort
is asserted to complete normally. `temma_guider_command_failure_recovers` injected a bad reply to
the relay mask command for the same reason; a relay mask command can now only fail on the
transport, so the case takes the transport away and requires the failed pulse to clear its
direction and both axes to work again on a fresh session. Two cases were added for the defects
above, bringing the suite from 13 to 15.

## Results (driver 0x0300000E)

| Suite | Run | Passed | Failed |
| --- | --- | --- | --- |
| `test_mount_temma_simulator` | 15 | 15 | 0 |

Guider pulse timing over the corrected protocol, 12 retained samples per row, software/protocol
edge timing and not physical relay output:

| Workload | Requested | Actual mean | Mean error |
| --- | ---: | ---: | ---: |
| Idle | 20 ms | 22.344 ms | 11.718 % |
| Idle | 100 ms | 102.758 ms | 2.758 % |
| Idle | 500 ms | 502.066 ms | 0.413 % |
| Mount tracking | 20 ms | 22.941 ms | 14.706 % |
| Mount tracking | 100 ms | 102.311 ms | 2.311 % |
| Mount tracking | 500 ms | 503.677 ms | 0.735 % |

## Not covered

* Physical hardware. No Temma mount is available; every result above is simulator backed.
* The exact wording of the `R4` and `R5` codes a GOTO can answer on firmware that reports more
  reasons than the manual names. The driver passes such a code through to the client verbatim.
* Whether a real controller needs the quarter second between commands the manual mentions. The
  no-reply commands are now sent without waiting for anything, which is what the reporter asked
  for and what cocoaTemma does, but it has not been measured against hardware.

## Final test summary for this repair

* Simulated tests: 15 run, 15 passed.
* Hardware tests: 0 run, 0 passed. No Temma mount is available.


# MountSim 2.3 (Temma) acceptance — 2026-09-23

## Scope and plan

Use the independently implemented Cocoa app through its real PTY. No physical mount is connected; physical hardware counts remain 0/0. Baseline INDIGO commit ef3387a3a; MountSim e394405. MountSim Debug build and universal Temma build succeeded on macOS arm64. Existing 15-case suite is running before production changes.

1. COMPLETE: add a shared opt-in launcher with isolated preferences, per-case processes, bounded cleanup and transparent serial capture; register sources in Xcode.
2. COMPLETE: run mount and guider property, command, coordinates, reachable GOTO/abort/park, shared lifecycle and software pulse timing acceptance against MountSim.
3. COMPLETE: investigate any discrepancy against multiple independent public sources; record reproducer, cause and repair before changing production code.
4. COMPLETE: reran the complete scope, recorded `MountSim 2.3 (Temma)`, regenerated the summary and checked upstream (no new commits/conflicts). Verified work is committed locally without pushing.

The launcher must not synthesize mount responses. Protocol-fault injection remains covered separately by the deterministic 15-case suite. Captures distinguish driver-to-app and app-to-driver traffic; pulse measurements describe software transport edges, not physical relays.

## Baseline and reproduced defects

The original deterministic suite passed 15/15. The first independent MountSim subset passed 7/7. Added reachable GOTO and park-arrival cases before changing production code. Park arrival fails reproducibly: target Dec 46 degrees, initial 45 degrees; trace sends P then STN-ON about 38 ms later, so motion stops short although the driver reports OK. Captures retained in `/tmp/temma-mountsim-before-fix` for this session. The GOTO/abort assertion initially read the coordinate cache before asynchronous completion; its test now waits for a terminal state, with no production change for that observation.

TM004 (reproduced, repaired): parking disables motors immediately after GOTO acceptance. Sources checked independently on 2026-09-23: [INDI Temma getCoords/Park](https://github.com/indilib/indi/blob/master/drivers/telescope/temmadriver.cpp) marks parking in progress and disables motors only after completion; [CCDASTRO Temma tracking/standby documentation](https://github.com/CCDASTRO/ASCOM.CCDASTRO.Temma#tracking-and-standby) identifies STN-ON as motors stopped. MountSim and the existing deterministic simulator both model that stop. The defect is in the driver; the previous simulator test asserted only the command and property result, not arrival.

Repair plan: add a bounded park finalizer that polls position/status, sends standby only after completion, publishes tracking OFF, handles read/stop failures, and cancels on abort/disconnect. Preserve the existing momentary park switch contract (one item, clears on completion); reject competing work using the existing parked guard and coordinate BUSY state. Add the same arrival and cancellation regression to the portable suite, without introducing any MountSim dependency there. Increase driver version 14 to 15 and regenerate using the unchanged generator.

## Coverage mapping and current results

The driver-only repair (3.0.0.15) passes the 16-case portable suite, including park arrival, abort, disconnect/reconnect and failed status read. Strict `clang -Wall -Wextra -Werror -fsyntax-only` passes. Xcode project syntax passes. MountSim production code has not required changes.

The independent suite has 12 named cases: transport loss during pending motion and healthy idle with fresh reconnect; location, pier side and actual manual-motion readback; park abort/reconnect; reachable GOTO completion/abort/fresh GOTO; park arrival before standby; mount property/lifecycle contract; driver-specific commands and rates; manual directions/rates/combined masks; RA hundredths and wrap; guider directions/replacement/overlap/zero; shared-device orders and pending disconnect; guider ON/OFF timing while idle and tracking.

The first 9-case run after repair passed 9/9. Initial 12-case run passed 11/12: its idle-loss case cut the port while coordinates were still ALERT from the preceding abort, then incorrectly demanded a new ALERT publication. INDIGO suppresses unchanged updates; the corrected test first requires a healthy OK poll. The isolated rerun passed. This is a harness correction, not another driver defect.

Platform isolation: all app tests and helpers are under `indigo_test/mountsim`; Makefile rules are guarded by Darwin and are not members of the portable test lists. The MIGRATION_STATUS count is 16 portable / 0 physical, excluding the 12 macOS-only app cases. Full instructions are in `indigo_test/mountsim/USAGE.md`.

Remaining scope limits: no physical mount, Linux or Windows execution; 600-second park timeout is implemented but its full wall-clock expiry is not exercised; protocol-error injection stays in the portable suite. PTY loss is client transport loss, not an app crash or electrical parity test. No generic framework or generator changes are needed.

## Final MountSim run and software timing

Final complete MountSim run: **12/12 passed**, driver **3.0.0.15**, macOS arm64. Every session response identifies app version 2.3 and model Temma. All original portable cases plus the added park lifecycle case passed **16/16**. The repaired wire sequence is P → repeated E/s polls → s0 → STN-ON; the separate park test asserts actual Dec arrival and tracking OFF. Abort cancels the pending park finalizer and sends PS; disconnect clears the momentary switch before reconnect. The unchanged generator regenerates the driver source.

Each timing row retains 12 samples (four directions, three repeats) after warmups. Endpoints are the relay forwarding each direction ON and OFF command into the real MountSim PTY, including host scheduling; they are not physical relay or motor-loop timestamps. Workloads are tracking disabled and tracking enabled with periodic coordinate polling. Signed and absolute errors are milliseconds; percentage is mean signed error/requested duration.

| Workload | Request | Actual mean | Min | Mean | Median | p95 | p99 | Max | Stddev | Max abs | Mean % |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| idle | 20 | 23.127 | 0.088 | 3.127 | 0.426 | 10.106 | 10.106 | 10.106 | 3.729 | 10.106 | 15.637 |
| idle | 100 | 105.083 | 0.141 | 5.083 | 4.519 | 14.786 | 14.786 | 14.786 | 4.509 | 14.786 | 5.083 |
| idle | 500 | 502.647 | 0.155 | 2.647 | 0.790 | 10.216 | 10.216 | 10.216 | 3.379 | 10.216 | 0.529 |
| tracking | 20 | 26.072 | 0.265 | 6.072 | 4.520 | 38.083 | 38.083 | 38.083 | 9.955 | 38.083 | 30.361 |
| tracking | 100 | 103.676 | 0.449 | 3.676 | 1.888 | 21.147 | 21.147 | 21.147 | 5.455 | 21.147 | 3.676 |
| tracking | 500 | 508.556 | 0.153 | 8.556 | 3.307 | 33.525 | 33.525 | 33.525 | 11.379 | 33.525 | 1.711 |

Darwin Makefile guarding was verified with `make -n -C indigo_test OS_DETECTED=Linux test-mount-temma-mountsim` and the Windows equivalent: each emits only the unsupported-platform message and exit, with no build or test prerequisites. Default integration registration is unchanged. No native Linux/Windows execution is claimed.

## Final verification and test summary

- MountSim Debug build: successful (app remains built in the MountSim checkout).
- Temma universal arm64/x86_64 build: successful; native execution was arm64.
- Strict driver syntax/warnings (`-Wall -Wextra -Werror`): passed.
- Regenerating the unchanged `.driver` source produced byte-identical C/H/main files.
- Portable suite: **16 run / 16 passed**; repeated with ASan+UBSan-instrumented driver and test: **16/16**, no sanitizer diagnostics. Shared libindigo and the external deterministic simulator were not instrumented in this run. Leak detection disabled because this macOS sanitizer does not support it.
- `MountSim 2.3 (Temma)`: **12 run / 12 passed**. Software simulated test cases in this work: **28 distinct / 28 passed** (16 portable + 12 independent MountSim); repeated runs are not counted twice.
- Physical hardware: **0 run / 0 passed**. No physical validation claimed.
- Xcode project syntax, generated summary, whitespace and non-macOS opt-in target guards checked successfully. Linux/Windows native execution remains unavailable.
- Full MountSim captures retained for this session under `/tmp/temma-validation/mountsim-results`; original failing traces under `/tmp/temma-mountsim-before-fix`. The before/after ordered wire comparison isolates the park change: immediate STN-ON is replaced by status polling through completion, then STN-ON. Other cases remain successful. Logs are temporary artifacts, not repository source files.

No MountSim code repair was necessary. The application tests stay excluded from portable default test targets. The only production change is Temma park sequencing/lifecycle in version 3.0.0.15.
