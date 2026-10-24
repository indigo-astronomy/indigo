# Verification

Verdict: pass

## Candidate, environment and independence

Base and included integration target: cef89cc21d56b0050c60fba4d75b8efaaeb09869. Working directory /Users/polakovic/Development/indigo. macOS arm64 with universal x86_64/arm64 normal C builds, Apple clang, Python3 and Bash. Root implemented USB/fake tests, integrated the two isolated script workers and ran combined checks; runtime verification is not independent of implementation. Independent final source/quality/security review is in quality-review.md. Exact model/effort unavailable; no provider or host switch.

## Baseline

`make -C indigo_test build/unit/test_usbserial_enumeration_macos` built the original USB production object against fake IOKit. `INDIGO_TEST_CASE_FILTER=all_endpoints_preserve_metadata_and_order indigo_test/build/unit/test_usbserial_enumeration_macos` passed 1/1 on the original source, preserving ordinary multi-device metadata/order. No old-code capacity-overflow or real USB enumeration was performed.

GPS worker ran the original copied script with the unchanged real framework and only a synthetic tool: initial eight-method suite exposed 14 assertions/subtest failures for optional absence and incorrect success exit; ordinary all-present and connection-preservation cases passed. OmniSim worker ran the original source against the same initial eight-case mock suite: eight expected failures, no real process/network. Final nine-case suites add restoration-only failure and readiness-success/exited-child coverage respectively; those final extra cases are not claimed as executed original-source baselines.

## Final commands and results

| Command / exact invocation | Result |
| --- | --- |
| `make -C indigo_libs libindigo` | exit 0, core/client libraries rebuilt; no driver/off-limits sweep |
| `make -C indigo_test build/unit/test_usbserial_enumeration_macos` and `indigo_test/build/unit/test_usbserial_enumeration_macos` | exit 0 each; 6/6 fake-IOKit cases |
| Native sanitizer compiler/link commands retained in usb-sanitizer.txt, then `/tmp/indigo-usb-enumeration-sanitize/test_usbserial` | exit 0; 6/6 with ASan/UBSan and no diagnostics; production USB translation unit/test instrumented, linked library dependencies uninstrumented |
| `make -C indigo_test test-unit` with permitted local loopback access for existing tests | exit 0; 332/332 complete native unit cases, including new USB suite |
| `make -C indigo_test test-omnisim-startup test-gps-compliance-script` | exit 0; initial integrated OmniSim 8/8 and final GPS 9/9 methods (18 isolated shell invocations), no network/hardware |
| `python3 -B indigo_test/unit/test_omnisim_startup.py -v` after liveness correction | exit 0; final 9/9 mock cases |
| `bash -n indigo_tests/gps_compliance.sh` | exit 0 |
| `./.factory/scripts/check` | exit 0; diff, findings, decision index and project boundaries pass; final repeat after bookkeeping passed |
| `./.factory/scripts/test` | exit 0; 188 tests in 111.178s |
| `./.factory/scripts/security` | exit 0; local procedural security boundaries pass |

## Acceptance evidence

AC-USB: capacity-one sentinel remains unchanged, exact capacity consumes no extra USB device, nonpositive capacity acquires no object, normal output retains vendor/product/serial/path metadata and order. Fake hub, missing path, descendant iterator failure and setup failure cases balance every acquired IOKit reference and CF property. CFNumber/CFString conversion/release is real; platform enumeration alone is faked. Both loops check capacity before obtaining an object, preserving existing cleanup paths.

AC-GPS: tests copy the actual GPS script and unchanged framework into a private directory, replacing only indigo_prop_tool with a stateful synthetic executable. Missing optional properties skip; present ones retain required item/value checks; mandatory failures and advanced-operation failures produce nonzero exit. Connection state is preserved/restored; a failure only during final disconnect is included in exit verdict. No broad legacy-suite run or physical GPS operation.

AC-OMNI: mocks cover successful ownership transfer/output close, readiness failure/interrupt, stop exception/interrupt preserving original failure, Popen failure/output close, already exited child, SIGKILL fallback and post-readiness liveness rejection. Root reran the final nine-case suite. Existing stop remains best effort; OS cleanup success and real OmniSim/ConformU behavior are not certified.

## Limits and final gates

No driver, generator, public property, dependency or protocol change; no README/test-summary run record needed because no driver changed. No hardware, real OmniSim child/service, external network or Linux/Windows runtime execution; synthetic GPS subprocesses and existing native-unit loopback checks only. The macOS test is Darwin-gated; other platform production branches remain unchanged. Existing local loopback unit checks need their usual permitted access. Test-clean passed, and `git merge --ff-only master` returned Already up to date. Findings validation passed (126 active /69 archived). Final Xcode/bookkeeping checks passed before commit as recorded in handoff. The immutable committed-range process gate and repeated configured/native/script checks are final delivery gates reported externally with final SHA. Integration/publication remain pending separate owner authorization.
