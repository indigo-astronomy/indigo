# Specification

Status: approved by the owner decisions of 2026-10-30

## Problem and outcome
`indigo_agent_solver` sends `indigo_send_message(device, IDLE_PROPERTY, "Solving with index %s...", label)` before
every index it tries. The line is moved to the driver debug log (`INDIGO_DRIVER_DEBUG`), so clients no longer receive
it while debug logging still shows the search order.

## In scope
- `indigo_drivers/agent_solver/indigo_agent_solver.c`: message demoted, `DRIVER_VERSION` incremented.
- `indigo_test/integration/test_agent_solver.c`: the search-order tests read the line from the debug log.

## Out of scope
`indigo_tools/indigo_solver.c` stdout output and its tests; other agent messages; search order and solve behavior.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | No client message containing "Solving with index" is sent during a solve | integration test: client messages checked while the debug log is captured separately |
| AC-002 | At debug log level the agent logs "Solving with index <label>..." for every index tried, in search order | the existing order assertions of `blind_search_goes_stage_by_stage` and `scaled_search_uses_fitting_index_first`, fed from the debug log |
| AC-003 | Solve results and all other agent behavior unchanged | full `agent_solver` driver test run and solver unit tests pass |
| AC-004 | `DRIVER_VERSION` higher than 0x03000018 | source inspection |

## Failure behavior and edge cases
Unchanged: a failing index load still logs an error and the search continues.

## Permissions, data, and security requirements
No new input, path or network access; the debug line carries the same index label as before.

## Compatibility and nonfunctional requirements
A client that parsed this message to show progress no longer gets it; the owner requested exactly that. Busy state,
"Solved" and failure messages are unchanged. Portable C only.

## Assumptions and open questions
None blocking.

## Owner decisions and requirement changes
2026-10-30, Rumen G. Bogdanovski: demote to debug rather than delete; the tool keeps its stdout line; keep the agent
order assertions through a debug log capture in the test file.
