# ADR-004: Separate incremental and full repository audits

- Status: accepted
- Date: 2026-11-01
- Decision owner: Peter Polakovic
- Related task: 20261101213045-9647-scale-repository-audits

## Context and constraints
A full maintained-repository sweep after20 work tasks or seven days does not scale for INDIGO's driver families. Existing task reviews, finding records and prior audit evidence must remain intact. The owner approved bounded incremental audits and closure of the current inspection as partial, retaining its findings and unchecked areas.

## Options considered
Retaining frequent exhaustive sweeps causes repeated unrelated work. Removing repository review loses dependency/finding checks. Separate incremental and full reviews preserves both with explicit scopes.

## Decision and rationale
Review every task diff with relevant checks. After20 completed work tasks, audit changes since the incremental checkpoint, affected dependencies/contracts and relevant open findings. Plan full audits separately every90 calendar days (quarterly interval) or before significant releases. Define scope before starting; reuse applicable evidence and do not repeat image recognition solely for naming.

Full and incremental checkpoints are distinct. A partial-audit completion closes only an explicitly owner-accepted bounded inspection, preserves findings and unchecked-area follow-up and advances neither actual review checkpoint. The one-time incremental cadence migration baseline is retained commit2b040e9b6daa120b78459e3f91242b0848615716; its visible historical completions are acknowledged for counting only, never asserted reviewed. Last full review and its historical uncovered work remain unchanged.

## Consequences and tradeoffs
Incremental reviews bound routine cost; unchanged components wait for their planned full review unless a concrete dependency/risk warrants earlier inspection. Partial closure is not security-risk acceptance, finding closure or release certification. No scheduler, downstream rollout, main merge or publication is authorized.

## Validation and reversal strategy
Synthetic maintenance/archive regressions distinguish kinds/checkpoints, threshold boundaries, baseline ancestry and unseen concurrent work. Retain old records and validate the combined committed candidate. Change policy through another reviewed owner-authorized task; never rewrite completion history or relabel partial work as full.
