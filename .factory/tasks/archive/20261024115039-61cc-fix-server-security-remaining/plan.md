# Plan

1. Inspect parser ownership, upload cache consumption and listener thread contracts; capture current security-suite baseline.
2. Add bounded per-property request budgets and parser-owned cleanup; enforce RO upload rejection and safe staging lifetime.
3. Centralize download cleanup, bound listener admission and initial protocol wait, preserving established sessions.
4. Extend existing isolated parser/cache/loopback security regressions; run instrumented and normal relevant tests and Factory checks.
5. Independent final diff quality/security review, resolve findings, update documentation and records, archive and commit tested candidate. Integration/publication remain pending owner authorization.
