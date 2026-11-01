# Atomic plan
1. Analyze current source, public contract, dependencies, failure paths, relevant docs and existing tests. Read-only planning completed; task-specific baseline audit follows before production edits.
2. Build/run original relevant suite; capture reproducible defect and preserved behavior. For drivers retain protocol/SDK/public-state reference trace; record no-hardware decision.
3. Add meaningful failing characterization/regression for AC-001, preserving original tests except independently reviewed obsolete expectations.
4. Implement smallest scoped fix with platform/error/ownership checks; update driver versions and unchanged-generator outputs where applicable.
5. Rerun full applicable scope, strict/sanitizer checks and trace comparison; final driver run via tools/run_driver_test.py, update only authorized Testing records/status counts.
6. Independent quality/security review, fix/retest as needed, reconcile findings, archive completed task and commit its outcome. Main publication/integration remain pending; inherited unfinished audit can block aggregate process gate.
Update actual result/evidence immediately after each step; never mark an unperformed gate passed.

Independent coordinator review before production: root inspected original finalizer and `failed_motion_and_polling` against base127036f; `finish_motion(1000)` stops at10 degrees while target50 was accepted and overlap60 refused. The original OK expectation is obsolete under finding012/AC-001. Preserve accepted50/refused60 and move counts; change terminal expectation only to ALERT with actual10/target50. This is a contract correction authorized by independent review, not weakening coverage.

Tolerance decision before production: authoritative manufacturer product page https://us.zwoastro.com/products/camera-angle-adjuster specifies0.02 degree/step separately from optical positioning accuracy<0.1 degree; official manual https://i.zwoastro.com/wp-content/uploads/2025/01/dfe48610abdd30aa57a31b0dd0431de8.pdf also identifies0.02 degree/step. Local CAA_API.h declares float angles, no decimal tolerance. Inference approved by coordinator: permit one step0.02 plus float rounding scaled by angle magnitude; compare absolute mechanical angles, no modulo360 (travel supports480). Test boundary and outside. Do not use optical0.1 accuracy as SDK readback tolerance.

Step1 source audit/read-only planning complete; step2 original driver+fake suite builds PASS. Added SDK/public-state trace instrumentation only; original11-case baseline follows before production changes. No hardware authorized/planned.

Steps2/3 complete before production: original11/11 PASS with full SDK/public-state baseline.trace. Original-source new absolute/relative stopped-short cases FAIL0/2; resolution-bound case FAIL0/1 at outside bound after valid boundary; mechanical360→0 FAIL0/1; external-motion preservation control PASS1/1. No expected assertions weakened. Implement owned-move check and independently reviewed recovery correction next.

Step4 implementation complete: v10→11, .driver and unchanged generator outputs synchronized. Added commanded_move ownership and absolute difference <=0.02°+FLT_EPSILON*max(abs(actual),abs(target)); abort handling remains prior to arrival check. Ownership survives read errors and clears on successful terminal/close/connect/sync or rejected SDK start. Original external target behavior retained per coordinator. Existing recovery expectation corrected only as independently reviewed. Final native and strict sanitizer full suites underway.

Step5 first16-case native/sanitizer suites PASS16/16, then separate source review found staleownership regression after stopped poll failure + rejected sync. New original-source control PASS1/1; first repaired candidate FAIL0/1. Returned step4: clear ownership when readiness positively confirms stopped (never on failed status read or active motor/manual/abort). Added17th case and updated count; final17-case rerun required.

Steps4/5 complete: final recorded17/17 PASS; strict production+fake/test ASan/UBSan17/17 PASS; repeat generation C/header/main hashes match. Ten unaffected original case traces identical; recovery changes only four terminal state lines (two OK→ALERT pairs), SDK interactions identical. No hardware0/0. Step6 independent review/canonical reconciliation/Git remains coordinator-owned. Developer releases writer after handoff.

Coordinator checkpoint: independent scoped quality/security/verification passes recorded. All implementation steps complete; source unchanged after review. Next canonical findings closure, final metadata review, archival/commit and batch assembly. No audit cadence reset.
