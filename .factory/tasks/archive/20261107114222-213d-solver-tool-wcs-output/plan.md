# Development plan

Status: complete

## Approach and affected components
Port `wcs_file_header()` of Ain Platesolver to C in the tool: `fits_card()`/`fits_number_card()` helpers and `write_wcs()` called from `main()` after a successful solve. Keep J2000 centre fields in `solver_result`. Extend the existing integration test with a WCS reader and a TAN inverse that checks the rendered stars.

## Baseline
- Commands and environment: `make -C indigo_test test-indigo-solver-tool`, Linux arm64, repository root
- Existing failures: none (23/23 pass with the changed tool before the test change)
- Repository revision / working-tree state: d65d5dc12 (master = origin/master), task branch

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `-w/--wcs`, `write_wcs()`, help, exit code 3 in `indigo_tools/indigo_solver.c` | AC-001..AC-008 | None | build, manual runs | done |
| 2 | WCS cases in `indigo_test/integration/test_indigo_solver_tool.c` | AC-001..AC-007 | 1 | suite passes; mutation detected | done |
| 3 | Independent review, task records | all | 2 | quality-review.md | done |

## Test strategy
Integration test of the built executable on the synthetic index and frames (no network, private temp dir). Truth is the catalog position of each rendered star, not the tool's printed solution. Additional out-of-suite check with Hipparcos fields rendered at 4 rotations x 2 parities and astrometry.net `wcsinfo` parsing (verification.md).

## Security risks and mitigations
See security.md: user-chosen output path only.

## Dependencies and architecture decisions
No new dependency. Convention deliberately shared with Ain Platesolver.

## Migration, rollout, and rollback
Not applicable: additive option; revert the commit to roll back.

## Deviations and owner decisions
None recorded.
