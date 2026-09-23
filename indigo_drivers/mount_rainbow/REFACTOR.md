# mount_rainbow refactoring record

## Scope and baseline

This record covers characterization and migration of the hand-written `indigo_mount_rainbow` driver to `indigo_generator`, a manufacturer-protocol audit of the host-side simulator, complete applicable simulator-backed mount coverage, and preservation of observable protocol ordering and behavior.

Baseline date and source: 2026-09-16, current working tree. The repository already contains unrelated in-progress `focuser_dsd` work and edits in `indigo_test/Makefile` and `indigo.xcodeproj/project.pbxproj`; this migration must preserve those edits and add only isolated RainbowAstro entries.

Baseline commands and results:

```sh
cd indigo_drivers/mount_rainbow
make -B -f ../../Makefile.drv

make -C indigo_test build/integration/test_mount_rainbow_simulator
cd indigo_test
build/integration/test_mount_rainbow_simulator
```

The universal x86_64/arm64 driver build passed without warnings. The simulator and test built successfully. An initial invocation from the repository root failed because the test's simulator executable path is relative to `indigo_test`; this was an invocation error, not a driver failure. Re-running from `indigo_test` passed the one registered case, 1 / 1. Teardown emitted `Failed to wait for (Bad file descriptor)`, which is retained as baseline evidence of the current close-before-reader-cancel ordering and requires a dedicated regression.

## Current-state audit

### Architecture and lifecycle

- The driver exposes one mount logical device, supports additional runtime instances, and connects either to a 115200-baud serial port or a `rainbow://` TCP endpoint using the portable unified-I/O layer.
- A dedicated reader timer continuously consumes `#`-terminated asynchronous responses. A separate one-second position timer writes compound polling sequences for RA/DEC/slew state, time/location, tracking and tracking rate.
- Property changes use zero-delay timers and a private port mutex. `rainbow_sync_command()` writes a request and polls a public property state for up to one second while the reader changes that state.
- Connection probes with `:AV#`, starts the reader, then performs an ordered initialization sequence. Disconnect closes the transport before synchronously cancelling the reader and position timers; teardown ordering and transport-loss behavior require characterization.
- Long-running slew and park completion arrive asynchronously as `:MM0#` and `:CHO#`. Manual motion uses firmware-dependent axis stop commands. The current implementation is not based on INDIGO 3.0 handler queues.

### Published capabilities and behavior

- The inherited mount property set is used; no driver-specific custom property is defined. The driver unhides serial ports, host-time and UTC-time properties; restricts coordinate-set modes to TRACK/SYNC, guide rate to RA only, and park to PARKED only.
- Supported operations are identity/firmware, coordinate polling, GOTO, SYNC, park/home command, manual RA/DEC motion at four rates, abort, tracking on/off, sidereal/solar/lunar/custom tracking rates, guide-rate read/write, geographic coordinates, UTC/time-zone read/write and host-time synchronization.
- Firmware `200625` gates date/time-zone queries and independent RA/DEC manual-motion stop commands. Older firmware uses local host date context, time-only polling and global `:Q#` stops.
- Coordinate commands convert between J2000 and device epoch. Longitude translation follows the mount protocol's EAST-negative/WEST-positive convention.
- Driver version is `0x0200000F`; any behavior fix or completed generated migration must increment it.

### Manufacturer protocol and simulator audit

- Manufacturer PDFs `RainbowAstro_191217.pdf` and `RainbowAstro_200629.pdf` specify case-sensitive 115200-baud commands, response formats, motion/stop semantics, guide-rate range, tracking states/rates, home outcomes, location signs and the firmware-200625 additions for date, time-zone and independent axis stops.
- The current host simulator implements the happy-path command vocabulary used by the driver, but immediately jumps to slew and park destinations, does not use `serial_motion.h`, provides no deterministic elapsed-time movement, exposes no firmware selector or fault injection, and cannot model partial/malformed/delayed replies, command rejection, transport loss, failed slew or failed homing.
- The simulator parser accepts concatenated `:`-prefixed commands and ACK byte 0x06, but protocol correctness, command casing, command/reply ordering and all failure branches need explicit tests against the manufacturer tables.

### Existing tests and gaps

- `indigo_test/integration/test_mount_rainbow_simulator.c` registers one smoke/compliance case. It checks a subset of property items and exercises guide rate, tracking, solar rate and idle abort only.
- Missing coverage includes complete property visibility, both firmware branches, initial value parsing, exact initialization/poll trace, GOTO BUSY/progress/completion/failure, SYNC without slew, all manual directions/rates/stops/reversal/simultaneous axes, abort during slew/manual motion, park success/failure and parked guards, all tracking rates, location/time conversions, malformed/partial replies, write/read failures, disconnect/reconnect, transport loss, concurrent requests and queue/teardown races.
- There is no retained normalized original-driver reference trace and no sanitizer/strict test result.

