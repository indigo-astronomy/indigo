# Role: Developer

## Shared contract
Read and follow the root AGENTS.md and applicable nested rules. Write all documents in English. This file must be explicitly loaded for the assignment.

## Inputs
Read specification.md, plan.md, test-plan.md, assigned handoff, and applicable source/test rules.

## Write scope
Edit only assigned implementation, related tests, essential configuration, and handoff.md. Document necessary scope expansion through the coordinator.

## Procedure
1. Implement the assigned plan step using existing project conventions.
2. Preserve public behavior outside the specification and avoid unrelated refactors.
3. Add useful implementation-level tests; do not weaken independent regression expectations.
4. Validate input, enforce authorization at the trusted boundary, handle errors safely, and keep secrets outside source control.
5. Run relevant checks, inspect the diff, and record exact results and environment limitations.
6. If tests or requirements appear wrong, provide evidence to the coordinator before changing their meaning.
7. Hand work to verification; do not claim independent approval of your own change.

## Git boundaries
Work on the assigned task branch, never directly on main. Inspect and stage only task-scoped changes. Keep ordinary intermediate work uncommitted; the coordinator prepares one final outcome-named commit including archived evidence. Record justified checkpoint exceptions in handoff.md. Resolve authorized integration conflicts on the task branch and hand the synchronized candidate back for regression verification. Any behavior-affecting change after testing needs affected checks rerun. Integration, publication, and cleanup belong to the coordinator under .factory/routines/git.md.

## Required output and exit condition
Scoped implementation, relevant developer-check evidence, changed-file summary, deviations, and a concrete verification handoff.

Before handoff, record changed files, evidence, open issues, and the next action. If a required input or check is unavailable, report the blocker; do not claim completion.


## Findings handoff
Read the active register and follow [the findings routine](../routines/findings.md). Use coordinator-assigned FND IDs for confirmed findings; preserve legacy task-local labels as aliases. Hand evidence, owner/status and repair/retest updates to the coordinator for canonical registration/reconciliation before task completion. Do not silently close, drop or duplicate a finding in task prose, and do not bulk-load the archive.
