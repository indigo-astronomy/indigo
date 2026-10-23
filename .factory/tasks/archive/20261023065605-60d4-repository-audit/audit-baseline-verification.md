# Audit verification

Verdict: changes required
Reviewer: current Codex agent, 2026-10-23. Same agent collected and reviewed evidence; no independent verification claimed.
Source revision: e0139d57ceb8bb6f4421efa8849c8541690e8b2c, with product sources unchanged. The audit task and nine finding records are uncommitted. All commands below ran from /Users/polakop/Development/indigo unless cwd is specified.
Environment: macOS arm64; Python 3.9.6; existing Apple Clang/Git/Node tools. No new dependency installed and no physical hardware exercised.

## Acceptance

| Criterion | Result | Evidence |
| --- | --- | --- |
| AC-001 | pass | inventory.json and driver-inventory.json retain tracked candidates, exclusions, class counts and file-level scope. Inventory-only bundles/assets are identified in the review. |
| AC-002 | partial | Repository-wide static coverage, all driver generation and targeted semantic inspection; remaining depth is explicit in repository-review.md. A full semantic pass is not claimed. |
| AC-003 | pass | Nine canonical findings have concrete source/evidence, severity/blocking, owner and next repair action; 42 prior active records unchanged. findings check validates 51 active/18 archived. |
| AC-004 | pass for evidence, findings remain | Passing and failing native/Factory/editor/compiler outcomes below are kept separate from environmental limitations. No failing result is represented as a pass. |
| AC-005 | partial | Reports are populated; full-audit qualification and final task completion are withheld because coverage and blocking finding gates remain unsatisfied. |

## Exact checks and outcomes

| Command / cwd | Exit | Evidence / result |
| --- | --- | --- |
| `./.factory/scripts/check` | 0 | Whitespace, finding registry, decision index and Factory project boundary checks pass. |
| `./.factory/scripts/security` | 0 | Local Factory boundaries pass; not an application security certification. |
| `./.factory/scripts/test` | 0 | 175 tests in 158.988s, OK. |
| `python3 .factory/tasks/20261023065605-60d4-repository-audit/run-analyzer.py` | 0 (controller) | analyzer-results.json: 82 requested C units, 77 exit 0; five compile failures/unavailable units. Warnings are candidates, not proof. Actual per-file arguments and diagnostics retained. |
| `python3 -` (stdlib AST/local Markdown scan) | 0 | static-inventory-checks.json: six legacy Python 2 parse candidates and 43 relative link candidates; five current instruction-guide links confirmed defects, wiki HOME links need intended-renderer triage. |
| `python3 -` (XML / node / bash inventory checks) | 0 | manifest-syntax-checks.json: XML well formed; six JS syntax checks and 24 shell syntax checks pass. Missing project sources are candidates needing target classification. |
| `plutil -lint indigo.xcodeproj/project.pbxproj` | 0 | Xcode project syntax OK. |
| `make -C indigo_drivers/shared/primalucelib BUILD_LIB=/tmp/indigo-audit-primalucelib-20261023 all` | 2 | primalucelib-build.log: clean build reaches configured libtool with invalid rcs arguments. Confirmed finding 001. |
| `make -C indigo_test test-unit` | 2 | native-unit.log: pre-existing Oct 20 framework binary lacks new serial API symbols; no test result inferred. |
| `make -C indigo_libs "$PWD/build/lib/libindigo.dylib"` (sandbox) | 2 | framework-build.log: bundled read-only FTDI image mount unavailable under sandbox. |
| Same targeted framework command outside sandbox | 0 | framework-build-unsandboxed.log: current framework built successfully; no driver build or install. |
| `clang indigo_tools/indigo_generator.c -o /tmp/indigo-audit-generator-20261023` | 0 | Fresh unchanged generator built outside repository outputs. |
| `python3 -B -m unittest discover -s indigo_tools/driver_editor -p test_editor.py` | 1 | editor-tests.log: 35 tests; two failures, five errors; three HTTP errors were sandbox permission failures. |
| `python3 -B -` (load test_editor, set GENERATOR to /tmp fresh binary, unittest loader; outside sandbox) | 1 | editor-fresh-generator-tests.log: 35 tests, two failures and two errors; local HTTP tests pass. Polaralign preview, device-id catalog and logical-device source mapping remain reproducible defects. |
| `python3 -B -m unittest discover -s indigo_test/buildsystem -p 'test_*.py'` | 0 | buildsystem-tests.log: seven tests pass. |
| Manual library archive build with clang and `/usr/bin/libtool -static -o`, followed by `make -C indigo_test test-unit PRIMALUCELIB=/tmp/indigo-audit-libprimalucelib.a` | 2 | native-unit-current-framework.log: default archive recipe deliberately bypassed only to test unchanged source; no claim that its recipe passes. Sandbox loopback restrictions and known serial assertion stop this aggregate run. |
| `python3 -B -` (each UNIT_TESTS executable individually, cwd indigo_test, outside sandbox; INDIGO_TEST_RESULTS points to task native-unit-results.tsv) | 1 | native-unit-summary.json plus per-suite logs: 17 suites, 16 pass; 262 planned cases, 261 pass, one fail. Only FND-000047 serial_open_reports_not_a_serial_port fails. Timeout bound 100s per suite; none timed out. |
| `python3 .factory/tasks/20261023065605-60d4-repository-audit/check-generated.py` | 0 | generation-results.json: 117 production definitions, zero generation failures and zero output differences. All work in disposable trees; repository outputs unchanged. |
| Compile/run primaluce-prefix-repro.c against unchanged source/current framework | 1 expected defect | Actual JSON parsed by unchanged source; absent ABS_POS_STEP reported present and read as 123. Compilation command uses clang -std=gnu11 -DINDIGO_MACOS -I. -Iindigo_libs -Ibuild/include, -Lbuild/lib -lindigo and current-library rpath. |
| Compile/run blob-compression-repro.c with clang -DINDIGO_MACOS -Iindigo_libs -Ibuild/include, -Lbuild/lib -lindigo -lz and current-library rpath | 1 expected defect | blob-compression-repro.log: 32-byte output is not a completed gzip stream; inflate returns -5, recovers 15/32 bytes. Initial size-only check was refined to decode the actual bytes; this is a defect reproducer, not a passing product check. |
| `./.factory/scripts/findings check` | 0 | 51 active, 18 archived, all references resolve. |

## Baseline and preservation

No product patch was made and no regression expectation was weakened. Stale binary and sandbox failures were isolated before confirming repository defects. FND-000047 reproduced independently; remaining old findings were not declared fixed. No installed service, user application or hardware endpoint started. Native test builds are cleaned only after useful evidence is retained. Compiler/test artifacts under build and /tmp are not deliverables. Audit evidence is intentionally live; committed-range process check and archival are not run or claimed while task acceptance remains partial.

Final retention check: `make -C indigo_test test-clean` exited 0 after logs/results were retained; the test build directory was absent at audit start and is removed again. Final configured check/security and whitespace checks exited 0; findings validate 51 active/18 archived. The audit-owned failed FTDI mount workspace was confirmed unmounted through hdiutil info and its empty directories removed. No live audit test process remains. Maintenance status still reports full_review_due=true; no completion was fabricated.