## Hardware-test decision

No compatible RainbowAstro mount is available. No hardware test will be performed, registered or claimed. Simulator/fake-transport evidence will be reported separately and must not be described as physical mount validation.

## Atomic plan

1. **Done - instructions, documentation, audit and baseline.** Read repository, driver and test rules; inspected the driver, simulator, existing test, build/project integration, generated-driver guide, serial-simulator contract and both manufacturer protocol PDFs. The driver build passed and the original one-case simulator test passed from its required working directory; the teardown bad-file-descriptor diagnostic is recorded above.
2. **Done - protocol-complete simulator foundation.** Audited commands against both protocol revisions. The simulator now has firmware selection, deterministic event traces, one-shot/sticky fault injection, split/delayed/malformed/drop/close actions, elapsed-time GOTO/park/manual motion through `serial_motion.h`, unsolicited completion replies and a retained PTY endpoint for reconnect tests.
3. **Done - original-driver property and lifecycle coverage.** Expanded the suite through public INDIGO APIs for property inventory, identity, firmware branches, initialization order, location/time, tracking/rates, guide rate, disconnect and reconnect.
4. **Done - original-driver motion coverage.** Covered GOTO, SYNC, BUSY/completion, J2000 command values, replacement while BUSY, every manual axis/rate combination, independent stops, abort, park success/failure and the parked-motion guard.
5. **Done - original-driver transport and concurrency coverage.** Added split, malformed and dropped polling replies, invalid device identity, write failure after active transport loss, reconnect and overlapping GOTO coverage with bounded waits.
6. **Done - original reference trace.** Retained `indigo_test/fixtures/mount_rainbow/original_reference_trace.txt`; the suite compares the ordered initialization protocol against it, including compound-command ordering.
7. **Done - generated source design.** `indigo_mount_rainbow.driver` is the source of truth. No generator implementation change or driver-local capacity override was made.
8. **Done - generate, build and compare.** Regenerated `.c`, `.h` and `_main.c`, incremented the driver to version 16, built the universal driver and ran the complete trace-backed suite after behavior changes.
9. **Done - generated lifecycle and async audit.** Writes run on the generated device handler queue; abort is urgent; GOTO and park use bounded finalizers; the asynchronous reader has explicit lifetime state and is stopped before transport close; polling and pending handlers are cancelled on disconnect.
10. **Done - repository integration.** Registered the `.driver`, refactor record and retained trace in project files, added the `.driver` to the Windows project, updated the simulator dependency and changed only the `mount_rainbow` status columns in `MIGRATION_STATUS.md`. The property surface did not add or remove names, so `PROPERTIES.md` required no change.
11. **Done - final verification.** Deterministic regeneration, normal and strict builds, the complete simulator suite, an ASan+UBSan suite, Xcode plist and Visual Studio XML validation, `git diff --check` and process cleanup all pass.

## Scenario-to-test mapping

| Standard area | Test coverage |
| --- | --- |
| Identity and coordinates | `rainbow_driver_info_and_property_inventory`, `rainbow_preserves_modern_initialization_order`, `rainbow_uses_legacy_firmware_protocol`, `rainbow_recovers_from_faulted_poll_replies`, `rainbow_rejects_non_rainbow_transport` |
| GOTO and SYNC | `rainbow_goto_sync_and_abort_have_distinct_protocols` covers exact J2000 RA/DEC commands, BUSY, completion, SYNC without `MS`, BUSY replacement rejection and abort/recovery. |
| Manual motion and abort | `rainbow_manual_motion_covers_all_rates_axes_and_stops` covers four rates, four directions and firmware-specific axis stops; GOTO test covers abort during slew. |
| Tracking and rates | `rainbow_tracks_and_selects_every_rate` covers on/off, immediate sidereal/solar/lunar rate commands, preservation of the rate-before-GOTO sequence and guide-rate units. Custom rate is not applicable because the driver publishes three rate items and no custom-rate property. |
| Park and home | `rainbow_park_reports_success_and_failure` covers BUSY/success, failure, parked state and parked-motion rejection. RainbowAstro `Ch` is exposed as INDIGO park; no separate home/unpark/set-position capability is published. |
| Time/location and options | `rainbow_writes_location_and_time` covers signed latitude/longitude, exact requested UTC offset conversion, invalid UTC rejection and host-time write. The firmware-191217 test covers the legacy time-only branch. |
| Polling and transport loss | `rainbow_recovers_from_faulted_poll_replies`, `rainbow_reconnects_after_clean_disconnect`, and `rainbow_handles_active_transport_loss` cover split/malformed/drop recovery, stale-input discard, reconnect, write failure and cancellation without closed-handle access. |

