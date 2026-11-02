# Task quality review

Verdict: pass
Reviewer and independence: separate read-only reviewer subagent (Sonnet) on the first rule draft; coordinator (author) reinspected the revised text — same-agent limitation for the reinspection
Reviewed revision and working-tree state: master `4c3bc0a29` plus uncommitted AGENTS.md bullet

## Scope and method
The new AGENTS.md bullet against root AGENTS.md Factory sections, `.factory/workflow.md`, `.factory/routines/git.md`, `process-checks.md`, `task-archive.md` and `.factory/scripts/process_check.py`.

## Quality findings
Reviewer findings on the first draft (wording only, nonblocking, not registered as they were resolved in the same draft before any commit):
- A: precedence named only AGENTS.md and git.md, while workflow.md, process-checks.md and task-archive.md also state the single commit.
- B: no rule for tasks without code changes; classification of root instruction files implicit.
- C: Factory file placement (completions, findings, archive move, state) not named.
- D/E: code commit alone fails the range check; range base and tested candidate unspecified.
- F: commit plan for approval should list both commits.
- G: pre-existing attribution-trailer question, outside this change.

## Fix reinspection
After the review the owner decided that agent instruction files are Factory changes; the bullet was changed accordingly (coordinator reinspection only). The revised bullet covers A–F: precedence lists all five sources; tasks without code changes get one Factory commit; agent instruction files are Factory (owner decision); Factory contents enumerated; reviews/archival/`--record-content` (base before the code commit) precede both commits; the code commit alone is not a candidate and the Factory commit is the tested, integrated candidate; both commits listed in the plan.

## Quality verdict
Pass: the rule is consistent with the checker and states its precedence; template-owned routines keep their single-commit wording (not changed by design, see brief.md).
