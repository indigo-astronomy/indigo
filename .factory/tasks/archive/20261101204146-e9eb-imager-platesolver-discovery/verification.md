# Verification

## Scope, environment and owner steering

Working directory `/private/tmp/indigo-20261101204146-e9eb-imager-platesolver-discovery`; baseline revision `3c137d7840cb965924ba106f083139fba1e6d72d`. macOS arm64 with `/usr/bin/clang`; normal driver and strict compilation cover universal x86_64/arm64 slices, test execution is native arm64. Linux/Windows execution and physical hardware were not performed. Developer checks are not independent review.

The owner subsequently instructed **do not repeat image recognition merely for this rename**. Coordinator relayed the instruction and explicitly replaced the broad final-driver recording requirement with targeted naming/registration/related-agent/config/build checks. The in-progress broad sanitizer and recorded runs were terminated, never restarted, and are not represented as complete. No README Testing or TEST_SUMMARY change was produced by the interrupted recorder; existing historical records remain intact.

## Baseline audit and prerequisites

The complete pre-production audit and hardware decision are in plan.md. Original hand-written driver version `0x03000043`; no generator source. Only the related-agent whitelist and version change. No property additions/removals, item counts/hidden flags, helper/tool/cache/config identities, queues, timers, transport order, resource ownership or platform registration changed. New production version `0x03000044` (3.0.0.68).

A fresh local Makefile.inc was generated. External BUILD_LIB/BUILD_INCLUDE point read-only to existing owner-checkout artifacts. Regular local copies of required unchanged driver archives and libindigo.a satisfy existing hardcoded test/recording prerequisites; no symlinks/shared writes. Exact SHA256 matching evidence is `dependency-provenance.txt`.

## Original-driver preservation baseline, completed before production edits

| Command at worktree root | Actual result | Evidence |
| --- | --- | --- |
| `make Makefile.inc BUILD_LIB=/Users/polakovic/Development/indigo/build/lib BUILD_INCLUDE=/Users/polakovic/Development/indigo/build/include` | PASS | configure.log |
| `python3 tools/run_driver_test.py agent_imager --no-record` | Original driver builds; inherited numeric fixture compilation fails before tests | baseline.log |
| `make -C indigo_test build/integration/test_agent_imager build/integration/test_agent_imager_guider_mount` | PASS | baseline-build.log |
| `indigo_test/build/integration/test_agent_imager` | PASS39/39 including cleanup, exit0 | baseline-imager.log |
| `indigo_test/build/integration/test_agent_imager_guider_mount` | PASS10/10 including cleanup | baseline-cooperation.log |
| `indigo_test/build/integration/test_agent_imager_instances` | PASS6/6 including cleanup | baseline-instances.log |
| `indigo_test/build/integration/test_agent_imager_cleanup indigo_test/build/integration/test_agent_imager_instances` | PASS4/4 | baseline-cleanup.log |
| `indigo_test/build/integration/test_agent_imager_numerics` | PASS4/4 after authorized fixture correction | baseline-numerics.log |
| `make -C indigo_test build/integration/test_agent_imager build/integration/test_agent_imager_numerics` | Test-only characterization/fixture build PASS | characterization-build.log |
| `indigo_test/build/integration/test_agent_imager 'discovery compatibility'` | PASS1/1, 16 preserved positive/negative names | original-related-trace.log; normalized original-related-trace.txt |
| `indigo_test/build/integration/test_agent_imager 'platesolver agent discovery'` | EXPECTED FAIL0/1: new name absent on original driver | expected-baseline-mismatch.log |

All63 existing preservation cases passed before production edits. Cooperation→instances→cleanup→numeric commands ran sequentially in one shell that exited0; each suite's passing summary is retained. The standalone main command separately exited0. Initial recording preflight required a regular local build/lib/libindigo.a; no tests ran on that preflight attempt. Baseline image recognition was completed before the owner's later stop instruction and is retained as actual history, not rerun.

## Found defects and fixes

1. **Expected compatibility gap, reproduced:** original validator omits Platesolver Agent, so the renamed peer cannot appear for selection. Minimal fix adds a prefix comparison using `sizeof("Platesolver Agent") - 1` (17 characters), preserves every legacy branch, and increases driver version. Public new-name regression now passes for bare, instance, remote and instance+remote names.
2. **Inherited numeric fixture compile defect, reproduced:** extracted platesolver start_exposure now references AGENT_START_PROCESS_PROPERTY/select_filter after prior prerequisite changes, but the fixture declared neither. `baseline.log` retains compiler errors. Coordinator approved a fixture-only correction: separate start-process property explicitly IDLE and a tracked filter stub. All original assertions remain; an added zero-call assertion proves the stub is outside the numeric scenarios. No production workaround. Corrected baseline numeric tests and independent coordinator numeric rerun pass4/4.

## Completed changed-source checks

