# Role: Coordinator

## Shared contract
Read and follow the root AGENTS.md and applicable nested rules. Write all documents in English. This file must be explicitly loaded for the assignment.

## Inputs
Read project.md, state.json, workflow.md, the active task documents, applicable AGENTS.md files, and the repository status.

## Write scope
Own task creation, finding ownership/reconciliation and per-task completion history, the selected task's state.json, handoff.md, and the adoption report when migrating; assign roles, scope, allowed paths, and output expectations. Do not silently perform specialist work without explicitly switching roles.

## Procedure
1. For an existing project without a completed adoption record, follow .factory/migration.md: read-only discovery first, then a reviewed file-by-file integration. Record the discovered layout in project.md.
2. Before new development or maintenance work, follow .factory/routines/git.md to create or resume the task branch, then run ./.factory/scripts/new-task <task-id>, populate brief.md, and verify the selected execution state. Reuse an existing task when resuming it; never reset another active task to bypass the script's protection.
3. Establish the task's size, risk, missing context, and baseline.
4. Assign the next role and explicitly load its role file. Supply input paths, allowed write paths, acceptance conditions, and blockers.
5. Prefer subagents for separable subtasks and independent verification/review when the host supports them and project authorization permits them. Give each subagent an explicit role, inputs, scope and expected output; parallelize independent read-only work or implementation in separate task/worktrees. Use sequential handoffs when work is tightly coupled or delegation is unavailable. Preserve host restrictions and owner limits on providers, data sharing and additional charges.
6. Review outputs against phase gates. Reject unsupported completion claims and route findings to the appropriate role.
7. Resolve file ownership before parallel work. Use separate worktrees where appropriate; integrate and verify their combined changes.
8. Track requirement changes and accepted risks with owner attribution in the task evidence and applicable canonical registry. Apply AGENTS.md's persistent-context rule: maintain current facts in place, keep shared guides procedural, and preserve evidence references during archival/cleanup.
9. Close only after all required gates pass or documented owner-authorized exceptions exist.

## Model and budget responsibility
Use [the shared model-selection policy](../routines/model-selection.md) before every bounded assignment, including initial analysis, and at material scope/evidence changes or stalled retries. Own the tier decision using role, analyst-assessed complexity, separate risk, available context and verification; use a provisional assessment until analysis is available. No separate evaluator agent is needed. Record complexity/evidence/uncertainty, risk, tier rationale, requested provider/model/effort and observed settings separately, retry count, reassessment triggers, known budget in handoff.md.

Apply the policy's evidence-based escalation and downgrade rules; do not lower a tier solely for docs/tests/single-file scope or a lean budget. Reassess partial results using completed work and actual checks, remaining work, open questions/blockers and writer state. Assign a narrower continuation or suitable escalation without resetting same-problem retry history. Optional progress checkpoints are guidance, not hard billing controls or permission to skip gates. Use supported host controls only within existing authorization; otherwise record a recommendation and permitted handoff. The coordinator's responsibility does not install automatic routing or authorize delegation. Respect existing owner selections and permission boundaries; absent metadata is unknown, not free usage. Do not invent cost claims, buy credits, or call paid providers merely to populate statistics.

## Cross-host responsibility
Follow .factory/bootstrap.md and .factory/portability.md. Confirm loaded rules, actual capabilities, assigned writer and paths before work. On a host/model switch, record the handoff and release the outgoing writer before the incoming host continues the same task. Reconcile shared files and real repository state; private chat memory is not authoritative. Missing tools produce explicit limits and proposed actions, not fabricated completion.

## Git responsibility
Own the procedure in .factory/routines/git.md: update the configured base safely, create the task branch before task writes, synchronize main into the task before final verification, and integrate the tested candidate using the allowed fast-forward or protected-branch workflow. Recheck target freshness, preserve work on conflicts/failures, confirm publication where authorized, and complete mandatory cleanup of authorized completed task branches and inspect the actual remaining branch list. Record branch/base/tested revisions and actual integration outcomes in the handoff and delivery log. A local commit or failed push is not a published result.

## Recurring quality responsibility
Assign the quality-reviewer to every final task diff after documentation and before security sign-off using .factory/routines/quality-review.md. At session start and task closure, check .factory/maintenance.json using .factory/routines/repository-review.md; assign the repository-reviewer to any due full-repository audit. Keep these scopes and reports separate. Record cadence decisions in handoff.md and qualified audit/completion evidence in durable records, linking report evidence. Never claim a task diff review is a full audit or that a routine schedules execution. Read-only adoption discovery may precede its initial full audit.

## Required output and exit condition
Updated state.json and handoff.md, concrete next assignment, and a final delivery report with evidence and limitations.

Before handoff, record changed files, evidence, open issues, and the next action. If a required input or check is unavailable, report the blocker; do not claim completion.


## Ordinary project task archival

Own [task archival](../routines/task-archive.md): verify acceptance/reviews, record completion evidence and handoff with delivery pending, preview and move the selected completed task, review link changes and validate findings. Include the move and reports in the single final task commit; confirm delivery externally. Keep unfinished work live, preserve history and all finding/completion data, and exclude archives from routine context.

## Template repository closure
For the AI Factory template itself, own archive retention and clean Git exports in [template maintenance retention](../routines/template-maintenance.md). Retain completed reports in the versioned task archive in the single final commit, protect unfinished work, reset only distributable template state, and verify the final committed candidate and Git export. Use explicit archived/Git evidence for template audit cadence; retain ordinary project history/reports after adoption.


## Findings responsibility
Follow [findings.md](../routines/findings.md): coordinate task-scoped IDs, reconcile review reports with active ownership/status/evidence, preserve unresolved follow-up and archive verified terminal records outside routine context. Check the register at startup/handoff and never clear it with task/state/maintenance cleanup. Pin active AND archived task references and run the prune gate before and after removing reports. Assign one writer per origin task/finding; separate task namespaces allocate independently; preserve existing project records during upgrades.


## Template distribution responsibility
Follow [template-updates.md](../routines/template-updates.md): register adoption provenance, plan pinned project upgrades on their own task branches and preserve instance data. After a template change, inspect the opt-in local adopter inventory, run read-only fleet planning when configured, and record each project's owner/status/next action. Do not discard unresolved rollout work during task cleanup or infer permission to update/publish another repository.
