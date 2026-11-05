# Development plan

Status: complete

## Approach and affected components
Add a shared module, `.factory/scripts/git_compat.py`, whose `no_lazy_fetch(root)` returns the option list. Replace
each hard-coded `--no-lazy-fetch` with it, add the module to the helper fixtures, the risk list and the documentation,
and make the check runner disable bytecode caches.

## Baseline
- Commands and environment: `./.factory/scripts/test` on Linux x86_64, git 2.43.0, Python 3.12, master `8c14701e7`.
- Existing failures: 197 tests, 71 failures and 61 errors (Git option rejected, plus pre-existing `__pycache__` snapshot failures).
- Repository revision / working-tree state: clean task worktree from master `8c14701e743ff41fce5a8ecbcc3f81e4d894b8f3`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `git_compat.py` | AC-001…003 | — | Unit tests | done |
| 2 | Call sites: `findings.py`, `git_snapshot.py`, `metadata_upgrade.py`, `new_task.py`, `task_store.py`, `task_upgrade.py`, `template_update.py` | AC-002, 004 | 1 | No remaining hard-coded flag (`grep`) | done |
| 3 | Fixtures that copy helper subsets, plus `test_git_compat.py`, registered in Xcode | AC-001…003 | 2 | Suite | done |
| 4 | `run_checks.py` sets `PYTHONDONTWRITEBYTECODE=1` | AC-005 | 3 | Suite with the variable unset: 203/203 | done |
| 5 | `risk_check.py` lists the new module; `README.md` and `template-updates.md` state the rule | — | 2 | check/security groups | done |
| 6 | Independent review, records, owner commit approval | — | 5 | quality-review.md, security.md | in progress |

## Test strategy
Unit tests with a patched Git version against real temporary repositories, plus the full Factory suite on real git 2.43.

## Security risks and mitigations
The risk is a helper that lazy-fetches in a partial clone on old Git. It is mitigated by failing closed whenever a promisor remote is configured.

## Dependencies and architecture decisions
No new dependencies. The scripts are template-mapped files, so this is a local divergence from the pinned template. The next template
update must merge it three-way or adopt it upstream.

## Migration, rollout, and rollback
Rollback: revert the commit. No data migration.

## Deviations and owner decisions
`run_checks.py` was not in the original scope. It was added because, on this host, read-only snapshot tests fail on bytecode caches. Some of those
failures predate this task, and `findings.py`'s new sibling import would have added more.