| Command | Actual result | Evidence |
| --- | --- | --- |
| `make -C indigo_test build/integration/test_agent_imager` | PASS | changed-build.log |
| `indigo_test/build/integration/test_agent_imager 'discovery compatibility'` | PASS1/1 | changed-related-trace.log |
| `indigo_test/build/integration/test_agent_imager 'platesolver agent discovery'` | PASS1/1, four new-name forms listed/selectable OK/removed on detach | changed-new-trace.log |
| Python extraction of `RELATED` lines and equality assertion | Preserved public trace byte-identical to original | original-related-trace.txt; changed-related-trace.txt |
| `python3 .factory/tasks/20261101204146-e9eb-imager-platesolver-discovery/developer_checks.py strict` | PASS exit0, both universal slices | strict-final.log |
| `ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 indigo_test/build/task-imager-sanitize/test_agent_imager_sanitize discovery` | PASS2/2 including cleanup, exit0, no sanitizer diagnostics | sanitize-focused.log |
| `git diff --check` | PASS | Executed after final scoped checks |
| `make -C indigo_test test-clean` | PASS; test binaries removed after logs retained | test-clean.log |

Focused sanitizer uses the actual changed production Imager object and test compiled with address+undefined sanitizers, native arm64, O1 and frame pointers. Shared library/simulator prerequisites are uninstrumented; ASan leak detection is disabled. No exposure or image recognition is requested by the two `discovery` cases. Exact compilation/link commands are retained at the beginning of sanitize-final.log and task-local developer_checks.py.

Independent coordinator report: native compatibility1/1, new-name1/1 and numerics4/4 reruns exited0; production prefix/version and numeric fixture idle/counter branch reviewed with no scoped issue. This report is coordinator-provided evidence; independent quality/security review remains coordinator-owned.

## Strict baseline limitations

The exact universal-clang `-Wall -Wextra -Werror -Wno-unused-parameter -fsyntax-only` command on original source failed on five inherited warnings: two unused-but-set locals at3382/3383, two signedness comparisons at3210 and one incomplete initializer at3893. `strict-baseline.log` retains them. The repeated scoped strict command additionally suppresses only `unused-but-set-variable`, `sign-compare`, `missing-field-initializers`; unchanged baseline and final both pass (`strict-baseline-scoped.log`, `strict-final.log`). No unrelated warning fixes applied. This is not a claim of unsuppressed warning cleanliness.

## Owner-interrupted broad attempts, not full validation

- `python3 .factory/tasks/20261101204146-e9eb-imager-platesolver-discovery/developer_checks.py sanitize` was stopped at the owner's instruction. Main suite had completed40 PASS cases out of intended41 with zero failure/sanitizer markers. The final exposure-failure case remained active; subsequent numeric sanitizer stage was never executed. Runner/test process group76084 exited143 after TERM. Evidence: sanitize-final.log. This is partial evidence only.
- `INDIGO_TEST_RESULTS=.factory/tasks/20261101204146-e9eb-imager-platesolver-discovery/final-results.tsv python3 tools/run_driver_test.py agent_imager` was stopped at the same instruction. Recorder overrides the environment path with its own temporary results file; that file was retained as interrupted-recorder-results.tsv before cleanup. It contains47 actually planned cases and46 passes: cleanup4 + numerics4 +38 main cases completed, one main case interrupted. Three main cases, cooperation10 and instances6 had not completed in this attempt. Intended driver inventory is65; **no65/65 result is claimed or recorded**. Process group76195 exited143 after TERM. Evidence: final-recorded.log and interrupted-recorder-results.tsv.
- Process listing/TERM needed automatically reviewed escalation because sandbox denied ps/kill. Only the two identified Imager groups were terminated; a concurrent Mount group was untouched. Follow-up process listing confirmed no matching remaining Imager/runner process. Two known private homes from the cancelled runs were removed, recorder temporary results retained then removed, and test-clean completed. Normal source/archive artifacts remain available to the coordinator.

## Acceptance mapping and limits

AC-001/002: production public INFO→related-list→selection→detach path, both legacy Solver prefixes/instances, all other allowed prefixes, truncated/case/embedded/unrelated negatives, four new-name forms; preservation trace identical. Existing prefix semantics intentionally remain unchanged; no delimiter tightening.
AC-003: source version67→68, prefix-only production change; no generator/property/build-registration edits needed. Coordinator owns global MIGRATION_STATUS count reconciliation (inventory65) and preserves Comment.
AC-004: completed targeted native/sanitizer and universal scoped strict checks above; broad final recording is intentionally incomplete under explicit owner steering. No image-recognition rerun is requested for this name-only change. No Linux/Windows execution, physical hardware or leak certification claim.
AC-005: no stage/commit, merge, publication, provider, generator or shared-artifact writes by developer. Existing overdue-audit/process blocker remains coordinator-owned; this task does not reset maintenance.

## Final test summary

- Final naming-focused simulator acceptance:2/2 native developer cases and2/2 ASan/UBSan cases passed, including cleanup. Independent coordinator targeted reruns: naming2/2 and corrected numerics4/4 passed.
- Original full preservation baseline:63/63 passed before production edits. Additional original compatibility characterization1/1 passed; new-name reproducer0/1 expected failure then fixed.
- Interrupted broad attempts: normal46 PASS out of47 actually planned (intended full inventory65), sanitizer40 PASS of intended41 main cases. Both cancelled by owner and exited143; neither is a full-suite pass.
- Physical hardware tests run/passed:0/0. Linux/Windows tests run/passed:0/0.

Verdict: pass

Independent mount_identity final source/test/documentation quality/security passes under explicit owner exception, no new recognition. Configured check/security results are in factory-check.txt/factory-security.txt. Common unchanged Factory tooling test group ran from main rename worktree,190/190 pass (factory-test.txt); this is process tooling, no image recognition.
