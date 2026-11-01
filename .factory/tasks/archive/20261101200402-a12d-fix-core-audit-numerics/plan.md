# Atomic plan
1. Analyze current source, public contract, dependencies, failure paths, relevant docs and existing tests. Read-only planning completed; task-specific baseline audit follows before production edits.
2. Build/run original relevant suite; capture reproducible defect and preserved behavior. For drivers retain protocol/SDK/public-state reference trace; record no-hardware decision.
3. Add meaningful failing characterization/regression for AC-001, preserving original tests except independently reviewed obsolete expectations.
4. Implement smallest scoped fix with platform/error/ownership checks; update driver versions and unchanged-generator outputs where applicable.
5. Rerun full applicable scope, strict/sanitizer checks and trace comparison; final driver run via tools/run_driver_test.py, update only authorized Testing records/status counts.
6. Independent quality/security review, fix/retest as needed, reconcile findings, archive completed task and commit its outcome. Main publication/integration remain pending; inherited unfinished audit can block aggregate process gate.
Update actual result/evidence immediately after each step; never mark an unperformed gate passed.

## Developer completion

Implemented002 in existing indexer/parser tests,008 in SER with existing `test_recording_io.c` (coordinator-approved replacement for initially proposed raw-image test),009 in shared Bayer interpolation/header and existing image math tests,010 in JSON number parser and existing protocol tests. No new global build/project metadata or driver/generator changes. Existing001 filesystem/parser ownership changes retained. Baseline and final results: verification.md and developer-checks.log. All assigned source/test edits are stable; next phase is independent verification/review.

Coordinator checkpoint: independent scoped quality/security/verification passes recorded. All implementation steps complete; source unchanged after review. Next canonical findings closure, final metadata review, archival/commit and batch assembly. No audit cadence reset.
