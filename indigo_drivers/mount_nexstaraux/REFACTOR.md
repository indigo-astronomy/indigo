# INDIGO 3.0 refactoring record for `mount_nexstaraux`

This record was created with the 2026-09-20 test coverage work. The driver's earlier migration to
`indigo_generator` predates this file and is deliberately not reconstructed here; only the changes
below are recorded, so nothing in this file is inferred history.

## Defects found while writing the test suite (2026-09-20)

Five defects were found and fixed. Driver version is now `0x0300000C`.

- **One lost reply killed the connection.** `nexstaraux_command()` read the answer with
  `indigo_uni_read()`, which on a socket with a receive timeout reports the timeout through
  `read_data()`. That records the timeout in `handle->last_error`, and every later read *and write*
  on the handle then fails immediately without touching the socket. A single unanswered request
  therefore silenced the mount for the rest of the session: polling stopped, no further command was
  sent, and the device still reported itself connected. The reads now wait for data first, so a
  missing answer is a recoverable timeout. The same rewrite removed a path through the reply loop
  that neither returned nor made progress when a header byte could not be read.
- **Losing the connection crashed the driver.** `nexstaraux_validate_handle()` passed
  `device->master_device` to `indigo_execute_handler()`. The mount is its own master and that pointer
  is NULL, so a dropped socket dereferenced NULL. It now falls back to the device itself.
- **Unparking parked the mount.** `MOUNT_PARK.on_change` ran the park slew for both items, so
  selecting UNPARKED slewed to the pole and parked. Unparking now only releases the mount and
  publishes the result, which the handler has to do itself because its slew branch references a
  finalizer and the generator suppresses the final update.
- **The guider stopped the wrong axis.** `guider_guide_ra_finalizer()` stopped the altitude axis and
  published `GUIDER_GUIDE_DEC`, and `guider_guide_dec_finalizer()` did the opposite, so every pulse
  stopped the axis it had not started. Each finalizer now ends its own axis.
- **Pulses shorter than a second were not timed, and failures were never reported.** The guide
  handlers scheduled the finalizer with `duration / 1000`, an integer division, so a 300 ms pulse
  expired immediately and a 1500 ms pulse lasted one second. They also set `INDIGO_ALERT_STATE`
  without publishing it, and the generator suppresses the final update for a handler that references
  a finalizer, so a refused pulse stayed BUSY forever. Both handlers now use the fractional duration
  and publish their own start and error states.

## Automated test coverage (2026-09-20)

`indigo_test/integration/test_mount_nexstaraux_simulator.c` was extended from two smoke tests to the
full mount and guider class standards in `indigo_test/DRIVER_TESTING_RULES.md`. Every scenario runs
in its own forked process against a freshly started simulator, so no state leaks between cases.

The simulator `mount_nexstaraux_simulator/mount_nexstaraux_simulator.c` was given a real motion
model, because it previously completed every slew instantly and could not report a mount in motion:

- A slew and a rate move are both a signed speed in encoder units per second, so `MC_SLEW_DONE`
  reports a motion that really takes time and a slew can be interrupted.
- `--profile <normal|no-version|slow-slew>` selects an unresponsive motor controller or a slow mount.
- `INDIGO_NEXSTARAUX_EVENTS` records the destination, command and payload of every accepted request,
  so a test can assert the bytes the driver put on the wire.
- `INDIGO_NEXSTARAUX_FAULT` arms a one-shot `silent`, `garbage` or `close` fault on the next request
  to a given destination and command. The `garbage` action answers with a reply for a command nobody
  asked about, which the driver has to skip.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Identity and property contract | `metadata`, `property_contract` |
| Initialization failures | `handshake_rejected`, `handshake_timeout` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` |
| GOTO and SYNC | `sync_coordinates`, `slew_to_coordinates`, `slew_command_failure`, `sync_command_failure` |
| Stop and abort | `abort_slew`, `abort_while_idle`, `abort_command_failure` |
| Manual motion and rates | `manual_motion`, `manual_motion_rates`, `manual_motion_failure` |
| Tracking and rates | `tracking`, `tracking_failure`, `guide_rate` |
| Park and home | `park_and_unpark`, `abort_park` |
| Polling and stale replies | `coordinate_polling`, `stale_reply_skipped` |
| Transport loss | `transport_loss` |
| Guider | `guider_metadata`, `guider_pulses`, `guider_pulse_failure`, `guider_rate` |
| Shared connection and ownership | `shared_connection` |

### Notes and gaps

- The mount has no park position, home command or side-of-pier report of its own, so those rows of
  the class standard do not apply; their absence is asserted in `property_contract`.
- `MOUNT_HORIZONTAL_COORDINATES` is published by the base driver from the equatorial readback, so it
  is only checked for visibility.
- The network autodiscovery branch of `nexstaraux_open()` is not exercised: the test points
  `DEVICE_PORT` straight at the simulator, and a passive discovery broadcast is not hardware free.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no SkyPortal module was available.

```sh
make -C indigo_test build/integration/test_mount_nexstaraux_simulator
cd indigo_test && ./build/integration/test_mount_nexstaraux_simulator
```

- Simulated tests run: 30; passed: 30.
- Hardware tests run: 0; passed: 0.

## Overlapping guide pulses (2026-09-20)

A guide pulse requested while another pulse on the same axis was still running was silently
discarded. `GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare `accept_while_busy = true` and zero
both axis items in `on_change_request`, and each handler drops the finaliser of the pulse it
replaces; without that the superseded finaliser would stop the axis on the old deadline, in the
middle of the new pulse.

`guider_pulses` gained two duration-measuring cases: a 2000 ms pulse replaced after 500 ms by a
600 ms pulse in the same direction (1106 ms measured) and by a 300 ms pulse in the opposite
direction (805 ms), the second also confirming the `ALT_MOVE_NEG` request reaches the motor
controller.

## Hardware acceptance run (2026-09-23)

The mount is a Celestron NexStar SE with a NexStar+ hand controller, reached over a SkyPortal WiFi
module (Zentri AMW007, `ZentriOS-WL-1.2.0.10`). Both motor controllers report firmware 5.20. The
suite ran on a Raspberry Pi 5 under Linux arm64 from
`indigo_test/hardware/test_mount_nexstaraux_hw.c`, one small scenario per case, through
`make -C indigo_test test-mount-nexstaraux-hw HW_PARK=1`. The driver went from `0x0300000E` to
`0x03000012` over the session.

