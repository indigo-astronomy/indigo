# DSUSB shutter driver coverage completion

## Current-state audit

- Audit baseline: repository commit `1db9fb0ffa2b484acdb82d377e6d733917522029`, macOS Darwin 25.6 arm64. The build emits universal arm64/x86_64 artifacts; only arm64 execution is available in this environment. The working tree contains a pre-existing edit to `indigo.xcodeproj/project.pbxproj`, which will be preserved.
- `indigo_aux_dsusb.driver` is the authoritative generator input. `indigo_aux_dsusb.c`, `indigo_aux_dsusb.h` and `indigo_aux_dsusb_main.c` are checked-in generated outputs. The driver is API 3, uses a generator-owned libusb hot-plug queue and a per-device handler queue, and calls the closed-source `libdsusb` SDK through one context per attached shutter.
- The driver supports DSUSB and DSUSB2, exposes `INDIGO_INTERFACE_AUX_SHUTTER`, and publishes `CCD_EXPOSURE.EXPOSURE`, `CCD_ABORT_EXPOSURE.ABORT_EXPOSURE` and persistent `X_CONFIG.FOCUS`. An exposure optionally energizes focus for one second, starts the shutter, counts down through delayed handlers, then stops it. Abort and disconnect cancel queued work and stop the output.
- The SDK header documents discovery, open/close, focus, start and stop entry points but no status readback. The fake SDK correctly replaces those calls and uses real INDIGO queues/timers, but models only one USB identity and records only the last output state rather than an ordered call trace.
- The existing hardware-free executable registers five cases. It covers queue/registration/attach/open rollback, duplicate arrival, one active removal/reload, one short exposure, direct start failure, focus-delay abort, abort/restart, natural stop failure, queued abort, and no SDK calls after the tested abort/removal intervals.
- Existing tests do not assert driver metadata, the concrete AUX shutter interface, before/after-connect property visibility, property item/type/permission/rule/range/default contract, a complete successful focus-to-start-to-stop sequence, focus failure, start failure after successful focus, abort stop failure, idle abort, BUSY overlap rejection, exposure timing under queue load, multiple distinct devices, burst/capacity behavior, unknown/rejected device discovery, removal of an unknown identity, or repeated INIT/SHUTDOWN.
- Several waits accept an already cached property state rather than requiring a fresh revision. The shared fake does not capture exact SDK call order/context, so stale completion, duplicate calls and cross-device ownership can escape detection.
- Production timing is based on repeatedly decrementing the public countdown by one and rescheduling relative to callback execution. Queue or scheduler delay therefore accumulates into shutter-open time, and the published countdown is used as the timing source. The fix must retain the exact requested duration separately and derive remaining time from `indigo_monotonic_time()`.
- `MIGRATION_STATUS.md` records `0 / 0` although five hardware-free DSUSB cases are registered. There is no DSUSB hardware test source or opt-in target. `indigo_docs/PROPERTIES.md` lists the right properties but cites only generated C rather than the authoritative `.driver` plus generated output.
- Hardware-only gaps are physical relay timing, electrical output behavior, real DSUSB/DSUSB2 identity and unplug/replug behavior, and Windows runtime validation. The repository README records a historical physical-device test but no reproducible current hardware test cases.

## Baseline evidence

- `make -f ../../Makefile.drv` from `indigo_drivers/aux_dsusb` passed on 2026-09-13, regenerating and building the universal driver archive, dynamic library and executable. The linker reported only the existing macOS deployment-version warning for the bundled closed-source SDK.
- `make -C indigo_test build/integration/test_aux_dsusb_usb` passed.
- `indigo_test/build/integration/test_aux_dsusb_usb` passed the existing **5 of 5** fake-SDK cases.

## Hardware-test decision

- No physical DSUSB or DSUSB2 is available, so no hardware test will be run. Fake-SDK timing measures driver call timing only and will not be presented as physical shutter or relay accuracy.

## Completion plan

