# Recurring repository review

## Purpose and evidence standard
Review the maintained repository's design, reliability, tests, documentation, configuration and agent workflow against current requirements and contracts. Identify defects, unnecessary complexity, duplication, weak verification and misleading claims using concrete impact and evidence. AI slop is one specific inspection area within this broader review; it does not define the audit's scope or imply AI authorship.

The coordinator assigns the explicitly loaded [repository-reviewer role](../roles/repository-reviewer.md). Prefer a reviewer context separate from implementation when available. Record same-agent review limitations. No delegation permission is implied.

## Cadence and bounded scope
Policy is recorded in maintenance.json and evaluated at session startup and task closure. No background automation is created.

1. **Every task:** final-diff quality review and relevant tests remain required. A task review does not advance repository-audit checkpoints.
2. **After twenty completed work tasks:** perform an incremental audit from the incremental checkpoint/baseline. Inspect changed paths, their callers/dependencies and affected contracts, plus relevant open findings. Record the exact commit range, covered task IDs, exclusions and residual gaps. Expand into unchanged code only for a concrete dependency or risk; do not restart exhaustive inspection of every driver family.
3. **Full repository audit:** plan a dedicated audit every ninety calendar days (a quarterly interval), or before a significant release. Establish scope, evidence and completion criteria before inspection. Initial full review remains part of adoption. Repeated quality/security failures or architectural changes can justify an earlier planned audit.

When incremental review is due, complete the bounded review at the next session before unrelated work unless the owner authorizes a deferral. When full review is due, record a concrete dedicated audit assignment; do not silently turn a small task or incremental review into an unbounded full sweep. Significant-release gates and security-risk acceptance remain in force.

Reuse retained source/test evidence when its identities and contracts still apply. Run new checks for changed behavior or a concrete unresolved risk. Do not repeat image recognition, hardware tests or broad unrelated application suites solely for naming or administrative audit closure. A missing relevant check is still a limitation, never a pass.

## Outcomes, coverage and tracking
Use the existing task/branch or create a dedicated audit task as appropriate; role changes never create duplicate tasks. Explicitly load repository-reviewer. The report declares **full**, **incremental** or **partial**, its actual reviewed revision/range, independence, findings, limits and links to verification.md. Quality-review.md still reviews the audit task's own final diff separately.

- A qualified full audit has complete maintained scope and a passing repository report. Only it advances the full checkpoint.
- A qualified incremental audit has a passing report for the declared changed range and affected contracts. It advances incremental cadence only, without making a whole-repository safety claim.
- A partial audit can be closed only with explicit owner acceptance of that bounded outcome. Retain reviewed coverage, findings and a concrete list of unchecked areas with accountable follow-up. Store a partial-audit completion and `Verdict: partial` in repository-review.md; do not fabricate full or incremental coverage. A partial closure is not security-risk acceptance or release certification.

Use the project map and existing exclusions. Preserve canonical finding IDs/status/evidence and unresolved follow-up through archival. Closing an inspection task does not close its findings. Verification.md/security.md may pass the accurate recording and authorized partial-closure criteria while explicitly retaining failed product checks and open security findings; these are distinct from a passing product-security verdict.

Maintenance derives status from durable completion records. Full, incremental and partial audit-only completions are excluded from work counts. Unseen concurrent work is never covered by timestamps. A project may authorize one immutable migration baseline anchored to a retained commit, with authority and partial-report reference: it starts incremental counting only, does not mark historical changes reviewed, and never resets the last full audit or erases the unchecked-area backlog. Do not refresh that baseline automatically. See [maintenance](maintenance.md).

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
Use origin-task-scoped FND IDs and the relevant category under [the central findings routine](findings.md). Historical SLOP-NNN labels remain source-task aliases. Record path and lines, evidence, impact, minimal remediation, confidence, and disposition. Use the common severity scale (critical/high/medium/low/informational) and a separate blocking/nonblocking gate decision based on behavior, safety, maintainability, or a material conflict with accepted scope; cosmetic preference alone is not a blocker. Separate confirmed findings from questions requiring investigation. Reference duplicate prior findings instead of generating repeated cleanup work.

