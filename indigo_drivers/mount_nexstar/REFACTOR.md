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

- The original migration used simulator validation without hardware. The 2026-09-24 interactive campaign below adds physical NexStar SE / NexStar+ 5.35 acceptance in EQ mode on a wedge; electrical guide-pulse accuracy remains outside its scope.

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

## Advanced VX MountSim 2.3 acceptance (2026-09-24)

The requested mode is a non-interactive simulator run of the exact `AVX`
profile (Celestron model byte 20). No physical AVX is available; hardware
validation is 0 run, 0 passed. The existing NexStar driver is 3.0.0.35.
Both the production driver and MountSim Debug app built before changes. The
baseline isolated `nexstar_park_completes_before_tracking_off` case failed 0/1
in `/tmp/avx-baseline-park`: the driver forwarded precise mechanical `b`,
MountSim rejected it, `tc_goto_azalt_p` returned -1, and no park-axis motion
followed. The original trace contains `b75555554,18E38E38` at monotonic
319184.283925; the trace and test log are retained there. The failed
`--terminator none` launcher spelling was a pre-launch argument parse error;
omitting the option selects binary raw capture as documented in the launcher.

Atomic AVX plan, before production edits:

1. Audit the official Celestron serial protocol and Advanced VX manual against
   the original trace, existing driver and independent INDI command encoding.
   Treat motor-axis index and physical home pose as unverified; declare the
   simulator convention used by a model-local implementation.
2. Give only AVX model 20 raw `B`/`b` motion and `Z`/`z` readback in a consistent
   mechanical frame with timed arrival, preserving signed southern park DEC.
   Correct only its slew-rate table to the manufacturer's AVX HC rates, then
   assert low and high indexed displacement through the unmodified PTY relay.
3. Examine fixed `P` guide commands separately from the ST4 `0x46/0x47` rate
   setting. The serial document maps fixed indices to HC speeds, while the AVX
   manual lists indices 1/2 as 2x/4x sidereal. The driver presently advertises
   those indices as 50%/100%; measure MountSim DEC motion and record that as a
   software observation, with physical speed and tracking behavior left open.
   Preserve the driver's deliberate native-pulse disable unless a separate
   compatible replacement is demonstrated.
4. Extend the existing exact-model suite to exercise raw axis/rate behavior
   and AVX ST4 readback. Rerun the complete AVX mount and guider scope after
   all fixes, including 20/100/500 ms guiding in four directions under idle
   and active workloads, transport loss, abort and park. Then run affected
   previous-model, portable and MountSim native regressions; record outcomes
   and limits here and in the driver Testing record, regenerate TEST_SUMMARY,
   review diffs and make separate INDIGO/MountSim commits without pushing.

