# Verification

Verdict: pass
Reviewer: repair_sdk_drivers (developer verification; independent source review by root)
Date: 2026-11-02
Source baseline: 5324bf6336d22fa5ae07e91d2ad900f7ac70b771; reviewed candidate is the task working-tree diff before product commits.
Environment: macOS arm64; universal x86_64/arm64 compilation; arm64 runtime. Existing baseline framework/static SDK-free library prerequisites copied into isolated ignored build directories; these libraries are not newly sanitizer instrumented. Makefile.inc absolute paths replaced in the ignored worktree copy to prevent shared output writes.

## Acceptance and executed checks

Working directory `/tmp/indigo-sdk-audit` for every command below.
- `make -C indigo_test build/integration/test_ccd_atik2_sdk build/integration/test_ccd_pentax_usb`: exit0. Existing unchanged production baseline suites: Atik2 27/27 and Pentax44/44. Logs baseline-build.txt, baseline-atik.txt, baseline-pentax.txt.
- New reproducer `indigo_test/build/integration/test_ccd_atik2_sdk guider_failed_replacement`: exit1 against original source, missing original stop keeps RA ALERT (reproducer-atik.txt). The initial test synchronized only BUSY publication; corrected test additionally waits for fake relay state to avoid counting publication as SDK execution, then reproduced the actual missing-stop defect.
- `INDIGO_TEST_CASE_FILTER='hotplug full registry' indigo_test/build/integration/test_ccd_pentax_usb`: exit1 against original source:3 unexpected retained USB references after3 rejected arrivals (reproducer-pentax.txt).
- `/Users/polakovic/Development/indigo/build/bin/indigo_generator indigo_mac_drivers/ccd_atik2/indigo_ccd_atik2.driver`: exit0; existing unchanged generator; source/outputs synchronized. Regeneration checked separately below.
- `make -C indigo_test build/integration/test_ccd_atik2_sdk build/integration/test_ccd_pentax_usb build/integration/test_ccd_pentax_usb_sanitize`: exit0 (fixed-build.txt). Existing macOS deprecated-function warnings occur in the SDK-free Pentax Darwin claim fixture; no new compiler failure.
- `indigo_test/build/integration/test_ccd_atik2_sdk`: exit0,29/29 (fixed-atik.txt). Both axes retain original pending stop after failed2000ms replacement of300ms pulse while other axis remains active1500ms. Successful replacement, disconnect, SDK failures, initialization and queue/lifecycle coverage retain existing tests.
- `indigo_test/build/integration/test_ccd_pentax_usb`: exit0,45/45 (fixed-pentax.txt). Full registry retains every slot, no new USB reference or attach; later arrival after capacity becomes available attaches and balances ownership.
- `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/integration/test_ccd_pentax_usb_sanitize`: exit0,45/45 (sanitize-pentax.txt). Driver compiled into instrumented test; framework dependencies uninstrumented. Initial detect_leaks=1 run aborted because leak sanitizer is unsupported on this macOS host; not counted as passing. USB reference balance and allocation gating checked directly by source/regression.

## Found defects

FND-20261101215425-fa40-complete-full-repository-audit-012: cancellation before a fallible replacement write removes the sole pending relay stop while old relay mask stays active. Cancel only after successful SDK write; failed replacement retains original pending callback/deadline. Dedicated failed-replacement regression now passes for RA and DEC with overlapping other axis. SDK failure is shown as ALERT until original stop completes, preserving preexisting completion semantics.

FND-20261101215425-fa40-complete-full-repository-audit-016: private allocation/device allocation/USB ref precede slot selection and become orphaned when full. Slot is selected under existing device_mutex before acquisition. Dedicated repeated-capacity/recovery test now passes. Properties unchanged; versions Atik2 18→19, Pentax0x03000005→0x03000006.

## Guiding software timing

72 measured samples:3 repeats per direction/duration/workload after1 discarded warmup;20/100/500ms, EAST/WEST/NORTH/SOUTH, idle and ongoing60s CCD acquisition. Endpoints are successful fake SDK direction ON/OFF entry. Redundant OFF ignored; failed replacement samples excluded. Raw values in fixed-atik.txt, complete signed ms and percent min/mean/median/p95/p99/max/stddev and maximum-absolute metrics in guider-timing.json. Idle maximum absolute error5.0302ms; acquisition5.0279ms. No host-dependent tight precision gate. This measures software fake SDK timing, not physical relays.

## Limits

No physical hardware, real camera SDK runtime, Linux or Windows runtime validation. This is a bounded bug repair, not a migration or complete recertification of every supported camera. Existing healthy suites are retained. Root independently reviewed the actual diff and reported PASS for resource ownership and serialized cancellation ordering on2026-11-02; root did not perform these runtime checks.

## Final test summary

Baseline simulated tests:71/71. Dedicated new baseline defect reproducers:0/2 (expected failure). Fixed normal simulated tests:74/74 (Atik2 29 plus Pentax45). Additional sanitizer simulated tests:45/45. Hardware tests:0 run,0 passed. Final scripted recorded runs and generation reproducibility are appended below after execution.

## Final recorded runs and reproducibility

`python3 tools/run_driver_test.py ccd_atik2 --no-build`: exit0,29/29; `python3 tools/run_driver_test.py ccd_pentax --no-build`: exit0,45/45. Final exact wrapper output retained in final-atik.txt and final-pentax.txt. --no-build skips redundant real-SDK driver archive build; production Atik2 driver object and complete Pentax included source were freshly rebuilt by focused test targets above. Records written only to driver README Testing sections; generated TEST_SUMMARY.md is coordinator-integrated to avoid cross-task overlap. Repeated unchanged-generator invocation produces identical .c/.h/_main.c bytes.

Factory structural and security configured checks: exit0 (factory-check.txt, factory-security.txt). Product commits59af3d906 and7adf2550d retain reviewed code; independent reviewer root inspected the same diff prior to commits.
