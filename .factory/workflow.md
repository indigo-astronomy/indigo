# Workflow and phase gates

The coordinator owns transitions. Each transition requires an updated handoff.md and the selected task's state.json (version 2); the global state is a schema marker. See [concurrent tasks](routines/concurrent-tasks.md). If blocked, retain the phase, set status to blocked, and describe the decision or missing input. Resume only when the blocker is resolved. Do not treat elapsed time as authorization.

| Phase | Responsible role | Required output and exit gate |
| --- | --- | --- |
| specification | analyst | Specification with identified acceptance criteria, scope, failure behavior, assumptions, and resolved material questions. |
| planning | planner | Incremental plan, affected components, baseline checks, dependencies, early security risks, and verification strategy. |
| test_design | test-designer | Test plan linking acceptance criteria to scenarios; useful regression tests or documented reasons to defer executable tests. |
| implementation | developer | Scoped implementation and evidence of relevant developer checks; explain deviations. |
| verification | verifier | Acceptance evidence, relevant regression results, and reviewed changes; no unresolved blocking failures. |
| documentation | documenter | Documentation updated to match verified behavior, setup, configuration, and compatibility changes. |
| security_review | security-reviewer | Threat-focused review and check evidence; findings fixed and reverified or explicitly accepted by the owner. |
| complete | coordinator | All applicable gates evidenced; final handoff includes remaining accepted limitations. |

## Entry gate: adoption and task creation
For an existing project, complete the discovery and integration gates in [migration.md](migration.md) before unrelated feature implementation. Read-only discovery may precede factory installation; record its evidence in the adoption task once bootstrapped. For every new task, create `.factory/tasks/<task-id>/` with `./.factory/scripts/new-task <task-id>`, fill brief.md, and verify its task-local state.json. Allocate the time-prefixed ID before creating its branch/worktree as documented in [concurrent-tasks.md](routines/concurrent-tasks.md). Resume existing tasks rather than recreating them.

## Model selection checkpoint
Before role assignments and after stalled retries, follow [model-selection.md](routines/model-selection.md). Record the tier/risk/budget decision in handoff.md; preserve quality gates and existing model preferences. Unknown spend must not be claimed under budget.

## Host/model handoff gate
Use [bootstrap.md](bootstrap.md) to start or change hosts. Preserve the same task, branch, role assignment, evidence, and outstanding findings. Confirm loaded rules and capabilities, release the outgoing writer, and inspect actual state before resuming. Missing file/shell access limits what can be verified; context-only proposals are not executed work.

## Git integration gate
Follow [the Git routine](routines/git.md). Create the temporary task branch before task writes. Before final verification, merge the latest configured main target into it; resolve conflicts and run the required regressions and reviews on the final committed candidate. Integrate via fast-forward only, or the required protected-branch workflow. Target advancement requires resynchronization and affected checks before another attempt. Confirm integration/publication before safe branch cleanup. Default to one outcome-named task commit including archived evidence; verify its final committed range. This is a delivery gate, not an extra state.json phase; report ready-for-integration separately from completed delivery.

## Task quality and recurring audit gates
Follow [the task quality routine](routines/quality-review.md) for the quality-reviewer pass on the final diff after documentation and before security sign-off. Record its scoped verdict in quality-review.md; keep exact acceptance/check/retest evidence in verification.md and concise assignments/delivery references in handoff.md. Fixes return to implementation, verification and re-review.

Separately follow [Recurring repository review](routines/repository-review.md), with the repository-reviewer and a full-repository repository-review.md report when an audit is due. The coordinator records cadence decisions and checks derived completion/audit status; a task quality review does not reset the audit clock. A due audit must complete before the next unrelated task/release unless the owner explicitly defers it. Neither review adds a state.json phase.

## Iteration
- Verification findings return to implementation, then verification.
- Security findings return to implementation, verification, affected documentation, and security review.
- Changed requirements return to specification; update dependent plans and tests before continuing.
- The coordinator records a phase waiver only with the reason and owner authorization. A review finding marked not applicable needs evidence rather than risk acceptance.

## Test design timing
For existing projects, capture a baseline and important preserved behavior. For bugs, prefer a reproducing test before the fix. For features, define acceptance tests before implementation; implementation-specific unit tests may be written alongside code. Do not create superficial tests for prose-only or similarly low-impact changes. Explain the appropriate verification instead.

## Independence
The test designer derives expected outcomes from requirements, not current implementation. The verifier and security reviewer should inspect evidence independently where possible. If the same agent fills these roles, record this limitation in verification.md and security.md.

## Completion checklist
- Every acceptance criterion has a result and evidence.
- Required check, test, and security command groups ran with recorded outcomes.
- Any pre-existing failures have an evidenced disposition and owner-approved exception if a required gate remains failing.
- Documentation matches the delivered behavior.
- Final-diff quality review is evidenced; blocking findings and due full-audit status have a valid disposition.
- All security findings have a disposition; claimed fixes have retest evidence.
- No unauthorized scope, unexplained changes, or accidentally committed secrets remain.
- Final handoff identifies delivery status, limitations, and any owner actions.
- Git integration used the validated candidate; required publication and task-branch cleanup are confirmed, or explicitly remain pending without claiming completed delivery.

## Completed task archival

Ordinary projects follow [task archival](routines/task-archive.md) after acceptance/review gates and before the final task commit, recording integration/publication as pending. The coordinator runs `./.factory/scripts/archive-task <task-id>` to move reports into `.factory/tasks/archive/` and retain the task-local completion state (or clear the legacy active pointer); it does not approve gates or deliver Git changes. Archived evidence is retained in version control, excluded from routine context, and read only by explicit selection.

## Manual orchestration
This is a workflow convention. There is no bundled agent scheduler or authenticated approval system. The [portable process checks](routines/process-checks.md) provide local structural evidence validation and baseline-owned diff risk rules, without requiring a hosting platform or CI service. A project may optionally invoke the same Python helper from its own CI configuration. Complete process.json from actual checks/reviews, record reviewed Git content identities (version 2; legacy source-commit evidence remains supported), and run the gate on the final committed range. It does not replace any phase, grant permissions or introduce human gates for routine work. Application checks and hosting protection remain project-specific.


## Template repository closure
For maintenance of the AI Factory template itself, [template retention](routines/template-maintenance.md) retains completed records in the versioned archive in the same final task commit. Git exports omit those reports, while workflow/maintenance export defaults return to idle. This does not waive phase gates or apply to ordinary adopted projects.


## Central findings gate
Use [the findings routine](routines/findings.md) and [active summaries](routines/findings.md) at session start, review handoffs and task completion. Report findings use the coordinator-assigned canonical FND ID; register severity/status/owner/blocking, task links and evidence before closing the review phase. Deferred/blocked findings remain active; closed resolutions are archived outside automatic context. Reconcile every confirmed finding and preserve open follow-up before pruning any report. `./.factory/scripts/findings check` is required for registry changes; task cleanup additionally requires `--prune-task <task-id>` before and after deletion. Per-record findings and durable completion history survive task cleanup; new-project export exclusions do not reset template history. This does not waive blocking findings or owner-only risk acceptance.


## Template upgrade and release gate
Apply [template-updates.md](routines/template-updates.md) when adopting or upgrading the workflow. Keep the old provenance pin until mapped changes/conflicts and unmapped dependencies are reviewed. Project verification/security gates follow application of the pinned update. Template releases record registered-adopter coverage and per-project outcomes/follow-up before evidence cleanup; project instance data is never reset by an upgrade.
