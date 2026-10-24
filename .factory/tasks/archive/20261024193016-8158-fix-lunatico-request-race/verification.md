# Verification

Verdict: pass
Reviewed source: working-tree candidate over923bca9a214d4b5d9ba2982e2964e982f26c007a; final source identities in process.json. Parent implementation agent performed runtime verification; fresh read-only agents independently inspected design/code and corrections. No independent runtime execution claimed.

## Environment and target

Working directory: `/Users/polakovic/Development/indigo`; test executables run from `indigo_test/`. Python 3.9.6; macOS 27.0.1 / Darwin 27.0.0 arm64; universal arm64+x86_64 driver builds. Latest configured origin/master fetched and merged before final runs, already at the baseline revision. No compatible physical hardware, Linux or Windows execution; no hardware test performed.

## Baseline and defect reproduction

- `make -C indigo_drivers/focuser_lunatico -f ../../Makefile.drv all` and equivalent rotator command: exit 0, original driver archives/dylibs/executables built.
- `make -C indigo_test build/integration/test_focuser_lunatico_simulator build/integration/test_rotator_lunatico_simulator build/integration/test_rotator_lunatico_udp_simulator`: exit 0.
- Original three executables with `INDIGO_SIMULATOR_TRACE_DIR` and `INDIGO_TEST_RESULTS`: exits0, focuser 57/57, rotator serial 29/29, UDP 29/29. Original ordered command traces and results retained in evidence/lunatico-baseline-*; public state/value outcomes are asserted by those named tests.
- Added bounded hold tests against unchanged archives with `AUX_TEST_FILTER=held_poll AUX_TEST_TRACE=1`: GOTO/SYNC lost across main/secondary ports, both exports and UDP (8 expected failures); relative characterization3/3 passed. Urgent-abort baseline also failed; the initial disconnect expectation was later corrected as described below.
- Candidate cached-position edge: `AUX_TEST_FILTER=held_poll_return AUX_TEST_TRACE=1 ./build/integration/test_focuser_lunatico_simulator`, exit 1 before cache correction. Simulator moved position1800->1900 during held readback; no goto1800 was issued. Final implementation retains successful private measured state even while protecting public request intent.

## Corrections and stale-run disposition

First candidate held-poll run:11/13 passed. The two failures were new-test defects: a15s follow-up afterSYNC0 used a10s state deadline, corrected by1kHz speed; disconnect was incorrectly assumed urgent although its connection handler is FIFO. Independent reviewer approved exact-target assertions with appropriate speed and stop-before-reconnect recovery, retaining the urgent-abort no-command assertion. No existing expectation was weakened. Read-only review also identified clone callback storage compatibility; per-port original callbacks now have library lifetime.

An initial complete candidate run was stopped and excluded: shared source edits alone had not rebuilt ordinary driver archives. Only task-owned runner/simulator process trees were terminated. Unchanged-generator regeneration, followed by both driver builds and all three test builds, produced object/archive timestamps newer than shared sources. Final recorded suites were restarted against those archives. Earlier partial output is not acceptance evidence.

## Completed checks

- `ASAN_OPTIONS=detect_leaks=0 AUX_TEST_FILTER=held_poll INDIGO_TEST_RESULTS=<isolated path> ./build/integration/test_focuser_lunatico_simulator_asan` and equivalent rotator executable: exits0,8/8 and3/3 passed. Driver C is instrumented, shared INDIGO library is not; leak checking disabled for the forked harness.
- Universal driver build and strict `/usr/bin/clang -fsyntax-only -Wall -Wextra -Werror -arch arm64 -arch x86_64 ... <generated driver.c>`: both exit 0, no warnings/errors. Exact compiler commands in evidence/lunatico-strict.json. Original and candidate `-Wall -Wextra` comparison also had no warnings.
- `build/bin/indigo_generator <driver.driver>` for both exports: SHA256 before/after comparisons match for C/header/main outputs. No generator source/DSL change. Versions increased focuser20->21 and rotator19->20; no property names/counts changed.
- `./.factory/scripts/test`: exit 0,188/188 Factory tests. `./.factory/scripts/check` and `./.factory/scripts/security`: exits0. Evidence logs retained. These checks validate configured Factory boundaries, not all native drivers.

## Final recorded runs and acceptance

Final full recorded runs passed: focuser 65/65, rotator 64/64 (32 serial + 32 UDP), both exit 0; total 129/129. Recorded driver versions are 3.0.0.21 and 3.0.0.20. README Testing sections and TEST_SUMMARY.md were updated by the runner. Invoked `python3 /private/tmp/run_indigo_driver_locked.py <driver> --no-build`, which imports and invokes `tools/run_driver_test.py` unchanged; only shared README/TEST_SUMMARY writes are serialized using flock. Each suite uses isolated simulator traces. Final logs and ordered commands are retained in evidence/final-*.txt. Command traces use ordinal filenames, mapped to the original/current registration lists before comparison. Ignoring periodic ismoving/getpos/temperature/analog reads, 54 preserved focuser traces and 28 preserved rotator UDP traces have identical configuration/motion operation order and arguments. Combined rotator runs reuse event filenames, so the retained final rotator traces are the later UDP run; both serial and UDP test outcomes are independently recorded by the runner. See trace-comparison.json. Intentional differences in new held-poll fixtures are the accepted goto/setpos argument and cancellation recovery.

All acceptance criteria pass: exact queued GOTO/SYNC/relative commands, final measured value and retained target; rejected duplicate recovery; fresh private readback; existing external/error/compensation cases; urgent abort and FIFO disconnect/reconnect. Both exports/transport suites pass. Simulated total for final recorded suites 129 run/129 passed; separate focused ASan 11/11. Physical hardware 0 run/0 passed. MIGRATION_STATUS counts 65/0 and 64/0 preserve Comment columns. Test build artifacts are cleaned after retaining evidence; committed-range checks remain the delivery gate.

Acceptance requires exact queued GOTO/SYNC/relative command and final target/value; rejected duplicate recovery; fresh private readback; external/error/compensation preservation; urgent abort and FIFO disconnect/reconnect; both export/transport suites. No hardware, supported-platform runtime or whole-repository certification is inferred.

Retained output is normalized only for trailing whitespace and final blank lines; empty warning logs are represented by the zero-warning JSON results. Xcode registration covers archived reports, evidence folder and completion record; `plutil -lint` passes.
