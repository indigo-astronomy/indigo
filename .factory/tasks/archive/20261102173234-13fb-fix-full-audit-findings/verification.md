# Verification

Verdict: pass

Reviewed final product candidate11277e778, including coordinator-owned integration metadata; configured main target5324bf633 remains included.

## Native solver baseline and found defects

Working directory `/Users/polakovic/Development/indigo`, base5324bf633, Darwin arm64 host, universal macOS production build. Root developer executed checks; independent agents reviewed source separately.

- `make -C indigo_drivers/agent_platesolver -f ../../Makefile.drv all`: baseline and fixed builds pass (x86_64/arm64 compilation only; no x86_64 runtime claim).
- `make -C indigo_test build/unit/test_agent_platesolver_core`: pass.
- Baseline `INDIGO_TEST_CASE_FILTER=load_raw_tiny_binning_rejected indigo_test/build/unit/test_agent_platesolver_core`: expected failure, valid1x1/downsample2 incorrectly returned0x0 image. Preserved baseline `load_raw_mono16_binned`:1/1 pass. The reported huge-header fatal-allocation path was not executed on vulnerable source.
- Fixed `INDIGO_TEST_CASE_FILTER=load_ indigo_test/build/unit/test_agent_platesolver_core`:12/12 pass. Header-only huge/zero/truncated dimensions across four RAW formats, tiny RAW/FITS downsampling, extreme Bayer downsample, four format conversions and prior FITS normalization/binning tests.
- Fixed `INDIGO_TEST_CASE_FILTER=image_stars_invalid_input ...`:1/1 pass, including actual detector failure for unsupported signature, empty dimensions and nonpositive capacities.
- Fixed `INDIGO_TEST_CASE_FILTER=compute_center_uses_astrometry_net_orientation ...`:1/1 pass (14 synthetic CD-matrix orientations/parities, no image recognition).
- `clang -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -I. -Iindigo_libs -Ibuild/include -Iindigo_drivers/agent_platesolver indigo_test/unit/test_agent_platesolver_core.c indigo_drivers/agent_platesolver/indigo_solver.c indigo_drivers/agent_platesolver/indigo_solver_utils.c indigo_drivers/agent_platesolver/indigo_solver_healpix.c -Lbuild/lib -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -lindigo -lindigocat -ljpeg -lm -lpthread -o /tmp/indigo-solver-sanitize/core`: pass. Same three filters:14/14 pass, no sanitizer diagnostic. Solver sources and test instrumented; prebuilt common library not instrumented.

021: unchecked extent narrowing let huge RAW header pass. Common RAW validator now checks input extents before conversion; solver bounds dimensions/pixels for signed detector arithmetic and normalized output.
024: zero binned geometry and ignored detector failure exposed uninitialized count. Reject zero post-bin geometry, initialize count, honor result and returned capacity; new tiny RAW/FITS and invalid extraction regressions pass.
022: radius is catalogue hint with field extent; corrected false fitted-centre bound, no solve algorithm change.
023: Astrometry parser maps `Field parity: pos` to+1 and hints+1 to`--parity pos`; native negative CD determinant maps+1. Header now matches mirrored/east-right convention; existing independent CD-matrix test passes.

No real-image recognition, catalogue downloads, hardware or Linux/Windows runtime checks. These are focused regression checks, not a full driver test-run certification. Detailed retained logs: evidence/.

## Combined evidence so far

Imported test-integrity task20261102173322-3b15 retains9/9 focused methods,5/5 ASan/UBSan methods,120 valid driver inputs with unchanged generated output, expected failing baseline and owner002 approval. Root independently inspected source and actual test harnesses.
Imported SDK task20261102173318-befe retains Atik2 29/29 and Pentax45/45 full final recorded runs, Pentax45/45 sanitizer cases,72 measured software pulse timing samples (not physical relay accuracy). Root independently inspected final code/tests and reports.
`make -C indigo_test test-harness-verdicts`: exit0,15 existing harness methods +5 custom helper methods +4 tokenizer methods.
`plutil -lint indigo.xcodeproj/project.pbxproj`: pass. `python3 .factory/scripts/decision_index.py --check`: pass.
`./.factory/scripts/test`: exit0,197 tests in114.468s; retained evidence/factory-tests.txt. Factory helpers unmodified, these retained results need no metadata-only repeat.

## Integration checks

