# Verification report

Verdict: pass
Reviewer: single agent, verifier role (same agent made the change; independence limited, disclosed)
Date: 21 Oct 2026 (Europe/Bratislava)
Revision and working-tree state: as in security.md.

## Acceptance evidence
| Criterion | Result | Evidence |
| --- | --- | --- |
| AC-001 | pass | AGENTS.md line 247 names amend, rebase, squash or fixup rewrites, reset that drops commits, filter-branch, filter-repo and force pushes, for local and pushed commits. |
| AC-002 | pass | The same line says to correct mistakes with a new commit and that it excludes the consolidation .factory/routines/git.md allows. |
| AC-003 | pass | `git diff --stat 38e71b647 c876bf44b`: one file, one insertion; the shared part of AGENTS.md and git.md are unchanged. |
| AC-004 | pass | c876bf44b was pushed alone (origin/master 38e71b647..c876bf44b, fast-forward) before these records existed; they are added in a new commit on top. |

## Executed checks
| Command | Exit | Result |
| --- | --- | --- |
| `git diff --check 38e71b647 c876bf44b` | 0 | clean |
| `./.factory/scripts/check` | 0 | passed |
| `./.factory/scripts/test` | 0 | 144 tests OK |
| `./.factory/scripts/process-check --base 38e71b647 --head HEAD` | recorded in handoff after the commit | covers the rule commit and this evidence |

## Manual checks and limitations
CLAUDE.md and GEMINI.md import or point to AGENTS.md, so all configured agents load the rule; no other instruction file restates Git rules that contradict it. Agents already running read it only when they reload their instructions.

## Verification verdict rationale
The rule is in place on origin, consistent with the shared rules, and validated by the Factory checks.

## Register reconciliation
No register change.
