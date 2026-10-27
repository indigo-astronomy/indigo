# Test and regression plan

Status: accepted

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-004 | unit `indigo_test/unit/test_agent_solver_core.c` `match_respects_limits` | synthetic catalogue, limits with a deadline in the past, then with an abort flag set | `indigo_solver_match_quads()` and `indigo_solver_match_stars()` return 0 with `exceeded` set and the reason; with no limits the synthetic field still solves | with implementation |
| T-002 | AC-008 | unit `solve_synthetic_field` | existing | `indigo_solver_compute_center()` reports matched count ≥ 6 and rms below 1" | with implementation |
| T-003 | AC-001 | agent `indigo_test/integration/test_agent_solver.c` `solve_with_position_hint` | RADIUS 2°, RA/DEC 0.5° from the rendered centre, EPOCH 2000 | same solution as `solve_raw_image` | with implementation |
| T-004 | AC-001 | agent `wrong_position_hint_fails` | RADIUS 2°, RA/DEC 30° away | WCS ALERT, "No solution found" | with implementation |
| T-005 | AC-002 | agent `parity_hint_rejects_mirror` | PARITY +1 on the rendered frame (parity -1) then PARITY -1 | first ALERT with "Solution has the wrong parity", second solves | with implementation |
| T-006 | AC-003 | agent `solve_with_depth_hint` | DEPTH 12 (the frame has 28 stars) | same solution | with implementation |
| T-007 | AC-006 | agent `bayer_raw_is_binned` | the rendered MONO16 frame followed by a `BAYERPAT= 'RGGB'` extension, DOWNSAMPLE 1 | solves; WCS scale and size in unbinned pixels | with implementation |
| T-008 | AC-006 | agent `bayer_fits_is_binned` | FITS of the frame with a BAYERPAT card | as T-007 | with implementation |
| T-009 | AC-007 | agent `solve_rgb_fits_image`, `solve_colour_jpeg_image`, existing `solve_rgb48_raw_image` | three-plane FITS with unequal channels, RGB JPEG | same solution | with implementation |
| T-010 | AC-008 | agent `solve_raw_image` | existing | message "Solved: N of M stars matched, rms" seen | with implementation |
| T-011 | AC-001 | simulator `indigo_test/integration/test_agent_solver_simulator.c` guider setups "with hint": every twenty-fifth sweep case sets RADIUS 3° and the rendered RA/DEC in the hints epoch (JNow for EPOCH 0 setups) | solves like the blind run | with implementation |
| T-012 | AC-005, AC-009 | simulator Rosetta cases, full sweep, agent and unit suites through `tools/run_driver_test.py agent_solver` and `make -C indigo_test test-unit` | existing | all pass, record written | after implementation |

## Baseline evidence
Commit `a1e7f53d0` (master `ac3c53e2c` includes it): README record 2026-10-27 19:48 3.0.0.7 simulator 141/141 OK,
unit `test_agent_solver_core` 10/10.

## Bug reproduction
Not a bug fix; the new cases fail before implementation where they assert new behaviour (hint rejection,
parity rejection, message text).

## Negative cases and permission boundaries
T-004 (hint outside), T-005 (wrong parity), T-001 (limits); property ranges are framework validated.

## Isolation and cleanup
Agent tests use the private home and a synthetic index as before; the simulator test uses the persistent index
cache without downloads when present.

## Deferred tests and coverage limitations
- A CPU-limit integration test is not deterministic (a limit of 1 s is met by the synthetic frame); the mechanism
  is covered at unit level. Owner: coordinator; no follow-up required.
- Camera raw (libraw) colour weighting has no fixture; the same conversion path is exercised by the RGB RAW case.
- Simulator Bayer case: the CCD Imager/Guider Simulators do not attach BAYERPAT (only the file simulator does), so
  Bayer coverage is at the agent level.
