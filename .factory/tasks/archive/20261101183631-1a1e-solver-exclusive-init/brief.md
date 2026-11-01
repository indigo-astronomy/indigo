# Task brief

Task: 20261101183631-1a1e-solver-exclusive-init
Owner: Peter Polakovic
Date: 2026-11-01

Implement mutually exclusive initialization of Astrometry Agent and Solver Agent using the existing conflicting-driver API, as approved after analysis. ASTAP is explicitly excluded. Source base: master e5e77e58edd20f102577a8f931651f802bdfc612. Prior analysis is retained in local commit f1c53ce17108a499bdcdc698f65ee13a0c55ac92 under its archived task analysis.md; no implementation is inherited from that branch.

Owner explicitly extended the overdue repository audit deferral to this implementation and its tests on 2026-11-01. Audit remains due after completion; this task cannot reset its checkpoint. Local implementation, tests and commit authorized; merge, push and publication are not authorized.

## Owner-approved test scope (2026-11-01)
Owner explicitly approved only local initialization and regression tests, omitting the large real-catalogue Solver sweep and substituting Factory evidence for the full README Testing record required by AGENTS.md:235–236. No indexes will be downloaded. The standard runner cannot select only the local synthetic suite without also selecting its catalogue opt-in suite, so execute the approved local make targets and retain their counts here. README and TEST_SUMMARY.md remain unchanged under this explicit waiver.
