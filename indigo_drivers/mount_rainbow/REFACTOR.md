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

## Reader replies on the device queue (3.0.0.21, 2026-09-27)

Findings TGT-B02, TGT-C02 and the rainbow parts of TGT-B03 and TGT-B05 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`.
Simulator run on Linux x64 only; no hardware and no MountSim run for this change (MountSim needs macOS).

Defects, reproduced against 3.0.0.20 before the fix:

- TGT-B02 / TGT-B05: the `rainbow_reader` thread wrote every reply straight into the properties, truly concurrent
  with the bus thread and the change handlers. The reply to the poll's `:AT#` or `:Ct?#` replaced a pending
  `MOUNT_TRACKING` or `MOUNT_TRACK_RATE` request, set it OK before its handler ran (reopening the BUSY guard), and the
  handler then read the overwritten value: a tracking ON request sent `:CtL#`, a solar rate request `:CtR#`.
- TGT-C02 / TGT-B05: the replies to `:GC#:GG#:GL#` wrote the mount clock into a pending `MOUNT_UTC_TIME` request and
  set it OK, so the handler sent the mount its own time back.
- A failed tracking request (transport lost) left the requested state shown with ALERT instead of the tracking the
  mount last reported.

Fix: the reader only reads replies, stores them in `PRIVATE_DATA->messages` under `message_mutex` and queues
`rainbow_process_messages()`, which applies them on the device queue in turn with the change handlers (the same
hand-over as dome_nexdome3). `rainbow_sync_command()` runs in the connection handler on that queue, so it takes the
replies over itself while it waits. The reply handling records the reported tracking, track rate, UTC time and offset
in private data and leaves `MOUNT_TRACKING`, `MOUNT_TRACK_RATE` and `MOUNT_UTC_TIME` alone while they are BUSY.
The tracking and track rate handlers send `indigo_get_switch_target()`, apply it with `indigo_apply_switch_targets()`
and on a failed write show the state last reported with ALERT; the UTC change branch records the request with
`indigo_mount_set_utc_target()`, the handler sends `indigo_mount_get_utc_target()`, writes it into the items once
written, and otherwise shows the mount clock last reported with ALERT. Commands and their order are unchanged
(`rainbow_preserves_modern_initialization_order` still matches the original reference trace).

TGT-B03 (rainbow part), won't fix: `:Gt`, `:Gg` and `:CU0=` are only replies to `:Gt#:Gg#` and `:CU0#`, which the
driver sends only from the connection handler and waits for there (the protocol documents no unsolicited frame), and
`MOUNT_GEOGRAPHIC_COORDINATES` / `MOUNT_GUIDE_RATE` are not defined until the connection completes, so no request can
be pending when they arrive; with the move onto the queue they are also serialized with the handlers.

Pre-existing Linux build failure fixed: the simulator uses `lround()` and did not link without `-lm` on Linux
(`indigo_test/Makefile`), so the suite could not be built there.

Regression tests (`indigo_test/integration/test_mount_rainbow_simulator.c`); the simulator answers the poll's command
800 ms late (existing `DELAY` injection) while a gate handler holds the device queue. Each case first sends the request
while the reply is outstanding (fails on 3.0.0.20), then again with the reply read before the request (proves the BUSY
guard: with the guard removed from the `:AT` handling the first OK after the OFF request shows ON):

- `rainbow_tracking_request_survives_poll_reply`: 3.0.0.20 published OK before the handler ran and sent no `:CtA#`.
- `rainbow_track_rate_request_survives_poll_reply`: 3.0.0.20 published OK early and sent no `:CtS#`.
- `rainbow_utc_request_survives_poll_reply`: 3.0.0.20 published OK early and sent the mount clock, not `:SL22:34:56#`.
- `rainbow_handles_active_transport_loss` now also checks that the failed ON request shows OFF (fails on 3.0.0.20).

