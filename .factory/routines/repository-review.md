# Recurring repository review

## Purpose and evidence standard
Review the maintained repository's design, reliability, tests, documentation, configuration and agent workflow against current requirements and contracts. Identify defects, unnecessary complexity, duplication, weak verification and misleading claims using concrete impact and evidence. AI slop is one specific inspection area within this broader review; it does not define the audit's scope or imply AI authorship.

The coordinator assigns the explicitly loaded [repository-reviewer role](../roles/repository-reviewer.md). Prefer a reviewer context separate from implementation when available. Record same-agent review limitations. No delegation permission is implied.

## Cadence
Defaults are recorded in [maintenance.json](../maintenance.json); the coordinator evaluates them at session start and task closure:

1. **Task diff reviews:** follow [quality-review.md](quality-review.md) with the quality-reviewer and a separate quality-review.md report. They do not substitute for this full-repository audit or reset its checkpoint. The maintenance field `repository_review.review_every_task` retains its existing per-task review requirement; it does not merge the two review scopes.
2. **Recurring repository audit:** due after five completed non-audit development/maintenance tasks since the last full audit, or seven calendar days since it, whichever comes first. Evaluate the date in the owner's timezone. No previous full audit means the initial audit is due when the adopting project is configured and ready for work. Audit-only tasks do not count toward the five-task threshold.
3. **Additional triggers:** complete an initial audit during existing-project adoption after discovery; review affected areas after a large generated change or repeated quality findings; check full-audit due status before a release. An initial audit recorded in the adoption task satisfies the recurring baseline.

A due audit must be completed before starting the next unrelated task or releasing. If another task is active, finish or explicitly hand it off first; never replace its active state to create an audit task. Record the due audit and its blocker in the active handoff. An impossible audit is blocked, not silently skipped; only the owner can authorize a deferral with scope, reason, and a new checkpoint.

This is a repository workflow checked during agent sessions, **not a background scheduler**. No audit runs while no agent is working. Calendar automation can be configured separately if explicitly requested. Apply changes to cadence through project.md and maintenance.json, keeping one consistent policy.

## Scope and tracking
For a repository audit, create a dedicated task, e.g. `./.factory/scripts/new-task 003-repository-review`, or use the adoption task for the initial audit. The task helper creates the standard task quality-review.md; explicitly copy the [audit report template](../templates/repository-review.md) into the audit task as repository-review.md. Keep full-repository audit coverage and findings in repository-review.md, acceptance/check/retest evidence in verification.md and cadence decisions/next assignments in handoff.md. Link those sections instead of duplicating tables or registry payloads. An initial audit during adoption uses the same report with full-repository scope.

Use project.md's discovered repository map. Cover maintained source, tests, documentation, configuration, and AI instructions; exclude dependencies, generated output, vendored sources, private data, and archived task evidence unless a specific risk justifies inspection. Identify excluded components and gaps. A sampled or limited audit is partial and does not reset the full-audit checkpoint. Include a source revision and the uncommitted state reviewed. Counts and dates in maintenance.json are manual evidence pointers, not an automatic counter.

After each non-audit task completes, the coordinator increments completed_non_audit_tasks_since_full_review once. A full audit resets the count only when all mapped maintained components were reviewed and all findings are dispositioned. Set last_full_review_date, last_full_review_task, and last_full_review_revision to the actual reviewed snapshot; retain report references for any subsequent fixes. At closure of each task, set last_task_review_task to the completed task quality review. Never update full-audit fields for a diff-only review. An owner-authorized deferral does not become a completed audit.

## Inspection checklist
| Area | Look for | Required evidence before calling it a finding |
| --- | --- | --- |
| Design and code | One-use abstraction layers, speculative extension points, duplicated logic, unused wrappers, dead code, needless dependencies, excessive configuration | Concrete location, current requirement or call path, maintenance cost, and a simpler behavior-preserving alternative |
| Reliability | Broad exception swallowing, fallback that conceals failure, redundant checks masking invalid states, placeholder implementations | Reproducible failure or contract mismatch; preserve validation at real trust boundaries |
| Tests | Assertions on implementation trivia, assertions that cannot fail meaningfully, mocks that bypass the behavior, duplicate coverage, disabled tests, false success from zero test discovery | Relevant acceptance criterion, behavior the test misses, and a stronger test or justified removal |
| Documentation | Boilerplate without project information, stale examples, unsupported claims, repeated explanations, comments restating obvious code | Specific excerpt or command, authoritative source, practical consequence, and proposed correction |
| Agent workflow | Contradictory rules, duplicated sources of truth, excessive reports or roles with no decision value, obsolete task state | Actual scope conflict, duplication, or wasted step; preserve necessary audit evidence |
| AI slop | Unintegrated generated scaffolding, generic filler, fabricated APIs or unsupported completion claims | Concrete contract/requirement mismatch or demonstrable maintenance cost; suspected AI authorship or writing style alone is not evidence |
| Scope | Unrequested scaffolding, generic frameworks for a small requirement, unrelated refactors | Difference from agreed specification and the smallest sufficient solution |

