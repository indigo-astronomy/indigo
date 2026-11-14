# Development plan

Status: complete

## Approach and affected components
Bonjour name and options, LAN address in the start-up lines, guide text.

## Baseline
- Branch `scheduler` at d23f31f4d.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Diagnose: control panel reachable, server not announced | — | None | curl, discover | done |
| 2 | Sandbox options and output | AC-001, AC-002 | 1 | discover, run | done |
| 3 | Guide | AC-003 | 2 | review | done |

## Test strategy
Run the sandbox, list it with `indigo_prop_tool discover`, fetch the control panel, rerun the night scenario.

## Security risks and mitigations
See specification.md.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
None.
