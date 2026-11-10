# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | unit `index_url`, `index_validation`, `remove_partial_indexes` | temp dirs | URL forms, header checks, only `.part` of known scales removed | after implementation |
| T-002 | AC-001 | unit `download_index` | `file://` mirror | success; missing source, junk, wrong scale and pre-aborted state fail with their reason and leave no file | after implementation |
| T-003 | AC-002 | unit `download_index_abort` | FIFO source, server signal mask blocked | abort from another thread ends the download within 3 s, no `.part` | after implementation; failed before the mask fix |
| T-004 | AC-004..AC-006 | integration `index_management_lists_downloads_and_removes` | `file://` mirror | list states, directory creation, download, solve with it, keep installed, invalid listed, remove | after implementation |
| T-005 | AC-005 | integration `index_download_failures` | `file://` mirror | exit 1, reasons, other scales still installed, no partial files, uncreatable directory | after implementation |
| T-006 | AC-007 | integration `index_management_arguments_are_refused` | none | exit 2 with the messages, help text | after implementation |
| T-007 | AC-008 | integration `interrupted_download_removes_partial_file` | FIFO source, SIGINT | exit 1, no partial file, next scale not downloaded, no curl left | after implementation |
| T-008 | AC-003 | integration `test_agent_platesolver` (existing) + manual live check | private server | unchanged results; download, abort, removal via X_AGENT_SOLVER_INDEX | after implementation |

## Baseline evidence
No separate baseline run was made in this task before editing; the suites had 29 agent, 29 core unit and 28 tool cases at HEAD and were recorded passing in earlier solver tasks (archived evidence), not re-run here before editing. After the change: 29, 34 and 32 cases, all passing (verification.md).

## Bug reproduction
T-003 with the server signal mask failed before the library fix (`elapsed < 3` assertion, curl kept running until the
source was closed); live agent abort did not stop curl (`/proc/<curl>/status` SigBlk 0x14003).

## Negative cases and permission boundaries
Invalid scales, empty values, conflicting modes, solving options with modes, `--index-url` without `--download`.

## Isolation and cleanup
Temporary directories under /tmp removed by the tests; FIFO opened `O_CLOEXEC` so curl never holds its write end.

## Deferred tests and coverage limitations
Windows (WinINet path) and macOS not built or run on this host; HTTPS covered by the manual live check only.
