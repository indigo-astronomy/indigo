# Specification

Status: accepted

## Problem and outcome
`indigo_mount_synscan` declares `MOUNT_EPOCH` = 2000 (read-only), so clients send and read
`MOUNT_EQUATORIAL_COORDINATES` as J2000. The driver, however, forms the hour angle of a GoTo target as the local
sidereal time minus the requested right ascension (`synscan_radec_to_steps()`), and reads the raw right ascension
back as the sidereal time minus the encoder hour angle (`synscan_read_mount_coordinates()`), both without precession.
The axes therefore point J2000 coordinates at the place they had in 2000, about 21′ from the place of the date in
2026, and the coordinates read back agree with the request only because both conversions share the omission.
The driver declared epoch 0 (JNow) until commit 7abd66639 (2024-10-04) removed it; the conversion was never there.

Outcome: J2000 stays the frame of the bus; the driver precesses between J2000 and the date at the axis boundary,
so a J2000 target is pointed at its place of the date and the raw, aligned and horizontal coordinates and the
alignment points are consistent J2000 values.

## In scope
- `synscan_radec_to_steps()`: after the alignment model, precess the raw J2000 target to the date with
  `indigo_j2k_to_jnow()` before forming the hour angle from the sidereal time.
- `synscan_read_mount_coordinates()`: form the right ascension of the date from the sidereal time and the encoder
  hour angle, precess it to J2000 with `indigo_jnow_to_j2k()` and publish that as `MOUNT_RAW_COORDINATES`; derive
  the horizontal coordinates from the aligned J2000 coordinates precessed to the date.
- `MOUNT_EPOCH` stays 2000 and read-only; alignment points (sync) keep storing the bus (J2000) values of both the
  raw and the aligned coordinates, as the mount base driver does.
- `MOUNT_PARK_SET` and `MOUNT_HOME_SET`: the mount base driver takes the CURRENT position as the sidereal time minus
  the aligned J2000 right ascension and the aligned J2000 declination, which is now off by the precession from the
  mechanical position; the driver takes it from the axis counters (hour angle and declination of the date) instead,
  and DEFAULT stays the equatorial home (6 h, the pole of the hemisphere).
- Driver version 21 -> 22; regenerated outputs.
- Simulator integration tests: targets expressed as J2000 places of a date hour angle, the fresh hour-angle helper
  precesses the J2000 raw coordinates back to the date, a new case checks the axis targets of a J2000 GoTo.
- Hardware suite: the side-of-pier case expresses its targets and readback the same way (compiled, not run).
- `MIGRATION_STATUS.md`: simulator test count of the `mount_synscan` row.

## Out of scope
The other mount drivers with a pinned epoch, the framework epoch convention, the protocol simulator, park and home
positions (mechanical hour angle and declination, no epoch), hardware validation.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A GoTo to J2000 coordinates sends the axes the hour angle and declination of the date of that place: the RA goto increment of a fresh controller (axes at the home counters) corresponds to the date hour angle within 0.005 h and the DEC increment to the date declination within 0.01°, while the J2000 hour angle differs by more than 0.01 h | new integration case `synscan_mount_points_j2000_coordinates_at_their_place_of_date` |
| AC-002 | After that GoTo the raw coordinates read back, precessed to the date, give the date hour angle within 0.01 h and the date declination within 0.01° (with tracking off the pointing stays put while the right ascension follows the sidereal time) | same case |
| AC-003 | GoTo targets on both sides of the meridian in both hemispheres reach the requested date hour angle within 0.01 h and the date declination within 0.05° with the correct side of pier | `synscan_mount_moves_the_requested_way_on_both_pier_sides` with J2000 targets |
| AC-004 | Park at a mechanical hour angle reads back the same hour angle through the J2000 raw coordinates (the fresh hour-angle helper precesses to the date) | `synscan_mount_parks_at_park_position_and_restores_it` |
| AC-005 | A sync still adds an alignment point that holds across polls and reconnects | `synscan_mount_sync_adds_alignment_point` (poll detection by revision) |
| AC-009 | `MOUNT_PARK_SET` CURRENT after a GoTo stores the hour angle and declination of the date of the pointing within 0.002 h / 0.002°, and a park right after it sends no motion start | new case `synscan_mount_takes_the_park_position_from_the_current_pointing` |
| AC-010 | `MOUNT_HOME_SET` CURRENT followed by a home completes with the axes already there | `synscan_mount_tracking_request_survives_home_finalizer` (unchanged) |
| AC-006 | `MOUNT_EPOCH` stays read-only with value 2000 | `synscan_mount_reports_identity_and_handshake` (unchanged) |
| AC-011 | While the axes stand, the pointing of the date is constant; the raw J2000 declination of a fixed pointing changes with the right ascension of the date (about 1e-5° per second), which the stopped-axis checks of the suite account for | the stopped-axis checks compare the declination of the date |
| AC-007 | Every other case of the suite passes unchanged | full recorded run `python3 tools/run_driver_test.py mount_synscan` |
| AC-008 | The driver version is 22 (`3.0.0.22`) | `.driver` source, regenerated `.c`, recorded test line |

## Failure behavior and edge cases
- Right ascension wraps: the raw right ascension is normalised to [0, 24) before and after the precession.
- At the home position the OTA is at the pole of the date, whose J2000 place is one fixed point: the raw J2000 right
  ascension there is a constant and no longer follows the sidereal time (a test that detected polls by it is changed).
- Targets near the pole precess in right ascension by large amounts; the hour angle math is unchanged, only its
  input is of the date.
- The precession uses the host clock (`indigo_jnow()`), like the horizontal coordinates of the mount base driver;
  `UTC_TIME` is not defined by this driver.

## Permissions, data, and security requirements
No new input; the change is arithmetic on already validated coordinates.

## Compatibility and nonfunctional requirements
No property change. Alignment points saved by the previous version hold J2000 aligned values and raw values that
were (unknowingly) of the date; a session with stale points should delete them (the sequencer recommendation of
the assessment) or sync again. Linux, macOS and Windows use the same portable C.

## Assumptions and open questions
Assumption: the simulator's home counters (`0x800000` RA, `0x800000` + a quarter turn DEC) model a mount at its
home position with the OTA at the pole, as the driver's zero positions assume.

## Owner decisions and requirement changes
2026-11-06: the owner asked to fix the driver so that it uses J2000 on the bus ("oprav to tak, aby pouzival
j2000") after the assessment of the user's report (chat).
