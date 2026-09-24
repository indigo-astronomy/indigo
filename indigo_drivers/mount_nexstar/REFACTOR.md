# Mount NexStar Refactor

## Current Code Findings

- The driver was still hand-written INDIGO 2.x style code in `indigo_mount_nexstar.c`; the source of truth is now `indigo_mount_nexstar.driver` and generated `indigo_mount_nexstar.c`, `indigo_mount_nexstar.h` and `indigo_mount_nexstar_main.c` must stay synchronized.
- The old connection lifecycle kept a private `count_open` and unlocked `serial_mutex` from inside the failed `mount_open()` path even though the caller owned that lock. The generated lifecycle replaces this with the generator-owned shared connection counter and a transactional `nexstar_open()` / `nexstar_close()` pair.
- The mount and guider shared one serial `libnexstar` connection. The migration keeps that topology and lets the generator serialize connection open/close ownership across both logical devices.
- The old guider RA/DEC handlers slept for the whole guide duration in the property handler. The migration changes guide pulses to a start handler plus `guider_guide_ra_finalizer()` and `guider_guide_dec_finalizer()` so the device queue is not blocked.
- The old `MOUNT_SLEW_RATE` change path marked and updated `MOUNT_GUIDE_RATE_PROPERTY` instead of `MOUNT_SLEW_RATE_PROPERTY`. The generated property handler now updates the handled slew-rate property.
- The old GPS detach path returned `indigo_guider_detach(device)` from a GPS device. The migrated custom GPS device returns `indigo_gps_detach(device)`.
- GPS remains dynamically attached only for the Celestron dialect, matching the old driver behavior. The generator owns mount and guider boilerplate; custom GPS code remains in the `.driver` code block because the old driver exposes GPS conditionally after mount identification.
- Baseline validation before migration: `make -C indigo_test build/integration/test_mount_nexstar_simulator` succeeded, but running `indigo_test/build/integration/test_mount_nexstar_simulator` exited with signal 11 before any migration changes.
- Coverage audit baseline (2026-09-13, commit `6b6b9bd69`, macOS Darwin 25.6.0 arm64): `make -C indigo_test build/integration/test_mount_nexstar_simulator` built the universal arm64/x86_64 simulator and test executable; a standalone `cd indigo_test && build/integration/test_mount_nexstar_simulator` run passed all 10 existing cases. `make -C indigo_test test-clean` removed the build artifacts afterward.
- The existing simulator has deterministic position, motion, tracking, location/time, guide-rate and GPS responses, but no command observation channel or failure injection. Consequently the existing tests cannot prove exact wire commands, malformed/short reply handling, command/read/poll/stop failures, transport loss or recovery.
- The mount test covers the Celestron happy path broadly, while the Sky-Watcher branch is limited to connection and identity. It omits parts of the visible property contract, coordinate boundary/sign cases, already-at-target and overlapping GOTO, full manual direction/reversal/simultaneous-axis behavior, failure recovery and reconnect.
- The guider test does not observe fresh property revisions. Three pulse requests wait only for a non-BUSY state and can accept the previous OK state before the queued handler starts. It also lacks simultaneous axes, disconnect/reconnect with a pending pulse, error recovery and the required timing measurements.
- The dynamic GPS test covers only a Celestron 3D-fix happy path; it does not assert the complete property/item contract, exact values, fix loss/reacquisition or shared-connection lifetime.
- Shared mount/guider ownership is not covered in both connection orders, and sibling survival/final-close behavior is not verified.

## Documentation Findings

- `README.md` describes serial/network NexStar protocol support, single startup instance plus runtime additional instances, `libnexstar` dependency, and two non-standard controls: `TRACKING_MODE` and `COMMAND_GUIDE_RATE`.
- `indigo_docs/PROPERTIES.md` already lists `TRACKING_MODE` and `COMMAND_GUIDE_RATE` as custom NexStar properties; after migration its source note needs to point to the `.driver` source and generated `.c` output.
- `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` listed the Arduino `.ino` simulator candidate even though a host-side C simulator exists. The migration moves the C simulator into the implemented simulator table and removes the stale candidate entry.
- The mount testing rules require simulator-backed coverage for connect/identity, GOTO and SYNC, manual motion and abort, tracking/rates, park/unpark, time/location/options, transport failure/recovery, and guider coverage for all guide directions.
- The bundled `celestron.pdf` and `skywatcher.pdf` protocol references document the command framing and replies used by the simulator, including precise/legacy RA/DEC GOTO and SYNC, tracking, fixed/variable slew, time/location, model/version/alignment and pass-through commands. The simulator audit and new command assertions will follow these documented encodings.

## Hardware-Test Decision

- No hardware testing will be performed in this work: no specific NexStar/SynScan mount and hand controller has been made available. Simulator results are software/protocol validation only and will not be presented as physical mount motion, tracking or electrical guide-pulse accuracy.

## Migration Plan

