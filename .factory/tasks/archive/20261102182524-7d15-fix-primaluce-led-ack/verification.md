# Verification

Verdict: pass

Working directory /private/tmp/indigo-led-ack; base b6d3ee192, macOSarm64 runtime, isolated builds. Initial missing build/bin environmental failure fixed by creating local build directories; no source changed.

## Baseline and found defect

`make -C indigo_test build/unit/test_primalucelib` passes. Original helper with new regression:37 existing cases pass and new LED acknowledgement case fails at expected false for named Error plus generic done; source currently accepts it. `make -C indigo_drivers/aux_alto -f ../../Makefile.drv all` and actual ALTOtest build pass. New ALTO LED rejection reproducer against original source fails missing ALERT, retaining known defect. Exact logs retained before production fix.

Root cause: setter reports only transport/verb success, unlike setting-specific ACK validation. Existing hardware protocols use either named DIMLEDS done or generic cmd done; strict preferred named field plus documented generic fallback preserves compatibility. No hardware operated.

## Repair and affected acceptance

The setter now requests the unchanged DIMLEDS frame, then validates the named field if present; otherwise it validates generic cmd acknowledgement (GIOTTO1.03.09 and SESTOSENSO3 firmware1.0.9). A named refusal/malformed field takes precedence over contradictory generic success. Both positive shapes are retained. Caller handlers already preserve selected LED item and ALERT on false; no public handler/property changes needed.

ALTO and GIOTTO existing silent-ACK cases now additionally inject nested named refusal and assert cached old selection. GIOTTO reconnect checks state remains unchanged; simulator previously mutated leds before refusing, so it now gates that mutation (simulator protocol revision2). Focuser existing fielderror injector now emits DIMLEDS named Error before mutation; its test retains generic/silent transport failure paths and successful recovery. Three generated driver versions increase ALTO8→9, GIOTTO8→9, focuser46→47; generated.c/.h/main output reproducibility checked by unchanged generator (generation-check.json). No generator implementation change.

All commands start at /private/tmp/indigo-led-ack unless noted.
- `make -C indigo_drivers/aux_alto -f ../../Makefile.drv all`, same for aux_giotto/focuser_primaluce: exit0, universal x86_64/arm64 build.
- `make -C indigo_test build/unit/test_primalucelib build/integration/test_aux_alto_simulator build/integration/test_aux_giotto_simulator build/integration/test_focuser_primaluce_simulator`: exit0.
- `indigo_test/build/unit/test_primalucelib`:38/38 pass. New case covers named refusal/reason, named done, generic done, missing ACK and malformed named42 plus contradictory generic done.
- `clang -g -O1 -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -D_GNU_SOURCE -I. -Iindigo_libs -Ibuild/include -fsanitize=address,undefined -fno-omit-frame-pointer indigo_test/unit/test_primalucelib.c indigo_drivers/shared/primalucelib/primalucelib.c -Lbuild/lib -Wl,-rpath,/private/tmp/indigo-led-ack/build/lib -lindigo -lm -lpthread -o /tmp/led-protocol-sanitize` then `/tmp/led-protocol-sanitize`:38/38 pass. Existing deprecatedsprintf warnings in unchanged helper/test code; no sanitizer diagnostics. Shared helper and test instrumented, common INDIGO library prebuilt/uninstrumented.
- `ALTO_TEST_FILTER=leds python3 tools/run_driver_test.py aux_alto --no-build --no-record`:1/1 pass.
- `GIOTTO_TEST_FILTER=settings python3 tools/run_driver_test.py aux_giotto --no-build --no-record`:2/2 pass (healthy generic ACK and rejected/silent ACK/cache/reconnect). An earlier typo settings_saved selected0 and wrapper correctly failed0/0; not a product failure or passing evidence.
- `PRIMALUCE_TEST_FILTER=settings_reported_failures python3 tools/run_driver_test.py focuser_primaluce --no-build --no-record`:2/2 pass (one motion suite plus selected fault/recovery case).
- `PRIMALUCE_TEST_FILTER=sestosenso3_profile python3 tools/run_driver_test.py focuser_primaluce --no-build --no-record`:2/2 pass (motion repeated plus healthy SESTOSENSO3 generic acknowledgement).
- Factory check/security commands: pass, local structural boundaries only. Configured197 Factory regression group is coordinator-owned on combined candidate, no duplicate child run.

Parent coordinator explicitly approved bounded relevant protocol38, LED-specific filters/allthree consumers and sanitizers, reusing retained b6 unrelated healthy motion evidence. This is a narrow shared-helper repair, not migration or a new full driver-run request. Filtered wrapper runs intentionally write no README/summary full-run record. Existing acceptance inventory counts34/30/76 and hardware counts remain unchanged; no new test-case registration/file/project reference needed.

## Final test summary

Protocol unit38/38, additional instrumented protocol38/38. Final filtered driver wrapper executions7/7 across6 unique selected cases (focuser motioncase repeats). No physical hardware:0 run/0 passed. No complete new140-case three-driver recertification, Linux/Windows runtime or physical LED test claim. No unrelated image recognition or dependency/network probe. Output transcripts normalized only for trailing whitespace; exact commands and outcomes retained.

## Independent review and delivery

Root independently reviewed source/tests and final artifacts on2026-11-02; catalog-membership agent separately inspected all13 source files and passed. Reviewers did not independently execute tests. Product commit bca08ca4e, imported by root as db3ed6e9c. No product change after review. Final execution archival/Factory commit and committed-range process gate remain this writer; canonical finding closure and integration/publication stay root-owned.
