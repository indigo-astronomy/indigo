# Specification

Status: approved by the owner's request (2026-11-07); routine choices below are the coordinator's

## Problem and outcome
`indigo_solver` writes the solution as a WCS FITS header file on request.

## In scope
`-w, --wcs <file>` of `indigo_tools/indigo_solver.c`, help and exit code documentation in `-h`, tests.

## Out of scope
WCS in the image file, SIP terms, other output formats, the agents and the web or Qt applications.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | With `-w <file>` a solved image produces `<file>`: a FITS header without data (SIMPLE T, BITPIX 8, NAXIS 0), printable ASCII 80 character cards up to END, padded with spaces to 2880 bytes | test `wcs_file_of_a_solution`, `read_wcs_file()` |
| AC-002 | The file is a TAN projection (`RA---TAN`, `DEC--TAN`, ICRS, EQUINOX 2000) with CRPIX at the image centre ((w + 1) / 2, (h + 1) / 2), IMAGEW/IMAGEH, and CRVAL/CD such that every star of the frame maps from its FITS pixel to its catalog position within the solve accuracy, for both parities and any rotation | tests `wcs_file_of_a_solution`, `wcs_file_of_a_mirrored_frame`; synthetic 8-orientation check in verification.md |
| AC-003 | The file is J2000 whatever `-e` selects for the printed result | test `wcs_file_of_a_jnow_solution` |
| AC-004 | The printed text, JSON and XML output is unchanged; with `-f json`/`xml` stdout still holds the document alone | tests `wcs_file_of_a_solution` (text checks), `wcs_file_with_structured_output` |
| AC-005 | No file is written when the image is not solved (exit 1) | test `wcs_file_not_written_when_not_solved` |
| AC-006 | A solved image whose file can't be created or written exits with 3 and an error on stderr; the solution is still printed | test `wcs_file_that_cannot_be_written` |
| AC-007 | `-w ''` and `-w` without a value are refused with exit 2; `-h` lists `--wcs`, the write-only-when-solved rule and exit code 3 | tests `invalid_arguments_are_refused`, `help_lists_switches_and_defaults` |
| AC-008 | Card layout and CD convention equal to Ain Platesolver `wcs_file_header()` (same keywords, order, comments, formula), CREATOR `indigo_solver` | review against `~/indigo_imager/ain_platesolver_src/wcsfile.cpp` |

## Failure behavior and edge cases
An existing file at the path is replaced. A file name with bytes outside printable ASCII appears in the COMMENT card with `?` for those bytes; the card is cut at 80 characters.

## Permissions, data, and security requirements
The tool writes only the path the user gives; no new input parsing beyond the option value.

## Compatibility and nonfunctional requirements
Portable C library calls only (`fopen`, `time`, `gmtime`, `strftime`). Existing options, outputs and exit codes 0/1/2 are unchanged.

## Assumptions and open questions
Low-risk assumptions: explicit file argument rather than an automatic `<image>.wcs`; exit code 3 for a failed write. No blocking questions.

## Owner decisions and requirement changes
2026-11-07: owner request "add wcs output to indigo solver" (session with the owner).
