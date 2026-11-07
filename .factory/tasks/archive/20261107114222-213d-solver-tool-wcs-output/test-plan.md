# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002, AC-004 | integration, `wcs_file_of_a_solution` | synthetic RAW and FITS frame, parity -1 | file valid, stars within 10" (RAW) / 30" (FITS, default binning), text output unchanged | after implementation |
| T-002 | AC-002 | `wcs_file_of_a_mirrored_frame` | RAW frame with reversed columns | parity 1, stars within 10" with mirrored x | after implementation |
| T-003 | AC-003 | `wcs_file_of_a_jnow_solution` | `-e 0` | JNow printed, file maps stars in J2000 | after implementation |
| T-004 | AC-004 | `wcs_file_with_structured_output` | `-f json` | stdout is the document, file valid | after implementation |
| T-005 | AC-005 | `wcs_file_not_written_when_not_solved` | wrong position hint; text file | exit 1, no file | after implementation |
| T-006 | AC-006 | `wcs_file_that_cannot_be_written` | path in a missing directory | exit 3, solution printed, error | after implementation |
| T-007 | AC-007 | `invalid_arguments_are_refused`, `help_lists_switches_and_defaults` | — | exit 2 / help text | after implementation |
| T-008 | all existing contracts | the 23 existing cases | — | unchanged pass | baseline and final |

## Baseline evidence
See verification.md: 23/23 before the test change.

## Bug reproduction
Not applicable: new feature. The mutation check in verification.md shows the new cases fail on a wrong CD matrix.

## Negative cases and permission boundaries
T-005, T-006, T-007.

## Isolation and cleanup
All files under the suite's `mkdtemp` directory, removed by `cleanup()`; `INDIGO_CACHE_BASE` points into it.

## Deferred tests and coverage limitations
The suite frame has one rotation (20°); other rotations are covered by the out-of-suite synthetic check only. No Windows or macOS run in this task.
