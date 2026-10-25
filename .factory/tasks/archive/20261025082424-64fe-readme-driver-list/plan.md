# Development plan

Status: complete

## Approach and affected components
README.md only.

## Baseline
- Commands and environment: macOS checkout at origin/master `64380bf9e`.
- Existing failures: not applicable.
- Repository revision / working-tree state: clean.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Compare driver directories with README list | AC-002 | none | mapping recorded in verification.md | done |
| 2 | Edit README.md | AC-001, AC-002 | 1 | review | done |

## Test strategy
Documentation review; package name checked on Debian 13.

## Security risks and mitigations
None.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Revert the README hunk.

## Deviations and owner decisions
None.
