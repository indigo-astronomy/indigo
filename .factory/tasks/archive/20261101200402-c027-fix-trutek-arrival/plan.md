# Atomic plan
1. Analyze current source, public contract, dependencies, failure paths, relevant docs and existing tests. Read-only planning completed; task-specific baseline audit follows before production edits.
2. Build/run original relevant suite; capture reproducible defect and preserved behavior. For drivers retain protocol/SDK/public-state reference trace; record no-hardware decision.
3. Add meaningful failing characterization/regression for AC-001, preserving original tests except independently reviewed obsolete expectations.
4. Implement smallest scoped fix with platform/error/ownership checks; update driver versions and unchanged-generator outputs where applicable.
5. Rerun full applicable scope, strict/sanitizer checks and trace comparison; final driver run via tools/run_driver_test.py, update only authorized Testing records/status counts.
6. Independent quality/security review, fix/retest as needed, reconcile findings, archive completed task and commit its outcome. Main publication/integration remain pending; inherited unfinished audit can block aggregate process gate.
Update actual result/evidence immediately after each step; never mark an unperformed gate passed.

Step 1 complete: bounded current-state audit in verification.md; no hardware authorized/planned. Step 2 original suite PASS12/12 with normalized wire/state trace; defect characterization under step3 still running before production changes.

Step 3 complete before production change: wrong-slot regressions FAIL0/2; malformed selection FAIL1/3 with original identity case passing; immediate matching ACK and overlap controls PASS1/1 each. Five malformed raw payload fixtures tested. Tests retain original 12 cases unchanged. Proceeding to smallest source fix and version bump.

Step 4 complete: .driver v9→10, query decoder rejects below0/abovemax before mutation, ACK rejects abovemax before mutation, both stopped completion paths compare actual to retained requested target. Generated via unchanged executable; native build passes. Simulator previously had no version marker; version2 identifies new fault/trace capability (normal protocol behavior preserved). Step5 final script and sanitizer full suites in progress.

Step5 first full run caught malformed-ACK actual-value failure (18/17). Returned to step4: restore cached actual on ACK failure; no expected behavior/test assertions changed. Affected full native/sanitizer retest required.

Steps4/5 complete after cached-value fix: final runner PASS18/18; separate strict ASan/UBSan full suite PASS18/18; all15 original wire traces and original public transitions identical. Generator repeat and diff whitespace pass. Step6 remains assigned independent review/coordinator registry and Git delivery; developer releases writes after handoff.

Coordinator checkpoint: independent scoped quality/security/verification passes recorded. All implementation steps complete; source unchanged after review. Next canonical findings closure, final metadata review, archival/commit and batch assembly. No audit cadence reset.
