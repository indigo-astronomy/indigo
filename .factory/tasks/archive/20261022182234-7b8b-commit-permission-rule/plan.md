# Development plan

Status: executed

## Approach and affected components
Add one bullet after the never-rewrite rule in root `AGENTS.md`.

## Baseline
- Commands and environment: Linux x86_64; Factory scripts through the Git wrapper described in task 008.
- Existing failures: `./.factory/scripts/test` cannot pass on this host (Git 2.43, see task 008 verification C20).
- Repository revision / working-tree state: `80d073bcb` (master), branch `task/20261022182234-7b8b-commit-permission-rule`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Rule bullet in `AGENTS.md` | AC-001, AC-002 | None | diff review | done |
| 2 | Checks and records | AC-003 | 1 | check group | done |

## Test strategy
Review and Factory check group; no executable tests for a prose rule.

## Security risks and mitigations
None beyond the already public name and e-mail.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Revert the bullet.

## Deviations and owner decisions
None recorded.