Verification: suite 18/18 on Linux x64, the three new cases 5/5 repeated, 18/18 under a temporary Linux
ASan/UBSan build (the Makefile's sanitize target is arm64 only), regeneration byte-identical, recorded run through
`tools/run_driver_test.py`.

### Final test summary for this change

Simulated: 18 run / 18 passed. Hardware: 0 run / 0 passed.

## Protocol review: direction, guiding, park, home and connection (3.0.0.22, 2026-10-04)

The driver was reviewed against the bundled protocol sheet `RainbowAstro_200629.pdf`.

### Found defects and changes

- North and south were swapped. The protocol sheet defines `:Ms#` as "manual DEC + move" and `:Mn#` as "DEC -",
  so north is `:Ms#`. `MOUNT_MOTION_DEC`
  NORTH now sends `:Ms#` (stop `:Qs#`), SOUTH `:Mn#` (stop `:Qn#`). The portable simulator already moved the
  declination up on `:Ms#`; no test had checked the direction.
- No pulse guiding. A guider device now pulses with `:RG#` and the direction command and ends the pulse with the
  stop of that axis (`:Qs#`, `:Qn#`, `:Qw#`, `:Qe#`), or `:Q#` on firmware older than 200625. Pulses are refused
  during a slew and while parked. Writes of the mount and the guider share a mutex; the guider reads the firmware
  version from the `:AV#` probe, so it works without the mount device connected.
- The interface and the protocol were never selected. The connection now sends `:AU#` (serial) or `:AW#` (network)
  and `:AR#` before the `:AV#` probe; a mount left in the LX200 protocol (`:AL#`) does not answer `:AV#` otherwise.
- The network default port was 4030; the WiFi module of the mount listens on 7100, which is the default now.
- Park was the homing command. `:Ch#` finds the mechanical origin, so it is `MOUNT_HOME` now (`:CHO#` success,
  `:CH0#` / `:CH<#` RA / DEC failure, abort and timeout end it in ALERT). `MOUNT_PARK` has PARKED and UNPARKED: a
  park converts `MOUNT_PARK_POSITION` (hour angle, declination; `MOUNT_PARK_SET` sets it) to altitude and azimuth,
  slews with `:Sa#`, `:Sz#`, `:MA#` and sends `:CtL#` on `:MM0#`; `:MML#`, `:MMU#`, `:MME#` end it in ALERT.
  Unpark sends `:CtA#`. The mount does not report a park, so a connection starts unparked.
- `MOUNT_TRACK_RATE` showed King and custom rates the driver could not set (King sent sidereal); it now has
  sidereal, solar and lunar. `MOUNT_GUIDE_RATE` is limited to the documented 0.1x to 1.0x (10 to 100 %).

Not taken over: side of pier (`:CG3#`, `:CY#`, reply layout not documented), forced meridian flip (`:Af0#`, `:Af1#`),
star alignment (`:CN`), slew speed setup (`:Cu1=` to `:Cu3=`), and the status queries for motors, temperatures
and voltage (`:GY#`, `:CP#`, `:CT#`, `:Cv#`).

### Simulator

`:AR#` / `:AL#` select the protocol and `--lx200-protocol` starts in LX200, where every other command is ignored;
`:AU#` and `:AW#` are accepted; `:Sa#`, `:Sz#`, `:MA#` slew to an altitude and azimuth through the site latitude
and the local sidereal time of the mount clock, without changing the tracking, and refuse a negative altitude
with `:MML#`.

### Tests

Changed: `rainbow_driver_info_and_property_inventory` (park, park position, park set, home, three track rates,
guide rate limits), `rainbow_manual_motion_covers_all_rates_axes_and_stops` (north `:Ms#`),
`rainbow_park_abort_and_pending_disconnect` (`:MA#`). Replaced `rainbow_park_reports_success_and_failure` by
`rainbow_park_slews_to_park_position`. New: `rainbow_park_follows_park_position`,
`rainbow_home_finds_mechanical_origin`, `rainbow_north_raises_declination`, `rainbow_selects_rainbow_protocol`,
`rainbow_guider_pulses_every_direction`, `rainbow_guider_uses_global_stop_on_legacy_firmware`.

First recorded run (2026-10-04 14:29): 24/22 Failed. Both park cases timed out on the unpark: the generated
MOUNT_PARK handler does not publish the property, and the unpark branch did not either. It does now.

Second recorded run (2026-10-04 14:31) on macOS arm64: 24/24 OK. MIGRATION_STATUS.md hardware-free count 18 -> 24.
The MountSim suite was not run; its RST135 model may still use the old north/south mapping and park semantics.

### Final test summary for this change

Simulated tests: **24 run, 24 passed** (second recorded run). Hardware tests: **0 run, 0 passed**.

## Mount testing rules coverage (3.0.0.23, 2026-10-04)

The suite was checked against the "Mount Drivers" chapter of `indigo_test/DRIVER_TESTING_RULES.md` (compliance scenarios
and every row of the Mount Driver Test Standard, the Guider standard for pulse guiding). Every new assertion was first
run against 3.0.0.22; the defects it found are fixed below.

### Found defects and changes

- `:MML#`, `:MMU#` and `:MME#` all reached the client as "Slew rejected or interrupted"; the message now names the
  reason (target below / above the altitude limit, slew canceled at the mount).
- An abort of a running GOTO or park published `MOUNT_EQUATORIAL_COORDINATES` OK; it now ends ALERT (an idle abort
  stays OK).
- A lost `:MM0#` left the GOTO or park BUSY for the 600 s deadline. A `:CL0#` after the poll reported the slew moving
  (`:CL1#`) now ends it the same way; a `:CL0#` that was in flight when the slew started still does not.
- `MOUNT_TRACKING` and `MOUNT_TRACK_RATE` were republished with every one-second poll; they are now published only when
  the reported state changes.
- A target just below 24 h was sent as `:Sr24:00:00.0#` and synced as `:Ck360.000...#`; it is 0 h / 0 degrees now.
- The prime meridian read back from `:Gg+000*00'00#` was published as longitude 360; it is 0 now.
- A disconnect during manual motion closed the port with the axis still moving; it sends `:Q#` first now and clears
  the motion items.
- A connection failed when any initialization reply was lost; only the identity (`:AV#`) is required now, a lost reply
  to another query leaves just its property ALERT. The `:AV#` probe in open is repeated (3 attempts) after a missing
  reply; a wrong reply is still refused.

### Simulator

`--tracking`, `--track-rate <0-3>` and `--guide-rate <D.D>` start the mount in a non-default state; the injection
action `NOCOMPLETE` executes a slew command but loses its `:MM0#` (event `LOST`).

### Scenario-to-test mapping (additions)

| Rule | Test |
| --- | --- |
| Connect publishes device state, no clock/site/setting writes, no unknown commands, placeholder `:CT3#` keeps the rate, unchanged state not republished, external change published once, guide speed range ends on the wire | `rainbow_connect_publishes_device_state` |
| Identity probe retry after a missing reply, lost initial readback marks only its property | `rainbow_connect_recovers_from_lost_initial_replies` |
| Refused identity leaves no mount property defined | `rainbow_rejects_non_rainbow_transport` |
| GOTO prerequisite sequence (rate, `:CtA#`, target, `:MS#`), stale `:CL0#` in flight, request while BUSY ignored and original target reached, tracking at the selected rate after the slew, targets as hour angles on both sides of the meridian, largest sub-unit, DEC pole clamp, 24 h / 360 degrees wrap for GOTO and SYNC, negative zero-degree DEC | `rainbow_goto_sequence_completion_and_encoding` |
| Controller reason in the client message, coordinates keep the real position | `rainbow_goto_ack_prefix_and_error_recovery` |
| Abort mid-slew: one stop, ALERT, two equal readbacks short of the target; idle abort keeps position and tracking; momentary item OFF | `rainbow_abort_stops_slew_once` |
| Lost completion of GOTO and park | `rainbow_lost_slew_completion_ends_by_status` |
| West/east RA direction, simultaneous axes, independent stop, reversal, disconnect stops motion, no stale motion after reconnect | `rainbow_manual_motion_axes_are_independent` |
| Manual motion released when its client detaches | `rainbow_manual_motion_released_when_client_detaches` |
| Park: coordinates BUSY while moving, tracking OFF when parked, GOTO/tracking/motion refused without a command showing the device state, unpark sends no motion, guard does not latch | `rainbow_park_slews_to_park_position` |
| Park interrupted by disconnect: no stale BUSY, not reported failed | `rainbow_park_abort_and_pending_disconnect` |
| One-item location change resends the other, prime meridian write and readback | `rainbow_location_partial_change_and_prime_meridian` |
| Time zone before clock, host-time momentary item | `rainbow_writes_location_and_time` |
| Legacy firmware: no axis stops, clock writes end ALERT without a command | `rainbow_uses_legacy_firmware_protocol` |
| Guider: zero pulse, item reset, independent axes, same-axis replacement with one stop | `rainbow_guider_pulse_zero_replacement_and_axes` |
| Guider with the mount: both connection orders, no pulse during a slew or while parked (accepted after unpark), SHUTDOWN refused, guider-only session does not poll, guider disconnect mid-pulse stops the axis | `rainbow_guider_shares_connection_with_mount` |

### Not covered, with reason

- Slew that never completes: the GOTO/park/home deadline is 600 s and not configurable, too long for an automated case.
- A completed slew with lost `:MM0#` that ends between two polls (no `:CL1#` seen) still waits for the deadline.
- Rejection of `:Sr#`/`:Sd#`: the protocol documents only the `1` acknowledgement; the driver does not read it.
- A setting acknowledged but not kept (published with the device value and ALERT): the driver does not read settings
  back in its handlers; the next poll shows the device value with OK (tracking, rate) and the guide speed is not polled.
- RA drift with tracking off, guide pulse displacement and tracking restore after an RA pulse: the simulator does not
  model the sky drift or the guide speed.
- Pier side, hemisphere-dependent direction, alignment, PEC, `MOUNT_STATE`, custom rate, home/park-set position
  workflows: not implemented by the driver (`MOUNT_PARK_SET` CURRENT is framework behaviour).
- Network (`rainbow://`, TCP 7100) transport: opt-in socket tests are out of the normal integration target.

## Protocol review: coordinate range, collisions, side of pier and status (3.0.0.24, 2026-10-04)

The driver was reviewed once more against the Rainbow protocol as the mount uses it, including the queries that the
protocol sheet does not document, and against the vendor's release notes, which list collisions of a pulse or a manual
move with homing and slewing as fixed defects. No hardware test was made; everything below is simulator-validated only.

### Found defects and changes

- A right ascension reported beyond 24 h or below 0 h (`:GR25:00:00.0#`) was published as it was. It wraps into
  [0, 24) now. A declination reported beyond a pole (`:GD+95*...#`) is mirrored back over it (85 degrees), the right
  ascension stays as reported. Found by source audit, reproduced by `rainbow_wraps_and_folds_reported_coordinates`.
- Nothing prevented operations that drive the same axes from overlapping. While the mount searched for home (`:Ch#`),
  a GOTO, sync, park and manual move were sent; while a guiding pulse ran, a GOTO, park, homing or manual move was sent
  and the pulse's axis stop (`:Qn#`, `:Qs#`, ...) could stop it; a GOTO or park accepted a manual move and homing; a
  manual move accepted homing and pulses; a pulse was accepted during a search for home. Each of these is refused now
  with a reason (`reject_change` on the mount properties, a refusal in the guider handlers); a sync during a pulse and
  a stop request are still accepted. The guider and the mount share the state through `homing`, `manual_motion`,
  `pulse_ra` and `pulse_dec` in the private data. Found by source audit, covered by
  `rainbow_refuses_motion_while_searching_for_home` and `rainbow_refuses_collisions_with_manual_motion_and_pulses`.
- The tracking-rate reply was matched by its first digit, so the temperature reply `:CT25.5|30.2|29.8#` would have
  been read as the lunar rate (`:CT2#`). A rate reply must now be `:CT<0-2>#`; a reply with `|` is the temperatures.
  Covered by `rainbow_reports_side_of_pier_and_status` (rate unchanged and not republished over several polls).

### New capabilities

- `MOUNT_SIDE_OF_PIER` (read-only): `:CG3#` gives the DEC axis angle of the aligned mount, `:CY#` the current DEC axis
  angle (7 characters), a separator and the RA axis angle; the OTA is west of the pier when the DEC axis is more than
  90 degrees from its aligned position. A forced meridian flip (`:Af0#`, `:Af1#`) is not supported, so the property is
  not writable.
- `X_RAINBOW_POWER` (`:Cv#` input voltage, `:CP<DEC>|<RA>#` motor power in percent), `X_RAINBOW_TEMPERATURE`
  (`:CT<board>|<RA>|<DEC>#`) and the light property `X_RAINBOW_STATUS` (`:GY#` characters 1, 3 and 4 are the telescope
  control system, the DEC and the RA motor, `O` is fine; `:GHO#` the home sensor was found).
- These queries are sent once at connect (`:CG3#:CY#:Cv#:CP#:CT#:GY#:GH#`, up to 1 s); only the answered ones are
  polled each second, the properties of the others stay hidden (`HOME` is left out of `X_RAINBOW_STATUS` when `:GH#` is
  not answered). Values are published only when they change.

Not taken over: forced meridian flip (`:Af0#`, `:Af1#`), star alignment (`:CN`), slew speed setup (`:Cu1=` to `:Cu3=`),
the auto-resume query (`:CR#`), `:SPH#` and `:SPE#` (meaning unknown), the mount's own altitude and azimuth (`:GA#`,
`:GZ#`; INDIGO computes them).

