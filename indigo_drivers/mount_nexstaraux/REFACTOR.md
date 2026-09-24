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

**The run is not complete.** Six defects were found on the mount and fixed, and every one of them
is reproduced hardware-free, but the suite never finished a clean pass end to end: the WiFi module
dropped off the network during the last attempt and did not come back, and the last two fixes - the
declination axis that was never stopped and the goto abandoned after a lost answer - have therefore
been verified only against the simulator. The best hardware pass of the session was 34 of 36 cases
with driver `0x03000011`, the two failures being measurements spoiled by the drifting declination
axis that the sixth defect explains. No hardware result is recorded in `README.md` for that reason.

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

### Not covered

- The mount has no park position, home command, side of pier report, site or clock of its own, so
  those rows of the class standard do not apply. Their absence is asserted in
  `publishes_the_mount_property_contract` and `publishes_the_sidereal_time`.
- The guide pulse measurement is software completion timing over the network transport, from the
  public request to the completion the driver publishes. It is not relay or motor timing and no
  electrical measurement was made.
- A controller found holding an autoguide rate below one percent of sidereal cannot be given that
  rate back, because the property starts at one percent. This mount was found at 0.39 percent and
  the run says so instead of pretending it restored it.

### Test summary

- Simulated tests run: 40; passed: 40. (`make -C indigo_test build/integration/test_mount_nexstaraux_simulator`
  then `./build/integration/test_mount_nexstaraux_simulator`, Linux arm64.)
- Hardware tests run: 36; passed: 34; the run is incomplete, see above. Best pass of the session
  was driver `0x03000011` against the NexStar SE with a NexStar+ hand controller; the two failures
  were the tracking and declination measurements spoiled by the undriven declination axis, which
  the last fix addresses and which has not been re-measured on the mount.
