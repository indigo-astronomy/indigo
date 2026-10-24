# Plan

1. Build/run original suites and retain baseline traces; add deterministic failing race regressions before production changes.
2. Preserve generated admission using a driver-local delegating callback; synchronize short request/publication bookkeeping without transport or worker-to-bus lock inversion. Cover compensation and cancellation ownership.
3. Bump both export versions and regenerate with the unchanged generator. Run focused regressions, full recorded suites, sanitizer checks and Factory checks.
4. Independently review the final diff, close the finding with evidence, reconcile documentation, archive execution evidence and commit one tested outcome. Integration/publication remain pending owner authorization.
