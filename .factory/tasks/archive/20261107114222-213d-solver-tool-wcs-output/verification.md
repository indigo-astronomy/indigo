# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those review bodies.

Verdict: pass
Reviewer: coordinator (implementer)
Date: 2026-11-07
Revision and working-tree state: d65d5dc12 with the uncommitted task diff of `indigo_tools/indigo_solver.c` and `indigo_test/integration/test_indigo_solver_tool.c`
Review independence: implementer; the diff was also reviewed in a fresh context (quality-review.md)

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `wcs_file_of_a_solution`: 2880 bytes, printable ASCII, END, space padding, SIMPLE/NAXIS cards |
| AC-002 | pass | `wcs_file_of_a_solution` and `wcs_file_of_a_mirrored_frame`: every rendered star (>= 10) within 10" of its catalog position; synthetic Hipparcos check below, 8 orientations, worst corner error 1.07 px |
| AC-003 | pass | `wcs_file_of_a_jnow_solution`; manual: CRVAL1 identical with `-e 0` and `-e 2000` |
| AC-004 | pass | `wcs_file_of_a_solution` runs `solution_matches_frame()` on the text output; `wcs_file_with_structured_output` runs `document_matches_frame()` on stdout |
| AC-005 | pass | `wcs_file_not_written_when_not_solved` |
| AC-006 | pass | `wcs_file_that_cannot_be_written` |
| AC-007 | pass | `invalid_arguments_are_refused` (also `-w` equal to the image path, image left intact), `help_lists_switches_and_defaults` |
| AC-008 | pass | same keywords, order, comments and CD formula as `~/indigo_imager/ain_platesolver_src/wcsfile.cpp` at indigo_imager 5e2cae0; see quality-review.md |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make -C indigo_test test-indigo-solver-tool` (before the test change) | repository root, Linux arm64 (Debian 13), gcc | 0 | baseline 23/23 pass with the changed tool |
| `make -C indigo_test test-indigo-solver-tool` (after the WCS cases) | same | 0 | 29/29 pass |
| `make -C indigo_test test-indigo-solver-tool` (final, after review fixes R3/R4/R6 of quality-review.md) | same | 0 | 29/29 pass ("indigo_solver tool: all tests passed") |
| pixel offset fit between the written WCS and the truth on the 8 synthetic fields (63 points each) | same | 0 | mean offset at most 0.24 px per axis, no half-pixel bias: CRPIX (w + 1) / 2 matches the solver's pixel convention |
| mutation: CD1_2 without the parity flip, tool rebuilt (checksum changed), suite rerun; then restored (checksum equal to the good build) | same | 1 | `wcs_file_of_a_solution`, `wcs_file_of_a_jnow_solution`, `wcs_file_with_structured_output` fail, star 0 1581" off; restored build passes |
| synthetic check: 3000 x 2000 frames of libindigocat Hipparcos stars around 6h +20°, 9"/px, rotations 0/30/120/250°, normal and mirrored, solved with `indigo_solver -c 60 -w`, WCS compared with the rendering WCS at the corners and centre by an independent TAN inverse (scratch scripts, not committed) | same | 0 | all 8 solved; CD determinant sign equals the truth in all; worst 3.4"-9.6" (0.37-1.07 px), i.e. the solve's own angle error |
| `wcsinfo` (Astrometry.net) on a written file | same | 0 | parsed: imagew 3000, imageh 2000, pixscale 9.004, orientation 30.018, ra/dec centre equal to CRVAL |
| `indigo_solver -w /nonexistent/dir/x.wcs`, `-w ''`, `-w` (no value), unsolvable file | same | 3 / 2 / 2 / 1 | messages as specified; no file for the unsolved image |
| `gcc -fsyntax-only -Wall -Wextra -Wformat=2 indigo_tools/indigo_solver.c` | same | 0 | only the pre-existing unused-parameter warnings |
| `./.factory/scripts/findings check` and `--prune-task 20261107114222-213d-solver-tool-wcs-output` | same | 0 | "Findings valid: 25 active, 292 archived; task evidence checks passed." |

## Baseline comparison and regressions
The 23 existing cases pass before and after; 6 cases added, 2 extended.

## Findings
None.

## Manual checks and limitations
Linux arm64 only; no macOS or Windows build in this task (portable C library calls only). The suite frame covers one rotation; other rotations are covered by the out-of-suite synthetic check. A side observation while preparing test material, not part of this task: 900 x 600 crops of a real DSLR frame (M8 region, 6.7"/px) did not solve with the installed indexes while the full frame did.

## Verification verdict rationale
All acceptance criteria verified by the suite and independent checks.

## Register reconciliation
No FND records.
