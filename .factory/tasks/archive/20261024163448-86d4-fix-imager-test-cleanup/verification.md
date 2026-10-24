# Verification

Verdict: pass
Verifier: root (also implemented the final integration); fresh independent source review is in quality-review.md. Working directory: `/Users/polakovic/Development/indigo`; platform macOS arm64, Apple clang 21 and Python3.9. Final source identities are in process.json. Production libraries are inherited from verified dependency commit `79dcf6d81e633460acfbefcf1701ca214d5089bc`.

## Baseline

The finding's retained prior run logged an attached CCD simulator preventing bus stop after the case had already recorded pass. Source inspection at base confirms cleanup ignored all shutdown/detach/stop results. Root built the existing instance binary with `make -C indigo_test build/integration/test_agent_imager_instances`; baseline `INDIGO_TEST_FAIL_CLEANUP='CCD simulator shutdown' INDIGO_TEST_RESULTS=/tmp/indigo-cleanup-baseline.tsv indigo_test/build/integration/test_agent_imager_instances 'independent instances'` exited 0 with one pass, showing the original binary had no enforced negative-result path. This baseline is supplementary to the real prior warning; it does not claim an SDK failure was injected into original code.

## Executed focused checks

- `make -C indigo_test build/integration/test_agent_imager_cleanup`: pass, builds the actual instance executable plus registered regression wrapper.
- Normal filtered instance case with private HOME: exit 0, one plan/one pass after cleanup; no attached-device warning.
- `indigo_test/build/integration/test_agent_imager_cleanup`: 4/4 pass. CCD shutdown, observer detach and bus stop failures each produce child exit 1 with exactly one fail and no pass; later cleanup continues. The fourth case supplies conflicting environment/argument filters and proves the selected callback actually executes.
- `git diff --check` and `plutil -lint indigo.xcodeproj/project.pbxproj`: pass.
- `./.factory/scripts/check` and `./.factory/scripts/security`: pass.
- `./.factory/scripts/test`: 188/188 pass (96.070 seconds).

## Sanitizer command and limits

Built the actual changed harness using `clang -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -Iindigo_libs -Ibuild/include -I. -Iindigo_test/integration -fsanitize=address,undefined -fno-omit-frame-pointer -g indigo_test/integration/test_agent_imager_instances.c build/drivers/indigo_agent_imager.a build/drivers/indigo_agent_guider.a build/drivers/indigo_ccd_simulator.a -Lbuild/lib -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -lindigo -lindigocat -lpthread -lm -o /tmp/indigo-cleanup-instances-san`.

Ran `/tmp/indigo-cleanup-instances-san 'independent instances'` and `python3 indigo_test/integration/test_agent_imager_cleanup.py /tmp/indigo-cleanup-instances-san`: normal case and all four negative regressions passed without ASan/UBSan diagnostics. The four expected-failure checks also passed with `ASAN_OPTIONS=halt_on_error=1:exitcode=86 UBSAN_OPTIONS=halt_on_error=1:exitcode=86`, so a sanitizer failure cannot be mistaken for the expected child exit 1. Existing production libraries are not instrumented; this validates the changed harness, not complete drivers.

## Acceptance matrix

| Criterion | Evidence |
| --- | --- |
| AC-001 | Ordered stop/join/disconnect source review and full instance scenarios |
| AC-002 | All cleanup results checked; three fault-stage regressions verify failure and continuation |
| AC-003 | Case callback includes cleanup; exact fail/no-pass and process status assertions |
| AC-004 | Fault regressions pass; final complete recorded scope and warning check follow below |
| AC-005 | Existing assertions retained; plan before fork; conflicting-filter regression |

No production driver behavior/version/property change, hardware, external service, Linux/Windows runtime or physical timing validation is claimed. This is non-interactive simulator verification for the requested repair. Final recorded result, evidence archival and post-commit gate are reported after execution; no unperformed check is a pass.

## Final recorded acceptance

`python3 tools/run_driver_test.py agent_imager` exited 0, rebuilt the driver/test prerequisites and recorded **61/61 OK** for version 3.0.0.65, mac arm64 simulator: four cleanup regressions, two numerical cases, 39 imager cases, ten cooperation cases and six instance-coordination cases. All six instance cases passed after their checked teardown. The complete output contains no `can't stop` or `Cleanup failed:` message. README Testing and generated TEST_SUMMARY.md were updated by the script. No driver version bump is needed because production code did not change.

Retained output (trailing whitespace normalized for diff hygiene, result text unchanged): [imager-recorded.log](imager-recorded.log), [factory-test.log](factory-test.log), and the supplementary [baseline-results.tsv](baseline-results.tsv). All five acceptance criteria pass. The requested finding is fixed; no unresolved new finding or waiver remains. `make -C indigo_test test-clean` removes generated test artifacts after the completed processes. Local master synchronization was already up to date; the dependency/base remains 79dcf6d81e633460acfbefcf1701ca214d5089bc. Final commit SHA and committed-range process result are reported externally, avoiding self-referential committed evidence.
