# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | unit `indigo_test/factory/test_git_compat.py::test_new_git_always_disables_lazy_fetch` | patched version 2.44, temporary repository | `['--no-lazy-fetch']` | with implementation |
| T-002 | AC-002 | unit `test_old_git_without_promisor_omits_the_flag`, `test_unknown_git_version_is_treated_as_old`, `test_version_parsing` | patched version 2.43 / None | `[]` | with implementation |
| T-003 | AC-003 | unit `test_old_git_in_partial_clone_fails_closed`, `test_old_git_with_invalid_promisor_value_fails_closed`, `test_old_git_never_lazy_fetches_from_a_filtered_clone`; existing `test_findings.py::test_missing_pinned_tree_never_starts_lazy_fetch` | promisor, partialClone, partialCloneFilter, real filtered clone | ValueError / rejected, no fetch | with implementation; T-003 filter case added after review F1 |
| T-004 | AC-002, AC-004 | integration: full Factory suite `./.factory/scripts/test` on real git 2.43.0; `test_installed_git_accepts_the_returned_options` | host Git | 205/205 | after implementation |
| T-005 | AC-005 | suite with `PYTHONDONTWRITEBYTECODE` unset | host | 205/205 | after implementation |

## Baseline evidence
`./.factory/scripts/test` at master 8c14701e7 on git 2.43.0: exit 1, 197 tests, 71 failures, 61 errors (verification.md).

## Bug reproduction
The baseline suite and `new-task` ("task ID history could not be checked") reproduce the failure before the fix.

## Negative cases and permission boundaries
Partial clones on old git (three config forms, plus an invalid bool) must fail closed. GIT_TRACE confirms that no fetch occurs (existing test).

## Isolation and cleanup
Temporary directories through `tempfile`. No network access: the filtered clone uses a local `file://` source.

## Deferred tests and coverage limitations
No real git ≥ 2.44, macOS or Windows host was available. The ≥ 2.44 branch is covered by a patched-version unit test.
