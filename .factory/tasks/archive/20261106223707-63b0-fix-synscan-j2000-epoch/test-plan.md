# Test and regression plan

Status: accepted

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 a J2000 GoTo is pointed at its place of the date | AC-001, AC-002 | integration, new `synscan_mount_points_j2000_coordinates_at_their_place_of_date` | fresh simulator, latitude 48, tracking off, target = J2000 of the date place at HA +2.5 h / DEC +30° | RA and DEC goto increments give HA 2.5 h ± 0.005 and DEC 30° ± 0.01; the J2000 hour angle is more than 0.01 h away; the raw readback precessed to the date gives HA 2.5 h ± 0.01 and DEC 30° ± 0.01 | before the fix (fails), after (passes) |
| T-002 both pier sides and hemispheres | AC-003 | integration, `synscan_mount_moves_the_requested_way_on_both_pier_sides` | targets through `synscan_j2000_ra_at_hour_angle()`, readback through `synscan_fresh_date_pointing()` | date HA within 0.01 h, DEC within 0.05°, side of pier as before | baseline and final (closed loop through the driver's own readback: passes before the fix too, see verification.md) |
| T-003 park position readback | AC-004 | integration, `synscan_mount_parks_at_park_position_and_restores_it` | `synscan_fresh_hour_angle()` precesses the J2000 raw coordinates to the date | parked HA within 0.01 h of the mechanical park HA | after the fix |
| T-004 sync keeps an alignment point | AC-005 | integration, `synscan_mount_sync_adds_alignment_point` | poll detection by the raw-coordinates revision (the raw RA at the pole is a constant J2000 value) | point count 1, synced coordinates hold across polls and reconnect | baseline and final |
| T-005 epoch property | AC-006 | integration, `synscan_mount_reports_identity_and_handshake` | unchanged | `MOUNT_EPOCH` read-only | baseline and final |
| T-006 preserved behaviour | AC-007 | the remaining cases | unchanged | all pass | baseline and final |
| T-008 park position from the current pointing | AC-009 | integration, new `synscan_mount_takes_the_park_position_from_the_current_pointing` | GoTo to a J2000 target at HA -1.5 h, then `MOUNT_PARK_SET` CURRENT, then park | park position = date pointing ± 0.002; no `:J` command for the park | after the fix |
| T-009 home at the current position | AC-010 | integration, `synscan_mount_tracking_request_survives_home_finalizer` | unchanged | home OK with the axes already there | baseline and final (failed between the epoch change and the set-current override) |
| T-010 stopped axes | AC-011 | integration, the `first`/`second` checks of the abort, disconnect and park cases | `synscan_fresh_date_dec()` | equal within 1e-6° | baseline and final |
| T-007 hardware side-of-pier case | AC-003 on hardware | hardware, `synscan_reports_side_of_pier` | J2000 targets, date readback | compiles; not run (no mount) | final (build only) |

## Baseline evidence
Recorded in verification.md after step 1.

## Bug reproduction
Step 2 builds the updated suite against the unchanged driver: T-001 fails at the goto increments (the hour angle
and declination of the J2000 coordinates are sent), reproducing the user's report hardware-free. T-002 cannot tell
the defect: its targets and its readback go through the driver's symmetric conversions.

## Negative cases and permission boundaries
Not applicable: no new input or permission boundary.

## Isolation and cleanup
The suite uses a private fixture directory and PTYs; the new case restores the latitude it sets;
`make -C indigo_test test-clean` after the run.

## Deferred tests and coverage limitations
No hardware run (no SynScan mount available); the hardware case is compiled only. Owner: Peter Polakovic;
follow-up: the reporting user's confirmation with the fixed driver.
