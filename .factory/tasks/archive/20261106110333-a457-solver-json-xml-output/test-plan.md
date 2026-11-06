# Test and regression plan

Status: done

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | existing 17 cases of `indigo_test/integration/test_indigo_solver_tool.c` | Synthetic data | pass unchanged | After implementation, before new tests |
| T-002 | AC-002, AC-006 | `json_output_of_a_solution` | Synthetic data | one object, values match frame and text run, progress on stderr | After implementation |
| T-003 | AC-003, AC-006 | `xml_output_of_a_solution` | Synthetic data | one document, values match frame; `-f text` unchanged | After implementation |
| T-004 | AC-004 | `structured_output_names_the_epoch` | Synthetic data | `JNow` in both formats | After implementation |
| T-005 | AC-005 | `structured_output_when_not_solved` | Synthetic data | `solved` false, message, exit 1; text stdout empty | After implementation |
| T-006 | AC-007 | `structured_output_escapes_the_file_name` | copies of the frame with special characters, control characters and non-UTF-8 bytes in the name | escaped name, `?` for the invalid bytes | After implementation |
| T-008 | AC-009 | `structured_output_not_given_for_wrong_arguments` | none | exit 2, empty stdout, message on stderr | After review |
| T-007 | AC-008 | `help_lists_switches_and_defaults`, `invalid_arguments_are_refused` | none | help lists `--format`; `yaml`, `JSON` refused | After implementation |

## Baseline evidence
See verification.md.

## Bug reproduction
Not applicable (new feature).

## Negative cases and permission boundaries
Unknown format values, unreadable file, unsupported file, no index files.

## Isolation and cleanup
The suite works in a private `mkdtemp` directory with `INDIGO_CACHE_BASE` pointing into it and removes it afterwards.

## Deferred tests and coverage limitations
The suite does not parse the documents with a JSON/XML parser; it checks the framing and every field. Every document the
suite produced was parsed once with Python `json` and `xml.etree` (verification.md).
