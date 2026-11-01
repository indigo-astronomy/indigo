# Verification evidence

Status: final local verification complete; committed delivery gate pending.
Source baseline before implementation: e5e77e58edd20f102577a8f931651f802bdfc612. Final working diff has the two guards, version bumps, tests and count update described below.
Working directory: /Users/polakovic/Development/indigo.
Host: macOS arm64; no hardware validation planned. Tests are non-interactive and local.

## Baseline and found defects
At the initial baseline neither agent checked the other's initialized state. Test developer captured existing-suite results and the failing new regression before the two production guards were added. Final acceptance passes and two discovered findings are recorded below.

## Baseline commands and outcomes
Test developer reports (all from repository root, unmodified production baseline):
- `make -C indigo_test test-agent-solver` passed 21/21; /tmp/solver-exclusive-baseline-solver.log.
- `make -C indigo_test test-agent-astrometry` initially passed 52/54; filesystem failure message and index-abort deadline cases failed. Each targeted rerun passed 1/1; source unchanged, hence baseline timing sensitivity rather than a newly introduced regression. Logs /tmp/solver-exclusive-baseline-astrometry.log, /tmp/solver-exclusive-astrometry-filesystem.log and /tmp/solver-exclusive-astrometry-abort.log. Final full scope must be rerun.
- New registry regression: `indigo_test/build/integration/test_agent_solver excludes` fails 0/2, both because second real agent INIT returns INDIGO_OK instead of INDIGO_FAILED; /tmp/solver-exclusive-baseline-new.log. Fresh inactive registration cases pass 2/2 and failed-dependency case passes 1/1.
- New tests built with `make -C indigo_test build/integration/test_agent_solver` against original agent source.

### Newly observed preexisting loader limitation
The test designer observed `indigo_remove_driver()` does not clear initialized, and later add_driver(init=false) can inherit that stale flag in a recycled slot. This affects custom registration sequences, not fresh static discovery in the normal server. Preserve a canonical nonblocking follow-up; do not expand this task into a loader repair. Inactive registration acceptance targets fresh normal-server discovery, not a guarantee that the preexisting slot-reuse bug has been repaired.

## Initial post-change results
- Test developer: narrow Solver build passed, `make -C indigo_test test-agent-solver` passed26/26 including five new real-registry cases. Log /tmp/solver-exclusive-fixed-solver.log.
- `make -C indigo_drivers/agent_astrometry -f ../../Makefile.drv all` and `make -C indigo_drivers/agent_solver -f ../../Makefile.drv all` exited0 on changed source; /tmp/solver-exclusive-build-astrometry.log and /tmp/solver-exclusive-build-solver.log.
- Loader unit baseline in sandbox failed one loopback-bind case; the unchanged suite rerun outside sandbox passed7/7 (test developer, /tmp/solver-exclusive-loader-unsandboxed.log). This is an environment restriction, not an initialization regression.
- Owner approved local initialization/regression scope and Factory evidence instead of the full README test record on 2026-11-01. Real-catalogue opt-in Solver sweep omitted, no indexes downloaded; it tests unchanged numerical solving and is outside the approved bounded run. No hardware tests run.

## Final local product verification
- Root `make -C indigo_test test-agent-astrometry`: full54/54 passed; /tmp/solver-exclusive-final-astrometry.log. This reruns the two initially failing cases in full scope.
- Test developer final `make -C indigo_test test-agent-solver`:26/26 passed after the unsigned-byte fixture correction; /tmp/solver-exclusive-final-solver.log.
- Full temporary ASan/UBSan Solver integration:26/26 passed; both agents and native solver/test instrumented, framework dynamic library uninstrumented, leak detection disabled consistent with existing sanitizer policy. /tmp/solver-exclusive-final-sanitize.log. Exact build invocation is retained in sanitizer-command.md.
- Five new cases cover both exclusion orderings/unload-retry, both fresh inactive registrations and failed Astrometry prerequisite initialization. Both reciprocal missing-exclusion failures observed before production change now pass.
- Native builds produced existing universal macOS x86_64/arm64 driver outputs; runtime tests use this macOS arm64 host. Linux/Windows and physical hardware validation were not performed.

## Found defects and disposition
FND-20261101183631-1a1e-solver-exclusive-init-001 remains open/nonblocking for a future custom-host loader-slot repair; normal server contract is unaffected and independently reviewed. FND-20261101183631-1a1e-solver-exclusive-init-002 (preexisting signed-byte test fixture UB) is fixed and reverified with unchanged rejection assertions. Supporting reproducer is in findings.md; no unrelated production change was made.

## Final root verification and workflow checks
- `INDIGO_TEST_RESULTS=/tmp/solver-exclusive-final-results.tsv make -C indigo_test test-agent-solver` exited0,26/26 cases passed; machine-readable plan26 plus26 pass records retained. Root rerun after final fixture edit: /tmp/solver-exclusive-root-solver.log.
- `./.factory/scripts/check` exited0; whitespace, finding integrity, decision index, Factory project boundaries passed.
- `./.factory/scripts/test` exited0;190 Factory tests passed in110.130seconds.
- `./.factory/scripts/security` exited0; local boundary validation passed.
- `git merge-base --is-ancestor master HEAD` exited0; master and origin/master both still e5e77e58edd20f102577a8f931651f802bdfc612, the included base. No merge necessary; no remote fetch/push or integration performed.
Tool versions: Python3.9.6, Git2.54.0 (Apple Git-157).

## Acceptance verdict
Verdict: pass
AC-001/002: reciprocal real-registry scenarios pass, rejected agent remains absent and retry after unload works; fresh inactive competitors and failed prerequisite initialization are handled. AC-003: versions increased to Astrometry0x02000023/Solver0x0300001A, repeat INIT/INFO checked, existing synthetic solving tests pass and no properties changed. AC-004: five baseline-sensitive new tests and approved local regression scope pass; no ASTAP/generator/framework change.

Review limitation: root authored production code and verified it; independent reviewer separately inspected exact source identities and evidence. No Linux/Windows runtime, hardware, real-catalogue sweep or Astrometry opt-in cooperation suite claim is made. Full runner/README record omitted under explicit owner waiver; baseline timing failures were rerun in complete local54-case scope and passed.

Final test summary:80 local integration cases (Solver26 + Astrometry54) passed;7 loader unit cases passed outside sandbox;26 sanitizer integration cases passed;190 Factory tests passed. Physical hardware tests:0 run/0 passed. Baseline runs, targeted debugging reruns and final repeated ordinary Solver runs are excluded from these unique final-case totals. Committed-range process gate and clean-tree checks remain post-commit delivery evidence.

Post-verification cleanup: `make -C indigo_test test-clean` exited0 and removed test build outputs after retained result artifacts were copied into this task.

The first committed-range process check detected a missing data-model review annotation for MIGRATION_STATUS.md. Its actual count-only diff was already inspected by independent review. Added explicit security.md coverage and process.risk_reviews metadata; product identities and runtime evidence are unchanged. Re-run committed gate after correcting the unpublished local evidence commit; final result belongs to external delivery evidence.
