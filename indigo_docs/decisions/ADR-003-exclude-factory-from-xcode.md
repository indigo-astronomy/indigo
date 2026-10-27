# ADR-003: Keep Factory metadata outside Xcode projects

- Status: accepted
- Date: 2026-10-25
- Decision owner: Peter Polakovic, explicit all-adopters request
- Related task: 20261025182421-7a16-exclude-factory-from-xcode

## Context and constraints

Factory task, finding and completion metadata changes independently across machines. Static IDE references force unrelated project.pbxproj edits and conflicts. The owner requests complete exclusion of .factory from every adopter's Xcode projects while preserving the data itself and application build contracts.

## Options considered

Static synchronized groups, directory references, or complete project-reference exclusion. Complete exclusion is owner-selected; either kind of reference retains unwanted IDE coupling.

## Decision and rationale

Xcode projects must not reference .factory or descendants through navigator groups, folder/file references or build membership. Findings lifecycle operations never inspect or mutate Xcode projects. Retire the sync-project command/helper and remove Factory-only navigator objects. Preserve all actual records, application objects, build settings and target memberships.

## Consequences and tradeoffs

Factory data stays available through filesystem/editor tools and Console monitoring. Metadata operations stop producing Xcode project changes. The existing template pin stays unchanged: this is an explicitly reviewed local customization, not a wholesale template upgrade. A later pinned update must preserve this policy. Historical reports remain unchanged.

## Validation and reversal strategy

Compare parsed PBX graphs for exact preservation outside Factory-only removal and hash retained records/vendor project. Run Factory check/test/security, finding transaction/isolation regressions and committed-range process validation. No hardware or broad native vendor build is needed. Reversal requires new owner policy and matching tests. The full audit is deferred only for this task by explicit owner reply, remains due afterward and is not reset by this change.
