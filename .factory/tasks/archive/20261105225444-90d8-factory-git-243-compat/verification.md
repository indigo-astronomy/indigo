# Verification report

Verdict: pass
Reviewer: implementing agent (Claude Code, claude-opus-5-5); independent diff review by a fresh subagent, see quality-review.md
Date: 2026-11-06
Revision and working-tree state: base master 8c14701e743ff41fce5a8ecbcc3f81e4d894b8f3, uncommitted task changes in worktree /home/rumen/work/indigo-task-factory-git
Review independence: the verifier also implemented the change. An independent reviewer covered the diff (quality-review.md).

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `test_new_git_always_disables_lazy_fetch` (also with a promisor configured); `grep` finds no hard-coded flag outside `git_compat.py` |
| AC-002 | pass | `test_old_git_without_promisor_omits_the_flag`, `test_unknown_git_version_is_treated_as_old`, `test_installed_git_accepts_the_returned_options`; full suite on real git 2.43.0 |
| AC-003 | pass | `test_old_git_in_partial_clone_fails_closed` (partialClone, promisor, partialCloneFilter on a remote name with a dot), `test_old_git_with_invalid_promisor_value_fails_closed`, `test_old_git_never_lazy_fetches_from_a_filtered_clone` (a real `--filter=blob:none` clone with `promisor` unset), and the existing `test_findings.test_missing_pinned_tree_never_starts_lazy_fetch` (GIT_TRACE has no fetch) |
| AC-004 | pass | `new-task` (created this task record without a shim), `new-task SLUG --print-id`, `findings list`, `maintenance status` (failed before with "local Git operation unavailable: rev-parse"), and the check/test/security groups |
| AC-005 | pass | `env -u PYTHONDONTWRITEBYTECODE ./.factory/scripts/test`: 205/205 |

## Executed checks
All commands were run at the worktree root on Linux x86_64, git 2.43.0, Python 3.12.

| Command | Environment | Exit code | Result |
| --- | --- | --- | --- |
| `./.factory/scripts/test` (baseline, master) | default | 1 | 197 tests, 71 failures, 61 errors |
| `env -u PYTHONDONTWRITEBYTECODE ./.factory/scripts/test` (final) | bytecode setting unset | 0 | 205 tests OK |
| `./.factory/scripts/check` | default | 0 | Factory project boundaries valid |
| `./.factory/scripts/security` | default | 0 | Factory local security boundaries valid |
| `python3 -B -m unittest discover -s indigo_test/factory -p test_git_compat.py` | default | 0 | 8 tests OK |
| `env -u PYTHONDONTWRITEBYTECODE python3 -m unittest discover -s indigo_test/factory -p 'test_*.py'` (direct, bypassing `run_checks.py`) | bytecode enabled | 1 | 205 tests, 12 failures, all `__pycache__` in read-only snapshot tests (see below) |

## Baseline comparison and regressions
The baseline failures were mostly the rejected Git option. Some were `__pycache__` snapshot failures in
`test_task_archive`, which appeared because the helpers already imported sibling modules (`task_store`,
`maintenance`) on master. The configured runner now disables bytecode caches, so `./.factory/scripts/test` is
deterministic on any host. `findings.py`, the only helper this task made import a sibling, sets
`sys.dont_write_bytecode` itself. Its read-only tests also pass in the direct run. The 12 direct-run failures that
remain are in helpers that imported siblings before this task. They are not introduced here, and they do not occur
through the configured runner.

## Findings
The independent review's F1 (promisor via `partialCloneFilter`) was fixed and retested in this task, so no registry record is needed.
F2 was partly fixed (`findings.py`); the remainder is the pre-existing condition described above. F3 (test gaps) was fixed.

## Manual checks and limitations
Not run on macOS or Windows, or on git ≥ 2.44 (no such host available). The ≥ 2.44 branch is covered by a unit test with a patched version.

## Verification verdict rationale
All acceptance criteria pass on the affected host, and the safety contract is covered by real-repository tests.

## Register reconciliation
No canonical FND records were created or changed.