1. **Complete — audit and baseline.** Inventory architecture, properties, SDK surface, generated sources, fake SDK, test/build registration and coverage gaps; record baseline evidence and the hardware decision before production changes.
2. **Complete — strengthen the fake SDK.** Added deterministic accepted/rejected identities, eight distinct contexts with per-context active-state validation, ordered and timestamped SDK tracing, and controllable timer latency. The fake and all ten registered DSUSB cases now live in the conventionally named `indigo_test/integration/test_aux_dsusb_sdk.c`; the old generic `test_usb_outputs.c` is unchanged and no longer builds DSUSB coverage.
3. **Complete — complete contract and functional coverage.** Added ten named cases with fresh-revision assertions. Against version 12, metadata/property, hot-plug identity/capacity/multiple contexts and repeated lifecycle passed. The focus failure case proved that a failed start left the focus output energized, and the timing case measured 3.535421 s for a requested 3.2 s exposure with deterministic 80 ms callback latency, exceeding the 3.40 s regression limit. These failures confirmed both production defects before their fix.
4. **Complete — complete lifecycle/hot-plug coverage.** The dedicated suite covers repeated/idempotent INIT and SHUTDOWN, guarded connected shutdown, rejected discovery, distinct identities and contexts, duplicate/burst arrivals, five-device capacity overflow and recovery, unknown removal, removal during exposure, balanced references/opens/closes, no calls after close, and fresh reconnect/reload operation.
5. **Complete — fix confirmed production defects.** Version 13 stores a monotonic exposure deadline separately from the displayed countdown, derives remaining time from `indigo_monotonic_time()`, and stops the output when shutter start fails after focus. Regenerated C/H/main outputs pass all timing, focus-failure and recovery regressions.
6. **Complete — integration and status documentation.** Registered `REFACTOR.md` and `test_aux_dsusb_sdk.c` in their Xcode groups, replaced the generic DSUSB build with `test_aux_dsusb_sdk`, added an opt-in ASan/UBSan target, updated the automated count from `0 / 0` to `10 / 0` without changing the Comment column, and cited the authoritative `.driver` plus generated C in the property reference. Added the repository driver-test naming rule to `indigo_drivers/AGENTS.override.md`.
7. **Complete — final verification.** Consecutive generator hashes matched. Universal arm64/x86_64 strict-warning driver and dedicated-test builds passed; all ten normal and all ten ASan/UBSan cases passed on macOS arm64. The three neighboring suites still using the unchanged shared harness (FCUSB, GPUSB and Atik wheel: 12 cases total) also passed after the split. Xcode syntax and scoped diff checks passed, and generated test artifacts were removed.

## Scenario-to-test mapping

| Required behavior | Automated scenario and evidence |
| --- | --- |
| Driver info, concrete AUX shutter interface and before/after-connect property/item metadata | `driver metadata, interface and property contract` |
| Direct start/stop order, BUSY/OK, start failure cleanup and fresh recovery | `direct exposure, SDK errors and recovery` |
| Focus/start/stop order, focus failure, start failure after focus and output cleanup | `focus sequence and distinct SDK failures` |
| Exact 3.2 s duration under deterministic timer latency, BUSY overlap rejection and reacquisition | `monotonic exposure timing, overlap and recovery` |
| Abort during focus and exposure, stop failure, active removal, callback cancellation and no calls after close | `abort, stop failure and active removal races` |
| Urgent abort overtaking a queued exposure and false abort behavior | `queued exposure abort and false abort` |
| Duplicate arrival, connected shutdown rejection, open rollback, removal/reload and reconnect | `duplicate discovery, shutdown guard and open rollback` |
| Queue creation, hot-plug registration and device attachment rollback | `queue, registration and attachment rollback` |
| Rejected/unknown identities, burst arrivals, five-device capacity, recovery and per-device SDK ownership | `hotplug identity, capacity and multiple contexts` |
| Three repeated cycles and idempotent INIT/SHUTDOWN actions | `repeated idempotent initialization and shutdown` |

Generic framework property copying and configuration-file storage are not duplicated. `X_CONFIG.FOCUS` persistence is framework-owned; its driver-owned behavioral effect is verified through exact SDK call order. Physical relay timing, electrical behavior, real-device identity/unplug behavior and Windows execution remain hardware/platform-only gaps.

## Final validation evidence

- Strict driver build: `make -B -f ../../Makefile.drv CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` passed for generated source, archive, dynamic library and executable; the bundled SDK retains its existing macOS 10.12-versus-10.10 linker warning.
- Strict dedicated integration build/run: `make -B -C indigo_test build/integration/test_aux_dsusb_sdk CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` and `indigo_test/build/integration/test_aux_dsusb_sdk` passed all 10 cases.
- Sanitizer build/run: `make -B -C indigo_test build/integration/test_aux_dsusb_sdk_asan CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` and `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/integration/test_aux_dsusb_sdk_asan` passed all 10 cases with the production driver instrumented and no ASan/UBSan report. LeakSanitizer is unavailable on this macOS runtime and is not claimed.
- Shared-harness regression: strict builds and all four cases each passed for `test_focuser_fcusb_usb`, `test_guider_gpusb_usb` and `test_wheel_atik_usb` after DSUSB was removed from the generic source.
- Generator reproducibility: SHA-1 hashes for generated C/H/main were unchanged after regeneration. `plutil -lint indigo.xcodeproj/project.pbxproj` and scoped `git diff --check` passed. Windows compilation/execution and physical hardware were unavailable.

## Final test summary

- Simulated/fake-SDK tests: **20 run, 20 passed** in final DSUSB validation (10 normal plus the same 10 under ASan/UBSan). The separate five-case baseline passed before implementation; the first expanded version-12 run intentionally failed the two regression cases described above.
- Hardware tests: **0 run, 0 passed**; no compatible device is available.
