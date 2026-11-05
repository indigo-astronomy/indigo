# Specification

Status: approved by request (owner instruction 2026-11-05: "fix it so that it would work with 2.43 and keep it like this").

## Problem and outcome
The Factory helpers must work with git 2.43 and must not lose their guarantee that a helper never lazy-fetches
objects from a promisor remote.

## In scope
All seven helper modules that pass `--no-lazy-fetch`, the shared fixtures that copy helper subsets, the check
runner, and the routine text.

## Out of scope
Product code, the upstream template, and any change to which Git commands the helpers run.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | With git ≥ 2.44, every helper still passes `--no-lazy-fetch`. | `test_git_compat.test_new_git_always_disables_lazy_fetch`; code review |
| AC-002 | With git < 2.44 (or an unparseable version) in a repository without a promisor remote, the helpers run without the flag. | `test_git_compat` old-git cases; full Factory suite on git 2.43.0 |
| AC-003 | With git < 2.44 in a partial clone (`extensions.partialClone` or `remote.*.promisor=true`), the helpers fail closed and start no fetch. | `test_git_compat.test_old_git_in_partial_clone_fails_closed`; `test_findings.test_missing_pinned_tree_never_starts_lazy_fetch` (GIT_TRACE shows no fetch) |
| AC-004 | `new-task`, `findings`, `maintenance status`, `check`, `test` and `security` succeed on this host. | Executed checks in verification.md |
| AC-005 | Configured checks give the same result regardless of the host's bytecode setting. | Suite run with `PYTHONDONTWRITEBYTECODE` unset |

## Failure behavior and edge cases
An error reading Git configuration fails closed (ValueError → the helper's existing error path). `findings.py`
treats it as an unavailable Git object, which is already a check failure.

## Permissions, data, and security requirements
No network access. Configuration reads (`git config`) do not touch objects and cannot fetch.

## Compatibility and nonfunctional requirements
Linux, macOS and Windows Git version strings (`2.44.0.windows.1`) are parsed. `--type=bool` needs git ≥ 2.18.

## Assumptions and open questions
Lazy fetching requires a promisor remote configured through `extensions.partialClone` or `remote.<name>.promisor`
(Git `partial-clone` documentation). No open questions.

## Owner decisions and requirement changes
- 2026-11-05, Rumen G. Bogdanovski: fix the helpers permanently for git 2.43.