Guider pulse timing is not applicable: this driver exposes only the mount guide-rate setting and no guider logical device or pulse properties. Hardware acceptance is deferred because no mount is available.

## Found defects

- The original disconnect closed the transport before stopping the reader, producing a reproducible bad-file-descriptor diagnostic and making reconnect unsafe. Fixed with explicit reader lifetime state, cancellation before close and stale-input discard on open; `rainbow_reconnects_after_clean_disconnect` passes without the diagnostic.
- The original park completion did not set `MOUNT_PARK.PARKED`, so framework parked guards could not work. Fixed and covered by `rainbow_park_reports_success_and_failure`.
- Changing `MOUNT_TRACK_RATE` did not send a mount command until the next GOTO. By explicit user request, the change now sends `CtR`, `CtS` or `CtM` immediately while GOTO retains the defensive rate-before-slew ordering.
- UTC writes ignored the requested `UTC_TIME.OFFSET` and used the process time zone. Firmware 200625+ now converts UTC to mount-local time using the requested offset and sends the protocol's opposite-sign `SG` value; old firmware rejects unsupported writes.
- Epoch behavior is now explicit: `MOUNT_EPOCH` is read-only J2000 and exact J2000 GOTO/SYNC command values are tested.
- A stale `CL0` poll could mark a newly started GOTO complete before `MM0`. The driver now tracks an active GOTO and accepts `MM0` as its completion event; replacement requests remain blocked while BUSY.
- The old unreachable custom-rate branch was removed from behavior: the published property contains only sidereal, solar and lunar items, matching the tested public surface.

## Test evidence

- Original-driver build: passed, universal x86_64/arm64.
- Original simulator suite baseline: 1 / 1 passed; teardown emitted a bad-file-descriptor diagnostic.
- Characterization suite and normalized trace were established before changing the production driver. Confirmed original defects were kept as failing reproducers until the generated implementation fixed them.
- Generated driver build: passed for the local universal x86_64/arm64 target. Windows project integration is updated but Windows compilation is unverified in this environment.
- Generated simulator suite: 12 / 12 passed.
- Strict compile: generated driver and simulator pass `-Wall -Wextra -Werror`; the integration test passes with the common harness's known unused static helpers/parameters excluded from warning-as-error treatment.
- Sanitizers: 12 / 12 passed with the generated driver and test compiled under AddressSanitizer and UndefinedBehaviorSanitizer. The expected injected transport-loss case logs one failed write and completes cleanly.
- Regeneration: SHA-1 checksums of generated `.c`, `.h` and `_main.c` are unchanged after a fresh generator run.
- Project/whitespace validation: Xcode project plist, both Visual Studio XML files and `git diff --check` pass.
- Hardware: 0 / 0, explicitly unavailable.

## Final test summary

- Simulated tests run/passed: 12 / 12.
- Hardware tests run/passed: 0 / 0; no compatible hardware is available.

## MountSim RST135 acceptance, 2026-09-23

### Baseline, audit and hardware decision

Both repositories were clean and pulled with `git pull --ff-only` (already current).
The actual baseline driver version is 17; earlier migration notes describe version 16.
Universal macOS build (`make -C indigo_drivers/mount_rainbow -f ../../Makefile.drv all`) and the existing portable `test-mount-rainbow-simulator` target passed 12/12 on arm64. The baseline emits coordinate/time updates before property definition. MountSim 2.3 built using `xcodebuild -quiet -project MountSim.xcodeproj -scheme MountSim -configuration Debug -derivedDataPath build CODE_SIGNING_ALLOWED=NO build`.

The generated `.driver` remains the source of truth. One mount device, no guider interface; firmware 190402 selects the legacy global-stop and read-only clock branches. Existing modern-firmware injection tests remain portable and separate. The new macOS-only suite reuses the shared launcher, isolated HOME/preferences, raw byte relay, real PTY loss and command captures. No physical hardware is available or tested, and no Linux/Windows run is claimed.

### Atomic plan and progress