1. Convert the driver to `indigo_generator` with `indigo_mount_nexstar.driver` as the source of truth.
2. Preserve serial protocol behavior through `libnexstar`, including Celestron and Sky-Watcher dialect detection, ST4 guide-rate handling, tracking-mode selection, mount time/location updates, side-of-pier probing and conditional Celestron GPS exposure.
3. Move long-running operations to generator queues and finalizers: mount park polling and guider pulse completion.
4. Expand the host-side serial simulator to cover elapsed goto/park motion, manual slew stop state, sync, tracking, time/location, guide-rate readback, Celestron GPS pass-through and failure injection needed by automated tests.
5. Replace the smoke-style NexStar test with full mount/guider simulator coverage following `indigo_test/DRIVER_TESTING_RULES.md`; keep Windows status unchanged because this migration explicitly does not solve `libnexstar` Windows support.
6. Run generator, build the driver/test target, run the full NexStar simulator test, clean test artifacts and update `MIGRATION_STATUS.md`, `indigo_test/CHANGES.md`, `indigo_docs/PROPERTIES.md` and `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` with the verified results.

## Coverage Completion Plan

1. **Complete** — Extended the NexStar host simulator with a monotonic `<ready-file>.events` command log and one-shot `<ready-file>.control` actions for dropped, short and malformed replies and transport closure. The existing documented binary command framing is unchanged. `make -C indigo_test build/integration/mount_nexstar_simulator` passed.
2. **Complete** — The shared harness now records both fresh property revisions and the latest revision for every property state, preventing transient BUSY/ALERT updates from being hidden by a subsequent poll. Mount and GPS property/item/range contracts now include LST, horizontal coordinates, epoch, numeric bounds, all GPS fix states, exact location and UTC offset.
3. **Complete** — Added exact command and state coverage for Celestron and Sky-Watcher identity/firmware, coordinate readback, signed/boundary coordinates, GOTO/SYNC completion and overlap rejection, all manual directions and simultaneous axes, tracking/modes/rates, park/unpark/abort, time/location translation, malformed/short/dropped replies and transport closure/recovery.
4. **Complete** — Added guider-only, mount-first and guider-first shared connections, sibling survival after either disconnect, final-close/reopen behavior, transport reconnect and disconnect/reconnect during a pending pulse. GPS connection and fix loss/reacquisition are exercised while sharing the mount transport.
5. **Complete** — Guider coverage now requires fresh BUSY-to-OK or BUSY-to-ALERT revisions for east, west, north and south; covers zero duration, same-axis BUSY rejection, concurrent independent axes, failed start, failed stop, recovery and pending-pulse disconnect/reconnect.
6. **Complete** — Added opt-in `bench_mount_nexstar_guider_timing`, measuring monotonic simulator command start-to-stop endpoints for 20, 100 and 500 ms pulses in all four directions, with one warm-up and four measured samples per matrix cell under idle polling and simultaneous mount motion. It reports min/mean/median/p95/p99/max/sd, maximum absolute error and mean percentage error without a host-dependent threshold.
7. **Complete** — Protocol failures exposed production bugs where polling replaced the last valid position/location/time and GPS fix with invalid data, failed location writes published unaccepted targets, and failed tracking changes left the requested rather than actual mode selected. These are fixed in `indigo_mount_nexstar.driver`; the driver version is 32 and the generated C/header/main outputs are synchronized. The firmware formatter also uses bounded `snprintf()`.
8. **Complete** — Driver, strict-warning test, normal integration test, combined ASan/UBSan test and timing benchmark all passed on macOS arm64. The unchanged generator produced byte-identical output on consecutive runs, `git diff --check` passed, no new persistent file needs Xcode registration, and the test cleanup removed its build and simulator artifacts.

## Scenario-to-Test Mapping

| Required behavior | Automated scenario(s) |
| --- | --- |
| Connect, identity, firmware and both dialects | `nexstar_mount_connects_both_protocol_dialects` |
| Sky-Watcher coordinates, tracking and manual motion | `nexstar_skywatcher_executes_coordinates_tracking_and_motion` |
| Celestron property/item/range contract and basic options | `nexstar_celestron_mount_passes_serial_compliance_checks` |
| NexStar HC time and location translation/readback | `nexstar_nexstar_hc_sets_time_and_location` |
| Signed/boundary SYNC, GOTO completion/overlap, abort and recovery | `nexstar_mount_tracks_syncs_and_aborts_coordinate_changes` |
| Unaligned coordinate rejection | `nexstar_mount_rejects_coordinates_when_unaligned` |
| Malformed, short and dropped replies; failed writes; transport close/reconnect | `nexstar_mount_recovers_from_protocol_and_transport_failures` |
| All manual directions, simultaneous axes, abort, park/unpark and parked guard | `nexstar_mount_moves_manually_and_parks` |
| Abort while park work is queued | `nexstar_mount_aborts_queued_park` |
| Alt/Az tracking modes AA, EQ, AUTO and off | `nexstar_tracking_mode_is_exposed_for_altaz_models` |
| GPS property contract, no/2D/3D fix, exact data, loss and reacquisition | `nexstar_celestron_gps_device_reports_fix` |
| Guide-rate commands, all directions, fresh transitions, overlap, concurrent axes and failures | `nexstar_celestron_guider_passes_serial_compliance_checks` |
| Shared ownership in both orders, sibling survival and pending-pulse reconnect | `nexstar_shared_devices_survive_both_connection_orders` |
| Guide timing for all directions/durations under idle and motion workloads | `bench_mount_nexstar_guider_timing` |

