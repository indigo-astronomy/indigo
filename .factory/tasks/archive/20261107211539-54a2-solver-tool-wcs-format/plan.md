# Development plan

Status: done

## Approach and affected components
Add `FORMAT_WCS`; turn `write_wcs()` into `print_wcs()` building the header in a stack buffer (stdout may be a pipe, so the old `ftell` padding cannot be used) and writing it with one `fwrite`; call it from the format switch only when solved; remove `wcs_file_name`, its parsing and the same-path check; update help. Rewrite the WCS test cases to redirect stdout to a file.

## Baseline
- Commands and environment: `make -C indigo_test test-indigo-solver-tool`, Ubuntu 24.04 x86_64
- Existing failures: none; 29/29 at 93807aff3 (archived task verification)
- Repository revision / working-tree state: master 41e74d877, clean task worktree

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `indigo_tools/indigo_solver.c` | AC-001..AC-007 | None | builds, real frame check | done |
| 2 | `indigo_test/integration/test_indigo_solver_tool.c` | AC-001..AC-007 | 1 | suite passes | done |

## Test strategy
Integration suite with the synthetic index and frames; one manual comparison of json and wcs on a real frame.

## Security risks and mitigations
Fewer than before: no output file is opened by the tool.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
`-w` users switch to `-f wcs > file`. Rollback: revert the commit.

## Deviations and owner decisions
None.
