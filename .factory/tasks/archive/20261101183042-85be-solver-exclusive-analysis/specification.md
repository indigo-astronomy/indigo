# Analysis specification

## Acceptance criteria
- AC-001: Locate the existing mechanism and distinguish driver exclusion from device access tokens and operation locks.
- AC-002: Determine whether it can exclude Astrometry and Solver in the normal server lifecycle, including reversed order and unload/retry semantics.
- AC-003: State synchronization, process and test-harness limits and propose the smallest integration and meaningful future tests.
- AC-004: Leave product behavior unchanged and exclude ASTAP from the recommendation.

## Assessment
The concurrency analysis has high reasoning complexity: initialization publishes registry state after callbacks, with several entry paths and caller synchronization requirements. Product risk of this investigation is low because it changes evidence only. The recommendation is conditional on one active solver driver per server; allowing both drivers loaded with serialized operations is a different contract.