The suite passes 36 of 36 against the mount with driver `0x03000012`. Six defects were found on the
mount over the session and all six are reproduced hardware-free as well. Seven cases report
themselves as not exercised on this rig rather than passing or failing, each for a reason the driver
cannot control; they are listed under "Not exercised on this rig" below.

Reaching that took several passes, and what made the difference was not the driver but the way the
measurements are taken. Anything judged by an arc the axes travelled has to survive a hand
controller that writes the same velocity registers, and anything judged by a session has to survive
a WiFi module that refuses one now and then. The measurements were rebuilt accordingly: motion is
compared between two directions so a common drift cancels, the tracking windows are validated
afterwards by what the undriven axis lost, a session is retried with a growing pause, and the guide
pulse sweep leaves 300 ms between pulses because a back-to-back burst of eighty lost nineteen of
them. None of that weakens an assertion about the driver; it separates the driver from the rig.

### What the hardware suite covers

| Class standard area | Scenario |
| --- | --- |
| Discovery | `discovers_the_mount_on_the_network` |
| Identity and property contract | `reports_its_identity`, `reports_the_motor_controller_firmware`, `publishes_the_mount_property_contract`, `publishes_the_guider_property_contract` |
| Coordinates and time | `polls_the_coordinates`, `publishes_the_sidereal_time` |
| Tracking and rates | `toggles_tracking`, `holds_the_right_ascension_while_tracking`, `selects_the_tracking_rates` |
| Slew rates and manual motion | `selects_the_slew_rates`, `moves_the_dec_axis_manually`, `moves_the_ra_axis_manually`, `moves_faster_at_a_higher_rate`, `keeps_tracking_after_manual_motion` |
| Stop and abort | `aborts_manual_motion`, `aborts_a_slew_and_accepts_a_fresh_one` |
| SYNC | `syncs_to_a_nearby_position`, `reports_a_negative_declination` |
| GOTO | `slews_to_a_nearby_target`, `tracks_after_a_slew` |
| Park | `parks_and_unparks`, `aborts_a_park` (opt-in, `HW_PARK=1`) |
| Guide rates | `writes_the_mount_guide_rate`, `writes_the_extreme_guide_rates`, `writes_the_guider_rate` |
| Guider | `guides_in_all_four_directions`, `guides_both_axes_at_once`, `replaces_a_guide_pulse_on_the_same_axis`, `moves_the_dec_axis_while_guiding`, `keeps_tracking_through_a_guide_pulse`, `measures_the_guide_pulse_duration` |
| Shared connection and lifecycle | `shares_the_connection_with_the_guider`, `refuses_an_unreachable_address`, `reconnects`, `reinitializes` |

Two things the suite does that a property level check cannot. Motion is judged by the arc the axis
actually turned, and the right ascension axis is measured as an **hour angle**, because the
published right ascension follows the sky whenever the mount is not tracking and over the seconds a
scenario needs that drift is as large as the motion being measured. Tracking is judged by whether
the mount holds its right ascension over thirty seconds: an axis that is not driven loses 0.00836 h
of it, which is ten times the tolerance a tracking mount is held to.

The suite was first run end to end against the simulator over TCP, with
`MOUNT_NEXSTARAUX_HW_URL` pointing at it, as a harness shakedown before it was pointed at the
mount. That found four defects in the suite itself and one in the simulator, all fixed before the
hardware run: a `+-90` guard that rejected a mount legitimately parked at 90.0046, state light
checks that sampled before the driver published, `reinitializes` losing the configured address and
reaching for whatever mount autodiscovery found, and a simulator goto that never completed when the
short way round the encoder crossed zero.

### Defects found on the hardware

All are reproduced hardware-free by the regression cases named against each one.

- **A southern declination was reported as 270 to 360 degrees.** The AUX position is a signed
  fraction of a full rotation, and `nextstar_get_coordinates()` read it unsigned through
  `fmod(raw / 0x1000000 * 360, 360)`. The mount, synchronized to -20 degrees, reported 339.9999
  back. Regression case `southern_declination`, which also pins the boundary at the celestial
  equator and a declination next to the southern pole.
  The simulator suite had a southern declination in `sync_coordinates` all along and it passed,
  because `coordinates_are()` accepted the first publication after a change request - and that
  publication carries the values the *client* sent, not the mount's readback. The helper now waits
  for a publication the polling callback made.
- **Every axis stop killed the sidereal drive.** A motor controller has one velocity register per
  axis, shared by the tracking rate, a rate move and a goto, so the zero rate that ends a guide
  pulse, a released manual motion or an abort stops the tracking drive with it. Measured on the
  mount: after a released manual motion with tracking on it lost 0.00427 h of right ascension over
  thirty seconds against 0.00106 h while tracking; after the fix the same measurement gives
  0.00090 h, and a guide pulse leaves 0.00016 h. The driver records the rate it last commanded in
  `PRIVATE_DATA` - the guider logical device has no `MOUNT_TRACKING` of its own and cannot reach
  the mount's property macros - and `nexstaraux_stop_axis()` writes it again after every stop of
  the right ascension axis. Regression cases `tracking_survives_a_guide_pulse`,
  `tracking_survives_manual_motion` and `tracking_survives_an_abort`, each measuring the drift
  rather than the accepted command.
- **A goto the controller gave up on was published as success.** Asked for 90 degrees of
  declination from -17.3, the altitude axis ran for 22.4 s, stopped at 59.9 and answered
  `MC_SLEW_DONE` with `0xff`. The driver trusted that answer, published `MOUNT_EQUATORIAL_COORDINATES`
  as OK and `MOUNT_PARK` as parked, so a client was told the mount stood at the pole while it was
  30 degrees away. `nexstaraux_reached_target()` now compares the reached position with the target
  within one degree and the finalizer publishes ALERT, leaves `MOUNT_PARK` unparked and sets the
  state light to ALERT when the mount stopped short. Regression case `goto_stopped_short`, against
  a new simulator profile `stalling` whose axis gives up a third of the way and still reports the
  goto complete.