## Validation Results

- Consecutive `../../build/bin/indigo_generator indigo_mount_nexstar.driver` runs produced identical SHA-1 hashes for `indigo_mount_nexstar.c`, `indigo_mount_nexstar.h` and `indigo_mount_nexstar_main.c`.
- `make -C indigo_drivers/mount_nexstar -f ../../Makefile.drv` passed after regeneration and rebuilt the archive, dynamic library and executable.
- `make -B -C indigo_test CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter' build/integration/test_mount_nexstar_simulator` passed for both the simulator and integration test.
- `cd indigo_test && build/integration/test_mount_nexstar_simulator` passed all 13 scenarios listed above.
- `make -B -C indigo_test build/integration/test_mount_nexstar_simulator_asan` passed. Running it with `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1` passed the same 13 scenarios with no AddressSanitizer or UndefinedBehaviorSanitizer report. LeakSanitizer is unavailable on this macOS runtime and was not claimed.
- `make -B -C indigo_test build/benchmark/bench_mount_nexstar_guider_timing` and the benchmark run passed. The 24 matrix cells produced 96 measured samples plus 24 warm-ups. Mean signed error ranged from +1.699 to +4.442 ms; maximum absolute error over all cells was 5.973 ms. These are host-side serial/scheduler measurements, not physical relay or mount accuracy.
- `git diff --check` passed. `make -C indigo_test test-clean` removed test build artifacts, and no `/tmp/indigo-serial-sim.*` directory remained after the final runs.
- Windows runtime remains unverified and unchanged; `MIGRATION_STATUS.md` keeps the existing `libnexstar` Windows TODO in the comment column.

## Final Test Summary

- Simulated functional test executions: **26 run, 26 passed** (13 normal plus the same 13 under ASan/UBSan).
- Timing samples: **96 measured successfully**, plus 24 warm-ups; timing data is reported as measurement rather than pass/fail behavior.
- Hardware tests: **0 run, 0 passed**. No compatible physical NexStar/SynScan mount was available, so hardware motion, electrical ST4 behavior and Windows runtime remain explicitly unverified.

## Overlapping guide pulses (2026-09-20)

