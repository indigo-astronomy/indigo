# Central findings register

## Files, ownership and context

[Active register](../findings/active.json) is the authoritative project-wide backlog. The coordinator owns allocation and reconciliation; reviewers report confirmed findings and use its IDs. [Record template](../templates/finding.json) is synthetic input guidance, never a real finding. Closed records live in `.factory/findings/archive/FND-NNNNNN.json`, one complete record per ID, versioned with the project.

Run `./.factory/scripts/findings check` before publishing/exporting updated registry data. At session start and handoff read the active register only. Do not recursively read or export the archive. `./.factory/scripts/findings list` returns only short active summaries; `show FND-000001` explicitly retrieves one record, including an archived record when requested. `check` reads archive files internally for integrity but emits only counts/errors, never their contents. The context exporter includes the active register and this routine when present and always omits archive contents, even when an archive path is supplied through --path. Keep descriptions concise and free of secrets, private payloads and raw logs.

This is a manually maintained register with a read-only standard-library validator, not an issue tracker service, automatic task scanner or hook. The validator cannot detect an unreported defect, prove a claimed test ran, or verify a human approval; coordinator/reviewer gates still apply.

## ID allocation and fields

Run `./.factory/scripts/findings next-id`, reserve that ID as coordinator, insert a real record in active.json and increment `next_id` in the same reviewed edit. This command only suggests an ID; it does not reserve or write it. IDs are `FND-000001` through `FND-999999`, unique across categories, active entries and archive. Never decrement/reset the counter or reuse an old ID. One registry writer at a time. Concurrent branches must coordinate reservations; at integration reconcile counter/ID collisions and all references, not just choose the larger counter. Do not silently renumber delivered IDs.

Each record has exactly the fields in the JSON template:

| Field | Meaning |
| --- | --- |
| id, title | Stable ID and concise observed defect |
| category | functional, security or quality |
| severity | critical, high, medium, low or informational |
| status | open, in_progress, blocked or deferred in the active file; closed only in the archive |
| owner | One accountable person, team or assigned role; never empty |
| blocking | Blocking classification while active; a verified terminal resolution satisfies that gate, and severity remains separate |
| tasks | One or more objects with task id, report basename and revision; logical references resolve to `.factory/tasks/<id>/<report>` or `.factory/tasks/archive/<id>/<report>`; duplicate task locations are invalid |
| summary, evidence | Reproduction/impact, next action and short supporting evidence references; not report copies |
| resolution | null while active; structured terminal resolution when archived |

Task revision is null only while its report exists in the current checkout, including the task archive. Moving a completed task with `archive-task` preserves logical references without changing this schema or pinning revisions. The validator resolves exactly one live/archive report location at the selected revision; historical pins made before archival remain valid. Before pruning a task, replace every affected null revision in active AND archived records with the full SHA of a committed report. `git show <revision>:.factory/tasks/<id>/<report>` retrieves it after deletion; use `.factory/tasks/archive/<id>/<report>` when the pinned commit contains the archived location. Task archival is a move, not pruning: run the ordinary findings check before and after moving; the prune gate still applies before any later deletion from either location. The report must exist in that immutable commit, and the commit must remain in the cleanup candidate's ancestry. Local validation ignores Git replacement refs and disables lazy fetching and optional locks; Git must support those flags. Missing partial-clone objects require an explicit authorized fetch before retrying. A branch name, abbreviated SHA, future task ID with no report, or handoff-only mention is not a durable task reference. Planned follow-up assignments may be described in summary; add a structured reference when the task/report exists. Preserve original discovery-task references when adding repair tasks.

Historical SEC-NNN/SLOP-NNN labels remain aliases scoped to their original task; preserve them in summary/evidence when migrating a still-open finding. All newly registered functional, security and quality findings use FND IDs. Never reinterpret every historical report as a currently open issue automatically.

## Lifecycle and archive

1. Confirm evidence, check for an existing finding, allocate one ID, assign owner/severity/blocking and link its discovery task. Mirror the ID in the relevant verification/security/quality report. Maintain the register as state/ownership changes; deferred and blocked findings remain active and visible. Creating a follow-up task does not close a finding.
2. Before task completion, reconcile every confirmed report finding with the register. Blocking findings require fixes and retest or the applicable explicit owner risk decision; changing a status/flag is not approval. Nonblocking follow-up retains owner, next action and task linkage in the active register, subject to the project's existing review gates.
3. Close only with evidence. Commit any report that must survive pruning first. Copy the complete record to `archive/<ID>.json`, set status closed and set resolution to an object with `kind`, `evidence`, `approved_by` and `duplicate_of`. Kind is fixed, duplicate, not_a_bug or accepted_risk. Evidence is required; accepted_risk also requires approved_by identifying the human owner authorization, date and rationale/follow-up. No agent self-acceptance. duplicate_of is a different known FND ID only for duplicate closures; otherwise null. Never create a duplicate cycle. A fixed finding needs actual verification, not merely an implementation claim.
4. Validate the archive copy, then remove that ID from the active list, leaving next_id unchanged. Review the complete move and run `./.factory/scripts/findings check` before committing. An interrupted move may leave both copies: retain evidence, resolve the duplicate explicitly and rerun validation; never delete both. Closed records do not stay in the active file as tombstones.
5. To reopen the same defect, first ensure the prior archive record is committed, restore its full record to active with the same ID, an active status, current owner/next action and resolution null, then remove its archive file in the same reviewed change. Keep next_id unchanged; Git preserves the previous closure. Recheck. A distinct defect gets a new ID.

## Task cleanup gate

The coordinator must complete this sequence before deleting any task report, including template maintenance:

1. Reconcile report findings, owners, dispositions and follow-up with the register. Never prune an unfinished task or use registry migration as a waiver of blocking findings.
2. Commit required evidence and pin EVERY affected task/report reference in active and archive to its full commit SHA. Keep the source report and its useful reproduction/evidence available through that history.
3. Run `./.factory/scripts/findings check --prune-task <task-id>` (repeat --prune-task for multiple tasks). It rejects unpinned, unavailable or unretained task evidence. This is an explicit workflow gate, not an installed filesystem/Git hook.
4. Only then prune the reviewed completed task records; rerun the same check after deletion and verify registry/counter/archive were not reset. State/maintenance export reset never clears this register.

If required squash/rebase integration would discard the evidence commits, retain the task reports in that integration candidate. Once the integrated report commit is stable in target history, pin it and prepare a separately verified cleanup candidate. Do not prune based on an ephemeral branch/commit or bypass branch protection. Missing evidence blocks cleanup, not the need to preserve a finding.

## Adoption and upgrades

A new application starts with `{ "version": 1, "next_id": 1, "findings": [] }` and an empty archive directory; do not import this template repository's live findings or archive. Existing projects preserve their own register, counter, archive and unresolved findings during adoption/upgrades. Reconcile pre-existing open reports into that project's register deliberately. Template maintenance itself NEVER resets its live register as part of task cleanup, even though exported state/maintenance checkpoints return to defaults. When preparing a new-project export with template-specific findings, initialize fresh registry data only in the new destination, leaving the upstream records untouched.
