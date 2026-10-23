# Specification

Status: approved

## Problem and outcome
See brief.md.

## In scope
One rule bullet in root `AGENTS.md`.

## Out of scope
Factory routines, scripts, other users' workflows.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Root AGENTS.md states that, when working for Rumen G. Bogdanovski, no commit is made before the task is complete, and that at the end the planned commits with files and proposed messages are presented for permission, together with the results of any tests run (running tests first is optional), and committed exactly as approved. | Review of the diff |
| AC-002 | The rule states its precedence over the "then commit" instructions in AGENTS.md and `.factory/routines/git.md`. | Review of the diff |
| AC-003 | No other line changes; Factory checks stay green. | `git diff --stat`, `./.factory/scripts/check` |

## Failure behavior and edge cases
Not applicable (documentation rule).

## Permissions, data, and security requirements
Publishes the requester's Git name and e-mail in AGENTS.md; both are already in every commit in the repository.

## Compatibility and nonfunctional requirements
Not applicable.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-10-22, Rumen G. Bogdanovski: rule requested; the due full audit deferred until after this task.