A guide pulse requested while another pulse on the same axis was still running was silently
discarded: the generated change branch dispatched both guide properties through the BUSY-guarded
`INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE` macro, so the second request never reached the
handler and the `indigo_cancel_pending_handler()` call the handler already carried was unreachable.
`GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare `accept_while_busy = true` and zero both axis
items in `on_change_request`, which is the pattern shared by every INDIGO driver that exposes a
guider.

The finaliser cancellation stays in `on_change`. It runs there on the device queue thread, where
`indigo_queue_remove()` skips its blocking wait; the same call from `on_change_request` runs on the
bus thread, where it blocks until a running handler finishes. Cancelling the pending *handler* from
`on_change_request` was tried and hung this suite, so it was not adopted.

`nexstar_celestron_guider_passes_serial_compliance_checks` asserted zero `50 02 10 25` events after
a reversing request, which recorded the discard. It now asserts the negative-guiderate command is
actually sent, and adds two duration-measuring cases: a 2000 ms pulse replaced after 500 ms by a
600 ms pulse in the same direction, and the same pulse replaced by a 300 ms pulse in the opposite
direction.

Because the request is now accepted while the property is BUSY, two requests arriving inside the
queue latency can queue two handlers that both act on the already-overwritten item values, so the
same command can reach the mount twice. The resulting pulse is still the second request's, with a
single finaliser; only the redundant command is observable.

## CGE MountSim acceptance (2026-09-23)

The unchanged 13-case portable NexStar suite passed before this campaign. The first
MountSim 2.3 CGE draft run passed 1 of 18 cases; raw traffic and individual case
logs are retained under `/tmp/cge-baseline`. That draft is not acceptance evidence:
its C clock used INDIGO's macOS `clock_gettime` wrapper (wall time) while the relay
used system monotonic time, and several cases accepted a BUSY coordinate target
before the mount reached it. The guide timestamp fix was verified with the
guider-first shared-lifetime case. MountSim's initially reported DEC is -90, so
long first slews need a real device-readback completion oracle and bounded time.

The CGE source/protocol audit found independently scoped issues. MountSim had
no Celestron GPS linked reply (`P ... B0 37`) or autoguide-rate `0x46`/`0x47`
replies. The driver also did not send autoguide-rate commands on Celestron:
vendored `libnexstar` guards its helpers with `VER_AUX = 0xFFFFFF`, which
excludes reported firmware 4.29 before I/O. The driver now uses a local
Celestron pass-through helper with byte conversion and reply termination
checks, retaining the SkyWatcher path and leaving vendored code unchanged.
MountSim now replies to these documented commands. Its single-axis zero-rate
command previously stopped both axes; it now stops only the addressed axis,
regardless of the last slew direction. Simultaneous-axis manual motion and
guider sibling survival passed against actual coordinate and raw edge oracles.

MountSim's inherited SYNC changed only a GUI pointing offset, leaving later
`E`/`e` coordinate reads and GOTO targets on the old motor solution. Its
NexStar implementation now maintains a sky-solution offset for reads and
inverse GOTO conversion; the CGE case verifies SYNC with later fresh reads and
physical GOTO arrival. Its inherited H/h controller clock ignored writes and
queried host time; the NexStar hand-controller clock implementation now advances the written time,
offset and DST through reconnect. Its inherited M abort stopped manual axes
but left GOTO active; the NexStar override cancels both. These fixes are
model-local to MountSim's NexStar class.

The driver also published four capability updates before their properties were
defined. Those premature updates were removed; the generated connection
handler defines the configured properties. Its existing `MOUNT_PARK_SET.CURRENT`
control is now visible. At the user's explicit direction, park command encoding
passes the signed park DEC to `tc_goto_azalt_p`: the library accepts -90..+90
and encodes negative angular values distinctly. The 14-case portable suite
checks negative and positive wire targets and CURRENT's signed value. This
proves sign preservation, not CGE physical park geometry.

CGE `B`/`b` and `Z`/`z` are motor-axis operations. MountSim now uses the timed
motor target path for `B`/`b`, reports motor-axis rather than geographic horizon
positions in `Z`/`z`, and preserves the mechanical target at GOTO completion.
The simulator's RA and DEC home steps map to the documented 90°/90° switch index;
the test checks a partial timed move and final `B`/`b` to `Z`/`z` arrival rather
than treating the command ACK as movement. The motor targets are reduced modulo
the actual steps per revolution before staging. A newly introduced conversion
rounded a southern park target to a whole revolution, which interacted with the
pre-existing `Steps.setRange` wrap inconsistency and made the simulator display
a four-digit mechanical declination. The modulo fix and native boundary checks
for both axes now keep the underlying mechanical declination bounded during
motion, including the southern signed-default park case. This runaway was
triggered by the new converter during this campaign, not measured in the
untouched baseline.

Public documentation does not establish a complete pier-side or hemisphere
transform between celestial HA/DEC and a physical counterweight pose. The
driver's historical `(HA + 12) * 15` maps its default HA 6 to RA axis 270°,
while Celestron documents an RA switch index of 90°; this difference could
reflect frame or pier conventions and remains unresolved. The signed DEC fix
preserves intent and the tests verify physical axis arrival, but neither proves
that every custom celestial park point is the intended hardware pose. Hardware
verification and a manufacturer-supported full transform remain gaps.

The final complete CGE run used `indigo_test/mountsim/run_mountsim.py` with
`--mount CGE`, binary forwarding and no trace terminator. Each case had an
isolated app, PTY and preferences directory; raw traffic is retained under
`/tmp/cge-final3-full`. A launcher failure before `test.log` creation now reports
the underlying case error cleanly instead of a secondary missing-log traceback.
The 20 cases cover identity and property lifecycle, both logical-device
connection orders, real SYNC/GOTO arrival, already-at-target, BUSY/abort, all
four manual directions and four rates, simultaneous axes, tracking/site/clock,
ST4 guide-rate readback, GPS, guide replacement and independent axes,
idle/active/pending-guide transport loss, pending-park loss and recovery.
Parking cases verify actual motion before completion, abort and transport loss.
The portable case verifies CURRENT's signed celestial capture and replay wire
target; the MountSim case sets HA -1, DEC +35 explicitly and then verifies the
`B`/`b` physical axis arrival. The southern default case verifies signed DEC
-90 and a physical target arrival. Raw
`B`/`b` versus `Z`/`z` motion is checked with no competing logical transport
owner. The raw CGE guide-rate regression also measures DEC motor displacement
at command rates 1 and 2; [Celestron's CGE manual](https://s3.amazonaws.com/celestron-site-support-files/support_files/CGE1100_11061.pdf) specifies 0.5× and 1×
sidereal, and the focused run measured 0.005244° and 0.010500° over 2.5 s.
The model now uses a CGE-specific documented rate table; other Celestron model
tables are unchanged. No optical or physical pier-side geometry was observed.

Guide pulse measurement observed raw forwarded ON/OFF `P` frames for 20, 100
and 500 ms in all four directions, with four measured repeats and one warm-up
per cell under tracking off and tracking on with coordinate polling. All 96
measured pairs and 24 warm-ups completed in the final run. The per-cell raw
software ON/OFF edge intervals and summary statistics are in the case logs.
The maximum absolute interval error across directions and workloads was
18.564 ms at 20 ms, 18.685 ms at 100 ms and 19.513 ms at 500 ms.
These are software-command edges, not optical or
electrical measurements. Timing used `clock_gettime_nsec_np(CLOCK_MONOTONIC)`
on macOS because the existing INDIGO global `clock_gettime()` wrapper ignores
its clock selector and returns wall time; the shared clock implementation was
not changed here.

The post-fix portable NexStar simulator suite and its AddressSanitizer and
UndefinedBehaviorSanitizer build each passed all 14 cases, including
malformed/short-reply and transport recovery paths. MountSim's five native
motor/serial/sky tests passed. Its isolated control/PTY and all-model
framing/identity suite passed on the final build. Hardware and Windows runtime
were not exercised. The first baseline and intermediate failed runs remain in
their separate `/tmp/cge-*` directories; only the last complete frozen run is
used for acceptance.

At the end of this campaign: **CGE MountSim 20 run, 20 passed; portable NexStar
14 run, 14 passed; portable ASan/UBSan 14 run, 14 passed; MountSim native 5 run,
5 passed; guide software-edge timing 96 measured and 24 warm-ups; hardware 0
run, 0 passed**. The driver version is 3.0.0.35. The changed CGE behavior was
not validated on physical hardware, Windows, or other NexStar models.

## SE wedge EQ MountSim acceptance (2026-09-23)

The selected MountSim profile is exact `SE` (model byte 12, NexStar 6/8 SE,
hand-controller firmware 4.29). This run targets an SE installed on a wedge in
EQ North and EQ South, as requested. Hardware testing is not part of this
non-interactive simulator run: final simulated results are reported below;
physical hardware tests run/passed: **0/0**. The Mac app is MountSim 2.3,
built with `xcodebuild -project MountSim.xcodeproj -scheme MountSim
-configuration Debug -derivedDataPath build CODE_SIGNING_ALLOWED=NO build`.
The driver and existing NexStar test binary compiled before this model work;
the CGE campaign's portable 14/14 and MountSim 20/20 results are the existing
cross-model baseline, not SE results. The first unmodified SE tracking/site
test passed 1/1 under `/tmp/se-baseline`; `/tmp/se-baseline/.../commands.events`
and `serial.raw` preserve the wire sequence. The SE-specific T2/T3 reproducer
was then added to the same test. Before the MountSim fix, source inspection
established that inherited `SynScan.getTrackingMode` always replies 2 when
tracking, and `setTrackingMode` interprets T3 as PEC; thus EQ South cannot be
read back. The dedicated raw T2/t2 and T3/t3 sequence now passes 1/1 under
`/tmp/se-tracking1`, including reconnect to the INDIGO driver. This is a
protocol readback claim only; no southern motor kinematics claim is made.

Current architecture: `indigo_mount_nexstar.driver` is the generated source
for mount, guider and GPS logical devices; the SE is not `TRUE_EQ_MOUNT`, so
INDIGO exposes `TRACKING_MODE` and hides the unsupported autoguide-rate
setter. `NexStar.m` inherits the SynScan serial parser and shares a GEM-like
`Motor` mapping. Celestron's [serial protocol](https://s3.amazonaws.com/celestron-site-support-files/support_files/1154108406_nexstarcommprot.pdf)
defines t/T values 0 off, 1 Alt/Az, 2 EQ North and 3 EQ South, plus B/b and
Z/z as fractions of turns about the mount axes. The [8SE product page](https://www.celestron.com/products/nexstar-8se-computerized-telescope)
lists EQ North/South modes and the wedge requirement; the [wedge guide](https://www.celestron.com/blogs/knowledgebase/understanding-wedges-for-alt-az-telescopes)
states the base is tilted and used with EQ alignment. In the unmodified SE
baseline, Z/z came from geographic horizon coordinates and B/b was rejected.
Optional simulated
SkySync GPS and hand-controller H/h clock are exercised separately; there is
no claim of built-in GPS or battery-backed RTC on an SE. The driver's park
property is HA/DEC while B/b is mount-axis position; the full transform is
not yet established. Physical single-fork clearance, wedge geometry, southern
motor direction and hardware park pose remain unverified.

Independent source-code cross-checks use the bundled libnexstar
`externals/libnexstar/src/nexstar.c`: its `TC_TRACK_EQ` selection sends the
southern protocol mode when site latitude is negative, and its B/b helper
sends raw mount-axis fractions. This supports the observed driver wire
selection; it does not establish the simulator's physical motor mapping.

Atomic plan and current state:

1. **Complete for non-park capabilities:** The 15-case non-park SE run covers
   lifecycle, coordinates, EQ North/South GOTO, manual motion, guide, loss and
   timing with explicit EQ mode. The shared locked launcher ran on a Mac
   arm64 host with an arm64 app and universal arm64/x86_64 INDIGO binary;
   x86_64 runtime was not exercised. Each case had a 300 s watchdog; evidence is under
   `/tmp/se-final-nonpark` and `/tmp/se-final-nonpark.log`.
2. **Complete for T/t and rates:** Checked the SE
   tracking protocol against Celestron documentation and an independent
   established implementation; retained
   original wire/property traces before any INDIGO driver behavior change.
   The T/t and manual-rate defects belong to MountSim; their model-specific
   fixes passed focused and full non-park regressions.
3. **Complete for modeled axes:** With the user's approved simulator scope,
   SE B/b now drives timed motor-axis motion and Z/z returns those axes with
   an explicit simulated 0°/90° home index. Raw precise/coarse roundtrip,
   intermediate movement and L arrival passed 1/1 under `/tmp/se-axis-focused`;
   the focused INDIGO park-to-axis arrival, abort and loss checks are part of
   the full suite. This establishes the model's internal axis behavior, not
   a physical wedge pose or celestial HA/DEC-to-axis transform.
4. **Complete:** The final frozen app/harness/driver hashes are in
   `/tmp/se-final-frozen-sha256.txt`; full SE 20/20, affected CGE 20/20,
   portable 14/14, portable sanitizer 14/14 and native 5/5 passed. The
   SE result is recorded in README Testing and regenerated TEST_SUMMARY.
   The root task handles the authorized push after scoped commits.

Found defects: `T3` on Celestron models was treated as SynScan PEC and `t`
collapsed all tracking-on states to EQ North. That prevented southern-mode
readback and AUTO discovery. The MountSim NexStar override now retains the
requested Celestron mode and answers `t` with that value while tracking is on;
the raw T2/t2 and T3/t3 reproducer passes. A GOTO started without a preceding
`T` can start the motor directly; the NexStar getter now reports the model's
default EQ North mode in that state, and the SE GOTO case checks raw `t=2`
after arrival. The SE B/b
absence and Z/z geographic interpretation are now also reproduced: the
unmodified SE park case under `/tmp/se-park-baseline` forwarded a precise `b`
target at raw timestamp 292850.881349, MountSim returned repeated `!#`
rejections, INDIGO briefly published BUSY, and neither coordinate axis moved
by 0.02° within 3 s. The case failed its physical-motion assertion. The
approved axis model fixed this simulator gap. The later first full run then
exposed the shared Motor's inappropriate geographic horizon check on the
mechanical target; the focused park case passed after bypassing that check
only for `mechanicalGoto`. The actual HA/DEC-to-physical-axis park mapping
and single-fork clearance are still unverified.

