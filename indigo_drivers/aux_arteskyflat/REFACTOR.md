# Artesky Flat Box USB Coverage Completion

## Current-State Audit

- Audit baseline: repository commit `5811f024e351804da26cc8f96d4ce788242fc667`, working tree with unrelated pre-existing changes, macOS Darwin 25.6.0 arm64. The build produces universal arm64/x86_64 artifacts; only arm64 execution is available in this environment.
- `indigo_aux_arteskyflat.driver` is the authoritative generator input. `indigo_aux_arteskyflat.c`, `indigo_aux_arteskyflat.h` and `indigo_aux_arteskyflat_main.c` are checked-in generated outputs. The driver is API 3, uses portable `indigo_uni_io`, generator-owned serialized handlers and one serial handle per logical AUX instance; there are no timers, long-running operations or shared logical devices.
- The device exposes `INDIGO_INTERFACE_AUX_LIGHTBOX`, runtime additional instances, `AUX_LIGHT_SWITCH` (`ON`, `OFF`) and persistent `AUX_LIGHT_INTENSITY` (`LIGHT_INTENSITY`, 0–100%, step 1). It translates ON/OFF to `>L000\n` / `>D000\n` and intensity to `>Bxxx\n`, with percent converted to the device's 0–255 scale by integer truncation.
- The repository Arduino reference documents replies `*L19000`, `*D19000` and `*B19xxx`. The host simulator implements those commands over a PTY and supports the standard headless ready-file contract. The manufacturer product page linked from `README.md` was unavailable during this audit and provides no repository-local protocol specification; the bundled Arduino simulator is therefore the concrete protocol reference.
- The command helper discards stale input, writes through portable I/O and reuses a private response buffer, but uses the older read helper and accepts any non-empty newline-terminated reply whose first byte is `*`. It does not verify complete framing, echoed operation, numeric device id or echoed value, so wrong/short replies can silently produce `OK`.
- The host simulator accepts every string beginning with `>L`, `>D` or `>B`, applies modulo conversion to malformed brightness text and has no command observation or failure-injection channel. It therefore cannot prove exact wire encoding, strict reply parsing, timeout handling, transport loss or recovery.
- The single integration case covers connect, property/item presence, intensity 100 and ON/OFF happy paths. It waits on potentially stale `OK`, asserts only the generic AUX interface bit rather than the light-box capability, and omits property metadata/ranges/defaults, exact commands and scaling, 0/50/100 boundaries, malformed/short/dropped replies, transport close, error recovery, failed open, repeated disconnect/reconnect and accepted-value behavior.
- Applicable AUX requirements are connection lifecycle, exact concrete interface and property contract, driver-owned writable controls, command/unit translation, malformed/short/timeout/transport failures and recovery. Abort, overlap, polling, timing benchmark, guider/shared-device lifecycle and hardware hot-plug are not applicable because both commands complete synchronously on one serialized queue and the driver exposes no such capabilities.
- Build/package integration already contains the driver, `.driver`, simulator and integration test in the Makefiles, Xcode project and Windows project files. `MIGRATION_STATUS.md` currently records one simulator case and zero hardware cases. `indigo_docs/PROPERTIES.md` lists the two standard AUX properties but its source note points only to generated C.

## Baseline Evidence

- `make -f ../../Makefile.drv` from `indigo_drivers/aux_arteskyflat` passed and regenerated/built the driver archive, dynamic library and executable.
- `make -B build/integration/test_aux_arteskyflat_simulator` from `indigo_test` passed.
- `build/integration/test_aux_arteskyflat_simulator` passed the existing **1 of 1** simulator case.

## Hardware-Test Decision

- No physical Artesky Flat Box USB is available, so no hardware test will be run. Simulator results will not be presented as verification of real panel brightness, illumination uniformity, USB electrical behavior or physical unplug/replug recovery.

## Completion Plan

