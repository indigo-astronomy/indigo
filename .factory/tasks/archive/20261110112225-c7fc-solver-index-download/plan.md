# Development plan

Status: complete

## Approach and affected components
Move the agent's index fetch (curl/WinINet), validation, partial file cleanup and abort into the solver library
(`indigo_drivers/agent_platesolver/indigo_solver.[ch]`, compiled into the agent and into the tools), switch the agent
to it, then add list/download/remove modes to `indigo_tools/indigo_solver.c`.

## Baseline
- Commands and environment: Linux aarch64 (Debian, kernel 6.12), gcc, curl; `indigo_test` unit and integration suites.
- Existing failures: no baseline run in this task before editing (see test-plan.md).
- Repository revision / working-tree state: master 7ea2c598099956c04fd82ca6a21c39c9ac76faf8 (equal to origin/master),
  clean apart from the untracked owner folders `indigo3-3.0-369~beta*-arm64/`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Library API in `indigo_drivers/agent_platesolver/indigo_solver.[ch]` | AC-001, AC-002 | None | unit tests | done |
| 2 | Agent uses the API, version 0x03000020 (`indigo_agent_platesolver.c`) | AC-003 | 1 | agent suite, live check | done |
| 3 | Tool modes (`indigo_tools/indigo_solver.c`, `indigo_solver.vcxproj`) | AC-004..AC-008 | 1 | tool suite | done |
| 4 | Tests (`indigo_test/unit/test_agent_platesolver_core.c`, `indigo_test/integration/test_indigo_solver_tool.c`) | all | 1-3 | suites, mutations | done |

## Test strategy
Unit tests of the library with `file://` URLs and a FIFO source; integration tests of the tool through the built
executable; the existing agent suite for regressions; a manual live check of the agent against the release URL.

## Security risks and mitigations
See security.md: untrusted downloaded content is validated before rename; curl is executed without a shell.

## Dependencies and architecture decisions
No new dependency; wininet.lib added to the tool's Windows project (already used by the agent).

## Migration, rollout, and rollback
Not applicable: no file format or property change; revert the commit to roll back.

## Deviations and owner decisions
Discovered during the live check: curl inherited the INDIGO server's blocked SIGTERM/SIGINT/SIGHUP/SIGCHLD mask, so
an agent abort never stopped a running download (pre-existing in the moved code). Fixed in the library by clearing the
mask in the child, as `indigo_client.c` does; a regression test blocks those signals around the download.
