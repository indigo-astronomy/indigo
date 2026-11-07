# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those review bodies.

Verdict: pass
Reviewer: coordinator (implementer)
Date: 2026-11-07
Revision and working-tree state: 41e74d877 with the uncommitted task diff of `indigo_tools/indigo_solver.c` and `indigo_test/integration/test_indigo_solver_tool.c`
Review independence: implementer; the diff was also reviewed in a fresh context (quality-review.md)

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `wcs_output_of_a_solution`: stdout redirected to a file holds 2880 bytes, printable ASCII, END, padding; stderr empty; exit 0 |
| AC-002 | pass | `wcs_output_of_a_solution` (RAW 10", FITS 30"), `wcs_output_of_a_mirrored_frame` |
| AC-003 | pass | `wcs_output_of_a_jnow_solution` |
| AC-004 | pass | `wcs_output_with_verbose_progress` |
| AC-005 | pass | `wcs_output_when_not_solved`: wrong hint, text file, missing file give a 0 byte stdout, the reason on stderr, exit 1 |
| AC-006 | pass | `structured_output_not_given_for_wrong_arguments` (`-f wcs -x 3`), `invalid_arguments_are_refused` (`--format WCS`, `-w frame.wcs` unknown) |
| AC-007 | pass | `help_lists_switches_and_defaults` |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make -s -C indigo_test test-indigo-solver-tool` | task worktree root, Ubuntu 24.04.5 x86_64, gcc 13.3.0 | 0 | 28/28 pass ("indigo_solver tool: all tests passed"), binary newer than the source |
| same, final after the stack buffer and comment rewrap | same | 0 | 28/28 pass |
| `gcc -fsyntax-only -Wall -Wextra -Wformat=2 -Wshadow -I. -Iindigo_libs -Iindigo_drivers -Ibuild/include indigo_tools/indigo_solver.c` | same | 0 | only the pre-existing unused-parameter warnings of `discard_log_message` |
| `indigo_solver -f json` and `-f wcs > h.wcs` on the owner's `horsehead_2025-12-20_Light_Ha+OIII_p001.fits` with the installed indexes | main checkout as data, task binary | 0 / 0 | json RA 5.68986495 h, Dec -2.40561610, scale 0.0007723726, parity -1; WCS 2880 bytes, CRVAL1/15 = 5.689864955, CRVAL2 = -2.40561610445, sqrt(abs(det CD)) = 0.00077237255 |
| `./.factory/scripts/findings check` | task worktree root | 0 | "Findings valid: 25 active, 292 archived; task evidence checks passed." |

## Baseline comparison and regressions
Baseline 29/29 at 93807aff3 (archived task 20261107114222-213d-solver-tool-wcs-output). The 23 non-WCS cases pass unchanged; 6 `wcs_file_*` cases replaced by 5 `wcs_output_*` cases (exit code 3 removed with its case), 3 cases extended.

## Findings
None.

## Manual checks and limitations
Linux x86_64 only; no macOS or Windows build in this task. A stdout write failure is not detected (specified, as json and xml).

## Verification verdict rationale
All acceptance criteria verified by the suite; a real frame confirms the header matches the json solution.

## Register reconciliation
No FND records.
