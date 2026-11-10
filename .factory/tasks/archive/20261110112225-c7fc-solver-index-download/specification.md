# Specification

Status: approved by the owner request of 2026-11-10 (see brief.md)

## Problem and outcome
Index download, validation and removal of partial files become solver library functions shared by the Platesolver
Agent and the `indigo_solver` tool; the tool gains index management modes.

## In scope
- Library: published base URL and approximate sizes, `indigo_solver_index_url()`, `indigo_solver_validate_index()`,
  `indigo_solver_remove_partial_indexes()`, `indigo_solver_download_index()` with an abortable
  `indigo_solver_download` state and `indigo_solver_abort_download()`.
- Agent: uses the library; behaviour of `X_AGENT_SOLVER_INDEX` (messages, states, abort, exit kill) preserved.
- Tool: `-l, --list`, `--download <scales|all>`, `--remove <scales|all>`, `--index-url <url>`.

## Out of scope
Progress bars, resume, checksums, new index scales.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `indigo_solver_download_index()` fetches `<base>/<index file name>` into `<path>.part`, accepts it only with a valid header of the requested scale and renames it into place; on any failure or abort no `.part` and no index file is left and a reason is returned | unit tests with `file://` URLs |
| AC-002 | `indigo_solver_abort_download()` stops a running download from another thread or a signal handler | unit test aborting a download of a FIFO that never ends |
| AC-003 | The agent downloads, validates, removes and aborts indexes as before, through the library | existing agent suites, build, diff review |
| AC-004 | `indigo_solver --list` lists every published index with its label, approximate size and installed state in the index directory | tool integration test |
| AC-005 | `indigo_solver --download <scales>` installs the missing indexes (creating the directory), skips installed ones, exits 0 on success and 1 when any download fails, leaving no partial file | tool integration test with `--index-url file://...` |
| AC-006 | `indigo_solver --remove <scales>` removes the installed indexes, a missing one is reported and not an error | tool integration test |
| AC-007 | The modes are mutually exclusive with each other and with an image; solving options are refused with a mode; `--index-url` only with `--download`; wrong use exits 2 | tool integration test |
| AC-008 | Ctrl-C (SIGINT) during `--download` stops the transfer, removes the partial file and exits 1 | tool integration test sending SIGINT |

## Failure behavior and edge cases
Unknown scale, empty list, unwritable directory, missing curl (reported as such), HTTP error, invalid content.

## Permissions, data, and security requirements
The URL is the fixed GitHub release URL unless `--index-url` is given by the local user; downloaded content is
untrusted and accepted only after header validation; curl runs without a shell (execlp), so the URL is never
interpreted by a shell. Removal touches only `index-*deg-*.isix` paths derived from known scales.

## Compatibility and nonfunctional requirements
Linux/macOS use curl as before, Windows uses WinINet as before; the tool's Windows project links wininet.lib.

## Assumptions and open questions
Windows build not verifiable on this host (assumption, recorded in verification.md).

## Owner decisions and requirement changes
2026-11-10: library first, then the tool (owner).
