# Development plan

Status: complete

## Approach and affected components
`git mv` into `indigo_drivers/agent_scheduler/doc/`; update the references; move the Xcode file reference into a new `doc` group (path `doc`) of the `agent_scheduler` group, keeping its ID.

## Baseline
- Branch `scheduler` at 4d86bff9c.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Move the file and fix its own link | AC-001 | None | git diff -M | done |
| 2 | Update references | AC-002 | 1 | link check, grep | done |
| 3 | Xcode groups | AC-003 | 1 | diff | done |

## Test strategy
Link and anchor check of all touched documents; grep for the old path outside archives.

## Security risks and mitigations
None.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
None.
