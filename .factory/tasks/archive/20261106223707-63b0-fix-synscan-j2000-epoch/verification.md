# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: implementing agent (Claude Code, claude-fable-5-1); independent static review by a fresh subagent, see quality-review.md
Date: 2026-11-07
Revision and working-tree state: master 237b84532 plus the uncommitted task diff on task/20261106223707-63b0-fix-synscan-j2000-epoch
Review independence: the verifier also implemented the change; all test runs were made by the implementer

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `synscan_mount_points_j2000_coordinates_at_their_place_of_date`: "J2000 23.7666 h +29.8508 deg is HA 2.5000 h DEC +30.0000 deg of the date, pointed at HA 2.5000 h DEC +30.0000 deg" (goto increments decoded from the simulator log); against the master driver the same case pointed at HA 2.5221 h DEC +29.8521 deg |
| AC-002 | pass | same case: "reached HA 2.5000 h DEC +30.0000 deg of the date" |
| AC-003 | pass | `synscan_mount_moves_the_requested_way_on_both_pier_sides`: HA ±2.5 h / DEC ±30 reached as HA ±2.5000 h / DEC ±30.0000 (±0.0001) with the expected side of pier at latitude ±40 |
| AC-004 | pass | `synscan_mount_parks_at_park_position_and_restores_it`: "parked at HA 2.5000 h DEC 40.0000" and the same after the power cycle |
| AC-005 | pass | `synscan_mount_sync_adds_alignment_point` (poll detection by revision) |
| AC-006 | pass | `synscan_mount_reports_identity_and_handshake` (`MOUNT_EPOCH` read-only) |
| AC-007 | pass | final recorded run 96/96 (94 pre-existing cases plus 2 new) |
| AC-008 | pass | `version = 22;` in the `.driver`, `DRIVER_VERSION 0x03000016` in the regenerated `.c`, recorded as `3.0.0.22` |
| AC-009 | pass | `synscan_mount_takes_the_park_position_from_the_current_pointing`: "pointing HA -1.5000 h DEC +40.0000 deg of the date, park position HA -1.5000 h DEC +40.0000 deg", no `:J` for the park |
| AC-010 | pass | `synscan_mount_tracking_request_survives_home_finalizer` (failed between the epoch change and the set-current override, see the found defects) |
| AC-011 | pass | the five stopped-axis checks on `synscan_fresh_date_dec()` pass (abort, disconnect and park cases) |

## Executed checks
All commands ran on macOS arm64 (Darwin 27.0.0, Apple clang) in /Users/polakovic/Development/indigo unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make -C indigo_test build/integration/test_mount_synscan_simulator && ./build/integration/test_mount_synscan_simulator` (baseline, master sources) | repository root / `indigo_test` | 0 | 94 PASS, 0 FAIL |
| `make -C indigo_test build/hardware/test_mount_synscan_hw` (updated hardware case, master driver; repeated with the final driver) | repository root | 0 | compiles without warnings; not run (no mount) |
| updated suite built against the master driver `.c`, `INDIGO_TEST_FILTER=points_j2000` and `both_pier_sides` | `indigo_test` | 1 / 0 | reproduction of the report: the new case FAILs at `fabs(pointed_ha - date_ha) < 0.005` with "pointed at HA 2.5221 h DEC +29.8521 deg"; the pier-side case passes (closed loop) |
| `make -C indigo_drivers/mount_synscan -f ../../Makefile.drv all` (edited `.driver`, three times) | repository root | 0 | `indigo_mount_synscan.c` regenerated and built without warnings |
| `python3 tools/run_driver_test.py mount_synscan` (first, before the test adaptations and the set-current override) | repository root | 1 | 95/87: 8 failures listed under found defects |
| filtered runs of the 8 failing cases plus the 2 new and the pier-side case after the fixes | `indigo_test` | 0 | all PASS (the park case first failed on its own request helper, fixed, then PASS) |
| `python3 tools/run_driver_test.py mount_synscan` (second, before R5/R6) | repository root | 0 | 96/96 OK |
| filtered runs `takes_the_park`, `tracking_request_survives_home`, `sends_rates_in_axis` on the final sources | `indigo_test` | 0 | all PASS; no park file left in the suite folder |
| `python3 tools/run_driver_test.py mount_synscan` (final) | repository root | 0 | 96/96 OK; README.md line `2026-11-07 00:30 3.0.0.22 mac arm64 simulator 96/96 OK`; TEST_SUMMARY.md regenerated |
| `make -C indigo_test test-clean` | repository root | 0 | test binaries removed |
| `./.factory/scripts/findings check` | repository root | 0 | 25 active, 292 archived; task evidence checks passed |

## Baseline comparison and regressions
No regression: the 94 cases of master pass before and after the change. Expectations changed deliberately where
the suite assumed the raw coordinates were of the date: the stopped-axis checks compare the declination of the date,
the park and autohome readbacks are precessed to the date, the sync case detects its poll by revision, and the
pier-side case sends J2000 targets.

## Found defects
| Defect | Found by | Impact | Fix | Regression test |
| --- | --- | --- | --- | --- |
| the driver formed the hour angle from the J2000 right ascension and read the right ascension back the same way, pointing J2000 targets at their place in 2000 (about 21′ off in 2026) | user report; reproduced hardware-free with the new case against the master driver | every GoTo from a J2000 catalogue misses by the precession since 2000 | `indigo_j2k_to_jnow()` after the alignment model before the hour angle; `indigo_jnow_to_j2k()` on the encoder reading; horizontal coordinates from the aligned coordinates precessed to the date | `synscan_mount_points_j2000_coordinates_at_their_place_of_date` |
| with J2000 raw coordinates the mount base driver's `MOUNT_PARK_SET`/`MOUNT_HOME_SET` CURRENT (sidereal time minus the aligned J2000 RA, aligned J2000 DEC) stores a position off by the precession from the mechanical one; a home at the current position then moved and `synscan_mount_tracking_request_survives_home_finalizer` failed | first full run | park/home "current" positions wrong by about 21′ | the driver's own `on_change` handlers take the position from the axis counters read at that moment (`synscan_set_position()`) | `synscan_mount_takes_the_park_position_from_the_current_pointing`, the home gate case |
| test suite: stopped-axis checks on successive raw declinations, park and autohome readbacks and the sync poll detection relied on raw coordinates of the date | first full run (7 cases) | false failures | pointing-of-the-date helpers; revision-based poll detection | the affected cases |

## Manual checks and limitations
- No hardware run: no SynScan mount available. The hardware case is compiled only.
- Linux and Windows were not run; the change uses only portable C and the precession helpers already used by the
  mount base driver.
- Alignment points saved by versions before 22 should be deleted once (see the specification's compatibility note
  and review R2).

## Verification verdict rationale
All acceptance criteria pass in the final recorded run, the defect is reproduced hardware-free against the pre-fix
driver through the simulator's command log (independent of the driver's own readback), the pre-existing cases pass,
and the independent review found no blocking issue.

## Test summary
- Simulated tests: 96 run, 96 passed (final recorded run).
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
No canonical FND records were created or changed.
