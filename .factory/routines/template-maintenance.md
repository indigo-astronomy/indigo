# Template repository evidence and exports

## Scope

This routine applies to maintaining the AI Factory template itself. Adopted projects retain their own completed task archives and real maintenance tracking under [task-archive.md](task-archive.md). Both use one outcome-named commit by default under [git.md](git.md).

## Complete and retain in one candidate

1. Create/resume the task branch and task normally. Complete acceptance, documentation, quality/security reviews, findings reconciliation and distribution assessment under [template-updates.md](template-updates.md). Preserve unfinished work. No configured adopter inventory is an explicit result; downstream writes/publication require their own authorization.
2. Record executed checks, reviewed content, limitations and pending delivery. Mark task execution complete, then move its reports into `.factory/tasks/archive/TASK-ID/` with `./.factory/scripts/archive-task TASK-ID`. Include this archive and any repaired incoming links in the same final commit as the source changes. A null finding revision continues to resolve to that archived report; moving is not pruning and requires no pin commit. Re-review archival changes before capturing final source identities.
3. With no unfinished task, restore the distributable version 2 repository marker (`version: 2`, `task_state: "per-task"`), with no execution pointer and maintenance policy only. Retain real per-task completion/audit records; never reset history or aggregate counters. Preserve cadence settings and all individual finding/completion records. This is new-project export state, never a claim that an audit occurred. Retain real template cadence decisions and their evidence in the completed task archive. If another task is unfinished, preserve its task-local state and do not describe that revision as a clean export.
4. Stage reviewed source and record version 2 identities; finish process.json and stage the archive. Create one result commit. Verify the committed range and a `git archive` export. Record final SHA/post-commit results externally, without pin/evidence/cleanup follow-up commits. Integrate/publish/clean branches only with existing authorization.

## Evidence stays versioned; exports omit it

The template-only root `.gitattributes` marks `/.factory/tasks/archive`, completion JSON, active/closed finding JSON and itself `export-ignore`. A normal clone/check-out retains committed reports; `git archive VERIFIED_COMMIT` omits template execution/finding history and the template-only attributes, retaining `.factory/tasks/.gitkeep` and reusable task templates. New projects therefore do not inherit this exclusion for their own archives. Do not map/install these attributes into adopted projects. Raw filesystem copies and clones do not apply export-ignore: use the documented Git export or explicitly omit template task evidence during reviewed adoption. No hidden ref, unreachable object, Git note or external service is needed for report retention.

Read a historical report only for a concrete question, using its archived path or `git show RELEASE_SHA:.factory/tasks/archive/TASK-ID/REPORT`. Do not routinely load archives. Findings and durable decisions can reference archived reports directly, without a self-referential future commit SHA. Existing full-SHA pins and historical evidence/prune commits remain valid; never rewrite published history or relocate old reports merely to normalize it.

Pruning is exceptional, not task closure. If authorized retention work actually deletes reports, preserve their useful evidence in a retained ancestor, pin affected active AND closed findings, and pass the existing [findings prune gate](findings.md#task-cleanup-gate) before and after deletion. Never squash away the only evidence copy. This exception may require separate meaningful retention work; ordinary task completion does not.

## Audit cadence

Run `.factory/scripts/maintenance status` against retained completion records at startup/closure. It derives actual template cadence without routinely loading archived reports. New-project Git exports omit completion JSON, template finding JSON and archived reports; these exclusions do not change the template's retained history or qualify an audit. Adopted projects retain their records and never install template export exclusions.
