# Atomic plan
1. Analyze current source, public contract, dependencies, failure paths, relevant docs and existing tests. Read-only planning completed; task-specific baseline audit follows before production edits.
2. Build/run original relevant suite; capture reproducible defect and preserved behavior. For drivers retain protocol/SDK/public-state reference trace; record no-hardware decision.
3. Add meaningful failing characterization/regression for AC-001, preserving original tests except independently reviewed obsolete expectations.
4. Implement smallest scoped fix with platform/error/ownership checks; update driver versions and unchanged-generator outputs where applicable.
5. Rerun full applicable scope, strict/sanitizer checks and trace comparison; final driver run via tools/run_driver_test.py, update only authorized Testing records/status counts.
6. Independent quality/security review, fix/retest as needed, reconcile findings, archive completed task and commit its outcome. Main publication/integration remain pending; inherited unfinished audit can block aggregate process gate.
Update actual result/evidence immediately after each step; never mark an unperformed gate passed.

Step1 complete: scoped source/docs/test audit recorded in specification. Step2 active: build existing driver and core, retain outputs before production edits.

Step2 complete: original driver build and original core suite pass; retained public-load reference outputs, no protocol/SDK in this pure parser path. Step3 active: added valid formats and malformed integer matrix, production unchanged.

Step3 complete: valid FITS format controls pass, fractional NAXIS1 accepted by original parser reproduces AC-001 defect (expected test exit1). Next sanitizer confirmation before production repair.

Step4 implemented: bytes conversion now follows BITPIX allowlist; dimensions finite/integer checks precede casts and retain bounds. Hand-written owning driver version1A→1B. No property/transport/lifecycle/generator change. Step5 active: rebuild/retest and full recorded driver run.

Step4 verified complete: local universal driver rebuild and core21/21 pass; actual-parser loader sanitizer7/7 and strict warning check pass.
Step5 complete with documented limitation: recorded agent/simulator159/159, CLI17/17; preservation baseline19 remains passing, same assertions. README Testing + generated summary updated only via script. Full core sanitizer original-source unrelated index assertion reproduced and sent to coordinator; no false full-core pass. test-clean and diff whitespace pass.
Step6 pending coordinator: source/evidence ownership released for independent quality/security review, canonical finding006 closure/new follow-up registration, final process/archive/Git. No staging, commit or integration performed by developer.

Coordinator checkpoint: independent scoped quality/security/verification passes recorded. All implementation steps complete; source unchanged after review. Next canonical findings closure, final metadata review, archival/commit and batch assembly. No audit cadence reset.