1. **Complete — audit and baseline.** Inventory architecture, properties, protocol, simulator, integration/build registration and gaps; record baseline build/test and hardware decision before production changes.
2. **Complete — protocol simulator.** Added deterministic `<ready-file>.events` exact command logging and one-shot `<ready-file>.control` actions for wrong-operation, nonnumeric-id, mismatched-value, short, dropped and transport-close replies. The dispatcher now accepts only exact five-character numeric command frames. `make -B build/integration/aux_arteskyflat_simulator CC='clang -Wall -Wextra -Werror'` passed; shared serial cleanup already removes the two sidecar files.
3. **Complete — functional coverage.** Replaced the smoke test with three fresh-revision cases covering the light-box interface/property contract, exact ON/OFF and 0/50/100 conversion (`000`, `127`, `255`), wrong-operation/nonnumeric-id/mismatched-value/short/dropped replies with accepted-value recovery, failed open, repeated disconnect, property deletion/redefinition, read and write failure after transport close, and fresh reconnect operation. The strict-warning build passed. Against version 7, contract/protocol and lifecycle passed while `arteskyflat_rejects_bad_replies_and_recovers` failed exactly because `*X19000` was incorrectly accepted as success, confirming the parser defect before its production fix.
4. **Complete — production fixes.** Version 8 now uses `indigo_uni_read_section2()` with explicit 1 s first-byte and 0.1 s inter-byte limits, requires the complete eight-byte line including terminator, validates `*`, echoed operation, two numeric id digits and the echoed three-digit value. Intensity changes copy targets and commit values only after a valid reply; light-switch failures restore the last accepted selection. Regeneration and `make -f ../../Makefile.drv` passed, followed by all three expanded simulator cases.
5. **Complete — integration/status documentation.** Registered `REFACTOR.md` in the existing Xcode driver group (`plutil -lint` passed), changed the property source note to the authoritative `.driver` plus generated C, and updated only the `aux_arteskyflat` automated count from `1 / 0` to `3 / 0`; its empty Comment column is unchanged.
6. **Complete — final verification.** Consecutive generator runs produced identical hashes. Universal arm64/x86_64 driver and test builds passed with `-Wall -Wextra -Werror`; the three-case normal and ASan/UBSan runs passed on macOS arm64. Xcode plist and scoped diff checks passed, test/temp artifacts were removed, and unavailable hardware/Windows environments remain explicit below.

## Scenario-to-Test Mapping

| Required behavior | Automated scenario and evidence |
| --- | --- |
| Interface, property/item/type/permission/rule/range/default contract | `arteskyflat_contract_and_exact_protocol` |
| ON/OFF operations and exact commands | `arteskyflat_contract_and_exact_protocol`: `>L000`, `>D000` |
| Intensity boundaries, unit conversion and exact commands | `arteskyflat_contract_and_exact_protocol`: 0/50/100% become `>B000`, `>B127`, `>B255` |
| Wrong operation, nonnumeric id, wrong echoed value, short reply and timeout | `arteskyflat_rejects_bad_replies_and_recovers` |
| Failed request preserves last accepted intensity/light state; next request recovers | `arteskyflat_rejects_bad_replies_and_recovers` |
| Failed serial open and rollback | `arteskyflat_handles_connection_failures_and_reconnects` |
| Disconnect twice, connected-property deletion, reconnect and fresh commands | `arteskyflat_handles_connection_failures_and_reconnects` |
| Transport close, read failure, subsequent write failure and fresh-simulator recovery | `arteskyflat_handles_connection_failures_and_reconnects` |

### Non-applicable and Hardware-Only Coverage

- Motion, abort, polling, timing/race finalizers, guider timing and shared-interface ordering are not applicable; this driver exposes two immediate serialized controls on one AUX device and has no background callbacks.
- A side-effect-free identity query is not present in the repository protocol reference, so connection establishes the serial handle and the first control reply establishes protocol compatibility. Failed OS open and every control reply/write failure are covered.
- `AUX_LIGHT_INTENSITY` persistence is generator/framework storage and the protocol has no readback command or connect-time restore command; generic CONFIG storage is not duplicated in this driver suite.
- Physical brightness, uniformity, real USB interruption and cross-platform Windows runtime require hardware/platform environments not available for this work.

## Final Validation Evidence

- Final strict driver build: `make -B -f ../../Makefile.drv CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` passed for generated source, archive, dynamic library and executable.
- Final strict integration build/run: `make -B build/integration/test_aux_arteskyflat_simulator CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` and `build/integration/test_aux_arteskyflat_simulator` passed all 3 cases.
- Sanitizer build/run: `make -B build/integration/test_aux_arteskyflat_simulator_asan` and `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/integration/test_aux_arteskyflat_simulator_asan` passed all 3 cases with the production driver source instrumented and no ASan/UBSan report. LeakSanitizer is unavailable on this macOS runtime and is not claimed.
- Generator reproducibility: SHA-1 checks for `indigo_aux_arteskyflat.c`, `.h` and `_main.c` were unchanged after regeneration.
- `plutil -lint indigo.xcodeproj/project.pbxproj` and a diff check scoped to this driver's files passed. Repository-wide `git diff --check` is blocked by pre-existing trailing whitespace in unrelated pending `indigo_windows.sln` changes; those user-owned changes were preserved.
- `make test-clean` removed `indigo_test/build`; no `/tmp/indigo-serial-sim.*` directories remained. Windows compilation/execution was unavailable.

## Final Test Summary

- Simulated tests: **6 run, 6 passed** in final validation (3 normal plus the same 3 under ASan/UBSan). The separate one-case baseline also passed before implementation.
- Hardware tests: **0 run, 0 passed**; no compatible physical device is available.