1. Done: build baseline and run portable 12/12; read repository instructions, class rules, MountSim README and protocol sources. Add and register independent MountSim suite under the Darwin-only opt-in Make block.
2. Done: characterize complete RST135 scope, preserve baseline captures, add failure reproducers before production edits. Initial 7-case run passed 6/7; guide-rate readback failed. Expanded suite reproduces SYNC and western-longitude readback failures; tests depending on SYNC are blocked by that defect.
3. Done: correct documented protocol defects in the responsible component, increment driver version, regenerate without changing the generator, and add portable regressions.
4. Verified: full MountSim and portable/sanitized reruns, strict checks, generated-output reproducibility, project/platform isolation and records/summary. Captures retained outside the build; test cleanup and local commits complete this step.

### Found defects and independent protocol evidence

Manufacturer's [RainbowAstro 191217 protocol](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/mount_rainbow/RainbowAstro_191217.pdf) (bundled copy, firmware 190402+) and [INDI Rainbow implementation](https://github.com/indilib/indi/blob/master/drivers/telescope/rainbow.cpp) independently establish case-sensitive `Cu0=` writes versus `CU0` reads, decimal-degree `Ck` synchronization, signed west-positive longitude, and asynchronous `MM0` on completed motion. INDI's `setGuideRate`, `Sync`, `updateLocation`, `isSlewComplete` are the relevant independent implementations.

- Driver: `CU0=` is a query spelling; guide-rate changes falsely report OK and revert on reconnect. Existing portable emulator accepted the same wrong spelling. Reproducer: `rainbow_tracks_and_selects_every_rate`, 70% readback after reconnect.
- Driver: western longitude 289.75 degrees is sent as -289.75 rather than +70.25, which MountSim rejects as outside signed 180 degrees. Reproducer: `rainbow_location_roundtrip_and_legacy_time`.
- MountSim: `Ck` parser omits the decimal separator in declination and uses an uninitialized fraction; generic GUI SYNC changes only its displayed pointing correction, while GR/GD keep reporting unchanged motor coordinates. Reproducer: `rainbow_sync_fractional_readback`.
- MountSim source audit: MS sends MM0 immediately, and Q stops manual slew but does not clear GOTO. Fix should defer MM0 until motor completion and abort active GOTO/home without a success notification. The reachable GOTO and park-abort cases will verify this after SYNC works.
- Driver source audit: abort cancels park finalizer but leaves MOUNT_PARK BUSY. A Q command stops homing per manufacturer; the driver must end its pending park state. Reproducer: `rainbow_park_abort_and_disconnect` after SYNC correction.

Baseline wire captures and logs are retained outside the test build under `/tmp/rainbow-*` for this session; exact command initialization order remains covered by the existing portable reference trace.

### Additional reproduced failures and fixes

- INDIGO: unterminated Sr/Sd acknowledgements can be coalesced as `11:MM0#`. The reader discarded that completion, leaving an already-at-target GOTO BUSY. Strip only leading acknowledgement bytes before a colon frame; portable regression injects the exact capture. Publish BUSY before writing so an immediate completion cannot be overwritten.
- INDIGO: MML/MMU/MME errors were ignored for up to the 600-second finalizer timeout. A genuinely below-horizon GOTO reproduced this; handle all three documented errors and permit fresh movement. Manufacturer error table and INDI `isSlewComplete` agree.
- INDIGO: active park disconnect left the motor moving, then reconnection failed because initialization required coordinate OK even when CL1 correctly reported motion. Stop owned GOTO/park on clean disconnect; accept fresh BUSY coordinate readback during initialization. Real PTY loss intentionally cannot stop a disconnected mount, so the active-loss test reconnects during motion and issues a fresh abort.
- INDIGO: coordinate/time callbacks published before class properties were defined. Baseline reported these violations in every connection. Guard publication with IS_CONNECTED while still collecting initialization values. The new portable settings roundtrip and MountSim inventory assert zero undefined updates.
- INDIGO source audit: SYNC declination below ten degrees was space padded rather than signed zero padded; use `%+07.3f` as required by the fixed-width protocol and INDI's sign plus `%06.3f` format.
- MountSim: after a longitude change, cached LST still used the original location. A valid declination-20 target near the new site's meridian produced MML. Refresh LST when longitude changes; this is shared location state, not a driver workaround. [USNO sidereal-time definition](https://aa.usno.navy.mil/faq/GAST) and public INDIGO `indigo_lst` in `indigo_libs/indigo_align.c` independently specify GMST plus east-positive longitude/15. Existing motor subscribers did not refresh that cache.
- MountSim: homing completed at stale staged equatorial coordinates and resumed tracking. The enhanced park case reproduced wrong HA and tracking-on despite CHO. The RST135-specific motor now stages its existing configured home (HA +90 degrees, DEC 0) and suppresses post-home tracking, without changing other mount motor subclasses.