The official [NexStar serial protocol](https://s3.amazonaws.com/celestron-site-support-files/support_files/1154108406_nexstarcommprot.pdf)
defines `B`/`b` and `Z`/`z` as revolution fractions, fixed `P` indices as HC
rates and index zero as stop. The official [Advanced VX manual](https://celestron-site-support-files.s3.us-east-1.amazonaws.com/support_files/Advanced%20VX%20Telescope%20Series_Manual_5lang_2021.pdf)
lists HC rates 1–9 as 2x, 4x, 8x, 16x, 32x sidereal, then 0.3, 1, 2 and
4 degrees/second. This supports a model-specific rate simulation; it does not
establish a measured physical AVX motor speed or mechanical zero index.

The original `b` failure is attributable to MountSim's AVX profile inheriting
a sky-coordinate handler for a mechanical-axis park request. The model-local
AVX correction uses the simulator's declared home convention of both raw axes
at 90°; it does not infer an AVX hardware encoder index or alter the driver's
RA mapping. Its focused raw `B`/`b`→`Z`/`z` timed arrival passed 1/1, and the
previously failing park case passed 1/1 after the change. The raw positive DEC
`P` rate measurements over 2.5 s were 0.02099° at index 1 and 0.04201° at
index 2; further 0.7 s index 7 and 0.5 s index 9 measurements were 0.72164°
and 2.09368°. These are simulated motor displacements, consistent with the
AVX manual's nominal 2x/4x and 1°/s/4°/s rates. The original generic table
would have produced a different result. The simulator test asserts broad
physical-unit bounds to detect wrong indexed profiles without using it as an
angular-precision benchmark.

The guide source audit also checked independent [INDI Celestron code](https://github.com/indilib/indi/blob/master/drivers/telescope/celestrondriver.cpp):
its fixed motion passes `rate + 1` unchanged to the MC, whereas autoguide-rate
configuration sends `0x46`. The official serial protocol says fixed indices
1/2 mimic the HC rates and may coexist with equatorial tracking; it does not
give a measured AVX correction curve. INDIGO's legacy `COMMAND_GUIDE_RATE`
`GUIDE_50`/`GUIDE_100` names remain for client compatibility, but AVX's
visible labels and success text now describe fixed HC index 1/2 and nominal
2x/4x, avoiding the false 50%/100% claim. No fixed-command encoding, pulse
duration or native-pulse capability changed. Hardware measurement of actual
AVX pulse motion and tracking behavior remains necessary before claiming
physical guide correction rates.

Final AVX verification used the rebuilt driver 3.0.0.36 and MountSim 2.3
selected as exact `AVX` by the control server. The complete isolated suite
passed **21/21** under `/tmp/avx-full1`; cases cover identity and reconnect,
raw mechanical axes and indexed rates, SYNC, reachable and already-target
GOTO, abort/BUSY recovery, four-direction manual motion and stop, tracking,
site and clock, ST4 write/readback, park arrival and signed southern default,
GPS/shared lifetime, guider directions/replacement and sibling connection,
idle/active/pending-park/pending-guide transport loss, and guide timing.
The guide case measured **96/96** complete forwarded ON/OFF intervals after
24 warmups: 20/100/500 ms in EAST/WEST/NORTH/SOUTH, four retained repetitions
per combination, both tracking-off with coordinate polling and tracking-on
with coordinate polling. Signed software transport error ranged from +0.382
to +21.166 ms with mean +5.250 ms. These endpoints are receipt of complete
direction ON/OFF frames at the transparent PTY relay; the values include host
scheduling and are neither physical relay timing nor motor/sky precision.
The separate direction case verified actual simulated sky motion in all four
directions, replacement, both axes and post-pulse completion.

After the AVX change, focused raw mechanical-axis preservation checks passed
for CGE **1/1** (`/tmp/cge-after-avx-axis`), CGEM **1/1**
(`/tmp/cgem-after-avx-axis`) and SE **1/1** (`/tmp/se-after-avx-axis`). The
portable NexStar suite passed **14/14** normally and **14/14** under
ASan/UBSan, with no sanitizer diagnostic. MountSim native tests passed
**5/5**. Physical AVX hardware tests were **0/0**. The AVX simulator cannot
establish an actual encoder index, physical signed park pose, mechanical
tracking response, pulse angular rates, electrical serial behavior, or
firmware-variant compatibility. `MIGRATION_STATUS.md` remains at its portable
14 / hardware 0 count because MountSim cases are opt-in macOS tests.

## CGX MountSim 2.3 acceptance (2026-09-24)

The requested run used the exact `CGX` control-server profile, Celestron model
byte 23, in non-interactive simulator mode. No CGX hardware was available, so
the hardware-test decision was zero physical runs. The original MountSim
profile handled raw `Z`/`z` as geographic Alt/Az and did not implement raw
`B`/`b` mechanical-axis GOTO. The existing driver's park path sends `b` and
expects `L` completion, making CGX park and signed southern park untestable
against that profile. Its rate table also assigned 0.5x/1x to fixed indices
1/2 and 3 degrees/second to index 9.

The atomic CGX work was: (1) audit the original driver, wire commands,
MountSim profile and official protocol/model documentation; (2) add CGX to
the existing 90°/90° simulated mechanical-axis frame and rebase, with `B`/`b`
timed motion and `Z`/`z` readback; (3) correct its fixed slew-rate table;
(4) extend the existing CGX case gates and share the AVX/CGX rate assertion;
(5) label fixed HC guide indices accurately for CGX, regenerate the driver,
and run the complete model, portable, native and affected-model suites.
Each step passed its verification. No new persistent file or Xcode project
entry was needed. The `.driver` remains the source of truth and generated
C/header/main files were regenerated with the unchanged generator.

The [official NexStar protocol](https://s3.amazonaws.com/celestron-site-support-files/support_files/1154108406_nexstarcommprot.pdf)
defines `B`/`b` and `Z`/`z` revolution fractions and says fixed `P` indices
correspond to hand-control speeds. The [CGX manual](https://celestron-site-support-files.s3.amazonaws.com/support_files/91530_CGX_EQ%20Mount%20and%20Tripod_Manual_5lang_Web.pdf),
English page 19, lists rates 1/2 as 2x/4x sidereal and rate 9 as 4 degrees/s;
its separate autoguide-rate setting is a percentage. Independent
[INDI Celestron code](https://github.com/indilib/indi/blob/master/drivers/telescope/celestrondriver.cpp)
forwards fixed rate indices to MC motion and handles `0x46` autoguide-rate
configuration separately. These sources support the simulator's index table
and honest property labels, but give no measured physical CGX guide
correction or encoder zero.

Found defects and regression evidence:

- **Reproduced simulator behavior:** before correction, raw `z` did not give
  the mount-axis index needed by `b` parking, and `b` was not a mechanical
  motion handler. The CGX-specific simulator branch now uses the same declared
  90°/90° simulated index as CGE/CGEM/AVX. The raw axis case passed, the driver
  park completed only after motion, and the signed southern default emitted
  270° DEC on `b` and reached 270° on subsequent `z` readback.
- **Simulator source-audit defect:** the CGX fixed rate table conflicted with
  the CGX manual. The corrected raw positive DEC rate 1/2/7/9 measurements
  were 0.02099° over 2.5 s, 0.04201° over 2.5 s, 0.71836° over 0.7 s and
  2.08318° over 0.5 s. The shared AVX/CGX documented-rate case now pins those
  model-specific bounds; these are simulated motor displacements.
- **Driver source-audit defect:** `COMMAND_GUIDE_RATE` retained client item
  names `GUIDE_50`/`GUIDE_100` but labeled the fixed `P` index 1/2 CGX
  fallback as 50%/100% sidereal, even though the CGX manual describes HC
  indices 1/2 as nominal 2x/4x. Driver 3.0.0.37 now applies the AVX style
  nominal labels and unverified-physical-correction message to CGX. The
  guider-first shared-lifetime case checks both labels; the command encoding
  and disabled native-pulse capability did not change.

The final full CGX run passed **21/21** under `/tmp/cgx-full`. It covered
protocol/readback, model/firmware, SYNC/GOTO arrival, manual motion, rate and
tracking properties, site/clock, ST4 readback, mechanical and signed parks,
abort and BUSY recovery, GPS/shared lifetime, guider directions/replacement,
reconnect and transport loss during idle, active, park and guide operations.
The timed-guide case recorded **96/96** complete ON/OFF intervals plus 24
warmups: 20, 100 and 500 ms in each of four directions, four retained samples
per cell, under tracking-off and tracking-on coordinate-polling workloads.
Mean signed error across the 24 cells was +6.221 ms; cell means ranged from
+2.172 to +16.375 ms and the maximum absolute sample error was 20.305 ms.
These are transparent PTY relay command-edge timings, including host scheduling.

After the CGX change, portable NexStar integration passed **14/14** normally
and **14/14** under ASan/UBSan without sanitizer diagnostics. MountSim native
motor, geometry, serial and sky-refresh tests passed **5/5**; its control and
protocol audit suite passed, including CGX identity. Focused preservation
cases passed for CGE mechanical axes, CGEM mechanical axes and higher rates,
AVX documented rates, and SE mechanical axes (**5/5**). `git diff --check`
passed. The 21 CGX and 5 focused other-model runs used the launcher's
exclusive `/tmp/indigo-mountsim-501.lock`, fresh app instances and raw
binary capture with no terminator option.

CGX simulator acceptance cannot establish a physical encoder index, signed
park pose, motor rate, pulse angular correction, tracking coexistence on
hardware, electrical serial behavior or firmware-variant compatibility.
The platform run was macOS arm64; Linux and Windows were not run for CGX.
`MIGRATION_STATUS.md` retains portable integration 14 and hardware 0.

## Final CGX Test Summary

- Simulated functional executions: **59 run, 59 passed** (CGX MountSim 21,
  portable normal 14, portable ASan/UBSan 14, MountSim native 5 and focused
  previous-model cases 5). The separate MountSim control/audit suite passed.
- Guiding timing: **96 measured ON/OFF intervals**, plus 24 warmups, across
  two workloads; software transport timing only.
- Hardware tests: **0 run, 0 passed**.

## SynScan V4 hand-controller MountSim acceptance (2026-09-24)

The selected MountSim profile is exact `SynScan`: Sky-Watcher HEQ5 Series,
model byte 1, hand-controller firmware 4.38.06. It uses the binary SynScan
hand-controller protocol through `mount_nexstar`; it is distinct from EQDIR
motor-controller control through `mount_synscan`. The manufacturer
[SynScan serial protocol 3.3](https://inter-static.skywatcher.com/downloads/synscanserialcommunicationprotocol_version33.pdf)
explicitly covers firmware 4.38.06, `B/b` GOTO and `Z/z` readback,
upper-24-bit precise positions, `T3` PEC, fixed `P` rates and no serial
ST4-rate readback command. The manufacturer
[V4 hand-control manual](https://inter-static.skywatcher.com/downloads/Synscan_V4_Hand_Control_Manual_SSHCV4-F-161208V1-EN.pdf)
gives rates 1 and 2 as nominal 1× and 8× sidereal field drift in tracking
mode. Neither document establishes this simulator's encoder index or a
physical guide correction amplitude.

The unmodified driver-backed MountSim baseline failed park: `b` returned a
simulator rejection and `tc_goto_azalt_p` failed. The first full 17-case run
passed 9/17; remaining failures identified simulator SYNC readback, GOTO
cancellation, fixed-rate RA direction, H/h controller time, signed park
motion, guider direction and park-loss recovery. The simulator now uses a
declared 90°/90° home axis frame with matching B/b and Z/z, a controller
coordinate offset for SYNC, an advancing controller clock, distinct SynScan
PEC tracking mode, and documented fixed-rate directions and rate table.
The B/b axis path preserves the nearest equivalent revolution before the
timed motor GOTO. Raw `b`/`z` and `B`/`Z` assertions require intermediate
motion, final arrival and rate-1/2 displacement. The southern park case
accepts motion on either axis because the simulated DEC can already equal
the requested signed pole; it still checks the signed `b` target and
independent final `z` readback.

The driver source `.driver` is version 38 and generated output is synchronized.
It suppresses the unsupported SynScan `CAN_GET_SET_GUIDE_RATE` capability so
the standard `MOUNT_GUIDE_RATE` ST4 property is absent without a failed 0x47
probe. Its existing fixed-rate `COMMAND_GUIDE_RATE` guider path remains
available; SynScan labels identify HC rate 1 and 2 with the manufacturer's
nominal drift values and expressly leave physical correction unverified.
The test checks those labels, property visibility, and raw `T2` then `T3/t=3`
PEC readback separately from Celestron's `T3` EQ South meaning. No GPS
logical device is expected on SynScan.

The full SynScan case registry maps to required behavior as follows:

| Scenario | Driver-backed cases |
| --- | --- |
| Identity, capability visibility, reconnect and mechanical axis protocol | `nexstar_model_identity_and_reconnect`, `nexstar_mechanical_axis_index_and_timed_motion` |
| SYNC, reachable/already-target GOTO, BUSY conflict and abort | `nexstar_sync_has_fresh_device_readback`, `nexstar_goto_arrives_and_handles_already_target`, `nexstar_abort_and_busy_conflict_recover` |
| Manual axes/rates, tracking and PEC, site and controller clock | `nexstar_manual_directions_rates_and_axis_stop`, `nexstar_tracking_and_site_roundtrip`, `nexstar_clock_write_advances_and_reconnects` |
| Park, signed southern target, abort and loss | `nexstar_park_completes_before_tracking_off`, `nexstar_southern_default_park_keeps_signed_pole`, `nexstar_park_abort_allows_new_motion`, `nexstar_pending_park_loss_recovers` |
| Guider directions, replacement, independent axes and shared lifetime | `nexstar_guider_directions_replacement_and_independent_axes`, `nexstar_guider_first_and_sibling_survival` |
| Idle, active and pending-guide transport loss | `nexstar_idle_transport_loss_recovers`, `nexstar_active_transport_loss_recovers`, `nexstar_pending_guide_loss_recovers` |
| Four-direction 20/100/500 ms pulse timing, warmups and repeats with tracking off/on | `nexstar_guider_transport_edge_timing` |

The manufacturer documents no compatible GPS commands, serial ST4-rate
readback, home command or homing status for this HC model, so those class
cases are not applicable. Protocol failure injection and malformed-reply
recovery remain in the portable NexStar suite. No physical HEQ5, Windows
runtime, actual park pose, guiding amplitude, tracking accuracy or electrical
ST4 signal was verified by MountSim.

The final exact-profile MountSim rerun passed **18/18** cases. The portable
NexStar protocol suite passed **14/14** in a normal build and **14/14** with
ASan/UBSan without diagnostics. MountSim's native suite passed **5/5**.
Focused identity/reconnect, park-completion and guider-first regressions passed
for each previously accepted CGE, SE, CGEM, AVX and CGX profile (**15/15**).
Those profiles retain their prior full-run records; SynScan's nearest-axis
motor method is additive and their command paths were not changed.

The guider timing case observed **96 ON/OFF transport intervals** after 24
warmups, covering four directions at 20, 100 and 500 ms under tracking-off
polling and tracking-on GOTO workloads. All timing assertions passed; the
largest observed absolute software wire-edge error was 21.807 ms. These
measurements verify emitted serial timing, not physical mount motion or ST4
electrical output. The functional campaign totals **66/66** executions
(SynScan 18, portable normal 14, portable ASan/UBSan 14, MountSim native 5,
focused prior-model 15). The separate MountSim control/audit suite also passed,
including SynScan audit identity and 20 interrupted binary parser/model
replacement cycles.


## Interactive NexStar SE / NexStar+ hardware acceptance (2026-09-24, in progress)

- Requested setup: physical NexStar SE with NexStar+ HC, **EQ mode on a wedge**.
  Interactive fixes require approval. Physical USB unplug/replug is excluded by
  the user; physical hot-plug coverage is not established.
- macOS arm64, driver 3.0.0.38; serial discovery found a Prolific 067b:2303
  USB-Serial Controller D. No other process held the selected port.
- Direct read-only protocol identification returned `V = 05 23 23`,
  `v = 11 23`, `m = 0b 23`, aligned `J = 01 23`, and tracking `t = 01 23`.
  The controller initially reported Alt/Az tracking despite the requested
  wedge test. No tracking write or commanded motion has been performed yet.
- Built the existing production driver with
  `make -C indigo_drivers/mount_nexstar -f ../../Makefile.drv all`.
  The new opt-in `build/hardware/test_mount_nexstar_hw` target compiles with
  strict warnings for arm64 and x86_64; only arm64 hardware execution is claimed.
- Added a first-stage public-bus identity/readback test with private configuration,
  real driver INIT/SHUTDOWN, bounded connection/disconnection and a libnexstar
  protocol log. This is the first stage, not a complete hardware acceptance suite.
  It failed reproducibly: `TRACKING_MODE` is absent on this SE, although the model
  string is correct. Coordinates, location, UTC and tracking readback reach OK.
  Diagnostic-only repeats confirm the same failure.
- Working diagnostic capture: `/tmp/nexstar-se-hw-identity.log` (temporary).
  All test processes exited and released the serial connection.

### Found defect HW-SE-01: binary firmware revision mistaken for terminator

The physical NexStar+ reports version 5.35 as `05 23 23`. The vendored
`_read_telescope(..., fl=1)` stops at the first `23`. Both `guess_mount_vendor()`
and `tc_get_version()` use that variable-length reader for `V`. The remaining
terminator shifts subsequent replies. The captured exchange includes:

```text
write 56 -> read 05 23
write 56 -> read 23
write 56 -> read 05 23
write 76 -> read 23
write 6d -> read 11 23
write 6d -> read 0b 23
```

Capability discovery consumes HC type 0x11 as model 17, classifies the SE as a
true equatorial mount, hides TRACKING_MODE, probes unsupported side-of-pier and
uses shifted replies as guide rates. A wedge does not change the SE model ID or
remove its tracking-mode selector, so the defect also blocks reliable EQ testing.

Proposed fix, **pending explicit user approval**: a length-aware version-reply
reader shared by vendor and firmware detection in
`externals/libnexstar/src/nexstar.c`, accepting all three Celestron bytes even
when a payload byte equals `#`, and all seven bytes for SynScan. Preserve
fixed-length and other variable-reply protocol behavior. Because this is
vendored code, request a specific exception to the root hygiene rule. Add a
selectable 5.35 simulator fixture and assert SE capability/mode/firmware identity,
then bump the driver version and regenerate its outputs. No production or
simulator fix has been made.

### Remaining atomic plan

1. **Complete (failure reproduced):** identify the controller and establish the
   original-driver hardware identity baseline.
2. **Pending approval:** reproduce HW-SE-01 in the portable simulator suite,
   implement the approved repair, rebuild and rerun the complete NexStar portable
   suite and hardware identity stage.
3. **Pending:** extend physical acceptance for the actual wedge setup: explicit EQ
   selection, supported property contract, small reachable GOTO/SYNC and fresh
   readback, manual directions/rates/independent stops, tracking/abort/recovery,
   nearby park/unpark and restoration of settings. Gate GPS and ST4 tests on
   actual hardware support; do not invent home/PEC support.
4. **Pending:** exercise guider directions, replacement, concurrent axes,
   shared-device connection orders, pending-pulse disconnect and reconnect,
   and 20/100/500 ms timing under idle and mount workload. Distinguish transport
   timing from physical motor/electrical timing.
5. **Pending:** record full scope and results in Testing and TEST_SUMMARY,
   reconcile migration counts, clean artifacts and commit this driver's run
   without pushing. Physical unplug/replug remains excluded.

### Current campaign test summary

- Portable simulator tests: **0 run, 0 passed** in this hardware campaign so far.
- Hardware identity stage: **1 unique case, 0 passed**, reproduced in three
  diagnostic executions. Remaining physical acceptance has not run.
- Hardware motion or guide timing samples: **0**. Campaign remains incomplete
  pending approval of HW-SE-01; this is not a passed hardware run.


HW-SE-01 update: the user explicitly approved the vendored-library repair.
The portable baseline passed all 14 existing cases and failed the new
`nexstar_binary_firmware_hash_keeps_se_capabilities` case, reproducing the physical
5.35 binary reply without changing default simulator firmware. The approved
length-aware reader is now shared by vendor/version detection; all other reply
readers remain unchanged. Driver version increments from 38 to 39.


### HW-SE-01 verification and second hardware stage

The rebuilt libnexstar and regenerated driver 3.0.0.39 passed **15/15** portable
cases, including the new 5.35 reproducer. The hardware identity stage passed;
its protocol capture now shows complete `version read 05 23 23` frames and the
SE exposes TRACKING_MODE without the false side-of-pier probe.

The initial eight-case wedge stage completed and reported eight assertion
passes, including EQ selection/reconnect, SYNC, small GOTO/arrival, all manual
directions, simultaneous-axis stop/abort, GOTO abort/recovery, guide directions,
replacement/independent axes and both shared connection orders. Arrival errors
were 0.0012–0.0212 degrees; manual displacements were 0.078–0.222 degrees.
**This is not an eight-case acceptance pass:** protocol inspection independently
found HW-SE-02 below in the site case, whose readback oracle accepted stale data.
The stage is therefore assessed as **8 run, 7 passed, 1 failed**.

### Found defect HW-SE-02: partial site write resets the other coordinate

The HC reported 48 degrees 09 minutes north, 17 degrees 07 minutes east.
A public request writing only the same latitude sent `57 30 09 00 00 00 00 00 00`,
resetting longitude to zero. Subsequent `w` reads confirmed the zero longitude.
Polling updates numeric `value` fields but leaves `target` fields at their
initial defaults; the generated target-copy handler preserves the untouched
zero target, and `nexstar_set_location()` serializes both targets.

Proposed repair (approval requested): synchronize idle site targets with valid
controller readback while preserving any pending property request. Regress
latitude-only and longitude-only changes against fresh device readback, including
initial controller values. Strengthen the hardware oracle to wait beyond the
command update; the original oracle accepted cached longitude before the request
completed. No production fix for HW-SE-02 has been made yet.

After all owned test processes exited, sent abort and both axis stop commands,
restored the original site with `57 30 09 00 00 11 07 00 00`, and verified
`w = 30 09 00 00 11 07 00 00 23`, `t = 02 23` (north EQ), `L = 30 23` (idle).
The mount is left in the user's requested EQ mode and the serial port is closed.

### Updated campaign summary (incomplete)

- Portable simulator: baseline **15 run / 14 passed**; repaired **15 / 15**.
- Physical identity before repair: **3 executions / 0 passed**; after repair:
  **1 / 1**. Wedge stage: **8 / 7**, including the protocol-discovered site defect.
- Overall physical executions so far: **12 / 8**, not full acceptance.
- Park/unpark, expanded time/options checks and guide timing remain pending.
- Physical USB unplug/replug excluded. No physical pulse timing claim.
- Awaiting interactive approval for HW-SE-02 before the next production edit.


HW-SE-02 update: user approved the repair. The corrected portable reproducer
fails on 3.0.0.39 because longitude changes after the latitude-only request.
Unchanged site properties are coalesced by the bus, so the oracle waits for two
actual `w` transactions before inspecting the cached result, rather than waiting
for an update that need not be emitted. Hardware uses the same fresh-transaction
principle. The fix synchronizes idle targets and keeps BUSY state/targets intact
while a queued site write is pending; failed writes restore targets from the last
accepted values. Version increments to 3.0.0.40.


### HW-SE-02 verification and park finding HW-SE-03

Driver 3.0.0.40 passed **16/16** portable tests, including both partial site
writes and a request pending during a dropped `w` read. The strengthened physical
site case passed with unchanged 48 degrees 09 minutes N / 17 degrees 07 minutes E.
The expanded physical stage ran **10 cases / 9 passed**; all motion and guider
functional cases and the transport timing matrix passed, but current park failed
its pre-motion coordinate comparison. GPS presence and rejected-port/driver
reinitialization stages have been added but have not yet run.

HW-SE-03: inherited MOUNT_PARK_SET.CURRENT derived park values from sky RA/DEC.
The resulting encoded park axes were 192.035321 / 28.355670 degrees, whereas a
fresh `z` query from the physical HC gave 208.960369 / 68.238359 degrees.
The SE wedge's axis frame is not interchangeable with this sky-coordinate
conversion. The test rejected the discrepancy **before sending any `b` park
command**, avoiding a large unintended movement.

Proposed repair (approval requested): handle CURRENT in the NexStar driver by
reading precise axes with `z` and storing the inverse of the driver's existing
park `b` encoding. Reject failed/invalid axis readback without replacing stored
park settings. Model distinct sky and mechanical axes in the selectable simulator
fixture and verify that CURRENT followed by park returns to those same axes.

Guide timing: 96 measured samples plus 24 discarded warm-ups across EAST/WEST/
NORTH/SOUTH, 20/100/500 ms, tracking off/on with normal coordinate polling. The
endpoints are libnexstar debug callbacks immediately after successful host writes
of nonzero fixed-rate ON and zero-rate OFF commands, timed with
`indigo_monotonic_time()`. Reported delays include HC acknowledgement and host
scheduling; they are **not physical motor or relay measurements**. Per-cell signed
error statistics and absolute maxima are in `/tmp/nexstar-se-hw-final.log`.

Cleanup sent abort/axis stops and restored the original site, then disconnected
all devices and shut down the driver. The mount remains in north EQ tracking.
No physical USB hot-plug was performed. Campaign remains incomplete at HW-SE-03.


HW-SE-03 repair explicitly approved by the user. The simulator now has optional
constant axis/sky offsets, applied inversely to `z`/`b` and `Z`/`B`, with unchanged
defaults. This fixture represents distinct coordinate frames, not full mount
kinematics. The physical mechanical values 208.960369 / 68.238359 degrees are
used as its initial axes. The NexStar CURRENT handler reads and validates all 18
reply bytes, stores inverse park encoding, and retains the old position on failed
readback. Default-pole behavior remains the inherited numeric convention.
Version increments to 3.0.0.41; no generator implementation changed.


The new park regression was checked against a temporary reconstruction of the
old behavior: the generated driver's new MOUNT_PARK_SET dispatch alone was
bypassed so requests again reached the inherited handler; the library and other
repairs were unchanged. `nexstar_current_park_uses_mechanical_axes` failed its
mechanical-RA assertion as expected. This is an isolated regression check, not a
claim that the full old driver was rerun. An earlier park-baseline command had a
simulator compilation error and then executed the existing 16-case binary;
that output is not evidence for the new park regression. The simulator's missing
math include was corrected before the actual 17-case suite was built.


## Completed physical acceptance, 2026-09-24 21:14

The final physical run passed **12/12** on Celestron NexStar 4/5 SE, NexStar+
5.35, north EQ tracking on a wedge, macOS arm64, driver **3.0.0.41**. The last
harness correction used `/dev/null` as an existing invalid serial endpoint:
a nonexistent path is rejected by framework DEVICE_PORT validation before it
reaches the driver, so expecting DEVICE_PORT OK for that earlier fixture was a
test error. No production change was made for it; the full 12-case HW suite was
repeated and passed.

Commands:

```sh
make -C indigo_drivers/mount_nexstar -f ../../Makefile.drv all
make -C indigo_test build/hardware/test_mount_nexstar_hw
MOUNT_NEXSTAR_HW_PORT=<selected-port> make -C indigo_test test-mount-nexstar-hw
```

The actual recorded run called the same built executable with `--run` directly.
The test is opt-in, isolated from simulator/default targets, stops at its first
failure and uses private INDIGO configuration. The new source is registered in
the Xcode hardware-test group. Both NexStar test targets depend on the linked
libnexstar archive as well as the driver archive.

### Physical scenario mapping

| Case | Evidence / result |
| --- | --- |
| `nexstar_identity_and_readback` | Correct SE model, HC 5.35, mount interface, fresh coordinates, site/time, tracking-mode visibility. |
| `nexstar_wedge_mode_and_property_contract` | Available and absent controls, explicit EQ selection, reconnect and tracking state. |
| `nexstar_site_clock_and_tracking` | Separate latitude/longitude writes retain the omitted value in subsequent HC reads; existing UTC written back; tracking off/on. |
| `nexstar_sync_and_small_goto` | Current-pointing SYNC, small relative GOTO, return and already-at-target GOTO, fresh arrival coordinates. |
| `nexstar_manual_rates_directions_and_abort` | Four slew-rate selections, all four directions with actual coordinate movement, simultaneous axes and independent stop, abort, fresh command. |
| `nexstar_goto_abort_and_recovery` | Abort a BUSY GOTO and accept a fresh small target. |
| `nexstar_guider_directions_replacement_and_axes` | Both command-guide rates; all directions; BUSY/OK and zero reset; zero, reversal/replacement, independent concurrent axes. |
| `nexstar_shared_connection_orders_and_pending_disconnect` | Mount-first/guider-first, surviving sibling, disconnect during pulse, reconnect, repeated disconnect and fresh operation. |
| `nexstar_guider_transport_timing` | 96 measured samples and 24 warm-ups, four directions, 20/100/500 ms, tracking off/on and normal polling. |
| `nexstar_current_park_and_unpark` | CURRENT encoded 211.278999 / 68.337407 degrees, HC axes 211.279364 / 68.337407; park arrival 211.294663 / 68.349466; tracking off, unpark and EQ tracking recovery. |
| `nexstar_gps_presence_and_sibling_survival` | No GPS accessory detected; connection refused with ALERT, mount polling remains healthy. Satellite acquisition is not applicable to this setup. |
| `nexstar_refused_port_and_driver_reinitialization` | Driver rejects invalid serial endpoint, reconnects and polls, completes SHUTDOWN/INIT and connects/polls again. This is not dynamic-library unload/reload. |

### Hardware timing statistics

Endpoints are the libnexstar post-write callbacks for a direction's fixed-rate
ON and axis OFF commands, using a monotonic clock. Statistics below are signed
actual-minus-requested errors in ms. They include HC acknowledgement and host
scheduling, and are not physical motor or electrical pulse accuracy. Each cell
has 4 measured samples after 1 discarded warm-up; p95 and p99 both equal the
sample maximum at this sample size. Functional completion passed independently
of the timing values.

| Tracking | Direction | Requested ms | Mean actual ms | Min error | Mean error | Median error | p95 / p99 / max | SD | Max absolute | Mean error % |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | EAST | 20 | 53.397 | 31.455 | 33.397 | 33.553 | 35.025 | 1.274 | 35.025 | 166.983 |
| 0 | EAST | 100 | 134.200 | 33.206 | 34.200 | 34.238 | 35.116 | 0.798 | 35.116 | 34.200 |
| 0 | EAST | 500 | 534.405 | 32.404 | 34.405 | 34.989 | 35.238 | 1.160 | 35.238 | 6.881 |
| 0 | WEST | 20 | 53.153 | 31.476 | 33.153 | 33.280 | 34.574 | 1.184 | 34.574 | 165.763 |
| 0 | WEST | 100 | 143.213 | 31.946 | 43.213 | 35.050 | 70.806 | 15.984 | 70.806 | 43.213 |
| 0 | WEST | 500 | 532.852 | 30.676 | 32.852 | 33.065 | 34.603 | 1.595 | 34.603 | 6.570 |
| 0 | NORTH | 20 | 53.787 | 32.116 | 33.787 | 34.010 | 35.012 | 1.143 | 35.012 | 168.934 |
| 0 | NORTH | 100 | 134.698 | 34.178 | 34.698 | 34.652 | 35.311 | 0.421 | 35.311 | 34.698 |
| 0 | NORTH | 500 | 533.814 | 33.100 | 33.814 | 33.535 | 35.088 | 0.758 | 35.088 | 6.763 |
| 0 | SOUTH | 20 | 53.887 | 31.648 | 33.887 | 33.952 | 35.995 | 1.652 | 35.995 | 169.433 |
| 0 | SOUTH | 100 | 133.258 | 31.438 | 33.258 | 33.280 | 35.032 | 1.275 | 35.032 | 33.258 |
| 0 | SOUTH | 500 | 532.770 | 30.343 | 32.770 | 32.628 | 35.479 | 1.843 | 35.479 | 6.554 |
| 1 | EAST | 20 | 55.272 | 33.917 | 35.272 | 35.593 | 35.985 | 0.801 | 35.985 | 176.359 |
| 1 | EAST | 100 | 134.808 | 34.135 | 34.808 | 34.471 | 36.156 | 0.796 | 36.156 | 34.808 |
| 1 | EAST | 500 | 534.978 | 32.729 | 34.978 | 35.421 | 36.340 | 1.393 | 36.340 | 6.996 |
| 1 | WEST | 20 | 53.359 | 32.156 | 33.359 | 32.626 | 36.027 | 1.568 | 36.027 | 166.795 |
| 1 | WEST | 100 | 135.777 | 33.671 | 35.777 | 36.136 | 37.163 | 1.297 | 37.163 | 35.777 |
| 1 | WEST | 500 | 532.028 | 30.690 | 32.028 | 31.945 | 33.532 | 1.020 | 33.532 | 6.406 |
| 1 | NORTH | 20 | 54.704 | 33.801 | 34.704 | 34.634 | 35.747 | 0.749 | 35.747 | 173.520 |
| 1 | NORTH | 100 | 133.103 | 32.530 | 33.103 | 33.057 | 33.766 | 0.528 | 33.766 | 33.103 |
| 1 | NORTH | 500 | 532.539 | 31.740 | 32.539 | 32.138 | 34.137 | 0.938 | 34.137 | 6.508 |
| 1 | SOUTH | 20 | 53.843 | 31.774 | 33.843 | 33.994 | 35.610 | 1.701 | 35.610 | 169.215 |
| 1 | SOUTH | 100 | 132.914 | 31.667 | 32.914 | 32.181 | 35.629 | 1.587 | 35.629 | 32.914 |
| 1 | SOUTH | 500 | 533.469 | 32.322 | 33.469 | 33.029 | 35.493 | 1.209 | 35.493 | 6.694 |

### Hardware scope limits and final device state

Physical USB unplug/replug was explicitly excluded. Actual cable-loss recovery
and electrical guide pulse timing were not established. Windows/Linux execution,
other NexStar models/HC versions, GPS satellite fix and multiple physical mounts
were not tested. Home, PEC and selectable tracking rates are not exposed for this
setup; ST4 guide-rate configuration is hidden by the SE capability branch. Default
pole travel was not commanded on the physical mount; nearby CURRENT park was
validated. Malformed replies/transport fault injection are covered by the
portable suite; physical failure injection is limited to the invalid endpoint.

Cleanup aborted motion, stopped both axes, restored the original observing site,
disconnected all logical devices and shut down the driver. An independent final
read confirmed `w = 30 09 00 00 11 07 00 00 23` (48d09m N, 17d07m E),
`t = 02 23` (north EQ tracking) and `L = 30 23` (no GOTO in progress).
The serial port was released. EQ tracking remains enabled as requested.


## Final verification and test summary (2026-09-24)

All three production defects found on hardware were fixed after explicit user
approval, including the specific libnexstar vendored-code exception. Each has a
portable reproducer/regression: `nexstar_binary_firmware_hash_keeps_se_capabilities`,
`nexstar_partial_site_changes_preserve_readback`, and
`nexstar_current_park_uses_mechanical_axes`. Park coverage additionally rejects
missing, short and malformed replies, preserves the saved position and recovers
on a valid subsequent request.

- Final portable functional verification: **34 executions / 34 passed**: the
  complete 17-case suite normally and the same 17 with AddressSanitizer and
  UndefinedBehaviorSanitizer (`halt_on_error=1`), with no sanitizer finding.
  Driver and test code were instrumented; the linked INDIGO/libnexstar libraries
  and the external simulator remained ordinary builds. LeakSanitizer is not
  claimed on this macOS host.
- Final physical verification: **12 cases / 12 passed** on macOS arm64, SE with
  NexStar+ 5.35 in north EQ mode on a wedge; **96 measured timing samples** plus
  **24 discarded warm-ups**. Maximum observed absolute transport-timing error
  was **70.806 ms**. No electrical/motor pulse accuracy claim.
- Including earlier diagnostic/baseline attempts, the campaign executed
  **146 simulated cases / 142 passes** (including one isolated pre-fix park
  reproducer and the explicitly invalid stale-binary baseline attempt) and
  **46 physical cases / 40 passes**. These totals retain expected defect
  reproductions and test-harness failures; acceptance is based on the final
  clean runs above, not on those earlier attempts.
- Strict `-Wall -Wextra -Werror` builds passed for the updated portable test and
  simulator and the hardware test, with the established unused/sign-compare
  suppressions. Universal arm64/x86_64 compilation succeeded; execution was
  arm64 only. Driver production build passed.
- Regeneration of `.c`, `.h`, `_main.c` produced identical SHA-256 hashes. Version
  is 3.0.0.41, higher than the pre-campaign 3.0.0.38. No generator code changed,
  no MAX_DEVICES override was added, and existing custom-property names were
  not renamed in this behavioral repair.
- Xcode project lint and `git diff --check` passed. PROPERTIES describes the
  CURRENT axis encoding; MIGRATION_STATUS records 17 / 12 and HW retesting,
  retaining the manual Windows comment. README Testing and TEST_SUMMARY contain
  the latest physical and simulator results.
- Other SynScan simulator processes were active during cleanup. To avoid
  interrupting their runs, cleanup removes only this campaign's NexStar test
  binaries/temporary sources instead of executing the repository-wide
  `make -C indigo_test test-clean`. Diagnostic logs in `/tmp/nexstar-*.log` are
  retained as evidence. No NexStar test or transport remains active.

## GPS hardware follow-up — 2026-09-24 21:25

The user attached a GPS accessory to the NexStar SE / NexStar+ 5.35 and requested detection only, indoors without requiring a satellite fix. This is a narrow interactive hardware follow-up, not a repeat of mount motion acceptance. Physical hot-plug remains excluded.

- Added `--gps` selection to the existing hardware executable; it runs only `nexstar_gps_presence_and_sibling_survival` and requires the accessory to be present. The existing full suite still supports the explicitly absent-accessory scenario.
- The connected branch checks GPS interface, firmware and a resolved no-fix/3D-fix status, disconnects GPS and verifies fresh mount coordinates. Cleanup also disconnects GPS on an assertion failure.
- Strict universal macOS build passed with `make -C indigo_test build/hardware/test_mount_nexstar_hw` (`-Wall -Wextra -Werror`). Executed `MOUNT_NEXSTAR_HW_PORT=<HC serial port> indigo_test/build/hardware/test_mount_nexstar_hw --gps`: **1/1 passed**.
- AUX destination B0 version request `50 01 b0 fe 00 00 00 02` returned `0b 01 23`: firmware **11.1**. Linked request `50 01 b0 37 00 00 00 01` returned `00 23`: **no fix**. Driver exposed this as NO_FIX ALERT; this is expected indoors and is not a connection failure.
- Protocol log: `/tmp/nexstar-gps-hw.log`. All transmitted commands were identification/status reads. No slew, sync, tracking, site or clock writes were sent. GPS and mount disconnected cleanly.
- Production code unchanged; driver remains 3.0.0.41. Registered case counts recalculated: 17 portable / 12 physical, unchanged because this selects and extends an existing case; MIGRATION_STATUS.md already has those counts.
- This closes the earlier accessory-presence hardware gap. Fix acquisition, position/time accuracy and physical hot-plug remain unverified. No production defect was found.

### Final test summary for this GPS follow-up

Simulated tests: **0 run, 0 passed** (no production change; prior 17/17 portable and 17/17 sanitizer acceptance remains separately recorded above).
Hardware tests: **1 run, 1 passed**, GPS detection/status and sibling survival only. The preceding full mount hardware acceptance remains **12/12**.
