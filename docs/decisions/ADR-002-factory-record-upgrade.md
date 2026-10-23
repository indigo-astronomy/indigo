# ADR-002: Ordered tasks and independent Factory records

- Status: accepted
- Date: 2026-10-23
- Decision owner: Peter Polakovic, explicit Factory upgrade request
- Related task: 20261023053418-4c0a-upgrade-factory

## Context and constraints

Independent contributors must create tasks/findings and complete work without editing shared allocation/cadence counters. Preserve INDIGO custom rules, all historical findings and accepted audit evidence. Shared pushes contain only completed tasks; no publication is authorized by this upgrade.

## Decision and rationale

Adopt AI Factory 495c6914325d1c1313fb18f4d93fa61b877e44fe. Task IDs use UTC seconds and four random hex digits; migrate all retained tasks including the upgrader. New findings use FND-<origin-task-id>-<sequence> with individual files; preserve delivered legacy IDs. Derive maintenance from durable completion records and explicit qualified audit coverage. Import actual INDIGO completion evidence and require exact legacy cadence parity. Retain owner audit coverage acceptance and the explicit upgrade deferral without resetting its checkpoint.

## Consequences and tradeoffs

Local cooperating locks do not guarantee uniqueness across independent clones; collisions and same-record edits need deliberate Git reconciliation. New task timestamps provide grouping, not a distributed logical clock. Findings and completion history remain project-owned. Immutable proof import requires a retained local migration checkpoint. Historical pins remain valid at their original commit/path; mutable links follow renamed tasks.

## Validation and reversal strategy

Run the configured Factory checks, synthetic regression suite, record-preservation checks and committed-range process/content validation. Retain mapping, raw inputs, plans and completion evidence in the upgrade task. Recover through reviewed forward/reverse changes, preserving findings/history; never reset counters or rewrite commits. Full repository audit remains due as authorized by the owner.
