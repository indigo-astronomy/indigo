# Task brief

- Task ID: 20261101183042-85be-solver-exclusive-analysis
- Owner: Peter Polakovic
- Date: 2026-11-01

## Problem and desired outcome
Investigate whether the existing exclusive-use mechanism can prevent simultaneous use of Astrometry Agent and Solver Agent. The owner explicitly removed ASTAP from scope during analysis. Deliver a source-supported recommendation; implementation is not part of this request.

## Scope and constraints
Read the two agent entry points, the conflicting-driver API, loader/server lifecycle, shared platesolver state and existing relevant tests. Write only this task's evidence and its generated completion record. Do not modify product code or revive obsolete ASTAP. No network, merge or publication is needed.

## Authority
The owner explicitly deferred the overdue repository audit for this analysis only on 2026-11-01. The audit remains due after this task, before unrelated work or release. No risk acceptance or audit checkpoint reset is implied.
