# Task quality review

Verdict: pass after fixes
Reviewer and independence: fresh general-purpose subagent (same host and model), read-only, independent of the implementation context
Reviewed revision and working-tree state: base 8c14701e7 + uncommitted task diff (2026-11-06). The fixes for F1–F3 were re-verified by the implementer.

## Scope and method
Reviewed `git diff master` plus the new `git_compat.py` and `test_git_compat.py`: promisor detection completeness,
call-site coverage, exception and exit-code paths, fail-closed behavior, the bytecode change, and test quality. The
reviewer ran the Factory suite and reproduced lazy fetching on git 2.43 in a temporary repository.

## Findings
| ID | Severity | Location | Finding | Disposition |
| --- | --- | --- | --- | --- |
| F1 | high | `git_compat.py` `has_promisor_remote` | `remote.<name>.partialclonefilter` alone makes a promisor remote. Old git lazy-fetched after `promisor` was unset (reproduced) | fixed: the filter key is treated as a promisor; real-clone regression test added |
| F2 | low | `run_checks.py` | The bytecode setting hides a new `__pycache__` write by `findings.py` in direct test runs | fixed for `findings.py` (`sys.dont_write_bytecode`); the remaining direct-run failures predate this task (verification.md) |
| F3 | low | `test_git_compat.py` | Missing tests for partialCloneFilter and an invalid bool | fixed |

Confirmed sound: key-case normalization, remote names with dots, config includes/worktree/`-c`, invalid bools failing
closed, matching cwd/`-C`, no missed call sites, and every new ValueError handled by an existing path with exit 2.

## Verdict
No open blocking issues. Note: `git_compat.py` is a local divergence from the pinned template (handoff.md).