Step 2 completed with 6/11 expanded baseline, dedicated rejected-GOTO 0/1 and home-coordinate/tracking 0/1 reproducers. Step 3 implemented driver version 18 and model-local Rainbow fixes, plus the required shared LST cache refresh. The new portable suite has 15 cases and passed 15/15 once; final full/sanitized validation remains pending.

### MountSim acceptance mapping and final verification

| Named case(s), prefix `rainbow_` | Assertions |
| --- | --- |
| `driver_info_and_property_inventory`, `uses_legacy_firmware_protocol`, `reconnects_after_clean_disconnect` | Entry-point metadata, one mount interface, visible/hidden capabilities, firmware 190402, no pre-definition updates, legacy query/global-stop gates, repeated connection and teardown. |
| `sync_fractional_readback`, `reachable_goto_completion_and_abort`, `rejected_goto_recovers` | Signed/fractional RA/DEC SYNC without MS, zero-padded small DEC, site/LST-relative targets, BUSY until arrival, actual coordinate readback, already-at-target completion, BUSY request refusal, abort then new GOTO, below-horizon failure then recovery. |
| `manual_motion_covers_all_rates_axes_and_stops`, `manual_motion_readback_and_active_loss` | All four rates and directions, exact commands, simultaneous RA/DEC motion and reversal, coordinate progress, stable DEC after abort, real cable loss during GOTO, failed abort on lost transport, reconnect during movement and fresh abort. |
| `tracks_and_selects_every_rate`, `location_roundtrip_and_legacy_time` | Tracking on/off, sidereal/solar/lunar selection plus later poll readback, 70% guide rate retained after reconnect, southern latitude/western longitude retained, legacy host/UTC writes rejected without SC and subsequent clock polls recover. |
| `park_completes_and_rejects_motion`, `park_abort_and_disconnect` | BUSY/OK home completion at HA +6h / DEC 0, tracking off, parked motion rejected without command, park abort leaves no BUSY, reconnect and disconnect while park pending. |
| `idle_transport_loss_and_recovery` | Real PTY loss from healthy connection, write failure ALERT, new PTY recovery and fresh command. |

Final checks:

- `test-mount-rainbow-mountsim`: **13/13 passed** through app version 2.3, model RST135, actual firmware reply 190402.
- `test-mount-rainbow-simulator`: **15/15 passed**, including the unchanged normalized modern initialization reference trace.
- `test-mount-rainbow-simulator-sanitize`: **15/15 passed** with ASan/UBSan on arm64 (`detect_leaks=0`, as the existing target specifies). No sanitizer error.
- Universal macOS driver build passed; strict `clang -fsyntax-only -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter` passed for generated driver, both test sources and portable emulator. Shared static harness helpers account for unused-function suppression.
- Regenerated `.c`, `.h`, `_main.c` are byte-identical to the verified generated output. Generator unchanged; driver 17 -> 18. No property names added or removed.
- Xcode project passes `plutil -lint`; `make -n OS_DETECTED=Linux test-mount-rainbow-mountsim` contains only the explicit unsupported-platform message/exit. No MountSim dependency was added to portable defaults. Linux/Windows execution remains untested.
- Capture evidence is saved outside the cleaned test build. The initial command trace is preserved; intentional differences are corrected Cu setter, signed western longitude, SYNC padding, stop-before-close for active owned work, acknowledgement framing, and property completion/error ordering described above.

No guider device is exposed, so pulse timing/shared-guider ownership are not applicable. There is no programmable park position, unpark property, side-of-pier, PEC, encoder, custom tracking or Alt/Az command support in this driver. Modern firmware and malformed/injected replies are tested by the portable emulator; the MountSim relay never fabricates replies. Physical serial electrical behavior, mechanical accuracy, actual library unload/reload and other firmware versions are not established by this run.

MountSim's independent `python3 tests/test_control.py --app build/Build/Products/Debug/MountSim.app` also passed: new RST135 wire regressions, Temma motion/protocol checks, every model constructor, 10 repeated sessions, 20 model framing/identity audits and 20 interrupted parser/model-replacement cycles. This broader run covers the shared longitude/LST correction's integration. Final fetch found both upstreams unchanged, so no conflict resolution was needed.

### Final test summary for this acceptance run

MountSim simulated acceptance: **13 run / 13 passed**. Portable simulated integration: **15 run / 15 passed**, repeated **15 / 15** under ASan/UBSan. Unique simulated scenarios: **28 / 28**. Physical hardware: **0 run / 0 passed**.
