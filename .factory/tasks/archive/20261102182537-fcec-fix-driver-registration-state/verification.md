# Verification

Verdict: pass

Developer executed scoped checks in `/private/tmp/indigo-registration-fix`, baseline b6d3ee192, final source dc6722023. macOS arm64 runtime. No claim of independent runtime verification.

## Baseline and found defect

Existing `make -C indigo_test build/unit/test_driver_loader` built successfully using isolated copied support libraries/config. Initial sandbox run passed6/7 legacy cases but the loopback capacity fixture could not bind; this environmental failure is retained in baseline.txt. It is not a production failure or passing result.
Expanded suite against unchanged baseline library, outside sandbox for local loopback fixture, returned1:8/10 pass with two expected lifecycle failures. Legacy7 cases pass there. The targeted `INDIGO_TEST_CASE_FILTER=recycled_inactive indigo_test/build/unit/test_driver_loader` returned1 with `assertion failed: !(second_initialized)`: first INIT, successful remove, then second registration with init=false reuses the slot and reports initialized without running second INIT. A second assertion checks cleared removed-slot state. Failed INIT already correctly remained false; refused SHUTDOWN correctly retains true, then successful removal exposed the stale flag.

Root cause: successful removal omitted initialized cleanup, and new registration omitted initialized=false when occupying a reused slot. Fix sets false before publishing new registration and after successful shutdown/resource release only. Existing duplicate registration and refused-removal early returns precede resets, preserving active lifecycle state. No ABI or public property change.

## Fixed commands and results

- Native build: `clang -g -O1 -DINDIGO_MACOS -Duint=unsigned -Iindigo_libs -Ibuild/include -DLOADER_FIXTURE='"/private/tmp/indigo-registration-fix/indigo_test/build/unit/indigo_test_loader_driver.dylib"' indigo_test/unit/test_driver_loader.c indigo_libs/indigo_client.c -Lbuild/lib -Wl,-rpath,/private/tmp/indigo-registration-fix/build/lib -lindigo -lpthread -lm -framework CoreFoundation -o /tmp/registration-fixed-test`: exit0. Actual changed client.c compiled directly into executable; copied support library/fixture unchanged. This avoids broad library/driver rebuilds or shared artifacts.
- `/tmp/registration-fixed-test`: exit0,10/10. Covers dynamic longpath loads, missing entry points, name/path limits, driver/server capacity, inactive recycled registration, failed init and refused/successful shutdown.
- Same compile plus `-fsanitize=address,undefined -fno-omit-frame-pointer`, output `/tmp/registration-sanitize-test`: exit0. `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 /tmp/registration-sanitize-test`: exit0,10/10, no sanitizer diagnostics. Actual client/test instrumented; supporting prebuilt library and fixture not instrumented. Leak sanitizer is not enabled on this macOS host.
- `clang -g -arch x86_64 -arch arm64 -mmacosx-version-min=10.10 -fPIC -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -Iindigo_libs -Ibuild/include -c indigo_libs/indigo_client.c -o /tmp/registration-client-universal.o`: exit0, no diagnostics. Compile validation only for x86_64, not runtime.

Final test assertion ordering was refined solely to make the baseline failure identify second initialized lookup directly; final native/sanitizer suites were recompiled/rerun after that change. Normal and sanitizer final processes had distinct temporary fixture directories and OS-assigned loopback ports. No hardware, image recognition, third-party services or unavailable Linux/Windows runtime checks. These are framework loader unit tests, not a driver certification run.

## Acceptance

AC-1 pass: successful removal clears initialized, refused shutdown retains registration and flag. AC-2 pass: exact same slot, second INIT count0 and initialized lookup false. AC-3 pass: failed init remains false and existing dynamic/static loader/capacity cases preserved. No solver matching/exclusivity implementation changes.

Detailed outputs retained as normalized.txt reports beside this document (only trailing whitespace normalized). Parent owns canonical finding closure and shared metadata/integration.

Final unit scope10/10 normal,10/10 sanitizer. Hardware0 run/0 passed.

Configured `./.factory/scripts/check` and `./.factory/scripts/security`: exit0 (factory-check.txt, factory-security.txt). Root independent source review passed dc6722023; no later implementation changes. Full197 Factory suite is coordinator-combined, not repeated by this child. Final committed-range process check follows the Factory commit and is recorded externally.
