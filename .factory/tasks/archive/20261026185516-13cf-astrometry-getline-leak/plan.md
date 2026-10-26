# Development plan

Status: final

## Approach and affected components
`free(line)` after the `getline()` loop in `execute_command()`; version 2.0.0.34.

## Baseline
- Commands and environment: valgrind on `build/integration/test_agent_astrometry "solve with filter"`, Linux x86_64.
- Existing failures: 3 × 120 bytes definitely lost per solve.
- Repository revision / working-tree state: master `b6cf75efa`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | fix and version in `indigo_agent_astrometry.c` | AC-001 | None | valgrind | done |
| 2 | recorded run, REFACTOR.md | AC-002 | 1 | run | done |

## Test strategy
Valgrind before/after; recorded run.

## Security risks and mitigations
None.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
None recorded.