The EQ South exercise verifies T3/t3 wire readback, a southern-site driver
selection and reachable GOTO coordinate motion. NexStar's T setter still
passes a tracking boolean to the shared GEM-style Motor; the Motor does not
switch SE wedge geometry or physical axis direction between T2 and T3.
Therefore this suite does not verify southern physical tracking kinematics.

The SE model also inherits an undocumented default manual-rate table with
rate 2 at 10× sidereal and rate 9 at 1500×. The manufacturer's [6SE/8SE manual,
page 19](https://celestron-site-support-files.s3.amazonaws.com/support_files/NexStar%206SE%20%26%208SE%20manual%20-%203%20languages.pdf)
specifies rates 1–9 as 0.5×, 1×, 4×, 8×, 16×, 64× sidereal, then 1°/s,
3°/s and 5°/s. The existing manual-motion case established command movement
but did not measure those rates; an SE-specific raw motor displacement
regression reproduced a failed baseline under `/tmp/se-rate-baseline`:
rate 1 moved DEC 0.005343° in 2.5 s, while rate 2 moved 0.105271° rather than
the approximately 0.0105° specified by the manual. This establishes the
incorrect table as observable in the simulator. The SE-only table now uses the
manufacturer's values; `/tmp/se-rate-fixed` passes with rate 1 at 0.005257°
and rate 2 at 0.010493° over the same interval. Other models retain their
existing tables. The full shared non-park
SE run passed 14/14 under `/tmp/se-nonpark2` after the EQ setup harness fix,
including raw T2/t2 and T3/t3, both hemispheric driver selections and guide
software edge timing. This does not include rate or park acceptance.

The prior frozen non-park binaries are identified in
`/tmp/se-frozen-sha256.txt`, and the final app, harness and driver archive in
`/tmp/se-final-frozen-sha256.txt`. The final full `--mount SE` run passed
**20/20** under `/tmp/se-final-full` with the real driver, each case in an
isolated MountSim 2.3 process and PTY. It includes raw 4/8-digit B/b→Z/z
roundtrip with intermediate motor motion and L completion; park BUSY, real
coordinate movement, axis arrival and tracking-off ordering; signed southern
default target; park abort and transport-loss recovery. It also includes
identity, two-device lifetime, EQ North/South protocol/site and reachable
GOTO, SYNC, all manual directions, SE rates, hand-controller clock, optional
simulated SkySync GPS, guider replacement, idle/active transport loss and
guider timing. `nexstar_guider_transport_edge_timing` observed all 96 software
ON/OFF wire-edge pairs after 24 warm-ups for 20, 100 and 500 ms in four
directions under idle and tracking/polling workloads. These are forwarded
serial-command intervals, not physical ST4 or optical measurements.
The maximum absolute measured interval error was 22.213 ms across 24
duration/direction/workload cells.

Before the final build, the full raw-axis run passed 19/20 under
`/tmp/se-park-full1`: `b75555554,18E38E38` was accepted but the driver's
park transitioned BUSY→ALERT without axis motion. Source audit found that
`Motor.gotoTarget` applied its geographic sky-horizon limit to the distinct
`mechanicalGoto` path. A direct B/b motor-axis target on a tilted wedge has
no sky-horizon interpretation. The final MountSim fix skips this filter only
when `mechanicalGoto` is active; its focused park case passed 1/1 under
`/tmp/se-park-fixed-focused`, and the complete SE suite then passed 20/20.

The post-fix portable NexStar simulator suite passed **14/14** under
`/tmp/se-final-portable.log`; its AddressSanitizer/UndefinedBehaviorSanitizer
build passed **14/14** under `/tmp/se-final-portable-asan.log`. Both were run
from `indigo_test`, where the simulator's relative path resolves. MountSim's
five native motor/serial/sky regression groups passed **5/5** under
`/tmp/se-final-native.log`. The affected CGE full MountSim regression after
the shared T/t change passed 20/20 under `/tmp/cge-after-se`; the final
post-mechanical-GOTO CGE regression also passed **20/20** under
`/tmp/cge-after-se-park`. No
physical hardware, Windows runtime or x86_64 runtime was exercised.

SE history totals **91 simulated case invocations, 87 passed**, including
the deliberately preserved baseline failures, a first harness EQ setup
failure and the first 19/20 raw-axis full run. The formatting-only final
harness rebuild passed a focused raw-axis case 1/1 under
`/tmp/se-postformat-focused`. The final acceptance is
**SE MountSim 20 run, 20 passed; portable NexStar 14 run, 14 passed;
portable ASan/UBSan 14 run, 14 passed; MountSim native 5 run, 5 passed;
affected CGE MountSim 20 run, 20 passed;
hardware 0 run, 0 passed**. Physical SE wedge park pose, physical southern
tracking direction, HA/DEC-to-physical-axis transform and fork clearance
remain unverified; the completed claim is simulator axis/protocol behavior.

## CGEM DX MountSim 2.3 acceptance (2026-09-24)

Hardware-test decision: no CGEM DX is available for this run; hardware tests
are 0 run, 0 passed. The exact selected MountSim model is `CGEM`, identifying
as Celestron CGEM DX, model byte 14, firmware 4.29. The existing generated
NexStar driver is version 3.0.0.35. No driver behavior change is planned
unless the CGEM acceptance exposes one independently of the emulator.

Baseline on macOS arm64: `make -C indigo_test build/mountsim/test_mount_nexstar_mountsim`
built the current driver-backed test. With the isolated launcher, CGEM identity
and reconnect passed 1/1 under `/tmp/cgem-baseline-identity`. The existing
park case failed 0/1 under `/tmp/cgem-baseline-park`: the forwarded precise
`b` command got a MountSim rejection, `tc_goto_azalt_p` returned -1 and no
axis moved by 0.02 degrees. The original serial capture is retained there.
This is an expected baseline defect reproducer, not a passed test. Existing
CGE/SE and portable results above are the preservation baseline.

Atomic CGEM plan:

1. **Complete:** Audit exact CGEM command and rate behavior against
   Celestron documentation, independent INDI command usage and the app source;
   the failed park trace is the original reference. The CGEM DX manual rates
   1–9 are 0.5×, 1×, 4×, 8×, 16×, 64×, 1°, 2°, 5°/s in its table, although
   nearby prose says rate 9 is 3°/s. The table drives simulator rate 9;
   its physical speed remains unverified.
2. **Complete:** Extend the existing CGEM-specific MountSim model to handle
   `B/b` and `Z/z` with a declared motor-axis index convention, and correct
   its documented hand-control rate table. Add raw axis-arrival and rate
   assertions to the existing opt-in NexStar test. Keep generated driver and
   other model branches untouched unless a separate defect is reproduced.
   The emulator convention is 90/90 at its simulated home, shared with CGE;
   it is not a claim about a physical CGEM DX index or park pose.
3. **Complete:** Rebuild MountSim and the NexStar test, run focused protocol
   reproducers and compare successful command/reply behavior with the baseline
   trace and independent protocol. The full final CGEM run passed 21/21
   under `/tmp/cgem-full1`; all cases used isolated app/PTY instances.
4. **Complete:** Run affected CGE/SE regressions, portable NexStar normal and
   sanitizer suites, and native MountSim tests. Update this evidence, app
   audit, driver Testing record and generated TEST_SUMMARY; review scoped diffs
   and clean test build artifacts before scoped commits. Full CGE
   regression passed 20/20; SE targeted regressions passed 4/4; portable
   NexStar passed 14/14 both normally and under ASan/UBSan; MountSim native
   passed 5/5. Driver README Testing and TEST_SUMMARY were updated.

Found CGEM defect (reproduced): MountSim's CGEM profile inherits SynScan's
`gotoAxisPosition:` rejection. The hand-controller `B/b` command is documented
by [Celestron's serial protocol](https://s3.amazonaws.com/celestron-site-support-files/support_files/1154108406_nexstarcommprot.pdf),
and [INDI's Celestron driver](https://github.com/indilib/indi/blob/master/drivers/telescope/celestrondriver.cpp)
sends it as `slew_azalt`. The observed impact is failed INDIGO park without
motor-axis arrival. The intended fix is model-local MountSim axis handling;
the same park reproducer and a direct `B/b` to `Z/z` arrival test will prove it.
The physical CGEM DX encoder index, actual mechanical home/park pose and sky-to-mechanical
axis mapping are unverified and will be identified as assumptions.

Focused post-fix evidence: app build and driver-backed test build passed.
Raw `B/b` to `Z/z` roundtrip, intermediate motion, `L` arrival and rate
1/2 passed 1/1 under `/tmp/cgem-raw-fixed` (DEC motor 0.005275° and
0.010547° over 2.5 s). CGEM rate 3/7 passed 1/1 under
`/tmp/cgem-high-fixed` (0.020256°/1.2 s and 0.712569°/0.7 s).
The original failed park case passed 1/1 under `/tmp/cgem-park-fixed`,
including real coordinate change and completion before tracking off.
The full CGEM scope passed **21/21** under `/tmp/cgem-full1`: identity,
exact model and capability inventory, raw axis protocol, SYNC and reachable
sky GOTO, manual directions and rates, tracking/site/time, ST4 rate readback,
park completion/signed southern target/abort/loss, GPS and shared lifetime,
guider replacement/independent axes, and idle/active transport loss.
The guider case observed all 96 software ON/OFF command-edge pairs after 24
warm-ups at 20, 100 and 500 ms in four directions with tracking off/on and
coordinate polling. Across its 24 duration/direction/workload cells, the
minimum signed error was +0.411 ms, mean signed error +7.766 ms and maximum
absolute error 23.380 ms. This measures forwarded serial command intervals,
not physical ST4 pulse or optical correction timing.

The affected CGE run passed **20/20** under `/tmp/cge-after-cgem` after the
CGEM change. SE's raw axis, documented rates, park arrival and tracking/site
cases passed **4/4** under `/tmp/se-after-cgem-*`; its previous full 20/20
acceptance remains recorded separately. From the `indigo_test` directory,
`build/integration/test_mount_nexstar_simulator` passed **14/14**, and
`build/integration/test_mount_nexstar_simulator_asan` passed **14/14** with
no reported ASan/UBSan error. `python3 tests/run_native.py --derived-data
build` in MountSim passed **5/5**. `git diff --check` passed in both repos.
No generated driver source, framework property, persistent file or portable
integration case changed; driver version remains 3.0.0.35, and
MIGRATION_STATUS's 14 / 0 portable integration/hardware count remains valid.
No physical hardware, Windows runtime or x86_64 runtime was tested.

The tested CGEM app executable SHA-256 was
`3eccbb5000555a18ae91152b9ca57ab30dba2a4e3b5c3a92bc13e8442fabc660`,
the MountSim NexStar test executable was
`a7beab629bb67e2d4b093df78c89ab30a2f0477408b268f50ab9b7b4646197db`,
and the unchanged NexStar driver archive was
`66084adeb10d088b5b8f3093bb173ad1082d257c0c442edb32200efbf15c2dad`.
These binaries were built before `/tmp/cgem-full1`; subsequent edits were
documentation only.

Final CGEM campaign test summary: simulated **83 run, 82 passed**, including
the one deliberately preserved failing baseline park reproducer. Of those,
CGEM MountSim acceptance was **21/21**, affected CGE **20/20**, targeted SE
**4/4**, portable normal **14/14**, portable sanitizer **14/14** and MountSim
native **5/5**. Hardware **0 run, 0 passed**. The CGEM run validates
simulated protocol and mechanical-axis behavior; real encoder indexing,
park pose, physical motion speeds (especially the conflicting rate 9),
southern tracking mechanics and guider output remain hardware-only gaps.