- **A goto that could never arrive never ended.** Asked to park, the same controller answered
  `MC_SLEW_DONE` with "not done" while both axes stood still, so the finalizer rescheduled itself
  every tenth of a second for as long as it was left to. `MOUNT_PARK` stayed BUSY for a quarter of
  an hour, and because the generated parked guard refuses every request while `PARKED` is selected,
  the mount was unusable for the rest of the run: tracking, motion and the coordinates all came
  back refused with "Mount is parked!". `nexstaraux_slew_stalled()` now watches the hour angle and
  the declination the polling callback publishes - the hour angle rather than the right ascension,
  which follows the sky on its own while the mount is not tracking - and after ten seconds without
  motion it stops the axes and lets the completion path judge the result. Regression case
  `goto_never_arrives` against a new simulator profile `never-arrives`, which measures how long the
  driver takes to give up; it gave up after 11 s.
- **The declination axis was never stopped, so the mount drifted all session.** In EQ mode only the
  azimuth axis carries the sidereal drive, and `nexstaraux_set_tracking()` only ever wrote that
  axis. The EQ setup in the protocol document stops *both* axes before it sets the rate, and it has
  to: an altitude rate left behind by a hand controller that was tracking in alt-azimuth keeps
  turning the declination axis for as long as the session lasts. Measured on the mount with nothing
  commanded and the driver's tracking switched off: the hour angle stood perfectly still while the
  declination turned 0.02 degrees every six seconds, about twelve degrees an hour. That is what
  swamped the motion measurements for most of the session - one manual motion measured +0.0121
  degrees of declination and the same one later measured -0.0029, the axis being carried the other
  way faster than the driver was turning it. `nexstaraux_set_tracking()` now stops the altitude
  axis first, as the document's own sequence does. `tracking` asserts the `ALT_SET_POS_GUIDERATE`
  stop on the wire.
- **A goto that lost an answer was abandoned while still busy.** `mount_slew_finalizer()` returned
  without rescheduling and without publishing anything when `nexstaraux_get_slew_state()` failed,
  so a single lost reply left `MOUNT_EQUATORIAL_COORDINATES` or `MOUNT_PARK` BUSY for ever, and the
  parked guard then refused tracking, motion and coordinates for the rest of the session. Seen on
  the mount when a read came back with `EAGAIN` during a park. The poll is now retried while the
  axes are still turning, the stall watch bounds the retrying, and `mount_slew_failed()` publishes
  ALERT and releases the park state when it runs out. Regression case `goto_loses_the_answer`,
  which closes the transport under a running goto.
- **A refused autoguide rate was published as the rate the mount guides at.** This controller
  acknowledges `MC_SET_AUTOGUIDE_RATE` and keeps the value it had. Confirmed on the wire
  independently of the driver: `GET` answers `0x01`, `SET 0x80` is acknowledged, `GET` answers
  `0x01` again. `nexstar_set_guide_rate_handler()` now reads the rate back and fails when it did
  not take, and both `MOUNT_GUIDE_RATE` and `GUIDER_RATE` replace the refused value with the one
  the controller is really holding before publishing ALERT. Regression case `guide_rate_not_stored`
  against a new simulator profile `deaf-guide-rate`.
- **The fastest autoguide rate wrapped round to a standing axis.** The controller stores the rate
  as a byte and the protocol document gives `value = pct / 100 * 256`, so the property maximum of
  100 percent produced 256, truncated to `0x00` by the cast. `nexstaraux_guide_rate_byte()` now
  rounds and clamps to `0xFF`, the 99.6 percent the controller can hold. Regression case
  `guide_rate_full_scale`. Found by source audit against the protocol document; this mount cannot
  demonstrate it, because it stores no autoguide rate at all.
- **The abort sent a 24 bit payload with an 8 bit command.** `nexstaraux_stop()` used
  `nexstaraux_command_24(..., MC_MOVE_POS, 0, ...)` where the document defines `MC_MOVE_POS` as
  taking an 8 bit rate. The controller tolerated the extra bytes. Found by source audit;
  `abort_slew` now expects the single zero byte.

### What the hardware taught the simulator

- **The axis velocity register is shared.** The simulator used to keep a `tracking` flag that
  nothing acted on, so a stopped tracking drive was invisible and the defect above could not be
  reproduced. `MC_SET_POS_GUIDERATE`, `MC_SET_NEG_GUIDERATE`, `MC_MOVE_POS`, `MC_MOVE_NEG` and the
  goto commands now all write one signed speed per axis, with the real sidereal, solar and lunar
  rates, so tracking is visible as motion exactly as it is on the mount.
- **`MC_SET_POSITION` does not stop the drive.** The simulator used to zero the speed on a
  synchronization; the mount keeps tracking across one.
- **The rate card.** Rate 2 of `MC_MOVE_POS` measured one times sidereal on the mount
  (0.0124 degrees of declination in three seconds), which fixes the hand controller's rate card as
  0.5, 1, 4, 8, 16, 32, 64 times sidereal followed by two fixed angular rates, rather than the
  doubling table the simulator assumed. `moves_faster_at_a_higher_rate` measures two of the rates
  against each other on the mount so the mapping stays checked: CENTERING against FIND came out at
  a factor of sixteen.
- **A goto can end short and still report done**, **a goto can stop and never report done at all**,
  and **the autoguide rate can be acknowledged and discarded**: all three are per model, so they are
  simulator profiles (`stalling`, `never-arrives`, `deaf-guide-rate`) rather than permanent
  behaviour, and the well behaved controller stays the one the other cases run against.

The simulator's goto rate is deliberately left much faster than a real mount, so the suite stays
short. That is the one place it does not model the hardware, and it costs the shakedown run the
slew abort scenario, which reports itself as not exercised there because the goto is over before it
can be interrupted.

### Observations about the rig, not the driver

- **A second client is accepted and never served.** Opening a second TCP session while one is live
  succeeds at the TCP level, but nothing on it is ever answered and the first session keeps
  working. A driver started against a mount another client already holds therefore sees exactly the
  failed handshake that `handshake_timeout` covers.
- **Rapid reconnects wedge the WiFi module.** Six connects with no gap between them left it
  broadcasting its UDP announcement on port 55555 while answering neither ARP nor TCP. It did not
  recover within twenty five minutes and needed the mount power cycled. Nothing in the driver
  reconnects by itself; the suite leaves a second between sessions for it.
