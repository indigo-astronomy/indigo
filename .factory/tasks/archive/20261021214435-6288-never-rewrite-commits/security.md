# Security review

Verdict: pass
Reviewer: single agent, security-reviewer role (same agent made the change; independence limited, disclosed)
Date: 21 Oct 2026 (Europe/Bratislava)
Revision and working-tree state: rule commit c876bf44b on origin/master; this review is part of the follow-up evidence commit on task/20261021214435-6288-never-rewrite-commits.

## Scope, assets, and trust boundaries
Process-policy change in AGENTS.md, flagged by the process-policy risk rule. Asset: integrity of the shared Git history and of task evidence pinned to commit SHAs.

## Risk coverage
| Area | Applicable? | Review evidence |
| --- | --- | --- |
| Authentication and authorization | Yes | The rule removes a permission: no agent may rewrite commits, even with routine-level authorization. It grants nothing new. |
| Input, injection, and output handling | No | No code change. |
| Data, secrets, cryptography, and logs | Partial | A committed secret can no longer be removed by an agent rewriting history; that would need an explicit owner decision outside this rule. |
| Filesystem and outbound network access | No | No code change. |
| Dependencies and configuration | Yes | The rule is stricter than .factory/routines/git.md, which already forbids rewriting published history and force pushes and allowed only an authorized pre-publication consolidation. The rule states that it overrides that exception, so the two do not conflict in practice. |
| Abuse and failure behavior | Yes | Keeping every commit preserves the full SHAs that findings and archived task evidence are pinned to. |

## Findings
None.

## Verdict and limitations
Pass. The change only narrows what agents may do with Git history. The review was made after the rule was pushed, because the owner required the rule to take effect first.
