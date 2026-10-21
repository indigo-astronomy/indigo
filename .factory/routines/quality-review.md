# Task quality review

## Scope and assignment

Every task receives a review of its final diff after documentation and before security sign-off/completion. Explicitly load the [quality-reviewer role](../roles/quality-reviewer.md). Review changed files, untracked deliverables and relevant surrounding code/tests/instructions. Prefer a separate reviewer context where authorized; record same-agent limitations. This grants no delegation, provider or publication permission.

This pass assesses unnecessary complexity, duplication, weak tests, misleading documentation and scope drift. Use the [evidence standard and inspection checklist](repository-review.md#inspection-checklist); judge concrete impact, never presumed authorship. A task diff review is distinct from [Recurring repository review](repository-review.md), which uses the repository-reviewer and a full-repository repository-review.md report. The coordinator retains that audit's cadence and checkpoints in maintenance.json.

## Report responsibilities

- quality-review.md owns reviewed revision/scope, method, independence, quality findings, fix reinspection and quality verdict.
- verification.md owns acceptance results, exact commands/environment/results, baseline comparisons and regression/retest evidence. Cite relevant sections instead of copying tables into the quality report.
- handoff.md owns current assignment, concise transitions, blockers and delivery state, linking review conclusions. It does not repeat the review or test report.
- The findings registry owns canonical FND IDs, owners and statuses; reports provide evidence and references rather than duplicate registry records. Security analysis belongs in security.md.

Use the [task quality template](../templates/task/quality-review.md). A no-findings verdict still records inspected scope and limitations, not whole-task acceptance or a universal absence of defects. Essential revision/independence metadata may occur in each review so its evidence remains interpretable.

## Findings and fixes

Apply [findings.md](findings.md): concrete location/evidence/impact, severity and separate blocking flag, minimal remediation and a reference to canonical disposition. Route implementation fixes through the coordinator/developer, then have verification.md record affected retests and reinspect changed areas for quality. Preserve behavior, security boundaries and accepted requirements. No cosmetic cleanup quota or test weakening.

Block completion for unresolved blocking quality findings under existing authority; only the owner may authorize a waiver or risk acceptance. Track nonblocking follow-up with an accountable owner. Do not reset a full-audit checkpoint for this pass. The coordinator updates last_task_review_task after completion and evaluates recurring-audit due status separately.

## Upgrade existing projects

On a dedicated upgrade task branch, preserve the old reports before changing live files:

1. Classify old ai-slop.md reports by actual scope. Use quality-review.md for task diff reviews and repository-review.md for full audits. Split mixed reports using evidence references. Follow [the audit name migration](repository-review.md#upgrade-existing-audit-names) for retained reports, mappings and maintenance data; preserve commit-pinned references.
2. Use quality-reviewer for task diff assignments in state, context CLI, review-policy required_roles and process references. Use repository-reviewer for recurring audits. Install both role/routine pairs; normal tasks use templates/task/quality-review.md, while audits explicitly add templates/repository-review.md. Older templates/task/ai-slop.md was the former default audit template, not a new path to install.
3. Preserve all cadence/checkpoint/custom values while migrating maintenance.ai_slop to maintenance.repository_review. review_every_task still requires a separate task diff review. Historical slop-reviewer packets and pinned paths keep their original bytes and identities.
4. Regenerate current cross-review evidence when its role, shared context or reviewed source changes. Never relabel immutable packets, hashes or approvals. Current process checks require quality-review.md; neither repository-review.md nor legacy ai-slop.md substitutes for it. Use the historical checker to validate historical ranges. Explicit context exports can read either audit report name without resuming the task.
5. Run affected lifecycle/context/process/review regressions, validate findings references and retain upgrade evidence before archival. Existing permissions and gates remain in force.
