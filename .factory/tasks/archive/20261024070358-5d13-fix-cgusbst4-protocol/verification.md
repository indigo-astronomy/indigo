# Numeric guide protocol repair verification

Verdict: pass

## Scope and reviewed source

Base: 51ee12368b9eef3ec57632461c1adb928f447b29, containing original master 42ff049e199348b083f36a0746a7518214ba2196 and prior hardware evidence. Single Codex writer, sequential developer/verifier/documenter/reviewer; same-agent limitation, no independent review. Current source changes exactly four direction constants plus version 9 to 10 in .driver and regenerated C. Header/main regeneration is byte-identical; generator implementation is unchanged. No property additions/removals, polarity swap, transport or lifecycle change. N/S/E/W are 0/1/2/3 with unchanged %4d duration framing, matching the upstream PHD2 source pinned in the prior experiment.

Local cwd: /Users/polakovic/Development/indigo. macOS arm64 host builds universal x86_64/arm64 driver output; runtime checks are arm64. RPi root is /dev/sda2 and original ~/indigo is clean at 42ff049e1. Candidate hardware builds/run in detached /tmp/indigo-cgusbst4-20261024070358, with generated host Makefile.inc and regular copies of the same-revision built framework libraries and generator. No system install, existing-source overwrite or service change. Physical CGUSB-ST4 04d8:ffe5 uses /dev/ttyACM0; NYX-101 firmware 1.32.1 uses /dev/ttyUSB0, linked over ST4.

## Baseline

- `python3 tools/run_driver_test.py guider_cgusbst4 --no-record`: seven planned, five passed; both transport tests failed because the old fake-open replacement did not intercept current indigo_uni_open_serial2(). baseline.log retains failures.
- Fixed only fake-open interception, rebuilt with `make -C indigo_test build/integration/test_guider_cgusbst4_transport`, ran `indigo_test/build/integration/test_guider_cgusbst4_transport`: 2/2 pass with original letter commands (baseline-transport-rebuild.log, baseline-transport-repaired.log).
- `make -C indigo_test test-guider-cgusbst4-simulator-known-defects`: original driver failed numeric directions as expected; wrapper returns 0 for expected failures, not a passing direction test (baseline-numeric.log).
- Reference trace initial invocation from repository root failed because the binary expects cwd indigo_test. Corrected cwd passed; trace capture initially produced no files because the suite manually unlinked them before the shared capture hook. Added explicit capture before unlink, created output directory, rebuilt and reran against original production driver. Baseline normalized semantic events match all five final cases; trace-comparison.txt names the correspondence. Property assertions cover BUSY/completion and value reset; these event logs do not serialize every public property payload. This is a protocol fix, not a full lifecycle migration.
- On RPi before production edit, `CGUSBST4_NYX_PORT=/dev/ttyUSB0 CGUSBST4_HW_PORT=/dev/ttyACM0 python3 tools/run_driver_test.py guider_cgusbst4 --hw --no-record`: original adapter tests 10/10, independent mount-response case failed, total 11/10 Failed. baseline-hardware.log; mount restored parked/not tracking/R6.

## Regressions and candidate checks

`python3 tools/run_driver_test.py guider_cgusbst4`: 7/7 pass, exact numeric wire mapping, wrong/silent identity, reconnect, reconnect during pulse, fake open rollback, connection recovery, shutdown rejection and write error. simulator-final.log. The former known-defect case is ordinary mandatory coverage; synthetic letter dialect is no longer a driver acceptance contract. Added parent case planning prevents crashed children disappearing from totals; existing fake transport source remains shared, only its CGUSB branch changed.

Trace command: from indigo_test, `INDIGO_SIMULATOR_TRACE_DIR=/Users/polakovic/Development/indigo/.factory/tasks/20261024070358-5d13-fix-cgusbst4-protocol/final-traces ./build/integration/test_guider_cgusbst4_simulator`. Compare each event after removing its last timestamp column; all five sequences equal original semantic events. The only intentional wire change is direction alphabet. Artifacts include both timestamped and normalized forms.

ASan/UBSan: compile generated driver and test_serial_outputs.c with /usr/bin/clang, -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer, existing OUTPUT_TRANSPORT_REPLACEMENTS plus -Dindigo_uni_open_serial2=output_open2; test compile also uses -D_GNU_SOURCE -DTEST_KIND=1 -DTEST_ENTRY=indigo_guider_cgusbst4 -DTEST_NAME='"CG-USB-ST4 Adapter"'. Link -Lbuild/lib -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -lindigo -lm -lpthread. Run /tmp/test-cgusbst4-sanitize: 2/2 pass, no sanitizer diagnostics (sanitizer.log). Framework shared library is not instrumented; not full-library sanitizer coverage.

Strict compile: `/usr/bin/clang -fsyntax-only -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -D_GNU_SOURCE -I. -Iindigo_libs -Iindigo_drivers -Wall -Wextra -Wno-unused-function -Wno-unused-parameter indigo_test/hardware/test_guider_cgusbst4_hw.c`: exit 0, no diagnostics (strict-hardware.log). Repeated unchanged generator invocation reproduces .c/.h/_main.c byte-for-byte (generation-check.txt). Windows and native Intel runtime unavailable, not passed.

## Hardware interpretation and test correction

