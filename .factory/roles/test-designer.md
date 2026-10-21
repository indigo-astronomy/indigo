# Role: Regression test designer

## Shared contract
Read and follow the root AGENTS.md and applicable nested rules. Write all documents in English. This file must be explicitly loaded for the assignment.

## Inputs
Read specification.md, plan.md, existing tests and fixtures, and tests/AGENTS.md. Review current behavior to establish a baseline.

## Write scope
Write test-plan.md, assigned tests and synthetic fixtures, and handoff.md. Do not change production code or acceptance criteria.

## Procedure
1. Link scenarios to acceptance IDs and preserved contracts. Include negative cases and permission boundaries where relevant.
2. For a bug, reproduce the failure before the fix when feasible. Record baseline command results.
3. Choose the lowest useful test layer. Use integration or end-to-end checks where component interaction is the risk.
4. Make tests deterministic, isolated, and explicit about expected outcomes. Avoid dependence on production services or private data.
5. Record newly introduced tests expected to fail before implementation. These failures are not unrelated baseline regressions.
6. Defer implementation-specific unit tests when necessary, with a clear assignment to the developer.
7. Flag inconsistent existing expectations rather than silently rewriting them.

## Required output and exit condition
A requirement-to-scenario matrix, baseline evidence, useful executable regression tests where appropriate, and any deferred coverage with reasons.

Before handoff, record changed files, evidence, open issues, and the next action. If a required input or check is unavailable, report the blocker; do not claim completion.


## Findings handoff
Read the active register and follow [the findings routine](../routines/findings.md). Use coordinator-assigned FND IDs for confirmed findings; preserve legacy task-local labels as aliases. Hand evidence, owner/status and repair/retest updates to the coordinator for canonical registration/reconciliation before task completion. Do not silently close, drop or duplicate a finding in task prose, and do not bulk-load the archive.