The reviewer writes the report and hands fixes to the coordinator/developer. Small fixes within the active task's scope may be assigned immediately; broader cleanup gets a separate task and must not delay unrelated work without a documented reason. Block completion for unresolved blocking findings. Nonblocking findings may be deferred with an owner of the follow-up, task reference, and rationale; defer security risks only through the established security-review/owner acceptance policy. Findings are not resolved just because a new task exists. Keep unresolved individual records in findings/active/ with owner/next action/task references; archive closed records only after verified resolution. Reconcile the register before task cleanup.

Verification after a fix must show preserved behavior and affected check results. Documentation-only fixes need appropriate static/manual verification. A no-findings verdict still records reviewed scope and limitations. Do not manufacture cleanup, rewrite working code to satisfy a stylistic preference, or claim the repository is free of all defects.

## Completion gate
- Actual full/incremental/partial coverage and findings are recorded in repository-review.md; the audit task's own final diff also follows the separate quality-review.md gate.
- Each confirmed finding is fixed and reverified, or nonblocking with a concrete follow-up; blocking findings are resolved or explicitly deferred by the owner with the next checkpoint.
- Any security issue is linked to security.md and processed through security review.
- Due incremental audits are completed or owner-deferred; full audits are separately planned and release gates retained. Partial closure requires explicit owner authorization and retained unchecked-area follow-up.
- Tracking fields reflect actual reviewed work; no scheduled execution or independent review is claimed when absent.


## Template export exception
For maintenance of the AI Factory template itself, [template retention](template-maintenance.md) retains reports and durable completion history in Git; template-only exports omit completion JSON and archived reports while retaining empty directories. Template Git exports omit the archive. Review the final archive/reset diff as well as the implementation; reset does not claim an audit or resolve findings. Consult that Git history for template audit due status. Adopted projects retain completed reports in the task archive and preserve counters; continue the normal tracking procedure above. Read archived audit evidence only by its specific checkpoint ID when required to establish cadence.


## Upgrade existing audit names

On the project's upgrade task branch, rename routines/ai-slop.md, roles/slop-reviewer.md and templates/ai-slop.md to repository-review.md, repository-reviewer.md and repository-review.md in their respective directories. Update current assignments, context/review CLI calls, review-policy required_roles and project-owned command references together. Do not treat an old role packet as a new review; regenerate current evidence when its role or shared context changes.

Rename maintenance.json's ai_slop object to repository_review, retaining every nested cadence setting, counter, checkpoint and custom value. If both keys exist, reconcile their values explicitly; never overwrite either object with defaults. This is a manual reviewed data migration; helpers do not reset project state or infer a completed audit.

Rename retained full-audit ai-slop.md reports to repository-review.md and repair their current links, including findings references with null revisions. Keep commit-pinned paths and immutable packets/manifests unchanged: those refer to historical bytes. Classify old task-diff reports under the separate quality-review upgrade procedure instead of relabeling their scope. Explicit context export accepts legacy ai-slop.md as historical input as well as the canonical report.

For provenance, retain the original lock/pin as the three-way baseline. The bundled updater automatically recognizes only the helper relocation into .factory/scripts; these audit renames appear as old-path deletions and unmapped new paths. Do not apply that plan as a complete audit migration. Manually compare each old upstream file, locally customized file and new upstream counterpart; merge local changes into the new destination, checking for collisions. After reviewing all mapped updates and omissions, update the three audit source/destination mappings and release pin together with the migrated files in the project upgrade task. Preserve the original baseline in Git and retain existing unrelated/custom mappings; do not replace the lock with fresh template defaults. Project-owned maintenance, state and retained report migrations remain explicit work. Run context/review/lifecycle/process/findings checks before delivery.
