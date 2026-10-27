# Development plan

Status: accepted

## Approach and affected components
All changes stay in the Solver Agent and its tests. The solver core (`indigo_solver.c/.h`) gains a limits
structure (deadline and abort flag) checked inside the matching loops, a cone filter over the loaded catalogue
quads, luminance weighting in the image loader with a Bayer flag, and match statistics from the centre fit. The
agent (`indigo_agent_solver.c`) reads the hints, converts the hint position to J2000, orders the indexes, runs the
depth stages per index, applies the parity check, forces the Bayer binning and formats the solved message.

## Baseline
- Commands and environment: Darwin arm64, `make -C indigo_drivers/agent_solver -f ../../Makefile.drv all`,
  `indigo_test/build/unit/test_agent_solver_core`, `indigo_test/build/integration/test_agent_solver`,
  `python3 tools/run_driver_test.py agent_solver`
- Existing failures: none; the recorded run of commit `a1e7f53d0` is 141/141 (README `## Testing`)
- Repository revision / working-tree state: branch `task/20261027192244-16bd-solver-hints-input-diagnostics` from
  master `ac3c53e2c`, clean apart from the task record

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `indigo_solver.h/.c`: `indigo_solver_limits` (deadline, abort pointer, exceeded flag, reason), checked every 4096 iterations in `indigo_solver_match_quads()` (hash loop and voting loop) and `indigo_solver_match_stars()`; `indigo_solver_compute_center()` returns matched count and rms | AC-004, AC-008 | none | unit test builds, existing unit cases pass | pending |
| 2 | `indigo_solver.c`: `indigo_solver_filter_quads()` keeps the hash-ordered catalogue quads whose four stars lie within a cone; `indigo_solver_load_image()` gets a `bayered` output and luminance weighting for RGB24/RGB48, three-plane FITS, colour JPEG and libraw colour | AC-001, AC-006, AC-007 | none | agent test builds | pending |
| 3 | `indigo_agent_solver.c`: hints (RADIUS/RA/DEC in hints epoch to J2000, PARITY, DEPTH stages, CPU_LIMIT deadline), abort flag wired into the limits, index order by distance from the field size, Bayer binning, messages; version 3.0.0.8 | AC-001..AC-006, AC-008 | 1, 2 | driver builds, agent and simulator tests pass | pending |
| 4 | Tests: unit `match_respects_limits`; agent cases `solve_with_position_hint`, `wrong_position_hint_fails`, `parity_hint_rejects_mirror`, `solve_with_depth_hint`, `bayer_raw_is_binned`, `bayer_fits_is_binned`, `solve_rgb_fits_image`, `solve_colour_jpeg_image`, message assertion in `solve_raw_image`; simulator: a guider setup solved with the JNow position hint | AC-001..AC-008 | 3 | all pass | pending |
| 5 | Docs and record: README hints section (owner approval), REFACTOR.md, MIGRATION_STATUS.md; recorded run `tools/run_driver_test.py agent_solver`, `make -C indigo_test test-unit` | AC-009, AC-010 | 4 | records updated | pending |
| 6 | Reviews (verification, quality, security), process.json, archive, single commit | all | 5 | process check passes on the committed range | pending |

## Test strategy
Unit level for the limit mechanism (deterministic, no timing); agent integration with the synthetic index and
rendered frame for hints, parity, depth, Bayer and colour inputs and the message; the simulator suite for the
JNow position hint through the real pipeline and for regressions of the whole solver; the recorded run as the
acceptance gate.

## Security risks and mitigations
No new trust boundary. Hint values come through framework-validated property items. The cone filter allocates
an array sized by a count pass; the loaders keep their size checks. No new dependencies.

## Dependencies and architecture decisions
None new. The parity hint cannot narrow the search because the quad codes are parity invariant by design (one
index serves both parities); it is applied as a check on the solution. Partial index loading would need a new
file layout and is out of scope.

## Migration, rollout, and rollback
No data migration; the driver version changes to 3.0.0.8. Rollback is the previous driver.

## Deviations and owner decisions
Simulator Bayer case: the simulator's BAYERPAT keyword is attached only by the file image path, so the Bayer
coverage is at the agent test level with a fixture that carries the keyword; recorded in test-plan.md.
