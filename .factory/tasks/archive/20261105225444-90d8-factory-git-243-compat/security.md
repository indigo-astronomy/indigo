# Security review

Verdict: pass
Reviewer: independent subagent review (quality-review.md) plus implementer reverification
Date: 2026-11-06
Revision and working-tree state: base 8c14701e7 + uncommitted task diff
Review independence: the review agent did not implement the change; the fix retest was done by the implementer.

## Scope, assets, and trust boundaries
The boundary is that Factory helpers read only local Git objects and never reach a remote. The asset is the network
and remote-access isolation of offline evidence checks (process-policy risk: `findings.py`, `git_snapshot.py`,
`task_store.py`, `task_upgrade.py`, `metadata_upgrade.py`, `risk_check.py`).

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No credentials or remote access added |
| Input, injection, and output handling | Yes | Fixed argument arrays, no shell; the config regex is passed as an argument |
| Data, secrets, cryptography, and logs | No | Error messages do not echo config values |
| Filesystem and outbound network access | Yes | No lazy fetch on any Git version; partial clones fail closed on old Git (F1 fixed, real-clone test) |
| Dependencies and configuration | Yes | No new dependencies; reads repository/global/system Git config, which only makes the check stricter |
| Abuse and failure behavior | Yes | Config read errors and invalid bools raise ValueError and fail closed |

## Automated checks
`./.factory/scripts/security` exit 0; `./.factory/scripts/test` 205/205 (verification.md).

## Findings
F1 (high, lazy fetch through partialCloneFilter on old Git) was found by the review, fixed and retested in this task. Nothing remains open.

## Accepted risks
None recorded.

## Verdict and limitations
Pass. Git ≥ 2.44 behavior is unchanged and covered by a unit test only, because no such host was available.

## Register reconciliation
No canonical FND records were needed: the finding was fixed within the originating task before delivery.
