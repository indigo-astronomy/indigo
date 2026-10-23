# Repair and partial-audit verification

Verdict: pass
Reviewer: root Codex agent, separate verifier pass after implementation; same implementer, with independent source review by core_audit. No independent execution or hardware certification claimed.
Base and included master/origin/master: e0139d57ceb8bb6f4421efa8849c8541690e8b2c (fetch confirmed unchanged before final run). Reviewed candidate is the task working tree; final committed SHA and delivery checks belong to the external delivery log.
Working directory for commands: /Users/polakop/Development/indigo unless stated. Environment: macOS arm64, Apple Clang, Python 3.9.6, existing local dependencies. No new dependency or physical device.

## Acceptance and approved scope

AC-001 passes with inventory.json and driver-inventory.json. AC-002/005 pass for the explicitly owner-approved closure scope: retain the partial audit and all remaining limits, do not qualify a full-audit checkpoint. AC-003 passes with twelve canonical new findings, two fixed/archived and ten nonblocking follow-ups; all 42 prior active records preserved. AC-004 passes with passing/failing/unavailable checks distinguished in audit-baseline-verification.md. Existing native serial FND-000047 and editor/example failures remain registered; they are outside the two repairs and are not represented as passing.
AC-006 passes clean configured macOS and ar-compatible rcs archive builds, symbol inspection and failed-rebuild preservation. AC-007 passes the parsed-key regression and library checks below. Final required recorded driver run passes 61/61.

## Repair verification

- Baseline: clean configured macOS archive failed (primalucelib-build.log). New parsed-reply regression failed against the old parser at the missing ABS_POS_STEP assertion (key-regression-baseline.log). No expectation weakened.
- `make -C indigo_test build/unit/test_primalucelib`, then `indigo_test/build/unit/test_primalucelib`: exit 0, 25/25 pass. Exact build/run output: library-fixed-build.log and library-fixed-tests.log. Includes shorter leaf and ancestor keys, longer leaf names and both exact/sibling key orders.
- Sanitizer build uses `make -C indigo_drivers/shared/primalucelib BUILD_LIB=/tmp/indigo-audit-primalucelib-sanitize CC=clang 'CFLAGS=-g -O1 -std=gnu11 -DINDIGO_MACOS -fsanitize=address,undefined -fno-omit-frame-pointer -I../../../indigo_libs -I../../../build/include' all`; unit executable compiled with the same sanitizer flags. `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/indigo-audit-test-primalucelib-sanitize`: exit 0, 25/25 pass, no sanitizer diagnostic. See library-sanitize-build.log and library-sanitize-tests.log. Framework binary is uninstrumented; no leak certification.
- Fresh default macOS archive and failed rebuild commands/results are retained in archive-repair-verification.json: build and nm exit 0 with symbol present; forced `AR=/usr/bin/false` rebuild fails as expected, preserves original archive SHA and removes private temporary files.
- `make -C indigo_drivers/shared/primalucelib BUILD_LIB=/tmp/indigo-final-ar-check AR=/usr/bin/ar ARFLAGS=rcs all` followed by `nm -g /tmp/indigo-final-ar-check/libprimalucelib.a`: exit 0 and public symbols including primalucelib_reply_has present. macOS ar warns about the fat archive; this is ar-compatible flag forwarding, not a Linux execution claim. Extra `ARFLAGS=-rv` experiment builds/preserves the archive but lacks the inspected symbol table; no symbol certification is claimed for that extra universal-object configuration. A separate native thin-object `AR=/usr/bin/ar ARFLAGS=-rv` build and nm both exit 0 with the symbol present (ar-thin-verification.json retains the full make arguments). This also exercises the Linux-configured flag spelling without claiming a Linux host run.
- Exploratory `python3 tools/run_driver_test.py focuser_primaluce --dry-run`: exit 0, 61/61; this option runs tests but skips recording. A source transition overlapped the exploratory run, so it is supplemental only. The subsequent final recorded run is authoritative.
- Post-repair `./.factory/scripts/test`: exit 0, 175 tests pass in 148.268s. Check/security and registry commands are repeated after final documentation and on the committed candidate. Configured command arrays are retained in process.json.
- Unchanged generator produced the version 29 .c; no .h/_main.c or generator implementation changes. No property or migration status column changed. All 117 production definitions matched their checked-in outputs in the baseline corpus check (generation-results.json).

## Baseline diagnostic scope

See audit-baseline-verification.md for exact initial commands and failures, analyzer per-unit outcomes, current-framework build, 261/262 native cases, 35 editor cases and seven passing build-system cases. Those diagnostic failures are retained rather than waived into green. No full Linux/Windows, full integration corpus or hardware run occurred.

## Delivery gates

Final recorded driver run and independent final documentation review are complete. Check/security/findings/decision validation pass on the final working tree; archive integrity and committed-range process checks must still succeed before integration. Owner explicitly authorized commit/merge/push on 2026-10-23. Full audit remains due; completion kind is work with audit=null. Integration/publication/cleanup are pending and must be confirmed externally.

## Authoritative final run

`python3 tools/run_driver_test.py focuser_primaluce > .factory/tasks/20261023065605-60d4-repository-audit/driver-final-tests.log 2>&1`: exit 0, 61/61 simulator cases, zero failing scenarios; recorded `2026-10-23 09:41 3.0.0.29 mac arm64 simulator 61/61 OK`. Full run used unchanged repaired source and updated README Testing/TEST_SUMMARY. Hardware 0. Final REFACTOR totals match. Target synchronization `git merge origin/master` reported already up to date at e0139d57c. Final isolated generation matches .c/.h/_main.c.

Independent core_audit final-doc review corrections are incorporated; final source remains its reviewed identities. Factory175, library25 and sanitizer25 results are retained above; no product changes followed these checks. Final `make -C indigo_test test-clean` removes only task-built test outputs after evidence retention. Required committed-candidate check/security/test/process validation and actual publication are recorded externally after the single final commit.

Retention preflight: `./.factory/scripts/findings check --prune-task 20261023065605-60d4-repository-audit` exited 2 because current task reports are intentionally unpinned; no report deletion is authorized or performed. Archival retains all reports and requires ordinary findings validation, which passes. This preflight does not block the retained archive and is not represented as a passing pruning gate.
