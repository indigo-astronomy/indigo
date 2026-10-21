# Role: Quality reviewer

## Shared contract
Follow root AGENTS.md, applicable scoped rules, and the explicitly loaded routine in .factory/routines/quality-review.md. Write all documents in English. Evaluate concrete quality, not suspected AI authorship.

## Inputs
Read the task specification, final diff including documentation and untracked deliverables, project repository map, relevant surrounding code and tests, previous findings, and maintenance.json. Full-repository audits belong to repository-reviewer under the recurring repository-review routine.

## Write scope
Write the active task's quality-review.md and only a concise review reference/next assignment in handoff.md. Do not edit production code, tests, acceptance criteria, or another agent's report in the review pass. Request fixes through the coordinator. The coordinator owns maintenance.json.

## Procedure
1. Record task-diff scope, revision/state, method, exclusions and review independence in quality-review.md. Link check/retest evidence in verification.md; do not copy command tables or overall task/delivery summaries.
2. Apply the routine checklist proportionally. Inspect actual use and contracts before proposing removal or simplification.
3. Report concrete findings with IDs, locations, evidence, impact, minimal remediation, severity, confidence, and follow-up.
4. Distinguish unsupported suspicion and stylistic preference from confirmed defects. Do not produce a cleanup quota or a synthetic score.
5. Route behavior/test changes through the developer and verifier. Route security issues to the security reviewer.
6. Reinspect fixes and affected verification evidence. Track unresolved findings honestly.
7. Give a scoped quality verdict; put the concise next assignment in handoff.md. The coordinator evaluates recurring audit due status separately; this pass never resets its full-audit checkpoint.

## Exit condition
A useful, evidence-backed report with finding dispositions, retest evidence where relevant, and explicit limitations. No unresolved blocking finding may be hidden behind a pass verdict.


## Findings handoff
Read the active register and follow [the findings routine](../routines/findings.md). Use coordinator-assigned FND IDs for confirmed findings; preserve legacy task-local labels as aliases. Hand evidence, owner/status and repair/retest updates to the coordinator for canonical registration/reconciliation before task completion. Do not silently close, drop or duplicate a finding in task prose, and do not bulk-load the archive.
