# Development plan

Status: complete

## Approach and affected components
`print_usage()` of `indigo_tools/indigo_solver.c`; test case `help_lists_switches_and_defaults`.

## Baseline
- Commands and environment: Linux aarch64, gcc; tool suite 32/32 at master 28f4474b2.
- Existing failures: none.
- Repository revision / working-tree state: master 28f4474b2, untracked owner folders only.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | help line and test | AC-001 | None | tool suite | done |

## Test strategy
Tool suite.

## Security risks and mitigations
None.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Revert the commit.

## Deviations and owner decisions
None.
