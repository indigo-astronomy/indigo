# Test and regression plan

Status: done

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002 | `wcs_output_of_a_solution` | synthetic RAW and FITS | header alone, stars matched, stderr empty | after implementation |
| T-002 | AC-002 | `wcs_output_of_a_mirrored_frame` | mirrored RAW | mirrored stars matched | after |
| T-003 | AC-003 | `wcs_output_of_a_jnow_solution` | `-e 0` | J2000 header matches catalog | after |
| T-004 | AC-004 | `wcs_output_with_verbose_progress` | `-v` | progress on stderr, no "Solved in", header valid | after |
| T-005 | AC-005 | `wcs_output_when_not_solved` | wrong hint, text file, missing file | empty stdout, reason, exit 1 | after |
| T-006 | AC-006 | `structured_output_not_given_for_wrong_arguments`, `invalid_arguments_are_refused` | bad arguments | exit 2, empty stdout, `-w` unknown | after |
| T-007 | AC-007 | `help_lists_switches_and_defaults` | `-h` | new texts | after |
| T-008 | existing contracts | the other 23 cases | unchanged | pass | after |

## Baseline evidence
29/29 at 93807aff3 (archived task 20261107114222-213d-solver-tool-wcs-output, verification.md).

## Bug reproduction
Before: `indigo_solver -w x.wcs image` printed the text solution on stdout as well (seen in the removed `wcs_file_of_a_solution`, which asserted the text output).

## Negative cases and permission boundaries
T-005, T-006.

## Isolation and cleanup
The suite works in its private temporary directory and removes it.

## Deferred tests and coverage limitations
A stdout write failure is not tested (not detected, as for json and xml).
