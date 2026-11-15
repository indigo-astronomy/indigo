# ADR-001: Adopt AI Factory while preserving INDIGO authority

- Status: accepted
- Note: the Factory mechanism adopted here was later replaced by Factory 2 (.factory2/); the INDIGO-authority decision remains in force. Task IDs refer to the retired .factory records kept in Git history.
- Date: 2026-10-21
- Decision owner: Peter Polakovic, explicit adoption request and clarification
- Related task: 20261021193138-70b4-adopt-ai-factory

## Context
INDIGO already has detailed root/scoped driver and test instructions, Xcode navigation and legacy incremental REVIEW.md tracking. The owner requested the pinned Factory workflow and persistent opt-in template registration without application changes or future upgrade/publication authorization.

## Decision
Keep Factory shared rules first in root AGENTS.md, followed by preserved INDIGO requirements. Rename the scoped driver entry to indigo_drivers/AGENTS.md and preserve its content. Retire exact REVIEW.md files after transferring unresolved issues with canonical FND IDs, legacy aliases and immutable historical source references. The actual automated suite remains indigo_test/; indigo_tests/ is excluded except the owner-approved retirement of its REVIEW.md.

Use task/<id> branches so the reviewed Factory write guards apply. Keep project-owned state, maintenance, checks, findings and private host/registry configuration outside automatic template maps. Preserve all existing source/build/README/SDK and Git history. Review future upgrades separately using the unchanged pinned baseline.

## Consequences
Factory integration adds local standard-library helper regression tests and Xcode groups, without a new package manager or provider invocation. Registration permits discovery of pending template updates, not automatic application. Initial audit completion and native host/test limitations need their own evidence; a copied workflow or migrated finding is not a passing audit or a verified fix.

## Evidence
The adoption report (`migration.md`) and the legacy reconciliation (`legacy-findings.md`) are retained in Git history only, at commit `3dedba4cb` under `.factory/tasks/archive/20261021193138-70b4-adopt-ai-factory/`.
