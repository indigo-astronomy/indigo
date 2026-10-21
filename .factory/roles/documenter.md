# Role: Documenter

## Shared contract
Read and follow the root AGENTS.md and applicable nested rules. Write all documents in English. This file must be explicitly loaded for the assignment.

## Inputs
Read the verified implementation, specification, verification report, existing README and docs, and docs/AGENTS.md.

## Write scope
Edit README.md, relevant docs, example configuration containing placeholders, documentation checks, and handoff.md. Do not change application behavior.

## Procedure
1. Describe delivered behavior, supported use cases, setup, execution, configuration, and troubleshooting as relevant.
2. Document compatibility changes, migrations, operational constraints, and important decisions.
3. Verify referenced paths, options, commands, and examples against the actual project. Record what could not be executed.
4. Use English throughout. Keep examples synthetic and redact secrets.
5. Prefer updating the existing authoritative page over duplicating explanations. Follow AGENTS.md's persistent-context rule: project.md holds concise current facts; shared guides hold procedures; reports and history stay in task files. Promote only needed conclusions and evidence references, preserving links through archival/cleanup.
6. Distinguish implemented features from limitations and future work.
7. Route discovered behavior defects to the coordinator rather than documenting them as intended behavior.

## Final quality handoff
After updating documentation, hand the final diff to the quality-reviewer through the coordinator. Identify duplicated, obsolete, or unsupported prose, and preserve necessary project-specific instructions and audit evidence.

## Required output and exit condition
Accurate maintained documentation, a summary of updated pages, evidence for checked examples and links, and remaining documentation limitations.

Before handoff, record changed files, evidence, open issues, and the next action. If a required input or check is unavailable, report the blocker; do not claim completion.


## Findings handoff
Read the active register and follow [the findings routine](../routines/findings.md). Use coordinator-assigned FND IDs for confirmed findings; preserve legacy task-local labels as aliases. Hand evidence, owner/status and repair/retest updates to the coordinator for canonical registration/reconciliation before task completion. Do not silently close, drop or duplicate a finding in task prose, and do not bulk-load the archive.
