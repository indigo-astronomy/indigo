# Task quality review

Verdict: pass
Reviewer and independence: fresh general-purpose subagent (same host and model), read-only, independent of the implementation context
Reviewed revision and working-tree state: master 5fcbb2697 plus the uncommitted task diff (2026-11-06); fixes re-verified by the implementer

## Scope and method
Static review of the diff of `indigo_tools/indigo_solver.c` and `indigo_test/integration/test_indigo_solver_tool.c`
against master, the escape helpers in `indigo_libs/indigo_json.c` and `indigo_libs/indigo_xml.c`, and the root AGENTS.md
formatting and C rules. Focus: text-mode compatibility, document validity, pointer lifetimes, tests.

## Quality findings
| ID | Severity | Location | Finding | Disposition |
| --- | --- | --- | --- | --- |
| R1 | low | `print_json`/`print_xml` | `%f` would print `nan`/`inf` for a non-finite value | kept: the solver returns finite values for an accepted solution |
| R2 | low | `solve()` | NULL message now reads "Unknown error" instead of "(null)" | kept, unreachable in practice and better |
| R3 | medium | file name escaping | control characters stay unescaped in JSON | fixed: `document_file_name()` |
| R4 | medium | file name escaping | control characters are illegal in XML 1.0 | fixed: same |
| R5 | low/medium | file name escaping | bytes that are not UTF-8 break both documents | fixed: same |
| R6 | medium | tests | no check that an argument error gives no document | fixed: `structured_output_not_given_for_wrong_arguments` |
| R7 | low | tests | documents not parsed by a real parser | kept: framing and every field are checked; one-off parse with Python recorded in verification.md |
| R8 | low | tests | escaping not tested for control characters | fixed |
| R9 | low | tests | "Not solved" stderr line not checked in json mode | fixed |
| R10 | low | tests | `unlink` skipped on failure | kept: the private directory is removed at teardown |

No style findings. The review confirmed: `result->message` always points to string literals, `result` is zero-initialised,
every path after argument parsing prints exactly one document, text mode keeps master's output, and the long escape
buffers do not truncate.

## Fix reinspection
`document_file_name()` copies valid UTF-8 sequences, and replaces control characters, DEL, invalid lead or continuation
bytes and sequences cut by the buffer end with `?`. Covered by the extended escaping test and the sanitizer run, see
verification.md.

## Quality verdict
Pass. Remaining limitation: R1 and R2 are accepted as unreachable.
