# Specification

Status: approved (owner request 2026-11-06; routine design choices made by the implementer)

## Problem and outcome
`indigo_solver` gains `-f, --format <format>` with `text` (default), `json` and `xml`.

## In scope
- `text`: unchanged output.
- `json` / `xml`: for every run that passes argument parsing, exactly one document on stdout: solved (exit 0), not solved
  or unreadable file (exit 1). The existing stderr messages are kept.
- With `json` / `xml`, the `-v` progress lines go to stderr, so stdout holds the document alone.
- Solved document fields: `file`, `solved`, `time` (s), `ra` (h), `ra_hms`, `dec` (°), `dec_dms`, `epoch` (`J2000`/`JNow`),
  `angle` (°), `scale` (°/pixel, unbinned), `field_width`, `field_height` (°), `image_width`, `image_height` (pixels),
  `parity`, `matched_stars`, `star_matches`, `rms` (arcsec), `index` (label).
- Not-solved document fields: `file`, `solved` (false), `time`, `message`.
- JSON: one object. XML: `<?xml version="1.0" encoding="UTF-8"?>` and a `<solution>` element with one child element
  per field, named like the JSON keys.
- Strings are escaped with `indigo_json_escape_b` / `indigo_xml_escape_b` (long buffer).
- The help lists the switch and explains where the document and the progress go.

## Out of scope
Output to a file, other formats, schema files, changes to the Platesolver Agent.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Text output and exit codes are unchanged (the 17 existing tool tests pass unchanged) | integration suite |
| AC-002 | `-f json` of a solved frame: stdout is one JSON object with all solved fields, values match the frame and the text output | `json_output_of_a_solution` |
| AC-003 | `-f xml` of a solved frame: stdout is one XML document with the same fields and values | `xml_output_of_a_solution` |
| AC-004 | The epoch is named (`J2000` / `JNow`) in both formats | `structured_output_names_the_epoch`, AC-002/003 |
| AC-005 | Not solved, unsupported image and unreadable file give a document with `solved` false and `message`, exit 1; text mode prints nothing on stdout | `structured_output_when_not_solved` |
| AC-006 | `-v` progress goes to stderr with json/xml and stays on stdout with text | `json_output_of_a_solution`, `depth_hint_and_verbose_progress` |
| AC-007 | File names with `"`, `&`, `<`, `>`, `\` are escaped; control characters and non-UTF-8 bytes become `?`, valid UTF-8 stays | `structured_output_escapes_the_file_name` |
| AC-009 | Argument errors (exit 2) give no document | `structured_output_not_given_for_wrong_arguments` |
| AC-008 | `-h` lists `--format`; unknown format values are refused with exit 2 | `help_lists_switches_and_defaults`, `invalid_arguments_are_refused` |

## Failure behavior and edge cases
Argument errors (exit 2) produce no document. A NULL reason from the image loader is reported as "Unknown error".
The library escape helpers keep control characters (except `\n`, `\r`, `\t` in JSON) and bytes that are not UTF-8,
which would make a document invalid; the tool replaces them with `?` in the file name before escaping. The other strings
are fixed texts of the tool and the library.

## Permissions, data, and security requirements
The file name is untrusted text and is escaped. No new file, network or privilege access.

## Compatibility and nonfunctional requirements
Portable C, no new dependency (the tool already links libindigo, which provides both helpers).

## Assumptions and open questions
None blocking.

## Owner decisions and requirement changes
- 2026-11-06, owner: "please add json and xml output of the solution of the indigo_solver cli tool".