- **The hand controller is a bus master of its own.** With its tracking on it writes the same axis
  velocity registers the driver does, so every arc a scenario measures is the sum of the two. It
  came and went during the session: the same manual motion measured +0.0121 degrees of declination
  in one run and -0.0029 in another, the second being the hand controller turning the axis faster
  the other way than the driver was turning it. No driver can prevent this, so
  `rig_is_quiet()` establishes before each measurement that the axes are still with nothing
  commanded, and the scenario reports itself as not exercised rather than blaming the driver. A run
  is only complete with the hand controller's tracking off.
- **The payload of the discovery announcement came back with a binary tail.** That is
  `indigo_perform_passive_discovery()` in `indigo_libs/indigo_uni_io.c`, which copied a datagram it
  never terminated; fixed separately.

### Not exercised on this rig

These are reported by the run itself, with the measurement that justifies each one.

- **The hand controller kept turning the axes.** `rig_is_quiet()` found the axes moving with nothing
  commanded before `moves_the_dec_axis_manually`, `moves_faster_at_a_higher_rate` and
  `aborts_manual_motion` (0.0021 to 0.0071 degrees in six seconds), and the validity gate rejected
  the windows of `holds_the_right_ascension_while_tracking` and
  `keeps_tracking_through_a_guide_pulse`, where the undriven axis lost 0.00685 h and 0.00441 h of
  the 0.00836 h the sky moved. The driver contract those two carry is still asserted whenever the
  bus is quiet, which it was for `keeps_tracking_after_manual_motion` and `tracks_after_a_slew` in
  the same run.
- **The pole was out of reach.** `parks_and_unparks` found the mount stopping at DEC 44.4 instead of
  90 and reported that the driver said so, which is the refusal path rather than the park workflow.
  An earlier pass in the same session did reach the pole (DEC 90.1298), refused a GOTO while parked
  and unparked cleanly, so the workflow itself is verified; it depends on where the hand controller
  left the encoders.
- **The autoguide rate is not stored.** `writes_the_extreme_guide_rates` cannot run on a controller
  that keeps its own rate, which `writes_the_mount_guide_rate` establishes first.

### Not covered

- The mount has no park position, home command, side of pier report, site or clock of its own, so
  those rows of the class standard do not apply. Their absence is asserted in
  `publishes_the_mount_property_contract` and `publishes_the_sidereal_time`.
- **Cord wrap bounds the return from the park position.** The park position is on the meridian, so
  coming back can be most of a turn of the azimuth axis, and `MC_POLL_CORDWRAP` answers `0xff` on
  this mount. The axis stops at its limit and the driver reports it; the return is therefore
  best-effort and the run says where the mount was left.
- **Whether a released manual motion physically stops** cannot be decided from the coordinates here:
  it runs at one times sidereal, slower than the drift the hand controller imposes. That the stop
  command reaches the motor controller is asserted on the wire by `abort_slew` in the simulator
  suite instead.
- The guide pulse measurement is software completion timing over the network transport, from the
  public request to the completion the driver publishes. It is not relay or motor timing and no
  electrical measurement was made.
- A controller found holding an autoguide rate below one percent of sidereal cannot be given that
  rate back, because the property starts at one percent. This mount was found at 0.39 percent and
  the run says so instead of pretending it restored it.

### Guiding pulse duration accuracy

Measured with `make -C indigo_test test-mount-nexstaraux-hw HW_PARK=1` on driver `0x03000012`,
48 pulses of 50, 100, 200 and 500 ms in all four directions, three samples each, two warm-up pulses
discarded, mount idle, 300 ms between pulses. The endpoints are the public `GUIDER_GUIDE_RA` /
`GUIDER_GUIDE_DEC` request and the completion the driver publishes, so this is **software completion
timing over the network transport**, including host and WiFi scheduling. It is not relay or motor
timing, and no electrical measurement was made.

Signed error in milliseconds: min 27.9, mean 32.5, median 32.0, p95 36.3, p99 52.7, max 52.7,
standard deviation 3.9, maximum absolute 52.7. Every sample is late, which is what a scheduled
completion confirmed over a network can be; none is early. No pulse was refused in that run.

### Test summary

- Simulated tests run: 40; passed: 40. (`make -C indigo_test build/integration/test_mount_nexstaraux_simulator`
  then `./build/integration/test_mount_nexstaraux_simulator`, Linux arm64.)
- Hardware tests run: 36; passed: 36. (`make -C indigo_test test-mount-nexstaraux-hw HW_PARK=1`,
  Linux arm64, driver `0x03000012`, Celestron NexStar SE with a NexStar+ hand controller over a
  SkyPortal WiFi module.) Seven of the 36 report themselves as not exercised on this rig for the
  reasons listed above.

## Mac hardware retest (2026-09-24)

On macOS 26.7 arm64, at commit `f4abdd01f` with a clean working tree, the current
`0x03000012` driver was rebuilt from source and linked into the opt-in hardware test. The physical
Celestron NexStar SE and NexStar+ hand controller were reached through the SkyPortal WiFi module at
`192.168.111.156:2000` (MAC `4C:55:CC:17:67:E0`, ZentriOS `WL-1.2.0.10`); both motor controllers
reported firmware 5.20. A read-only `MC_GET_VER` packet returned a motor-controller response before
the run. The host was this Mac, not the Raspberry Pi used for the earlier acceptance.

`MOUNT_NEXSTARAUX_HW_URL=nexstar://192.168.111.156:2000 make -C indigo_test
test-mount-nexstaraux-hw HW_PARK=0` exited 0 with 36/36 cases passing. Nine cases printed `not run`
within their PASS result: six motion or tracking measurements because another master on the AUX bus
moved the axes while no driver command was active; both opt-in park cases because the earlier
hardware run found the pole sometimes unreachable and the return bounded by cord wrap; and the
extreme guide-rate write because this controller acknowledges but does not store autoguide-rate
changes. The network autodiscovery case separately reported that an explicit URL prevented its
exercise, making ten cases not fully exercised on this Mac. Physical unplug/replug was not
performed, so hot-plug coverage was not established. The suite restored the state it could and
disconnected both logical devices.

