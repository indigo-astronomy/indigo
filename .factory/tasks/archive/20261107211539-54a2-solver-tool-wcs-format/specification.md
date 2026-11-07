# Specification

Status: approved by the owner (2026-11-07, "yes add -f wcs")

## Problem and outcome
`-w` mixed a side file with the stdout format. `-f wcs` writes the solution as a WCS FITS header alone on stdout.

## In scope
The `-f/--format` value `wcs`, removal of `-w/--wcs`, exit codes, help text, tests.

## Out of scope
WCS header content and conventions (unchanged from 93807aff3), other tools.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `-f wcs` / `--format wcs` on a solved image writes exactly the 2880 byte header (printable ASCII, END, space padding) to stdout and nothing else; exit 0 | integration test |
| AC-002 | The header puts every rendered star on its catalog position, normal and mirrored frame, RAW and FITS | integration test |
| AC-003 | The header is J2000 when `-e 0` is given | integration test |
| AC-004 | `-v` progress goes to stderr; stdout stays the header alone | integration test |
| AC-005 | Not solved, unsupported or missing image: nothing on stdout, the reason on stderr, exit 1 | integration test |
| AC-006 | Wrong arguments with `-f wcs`: nothing on stdout, exit 2; `-w` is an unknown option; `WCS` (upper case) is refused like `JSON` | integration test |
| AC-007 | Help lists `wcs` in the format option, describes it and the exit codes 0/1/2; `-w` and exit code 3 are gone | integration test |

## Failure behavior and edge cases
A write error on stdout is not detected, the same as json and xml. A shell redirect creates or truncates its file even when the image is not solved; the exit code tells.

## Permissions, data, and security requirements
The tool no longer opens a user-named output file; stdout only.

## Compatibility and nonfunctional requirements
`-w` (published on origin/master in 93807aff3 on 2026-11-07, after release 3.0-10 of 2026-11-04, so not in any release) is removed without a compatibility alias. The header has no newline bytes, so text mode stdout on Windows does not alter it.

## Assumptions and open questions
None blocking.

## Owner decisions and requirement changes
2026-11-07: owner approved replacing `-w` by `-f wcs` (conversation).
