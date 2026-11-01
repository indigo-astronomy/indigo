# Atomic plan
1. Analyze current source, public contract, dependencies, failure paths, relevant docs and existing tests. Read-only planning completed; task-specific baseline audit follows before production edits.
2. Build/run original relevant suite; capture reproducible defect and preserved behavior. For drivers retain protocol/SDK/public-state reference trace; record no-hardware decision.
3. Add meaningful failing characterization/regression for AC-001, preserving original tests except independently reviewed obsolete expectations.
4. Implement smallest scoped fix with platform/error/ownership checks; update driver versions and unchanged-generator outputs where applicable.
5. Rerun full applicable scope, strict/sanitizer checks and trace comparison; final driver run via tools/run_driver_test.py, update only authorized Testing records/status counts.
6. Independent quality/security review, fix/retest as needed, reconcile findings, archive completed task and commit its outcome. Main publication/integration remain pending; inherited unfinished audit can block aggregate process gate.
Update actual result/evidence immediately after each step; never mark an unperformed gate passed.

Steps1–3 baseline: current helper/Makefile/orchestrator audited; actual-header/orchestrator tests added before production and reproduced13 assertions/subcases+2 errors in11 methods. Positive runner/dome/exercise controls pass. See verification.md. Physical hardware0; Valgrind unavailable macOS, orchestration fixtures used. Proceed scoped production repair; add new existing-layout test file to Make target and Xcode group only.

Coordinator checkpoint: independent scoped quality/security/verification passes recorded. All implementation steps complete; source unchanged after review. Next canonical findings closure, final metadata review, archival/commit and batch assembly. No audit cadence reset.