The Mac received the module's genuine UDP announcements through a Python UDP socket and connected to
the motor controller over TCP, but a run with the driver's default `nexstar://` address did not
connect. A first attempt in the tool sandbox reported `Failed to bind passive discovery socket`; an
escalated attempt timed out without a discovery result. A narrow Mac C probe using
`indigo_perform_passive_discovery(55555, 3, ...)` also timed out, while a plain C UDP socket bound
successfully but `recvfrom()` returned `EAGAIN` after four seconds. A Python UDP socket on the same
Mac received repeated announcements from the same MAC and IP. This leaves Mac process-specific UDP
delivery unresolved; it is not evidence of a mount driver defect or of a completed autodiscovery
case. Direct TCP and every other exercised scenario passed.

The guide pulse timing case measured 48 software completion intervals for 50, 100, 200 and 500 ms
pulses in four directions: signed error min 31.4, mean 38.7, median 38.0, p95 47.4, p99 66.3, max
66.3 and standard deviation 5.7 ms. This includes Mac and WiFi scheduling; it is not an electrical
or motor-timing measurement. No pulse was refused.

An initial Mac attempt had accidentally linked a stale 2026-09-22 driver archive, although the
current source was newer. It reproduced the already-fixed southern-declination and SYNC failures
and aborted during the slew test. After rebuilding the archive and relinking the test, the full run
above passed; the earlier attempt is not counted as validation of the current driver. A read-only
`MC_SLEW_DONE` check before the repeat found both axes reporting complete.

## 2026-09-26 the mount is its own master again

The generator now sets `master_device` of the master device to itself, as this driver did before its
migration (`mount->master_device = mount`). `indigo_disconnect_slave_devices()` therefore disconnects
the mount too when the connection is lost, as the hand-written driver did, instead of leaving it
connected with every request ending in ALERT. `goto_loses_the_answer` and `transport_loss` now require
the mount to be disconnected after the loss; the fault stops the simulator, so a fresh session stays
covered by `reconnect`. The `device->master_device == NULL ? device : ...` fallback in the `.driver`
source is gone.

## Switch and number targets (2026-09-26)

Version 22, findings TGT-006, TGT-B04 and TGT-B05 (nexstaraux parts) and TGT-C05 of
`indigo_drivers/REVIEW_SWITCH_TARGETS.md`.

- TGT-006: `mount_slew_finalizer` switches `MOUNT_TRACKING` on when a goto ends. It is an
  `INDIGO_TASK_PRIORITY_TIME` task re-queued every 0.1 s during the goto, the tracking handler a normal
  one, so a tracking request copied just before the goto ended was overwritten and the handler sent
  tracking ON and reported OK. The handler now reads the request with `indigo_get_switch_target()`,
  applies it with `indigo_apply_switch_targets()` and on failure shows the rate the mount was last
  given (`track_rate`) with ALERT.
- TGT-B05: the same finalizer published `MOUNT_TRACKING` OK/ALERT over the pending BUSY. It still starts
  tracking and writes the value, but leaves state and publication to a pending request.
- TGT-B04: `GUIDER_GUIDE_RA`/`DEC` accept a pulse while one runs. A pulse copied while the previous
  pulse's URGENT finalizer was due was zeroed by that finalizer and the new handler read 0, dropping the
  pulse with OK. The finalizers already cleared only the values; the handlers now restore the values from
  the targets before reading them.
