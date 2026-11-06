# Development plan

Status: done

## Approach and affected components
- `indigo_drivers/mount_synscan/indigo_mount_synscan.driver`: `synscan_radec_to_steps()` precesses the raw J2000
  target to the date before the hour angle; `synscan_read_mount_coordinates()` precesses the right ascension of
  the date to J2000 for `MOUNT_RAW_COORDINATES` and precesses the aligned J2000 coordinates to the date for the
  horizontal coordinates; a `synscan_range24()` helper; `synscan_set_position()` behind `on_change` blocks of
  `MOUNT_PARK_SET` and `MOUNT_HOME_SET` so CURRENT is the mechanical position of the axes; version 22. Regenerate
  `indigo_mount_synscan.c`, `.h` and `_main.c`.
- `indigo_test/integration/test_mount_synscan_simulator.c`: `synscan_fresh_date_pointing()`,
  `synscan_fresh_date_dec()` and `synscan_j2000_ra_at_hour_angle()` helpers, J2000 targets in the pier-side case,
  the stopped-axis, park and home checks on the pointing of the date, poll detection of the sync case by revision,
  the new epoch case and the new park-position case.
- `indigo_test/hardware/test_mount_synscan_hw.c`: the side-of-pier case with J2000 targets and date readback.
- `MIGRATION_STATUS.md`: simulator test count of the `mount_synscan` row.

## Baseline
- Commands and environment: macOS arm64 (Darwin 27.0.0), clang; INDIGO library and drivers already built under
  `build/`; `make -C indigo_test build/integration/test_mount_synscan_simulator` and the binary.
- Existing failures: see verification.md (baseline section).
- Repository revision / working-tree state: task branch from master 237b84532.

## Hardware-test decision
No hardware test: no SynScan mount is available in this session. The hardware suite's side-of-pier case is updated
for consistency and compiled only; hardware validation is not claimed.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | baseline: build and run the unchanged suite | AC-007 | None | result recorded in verification.md | done: 94/94 |
| 2 | tests: helpers, J2000 targets, new case; run against the unchanged driver to reproduce the defect | AC-001..004 | 1 | the new case fails with the master driver | done: pointed at HA 2.5221 h DEC +29.85 (verification.md); the pier-side case is closed loop and passes either way |
| 3 | driver: precession at the axis boundary, `synscan_range24()`, version 22; regenerate | AC-001..006, AC-008 | 2 | regenerated outputs differ only as explained | done: version define, edited functions, two generated set handlers |
| 4 | hardware suite case; `MIGRATION_STATUS.md` count; final recorded run `python3 tools/run_driver_test.py mount_synscan` | AC-007 | 3 | all cases pass, README and TEST_SUMMARY.md updated | done: 96/96 after the test adaptations and the set-current override |
| 5 | independent quality review; security review; verification | all | 4 | reports with `Verdict: pass` | done: R1, R5, R6 fixed; R2-R4, R7 dispositioned |

## Test strategy
Simulator-backed integration tests through the serial PTY simulator. The new case reads the goto increments the
driver sends to the simulated motor controller, so the pointing of the date is checked independently of the
driver's own readback (which is symmetric and could not tell the defect).

## Security risks and mitigations
None: arithmetic on validated coordinates, no new input.

## Dependencies and architecture decisions
No new dependency. The bus frame stays J2000 with `MOUNT_EPOCH` = 2000 (owner decision); the precession helpers of
`indigo_align.h` are used as the mount base driver uses them.

## Migration, rollout, and rollback
Users with alignment points saved by the previous version should delete them once; revert the commit to roll back.

## Deviations and owner decisions
- The first recorded run (95/87) showed that the suite's stopped-axis checks, the park readback, the autohome pole
  check and the sync poll detection relied on the raw coordinates being of the date; they now use the pointing of the
  date. It also showed the home-at-current-position case failing because the mount base driver's set-current code
  takes the position from the J2000 coordinates; the driver now takes it from the axis counters (AC-009, AC-010).
- Review R1 (timing margin of the raw readback check) fixed by checking the pointing of the date instead.