### Simulator

`:CG3#` answers `:CG3000.00000#`; `:CY#` reports the DEC axis angle within 90 degrees of the alignment east of the pier
and beyond it west of the pier, which a GOTO or altitude/azimuth slew to a target east of the meridian selects, and the
RA axis angle from the hour angle; `:Cv#`, `:CP#`, `:CT#` and `:GY#` answer fixed healthy values; `:GH#` reports the
home sensor found after a completed homing (it reported the park state before). `--no-diagnostics` ignores all seven
queries. The Arduino sketch `mount_rainbow_simulator.ino` was not changed.

### Tests

New: `rainbow_wraps_and_folds_reported_coordinates`, `rainbow_reports_side_of_pier_and_status` (initial values,
read-only side of pier, no republishing on unchanged polls, changed temperature and power, a motor needing a check,
west/east from injected and from simulated axis angles after GOTOs on both sides of the meridian, home sensor found by
homing), `rainbow_hides_unanswered_status_queries` (no property, one probe each, nothing polled, also after a
reconnect), `rainbow_refuses_motion_while_searching_for_home`, `rainbow_refuses_collisions_with_manual_motion_and_pulses`.
Changed: `rainbow_driver_info_and_property_inventory` (side of pier and the three status properties are defined),
`rainbow_home_finds_mechanical_origin` (the abort case loses `:Ch#`; the mount was already at home and could answer
`:CHO#` before the abort, which failed under the sanitizer build).

The sanitizer build (`make -C indigo_test test-mount-rainbow-simulator-sanitize`) passed twice after that change.
Recorded run (2026-10-04 21:09) on macOS arm64: 39/39 OK. MIGRATION_STATUS.md hardware-free count 34 -> 39.

### Not covered, with reason

- The meaning of the `:CY#` separator character and of `:GY#` character 2 is not known; the driver ignores both.
- Whether a mount reports a declination beyond a pole with the right ascension of the other side is not known; the
  driver keeps the right ascension as reported.

### Final test summary for this change

Simulated tests: **39 run, 39 passed** (recorded run). Hardware tests: **0 run, 0 passed**.
