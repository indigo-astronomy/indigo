# Development plan

Status: done

## Approach and affected components
`indigo_tools/indigo_solver.c`: `solve()` fills a `solver_result` instead of printing; `main()` prints it with
`print_text()`, `print_json()` or `print_xml()`. Progress lines use a `progress` stream (stdout for text, stderr otherwise).
`indigo_test/integration/test_indigo_solver_tool.c`: stdout-only runner and five new cases.

## Baseline
- Commands and environment: Linux x86_64, gcc 13; `make init && make -C indigo_libs all && make -C indigo_tools all`,
  the agent archive via `make -C indigo_drivers -f ../Makefile.drvs folders agent_platesolver/ OP=all`.
- Existing failures: none. The unchanged suite (17 cases) passed against the changed tool, see verification.md.
- Repository revision / working-tree state: task branch from master 5fcbb2697, uncommitted task diff.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | result struct, format switch, printers in `indigo_tools/indigo_solver.c` | AC-001..009 | None | builds without new warnings | done |
| 2 | unchanged suite against the new tool | AC-001 | 1 | 17/17 | done |
| 3 | new cases in `indigo_test/integration/test_indigo_solver_tool.c` | AC-002..008 | 1 | 22/22 | done |
| 4 | independent review; file name sanitizing and two more checks | AC-007, AC-009 | 3 | 23/23, quality-review.md | done |

## Test strategy
Integration tests run the built executable on the synthetic index and frames of the existing suite.

## Security risks and mitigations
Untrusted file name in the document: escaped with the library helpers.

## Dependencies and architecture decisions
No new dependency.

## Migration, rollout, and rollback
Not applicable: the default output is unchanged; revert the commit to roll back.

## Deviations and owner decisions
None recorded.