Main target refreshed by authorized fetch: origin/master remains5324bf633; no target source advancement. All child branches start from that source, and shared-library/framework code outside selected PrimaLuce function and native helper is unchanged. Reused isolated relevant driver execution evidence rather than repeating unrelated suites after clean cherry-pick integration. `./.factory/scripts/check` and `./.factory/scripts/security`: pass, scoped Factory validation only. Initial product-only risk inspection had no path-pattern signal. The final status-table change triggers the conservative data-model filename rule; its review is explicitly recorded in security.md. Substantive native/driver trust-boundary review is also recorded there.

All ten fixed driver versions exceed their pre-change values; see evidence/driver-versions.txt. New native core cases belong to unit inventory and do not alter the recorded driver-run count162 (integration29 + simulator133); no full real-image run was recorded. Hardware counts and manually owned Comment cells in MIGRATION_STATUS remain unchanged. Shared TEST_SUMMARY is coordinator-generated from final driver README Testing records to avoid concurrent writers; source commits and shared-registration commit remain separate, retained exceptions to per-driver record co-location. No published history is rewritten.

`make -C indigo_test test-clean`: pass; retained test logs remain under task evidence.

## Final combined acceptance

| Acceptance | Result |
| --- | --- |
| AC-1 | Pass: bounded RAW validation and valid format controls. |
| AC-2 | Pass: tiny post-bin/detector failure/capacity regressions and sanitizer checks. |
| AC-3 | Pass: corrected radius hint prose and parity header; compared Astrometry parser/hints and native CD-matrix tests. |
| AC-4 | Pass: all17 requested findings repaired, reviewed and archived; origin references retained. |

Serial task20261102173307-8455 retained final unfiltered wrapper runs: Falcon16/16, Trutek19/19, Astromechanics8/8, FlipFlat17/17, ALTO34/34 with combined019 assertion, GIOTTO30/30 and focuser76/76 (200/200 total); protocol unit37/37. Strict native checks cover8 implementation units without diagnostics; numeric helper locale and sanitizer proof retained in child evidence. Root reviewed final report and imported matching source bytes.

Final independent reviewer review_repair_candidate passed source composition2f6797bc6, confirmed all33 serial source/doc/test files matched the tested child, and registrations/counts/HW/Comment preservation/protected-path exclusion. Its nonblocking note was corrected in11277e778:014 simulator test now calls raw primalucelib_request instead of requiring the known wrong setter verdict. It sends identical wire bytes but stays independent of the separately tracked LED setter bug. Reviewer confirmed the one-line correction. `make -C indigo_drivers/aux_alto -f ../../Makefile.drv all` and `make -C indigo_test build/integration/test_aux_alto_simulator`: pass. Initial direct test from repository root failed because the fixture path expects indigo_test cwd; corrected invocation passed1/1. Final `ALTO_TEST_FILTER=refused_simulator_led_write_retains_state python3 tools/run_driver_test.py aux_alto --no-build --no-record`: exit0,1/1; intentionally filtered, no full-run record rewritten. Production unchanged, so earlier full driver acceptance is retained with this focused test refinement qualified.

Central findings integrity:45 active/245 archived after17 requested closures plus separate new medium functional nonblocking LED ACK finding FND-20261102173307-8455-repair-serial-audit-findings-001. New issue has accountable coordinator follow-up and retained exact reproducer; it is not waived or silently included in requested AP closure. Seven unrequested fa40 findings remain active. Full audit source/checkpoint unchanged.

Final test summary: driver acceptance274/274 (serial200 plus SDK74); native focused14/14; native sanitizer14/14, SDK sanitizer45/45, helper sanitizer5/5; protocol unit37/37; existing/custom/tokenizer harness24/24; Factory197/197. These are separately scoped counts, not an additive certification score. Additional final014 focused refinement1/1. Physical hardware0 run/0 passed; no real-image recognition, index download or unavailable platform runtime claim.

Final committed-range process gate is run after the Factory commit; its immutable result is recorded externally, as required by the archival/commit ordering. Integration/publication remain pending for this repair task.

Retained console output is normalized only for trailing whitespace to satisfy repository whitespace checks; commands, values, failures and exit outcomes are preserved. A process invocation accidentally started before the Factory commit existed is not final evidence; the final immutable range is checked only after the successful commit.

Post-commit process check of deb3a3d02 found a missing data-model review binding for MIGRATION_STATUS.md. The already-reviewed status-table scope is now explicitly bound to security.md; no product change or risk-policy weakening. Final immutable gate is repeated on the additive Factory correction commit; earlier failed gate is not a passing result.