Two candidate test failures were investigated rather than changing production timing/polarity. First sampled lowercase g once at ~100 ms, second polled lowercase g; NYX actually returned uppercase G after ~100 ms at R1. Both G (pulse-guide) and g (manual guide) are documented by the bundled NYX protocol; a direct raw serial cross-check independently confirmed uppercase G and motor movement for all numeric codes while letters did nothing. See hardware-first-candidate.log, hardware-status-investigation.log and raw-crosscheck.jsonl. The corrected hardware case accepts either active mode, polls boundedly, and still asserts motor movement. It does not turn a timer-only pass into hardware proof.

The optional mount case is selected by CGUSBST4_NYX_PORT; absent it the existing ten adapter-only cases run and no mount is opened. It requires initially parked, tracking off and R6, uses 500 ms public guide requests at R1, measures idle RA drift, asserts nonzero DEC movement and oppositely signed idle-corrected RA motion, then halts, disables tracking, parks, restores R6 and closes the port. Cleanup attempts each safety command even if an earlier command fails. Identity/status preflight failure before state change leaves the original state alone. No slew, home, firmware flash or hot-plug operation performed. Existing explicit motion/free-space approval persists.

The ST4 test observes NYX-reported motor steps and active state, not optical movement or electrical edge timing. It intentionally makes no global N/S compass sign assertion near the pole; owner requested no N/S swap. Prior setup-specific DEC qualification is retained in driver REFACTOR, not silently converted to a production polarity change. NYX coordinate-transform behavior cannot be reproduced by the adapter simulator, which has no attached mount; numeric dialect behavior is already modelled and pinned by the ordinary regression.

## Acceptance

AC-001 pass: exact PHD2 constants, version 3.0.0.10, unchanged framing/lifecycle and reproducible generation.
AC-002 pass: 7/7 recorded normal tests and 2/2 sanitizer variant; known numeric failure before fix, pass after.
AC-003 pass: final recorded RPi hardware run 11/11 OK, all four repaired-driver public guide requests yielded independent NYX motor response; cleanup parked/not tracking/R6.
AC-004 pass: README records copied from recorded runners and TEST_SUMMARY regenerated; final remote run uses TZ=Europe/Bratislava. FND-000006 closure references this verified repair; final committed-range checks are reported after commit. No manual pass count invented.

## Coverage limits

No full driver migration, Windows runtime, native Intel runtime, physical hot-plug, direct electrical relay timing, optical guiding performance, or new proof of same-axis replacement/zero-cancellation at the relay. Existing hardware timer tests cover host state and are described accordingly. Scope is the owner-requested numeric protocol repair and same-setup verification. Full repository audit remains due; owner prioritizes this related FND-000006 continuation, then audit before unrelated work/release. No audit reset or general risk acceptance.

## Final repaired-driver hardware run

Remote cwd /tmp/indigo-cgusbst4-20261024070358, exact final invocation: `CGUSBST4_NYX_PORT=/dev/ttyUSB0 CGUSBST4_HW_PORT=/dev/ttyACM0 TZ=Europe/Bratislava python3 tools/run_driver_test.py guider_cgusbst4 --hw --type "CG-USB-ST4 with NYX-101 ST4" -- HW_DEBUG=1`, reached with owner-requested `ssh -A -o BatchMode=yes indigo@indigosky.local`. Exit 0, 11/11 OK, version 3.0.0.10 (hardware-final.log, remote-test-record-final.txt). The final rerun follows cleanup review, which made halt/tracking-off/park independent attempts rather than short-circuiting subsequent safety commands.

Idle: RA 151.319 steps/s, DEC unchanged. NORTH/SOUTH 500 ms through repaired driver: DEC -17/+17 steps. EAST/WEST: RA idle-corrected -30.476/+28.522 steps. Final GU `nNPzET160` confirms parked, not tracking, restored R6. State and motor observations are both required by the new case. No global N/S swap applied.

Hardware timing: idle standalone guider, NYX parked, request to public OK endpoints, all four directions at 20/50/100/200/500 ms, one discarded warm-up and three measurements per cell, 60 measured samples total. Cell mean error +0.53…+1.39 ms, maximum absolute sample error 1.39 ms; per-cell min/mean/median/p95/p99/max, SD and signed percent errors are retained in hardware-final.log. No electrical relay timing claim or imaging workload for this standalone device.

Factory support checks: `./.factory/scripts/test` exit 0, 188 tests in 110.516 seconds (factory-test.log); `./.factory/scripts/security` exit 0 (factory-security.log). The configured check and final diff/process gate run after findings/doc finalization; these checks do not substitute for driver/hardware results.

## Final housekeeping and support checks

Configured `./.factory/scripts/check` passed after finding closure: whitespace, findings 98 active/52 archived, ADR index and project boundaries (factory-check.log). Local and remote `make -C indigo_test test-clean` succeeded. Remote cleanup initially refused untracked transfer metadata; inspection identified only AppleDouble sidecars of the exact transferred files, their magic/paths were verified, and they were removed. Candidate source SHA-256 values matched local final files before restoring/removing the agent-owned temporary worktree without force. Original remote checkout remains clean (remote-cleanup.log). No system driver installed. Final local master remains 42ff049e1; no newer target changes to merge.

Retained console logs normalize trailing horizontal whitespace only; values, command framing within lines, timestamps, diagnostics and result counts are preserved. Logs are explicitly included despite the repository-wide *.log ignore rule.