- TGT-C05 (won't fix): `MOUNT_UTC_TIME` stays hidden in this driver, so it is never defined and no client
  request exists for the poll to overwrite.

Regression tests in `integration/test_mount_nexstaraux_simulator.c`:

- `tracking_request_survives_slew_end`: a gate handler is queued from the debug log of the finalizer's
  declination `MC_GOTO_SLOW` write, tracking OFF is requested while the gate holds the queue and the slow
  approach ends, so the finalizer's completion runs ahead of the handler. Before the fix tracking ended ON
  (fails at the OFF assertion); with only the handler fixed the finalizer still published a second result
  (fails at `expected 1, got 2`); now the mount gets `00 00` after the finalizer's `FF FF` and only the
  handler publishes.
- `guider_pulse_survives_previous_finalizer`: a gate holds the queue past the deadline of a 200 ms pulse,
  the opposite 300 ms pulse is requested, then the gate is released, on both axes. Before the fix the
  second pulse was never sent ("was dropped"); now it runs about 300 ms.

Both cases failed against the version 21 driver and pass with version 22. Recorded run through
`tools/run_driver_test.py mount_nexstaraux` on Linux x64: 42/42. No hardware run for this change.

Final test summary for this change: simulator suite 42 run / 42 passed; hardware 0 run / 0 passed.

## AUX protocol audit (2026-10-04)

The driver was compared command by command with the AUX motor controller protocol as other
AUX-speaking control software uses it, and with `nexstar_aux_commands_10.pdf`. The facts the
comparison rests on are stated here as protocol facts.

### Protocol facts the driver did not use

- **Rate unit.** The 24 bit form of `MC_SET_POS_GUIDERATE` / `MC_SET_NEG_GUIDERATE` (0x06 / 0x07)
  carries the axis rate in 1/1024 arcsecond per second; the 16 bit form is the upper two bytes of
  it, except for the named rates 0xFFFF (sidereal), 0xFFFE (solar) and 0xFFFD (lunar). The protocol
  document's own alt-azimuth tracking example (`0x00 0x1d 0xef`, 7.5"/s) is consistent with it.
- **Controller-timed guide pulses.** `MC_AUX_GUIDE` (0x26) takes a signed byte, the rate in percent
  of sidereal added to the running drive, and an unsigned byte, the duration in units of 10 ms
  (at most 2.55 s). `MC_IS_AUX_GUIDE_ACTIVE` (0x27) answers 1 while the pulse runs. Motor controller
  firmware 6.50 and newer implements them; older firmware has to be guided by changing the axis
  rate for the length of the pulse.
- **Autoguide rate.** `MC_SET_AUTOGUIDE_RATE` (0x46) is the rate of the ST-4 autoguider port. Only
  models with such a port store it; on the others (NexStar SE 4/5, SLT, GT, Evolution, ...) it is
  acknowledged and discarded, which is exactly what the NexStar SE of the hardware run did. The
  rate a computer-commanded pulse runs at is not a controller setting at all.
- **Mount model.** `MC_GET_MODEL` (0x05) sent to the azimuth controller answers one byte, the model
  number: 1, 2 NexStar GPS, 3 NexStar i, 4 NexStar SE, 5 CGE, 6 Advanced GT, 7 SLT, 8 Legend,
  9 CPC, 10 NexStar GT, 11 NexStar SE 4/5, 12 NexStar SE 6/8, 13 CGE Pro, 14 CGEM, 15 LCM,
  16 SkyProdigy, 17 CPC Deluxe, 18 NexStar GT, 19 StarSeeker GT, 20 AVX, 21 Cosmos GT,
  22 Evolution, 23 CGX, 24 CGX-L, 25 AstroFi, 26 to 28 Sky-Watcher mounts on the AUX bus,
  29 Origin. The models with an autoguider port are 5, 6, 9, 12, 13, 14, 17, 20, 23 and 24; the
  models from 20 on slew fast enough that their goto approach point is 1 degree from the target
  rather than 2.5.
- **Goto approach.** `MC_GET_APPROACH` (0xFC) answers 0 for a positive and 1 for a negative
  approach per axis. The motor controller does not apply it by itself: a goto is made as a fast goto
  to a point 2.5 degrees (1 degree on the fast models) short of the target on the approach side,
  followed by slow gotos to the target, so that every goto ends with the gears loaded the same way.
  The slow goto is repeated (three passes in all) because the target moves on while the mount is
  not tracking.
- **`MC_SLEW_DONE` has three answers**: 0x00 running, 0xFF finished and 0xFE aborted.
- **Polling rate.** The protocol document warns that polling the controller in quick succession
  during a goto can make it miss its destination and keep rotating. Polling every 500 ms is safe.
- **Unsolicited reports.** A motor controller sends `MC_SEND_WARNING` (0x50) with 0x00 for a low
  battery and 0x01 for a slew limit that stopped the axis, and `MC_SEND_ERROR` (0x51); each is
  acknowledged by echoing the command back to the controller with no data.

### Defects and gaps found

1. **Right ascension guide pulses moved the wrong way and at the wrong rate.** A pulse was an
   `MC_MOVE_POS` / `MC_MOVE_NEG` rate move at hand controller rate 1, which writes the same
   velocity register as the tracking drive. While a pulse ran, the axis turned at +-0.5 times
   sidereal *instead of* the sidereal drive, so relative to the sky an east pulse slowed the axis by
   0.5x and a west pulse slowed it by 1.5x: both directions moved the mount the same way. East was
   also sent as the positive (westward) direction, the opposite of the INDIGO convention in which
   a west pulse runs the drive faster. Declination pulses had the right sign but, like RA pulses,
   ignored `GUIDER_RATE` and `MOUNT_GUIDE_RATE` entirely and always ran at 0.5x. No hardware case
   measured the RA direction, which is how this survived the acceptance run. Source audit;
   to be reproduced in the simulator and on the hardware.
2. **The guide rate was refused on every mount without an autoguider port.** Because the setting is
   only stored by models with an ST-4 port, the readback check made `MOUNT_GUIDE_RATE` and
   `GUIDER_RATE` fail on the NexStar SE of the hardware run; the published rate was whatever the
   controller held (0.39 percent on that mount).
3. **An aborted goto was taken for a finished one.** 0xFE from `MC_SLEW_DONE` was treated as done.
4. **The goto polled ten times a second.** `mount_slew_finalizer` re-ran every 100 ms.
5. **No goto approach.** The fast goto went straight to the target and one slow goto followed, so
   the final direction of motion, and with it the backlash, depended on where the mount came from.
6. **Model and warnings.** `MOUNT_INFO` named every mount "NexStar AUX", and warning and error
   reports of the controller were skipped silently.

### Hardware decision

Hardware testing will be performed on the Celestron NexStar SE with a NexStar+ hand controller over
the SkyPortal WiFi module (motor controller firmware 5.20, so the rate-change guide path). Planned
scenarios: the existing acceptance suite, plus a new case that measures the hour angle a west and an
east RA pulse move with tracking on and asserts opposite signs, and the guide rate cases updated to
the new semantics. The `MC_AUX_GUIDE` path cannot be exercised on this firmware and is covered by
the simulator only.

### Plan

1. Simulator: correct 24 bit rate unit, `MC_GET_MODEL`, `MC_GET_APPROACH`, `MC_AUX_GUIDE` /
   `MC_IS_AUX_GUIDE_ACTIVE` superimposed on the drive, profiles `aux-guide` (firmware 7.11,
   Evolution) and a model without autoguider port for `deaf-guide-rate`, fault actions `aborted`
   (`MC_SLEW_DONE` answers 0xFE) and `warn0` / `warn1` (unsolicited warning before the answer).
2. Simulator tests: reproduce defects 1 to 6 against the version 22 driver, adjust the cases whose
   expectations encode the old guiding and guide rate semantics.
3. Driver: rate-based guiding with the `MC_AUX_GUIDE` path, guide rate kept by the driver and
   written to the controller only on models with an autoguider port, model identification, approach
   goto with three slow passes, 0xFE handling, 500 ms goto polling, warning reports. Version 23.
4. Hardware suite: RA guide direction case, guide rate cases for the new semantics.
5. Recorded simulator run and recorded hardware run through `tools/run_driver_test.py`.

### Results (2026-10-04)

- Steps 1 to 4 done. Against the version 22 driver the new and changed simulator cases failed as
  expected: `guide_pulse_direction` measured +0.00249 h for a west and +0.00110 h for an east pulse
  (both the same way), `goto_polls_gently` counted 37 polls in 4 s, `goto_aborted_by_mount`,
  `controller_warning_acknowledged`, `controller_timed_guide_pulses`, `guide_rate_not_stored`,
  `guider_pulses`, `metadata` and `slew_to_coordinates` failed on the missing behaviour.
- Two further defects were found while fixing: the guide rate read on connect was written to the
  item values but not their targets, so a request changing only the declination rate sent the
  default right ascension rate (regression: `guide_rate_not_stored`); and a reply length byte
  above 12 was read into the 16 byte reply buffer (source audit, now refused). A refused slow
  approach also left the goto BUSY; it now ends in ALERT.
- Version 23: recorded simulator run `tools/run_driver_test.py mount_nexstaraux`, mac arm64,
  47/47 OK (west -0.00085 h, east +0.00082 h; 7 polls in 4 s).
- Step 5 hardware run **not done**: the NexStar SE / SkyPortal module answered neither ARP nor its
  UDP announcement from the Mac or from indigosky on 2026-10-04. The hardware suite is built
  (37 cases, new `nexstaraux_guides_the_ra_axis_both_ways`) but unvalidated against version 23.

Test summary for this change: simulated tests run 47, passed 47; hardware tests run 0, passed 0.

### Hardware runs and the transport defects they found (2026-10-04)

The first hardware run of version 23 (mac arm64, NexStar SE answering model 11 "NexStar SE 4/5",
firmware 5.20 / 5.20) passed 35 of 37. `nexstaraux_guides_the_ra_axis_both_ways` measured
-0.00132 h for a 5 s west and +0.00122 h for a 5 s east pulse at the sidereal rate (expected
-+0.0014 h), against both directions moving the same way before the fix. The two failures were the
identity case, which still expected the model text "NexStar AUX" (the suite now prints the model
and accepts what the mount reports), and `reinitializes`, where the WiFi module refused new
sessions. A second full run passed 36 of 37: `keeps_tracking_through_a_guide_pulse` lost 0.00628 h
of right ascension in the 30 s after a west pulse, a quarter of the sidereal drive missing, with
the hand controller idle waiting for its alignment. It did not repeat in three further runs of the
guider cases with a wire log, but one of them caught the transport defect behind it:

- **A packet whose bytes arrived apart silenced the mount.** The module sent `3B 04` and the rest
  of the answer more than a second later. `nexstaraux_read()` waited for the first byte and then
  read the whole body with `indigo_uni_read()`, whose second `read()` timed out with EAGAIN; that
  error latches on the handle, and every later request failed without reaching the mount while the
  socket stayed valid. The read now takes only what has arrived after each wait, and the body of a
  packet that has begun is given 3 s. Regression case `split_answer_keeps_the_mount_alive`, against
  a new simulator fault `split` that pauses an answer for 1.5 s after its fourth byte; it failed
  against the version 23 driver before this fix with the EAGAIN latch.
- **A late answer was taken for the answer to the next request.** Answers are matched by command
  and addresses only, so after a request timed out its late answer acknowledged the next request of
  the same kind. For the rate restored at the end of a guide pulse that is exactly a pulse reported
  complete while the drive was never set back, which matches the loss measured above. Every request
  now discards what is waiting before it is sent. Regression case `late_answer_not_taken`, against a
  new simulator fault `late` whose `MC_GET_POSITION` answer comes 1.5 s late and claims a quarter
  turn: before the fix the driver published a declination of 90.000, now 20.000. A controller
  report that arrives between requests is discarded with the rest; reports sent while a request
  waits for its answer are still acknowledged.

## Mount testing rules coverage (2026-10-04)

Version 23. The simulator suite was checked item by item against the extended "Mount Drivers" chapter
of `indigo_test/DRIVER_TESTING_RULES.md` and the missing driver-relevant scenarios were added. Seven
defects were found by the new cases and fixed in `indigo_mount_nexstaraux.driver`:

- **A park the controller refused stayed BUSY for ever.** The failure branch of `MOUNT_PARK` set ALERT
  without publishing it (the handler references a finalizer, so the generator adds no final update)
  and left `PARKED` selected. It now publishes ALERT with `UNPARKED` selected.
- **A park left `MOUNT_TRACKING` ON over a stopped drive, and the coordinates OK while moving.** The
  park stops the drive on the controller; it now also publishes `MOUNT_TRACKING` OFF (unless a request
  is pending) with the state light IDLE, and publishes `MOUNT_EQUATORIAL_COORDINATES` BUSY while the
  mount moves to the park position.
- **A parked mount could be guided.** The guide handlers now refuse a pulse with ALERT while the mount
  is parked or parking, like the generated parked guards of the mount properties.
- **Disconnecting did not stop the mount.** A goto, park or manual motion kept running after the mount
  was disconnected, and a pulse cut short by a guider disconnect kept its axis turning at the guide
  rate because its finalizer was cancelled. Both devices now stop their axes in `on_disconnect`, and
  the mount clears its motion items and its slew/park flags so the next session starts clean.
- **A lost answer to the tracking restore left the mount standing with tracking ON.**
  `nexstaraux_stop_axis()` now resends the rate once; when it still fails it reports `MOUNT_TRACKING`
  OFF with ALERT through `nexstaraux_tracking_lost()`.
- **A failed coordinate poll was silent.** It now publishes ALERT with the last valid position, and the
  next good poll restores OK (only an ALERT it set itself, never the ALERT of a failed goto).
- **The guide rate was not shared.** `MOUNT_GUIDE_RATE` and `GUIDER_RATE` write the same controller
  registers; a successful write through one is now published on the other. The private data keeps
  both device pointers for this (`on_attach`/`on_detach`).

The simulator's `INDIGO_NEXSTARAUX_FAULT` file takes an optional count, so a fault can persist over
several requests (`<dst> <cmd> <action> [count]`).

### New and extended cases

| Rule area | Case |
| --- | --- |
| Handshake order, negative capability contract, track-rate items | `metadata`, `property_contract` (extended) |
| MOUNT_STATE lights, order against the follow-up tracking restart | `mount_state_lights` |
| Tracking setting kept through the goto, resumed at the selected rate | `goto_keeps_tracking_setting` |
| Refusal at the first and last command of the goto sequence | `goto_refused_mid_sequence` |
| Lost status reply during a goto | `goto_survives_a_lost_status_reply` |
| Busy goto ignores a second request | `busy_goto_ignores_second_request` |
| Abort landing mid-slew, one stop per axis, ALERT never OK | `abort_lands_mid_slew`, `abort_slew` and `abort_while_idle` (extended) |
| Direction readback of every manual direction, guide pulse displacement | `motion_directions_read_back` |
| Parked guards incl. guide pulses, no latch after unpark | `parked_guards` |
| Park refused by the controller, park with tracking on, unpark sends nothing | `park_refused_by_controller`, `park_and_unpark` and `abort_park` (extended) |
| Disconnect during goto, manual motion and park | `disconnect_stops_motion` |
| Guider disconnect during a pulse, guider-only session, GOTO during a pulse | `guider_disconnect_during_pulse`, `guider_only_session`, `goto_during_guide_pulse` |
| Tracking restore retry and persistent failure | `tracking_restore_retried` |
| Failed initial readback, failed poll ALERT/recovery | `initial_readback_failure`, `coordinate_polling` (extended) |
| Guide rate shared by mount and guider | `guide_rate_shared_with_guider` |
| Hour-angle targets, encoder wrap | `hour_angle_targets` |
| Refused switch shows the real state, SYNC not retried, no update for undefined properties | `tracking_failure`, `sync_command_failure`, `reconnect` (extended) |

### Gaps left open

- **Southern hemisphere.** The driver flips the tracking direction (`MC_SET_NEG_GUIDERATE`) south of
  the equator but not the encoder-to-hour-angle conversion of coordinates, gotos and syncs, so the two
  cannot both be right for a mount in the south. Which one the controller expects needs a southern
  hardware run; nothing was changed and no south-specific motion case was added.
- **Guide pulses ignore the guide rate.** Pulses are `MC_MOVE_POS/NEG` at rate 1, half sidereal
  absolute, replacing the tracking rate. At the default 50 % a declination pulse moves guide rate x
  duration (asserted), but another `MOUNT_GUIDE_RATE`/`GUIDER_RATE` has no effect on pulses, and on a
  tracking mount a WEST pulse moves 1.5x sidereal against 0.5x for EAST. Fixing it needs the 24 bit
  `MC_SET_POS_GUIDERATE` rate, whose unit the protocol document does not give.
- The tracking restore after a pulse stops the axis first instead of writing the rate on the fly;
  kept because it is what was verified on hardware.
- Connect stops the tracking drive because the protocol has no way to read it; the guide rates are
  read back (`guide_rate`).
- Discovery, transport changes and alignment/PEC/home/pier-side rows do not apply (no such commands).

The seven defects above were reproduced by the new cases against the version 22 driver
(`mount_state_lights`, `parked_guards`, `park_refused_by_controller`, `park_and_unpark`,
`disconnect_stops_motion`, `guider_disconnect_during_pulse`, `tracking_restore_retried`,
`coordinate_polling`, `guide_rate_shared_with_guider` failed) and pass with version 23. Recorded run
through `tools/run_driver_test.py mount_nexstaraux` on macOS arm64: 59/59. No hardware run for this
change.

## Merge of the two version 23 lines (2026-10-04)

Version 24. Two independent changes both called themselves version 23: the AUX protocol audit above
(guiding, guide rate, model, goto approach, aborted goto, polling rate, controller reports, transport)
and the mount testing rules coverage (refused park, park stops tracking, parked guider, stop on
disconnect, retried tracking restore, failed poll reported, guide rate shared with the guider). They
were merged into one driver; every behaviour of both is kept. Where they met:

- The retried tracking restore now lives in `nexstaraux_restart_tracking()` and is used both after an
  axis stop and at the end of a guide pulse, so a pulse whose restore keeps failing also reports the
  mount as not tracking.
- `nexstaraux_share_guide_rate()` publishes the rate on both devices, and that rate is the driver's
  own one the pulses run at, written to the controller only on models with an autoguider port.
- The cases of the testing rules coverage that encoded the old guide pulse (`MC_MOVE_POS` at rate 1)
  now expect the 24 bit rate of half sidereal and the zero rate at its end
  (`motion_directions_read_back`, `parked_guards`, `guider_disconnect_during_pulse`,
  `goto_during_guide_pulse`); `metadata` expects the model and approach queries in the handshake, and
  `hour_angle_targets` checks the fast goto at its approach point and the slow goto at the target.

### Gaps of the testing rules coverage, resolved

- **Guide pulses ignore the guide rate.** Resolved by the rate-based guiding of the audit: the unit
  of the 24 bit rate is 1/1024 arcsecond per second, and a pulse runs the axis at the tracking rate
  plus or minus the guide rate. Measured on the NexStar SE: 5 s pulses at the sidereal rate moved the
  right ascension by -0.00132 h west and +0.00122 h east.
- **Southern hemisphere.** On a wedge south of the equator the mount is the mirror image of the
  northern one: the polar axis turns the other way, which is what the negative tracking drive already
  assumed, so it reads minus the hour angle, and the declination axis stands half a turn from the
  declination, reading 90 degrees at the southern pole. `nexstaraux_to_axes()` and
  `nexstaraux_from_axes()` now convert both ways for gotos, syncs and the poll, the approach side of
  the polar axis is turned with it, and a manual WEST runs the polar axis the negative way. Regression
  case `southern_hemisphere`: sync writes 0xE00000 for an hour angle of +3 h and 0x555555 for -60
  degrees, the tracking mount holds its right ascension (0.00012 h over 12 s), WEST is
  `MC_MOVE_NEG`, a goto to -45 arrives and the park ends at -90. With the northern conversion the
  sync would have written 0xA00000 and the tracking mount would have run away at twice the sidereal
  rate. This follows the mounting geometry and is consistent with the drive direction; it has not
  been run on a mount in the southern hemisphere, which no bench here has.

### Hardware run of version 24 (2026-10-04)

`MOUNT_NEXSTARAUX_HW_URL=nexstar://192.168.111.156:2000 python3 tools/run_driver_test.py
mount_nexstaraux --hw`, mac arm64, Celestron NexStar SE (model 11, NexStar SE 4/5, motor controllers
5.20 / 5.20, no autoguider port) over the SkyPortal WiFi module, hand controller idle waiting for its
alignment: **37/37 OK**. The two park cases report themselves as not run (opt-in `HW_PARK=1`).

- Tracking: 0.00891 h lost with tracking off and 0.00023 h with it on over 30 s; 0.00000 h after
  manual motion, 0.00004 h after a slew, 0.00023 h / 0.00014 h after an east / west pulse.
- GOTO of 3.092 degrees arrived 0.015 degrees from the target.
- RA guiding: 5 s pulses at the sidereal rate moved the right ascension by -0.00155 h west and
  +0.00140 h east (expected -+0.0014 h).
- Guide pulse completion timing (software, request to published completion over WiFi, 48 pulses of
  50 to 500 ms): signed error min 65.5, mean 127.6, median 120.6, p95 195.6, p99 231.1, max 231.1,
  sd 43.3 ms. Earlier runs the same evening measured means of 42.5 ms and 101.6 ms with the same
  pulse logic, so most of the spread is the module; the discard before each request adds up to
  10 ms to every request, two per pulse.

Test summary for version 24: simulated tests run 67, passed 67; hardware tests run 37, passed 37.