Searches and linters help locate candidates; inspect call sites, requirements, runtime use, reflection, plugin loading, and external consumers before declaring code unused. Do not remove error handling, authorization, validation, compatibility paths, or regression tests merely because they look repetitive. Generated and vendored files should usually be fixed at their source or left alone.

## Findings and action
Use coordinator-assigned project-wide FND-NNNNNN IDs with category quality under [the central findings routine](findings.md). Historical SLOP-NNN labels remain source-task aliases. Record path and lines, evidence, impact, minimal remediation, confidence, and disposition. Use the common severity scale (critical/high/medium/low/informational) and a separate blocking/nonblocking gate decision based on behavior, safety, maintainability, or a material conflict with accepted scope; cosmetic preference alone is not a blocker. Separate confirmed findings from questions requiring investigation. Reference duplicate prior findings instead of generating repeated cleanup work.

The reviewer writes the report and hands fixes to the coordinator/developer. Small fixes within the active task's scope may be assigned immediately; broader cleanup gets a separate task and must not delay unrelated work without a documented reason. Block completion for unresolved blocking findings. Nonblocking findings may be deferred with an owner of the follow-up, task reference, and rationale; defer security risks only through the established security-review/owner acceptance policy. Findings are not resolved just because a new task exists. Keep unresolved records in findings/active.json with owner/next action/task references; archive closed records only after verified resolution. Reconcile the register before task cleanup.

Verification after a fix must show preserved behavior and affected check results. Documentation-only fixes need appropriate static/manual verification. A no-findings verdict still records reviewed scope and limitations. Do not manufacture cleanup, rewrite working code to satisfy a stylistic preference, or claim the repository is free of all defects.

## Completion gate
- Full audit coverage and findings are recorded in repository-review.md; the audit task's own final diff also follows the separate quality-review.md gate.
- Each confirmed finding is fixed and reverified, or nonblocking with a concrete follow-up; blocking findings are resolved or explicitly deferred by the owner with the next checkpoint.
- Any security issue is linked to security.md and processed through security review.
- Due full audits have been completed or explicitly deferred by the owner.
- Tracking fields reflect actual reviewed work; no scheduled execution or independent review is claimed when absent.


## Template export exception
For maintenance of the AI Factory template itself, [template retention](template-maintenance.md) retains reports and cadence evidence in the versioned task archive before restoring zero/null export checkpoints. Template Git exports omit the archive. Review the final archive/reset diff as well as the implementation; reset does not claim an audit or resolve findings. Consult that Git history for template audit due status. Adopted projects retain completed reports in the task archive and preserve counters; continue the normal tracking procedure above. Read archived audit evidence only by its specific checkpoint ID when required to establish cadence.


## Upgrade existing audit names

On the project's upgrade task branch, rename routines/ai-slop.md, roles/slop-reviewer.md and templates/ai-slop.md to repository-review.md, repository-reviewer.md and repository-review.md in their respective directories. Update current assignments, context/review CLI calls, review-policy required_roles and project-owned command references together. Do not treat an old role packet as a new review; regenerate current evidence when its role or shared context changes.

Rename maintenance.json's ai_slop object to repository_review, retaining every nested cadence setting, counter, checkpoint and custom value. If both keys exist, reconcile their values explicitly; never overwrite either object with defaults. This is a manual reviewed data migration; helpers do not reset project state or infer a completed audit.

Rename retained full-audit ai-slop.md reports to repository-review.md and repair their current links, including findings references with null revisions. Keep commit-pinned paths and immutable packets/manifests unchanged: those refer to historical bytes. Classify old task-diff reports under the separate quality-review upgrade procedure instead of relabeling their scope. Explicit context export accepts legacy ai-slop.md as historical input as well as the canonical report.

For provenance, retain the original lock/pin as the three-way baseline. The bundled updater automatically recognizes only the helper relocation into .factory/scripts; these audit renames appear as old-path deletions and unmapped new paths. Do not apply that plan as a complete audit migration. Manually compare each old upstream file, locally customized file and new upstream counterpart; merge local changes into the new destination, checking for collisions. After reviewing all mapped updates and omissions, update the three audit source/destination mappings and release pin together with the migrated files in the project upgrade task. Preserve the original baseline in Git and retain existing unrelated/custom mappings; do not replace the lock with fresh template defaults. Project-owned maintenance, state and retained report migrations remain explicit work. Run context/review/lifecycle/process/findings checks before delivery.
