# INDIGO 3.0 refactoring record for `wheel_mi`

## Goal and scope

Migrate the hand-written Moravian Instruments standalone filter-wheel driver to `indigo_generator`, make its `.driver` definition the source of truth, move SDK and lifecycle work onto generated handler queues, and add complete applicable hardware-free integration coverage with a deterministic fake `gxfw` SDK. Preserve supported positioning and reinitialization behavior except where a documented defect requires a tested correction.

The supported implementation and validation scope is macOS and Linux. Windows is explicitly out of scope because no compatible Moravian Instruments Windows SDK is available.

## References read

- Root `AGENTS.md`, `indigo_drivers/AGENTS.override.md`, `indigo_test/AGENTS.md`, and the shared/wheel sections of `indigo_test/DRIVER_TESTING_RULES.md`.
- `README.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, relevant generated SDK-wheel implementations, the driver README, source/header/main files, build and Xcode integration, `MIGRATION_STATUS.md`, and the `wheel_mi` section of `indigo_docs/PROPERTIES.md`.
- Bundled `gxccd.h`, especially the standalone-wheel enumeration, initialization, parameter, positioning, reinitialization, release, and error contracts.

## Baseline audit

### Repository and environment

- Baseline revision: `2f16e6743da751bb977ed8118d8f4165a51680a9`; the working tree already contains unrelated in-progress `ccd_mi` refactoring changes, which this work must preserve.
- Host: macOS 26.6.2, Darwin 25.6.0, Apple Silicon arm64; Apple clang 21.0.0.
- Baseline driver version: `0x02000003`.
- The driver README declares Linux x86 32/64-bit, ARM v7/v8 and macOS Intel/ARM64 support through the bundled closed-source SDK. Windows is unavailable because the corresponding compatible SDK is absent.

### Architecture, lifecycle, and SDK

- The baseline is a 339-line hand-written C driver plus hand-written public header and executable main; no `.driver` input exists.
- It is a standalone wheel driver using the `gxfw` portion of the bundled Moravian Instruments SDK through the `wheel_mi/bin_externals` symlink to `ccd_mi/bin_externals`.
- USB hot-plug filters vendor `0x1347`, then asynchronously enumerates SDK ids. A global `new_eid` selects the last unrepresented id, and removal is matched to the libusb bus/address captured at arrival. Up to ten physical wheels are stored in a hand-written device registry.
- Connection obtains the INDIGO global lock and calls `gxfw_initialize_usb(eid)`; disconnect releases the SDK handle and global lock. Move and reinitialization callbacks use two INDIGO timers.
- The SDK documents `gxfw_initialize_usb()` returning `NULL` on failure, all parameter/move/reinit calls returning `0` on success and `-1` on error, and `gxfw_release()` invalidating the handle. It exposes no current-position/status getter or stop command. Therefore successful `gxfw_set_filter()` is the only available positioning acknowledgement; actual position polling, moving/unknown-position readback, stop, and mid-move abort cannot be implemented or tested through this SDK.

### Public properties and behavior

- Before connection the normal common properties are visible. After connection the wheel base exposes `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, and `WHEEL_SLOT_OFFSET`.
- The driver adds a connected-only, at-most-one switch property `MI_SFW_COMMANDS` with item `MI_SFW_REINIT`; activation calls `gxfw_reinit_filter_wheel()`, updates the detected slot count, resets public position to slot 1, and clears the switch.
- `MI_SFW_COMMANDS` violates the repository rule that driver-specific custom property names start with `X_`. The migration will rename it to `X_MI_SFW_COMMANDS` and update `PROPERTIES.md`; the item name can remain `MI_SFW_REINIT`.
- Connect reads model, four firmware-version components, serial number, and filter count, publishes wheel metadata, assumes initial slot 1, and asynchronously commands SDK filter index 0. Public slots are one-based and SDK indices are zero-based.
- Slot names and offsets are framework-owned generic storage; the driver does not enumerate SDK filter names/colors/offsets or persist device-side values.
- Reinitialization is the only driver-specific control. The SDK supplies a returned filter count but no observable calibration progress or separate reset operation.

### Synchronization, errors, and risks

- SDK enumeration is serialized only by a private mutex; connect/move/reinit work is not serialized with discovery/removal on one driver queue.
- Most connection-time parameter results are ignored. Failed filter-count output can be uninitialized and used as property counts/ranges; failed description/version/serial reads can publish invalid or stale metadata.
- A move failure correctly makes `WHEEL_SLOT` ALERT, but the callback leaves its target distinct from the accepted current slot. Reinitialization failure leaves the trigger switch selected and can make subsequent user intent ambiguous.
- Reinitialization accepts any returned integer without validating it against positive values and the allocated slot-name/offset capacities.
- Hot-plug association through global `new_eid` plus bus/address can mismatch SDK identity when enumeration/event order differs, and arrival capacity/attachment failures have no retry or explicit rollback proof.
- Detach/disconnect can race queued timer callbacks unless pending work and SDK lifetime share deterministic ownership.

These are source-audit risks. The found-defects section will claim a defect only after its impact, fix, and regression evidence are established.

### Existing build and test coverage

- Baseline build command on 2026-09-14: `make -B -C indigo_drivers/wheel_mi -f ../../Makefile.drv all`.
- Baseline build result: PASS for the universal x86_64/arm64 archive, dylib, and executable with no compiler diagnostics. The linker emitted existing warnings because x86_64 objects in bundled `libgxccd.a` target macOS 10.12 while INDIGO links for 10.10.
- No driver-specific simulator, fake SDK, integration target, or hardware target exists. Baseline relevant automated coverage is 0 fake and 0 hardware cases.

## Hardware-test decision

No hardware testing will be performed because no standalone Moravian Instruments filter wheel is available. No hardware validation will be claimed. Physical position, motor behavior, USB interruption, and real SDK/device timing remain unverified.

## Atomic plan

1. **Baseline, audit, and mandatory record — complete.** Read the governing rules, source, SDK contract, references, build/project/property integration, and existing tests; record the baseline build, platform, risks, zero coverage, and no-hardware decision above.
2. **Map the generator design — complete.** Inspected the generator migration contract and generated Player One, ASI, Astroasis, and MI camera SDK hot-plug patterns. No generator change is needed. Chosen design: generated per-driver discovery/connection queue with six arrival retries; enumeration-id ownership and presence-based unplug matching; one transactional `wheel_mi_open()`/`wheel_mi_close()` pair; mandatory positive bounded filter count; optional model/serial/firmware metadata fallbacks; queued synchronous-acknowledgement move/reinit handlers; standard BUSY guard for overlap; and generated cancellation/teardown.
3. **Create the `.driver` source of truth — complete.** Reverse-extracted with `../../build/bin/indigo_generator -c indigo_wheel_mi.driver`, replaced the skeleton with the completed definition, raised the version from `0x02000003` to generated `0x03000004`, preserved the 2024-2026 license/history, and regenerated `.c`, `.h`, and `_main.c`. The first generated build exposed the extracted item-handle naming (`REINIT_ITEM`); the `.driver` was corrected and the universal build then passed.
4. **Implement generated discovery and lifecycle — complete.** Replaced the mutex/global-id hot-plug implementation with a generated driver queue, six bounded discovery retries, enumerated-id ownership/presence matching, transactional global-lock/SDK-open rollback, generated reference ownership, and deterministic cleanup. Fake tests cover reversed enumeration, duplicate names, attach retry, five-device capacity/recovery, removal during a blocked SDK call, and balanced handles/locks/USB references.
5. **Implement connection/property contracts — complete.** A positive filter count within the base property's allocated 16-slot capacity is mandatory; model, serial, and optional firmware components use initialized fallback values. Slot ranges/counts rebuild on connection, and the custom property is now `X_MI_SFW_COMMANDS`. Tests enumerate before/after connection/disconnect and verify metadata, interface, types, permissions, rules, items, visibility, ranges, counts, values, mandatory failures, optional fallbacks, and reconnect.
6. **Implement queued move and reinitialization behavior — complete.** Generated handlers provide BUSY dispatch and serialized SDK calls. Moves translate one-based public slots to zero-based SDK indices, preserve accepted values on failure, and suppress already-selected writes. Reinit validates the returned count, resets its trigger on every outcome, refreshes and redefines dynamic slot-name/offset vectors when their count changes, and has mutual BUSY exclusion with positioning. Fake regressions cover every branch and recovery.
7. **Add the fake `gxfw` SDK integration suite — complete.** Added `integration/test_wheel_mi_sdk.c`, normal and arm64 sanitizer build targets, and registered the normal executable in `INTEGRATION_TESTS`. Production generated code is compiled separately against fake SDK/USB hooks and exercised through the public driver entry point and bus requests with real handler queues.
8. **Complete the wheel acceptance matrix — complete.** Twelve fresh-fixture cases cover the complete applicable matrix: metadata/property contract; lock/open/filter-count failures and optional metadata fallbacks; initial index-zero command failure/recovery; first/intermediate/last/no-op moves; move failure/value preservation/recovery; reinit no-op, slot-count changes, SDK/invalid-count failures and recovery; move/reinit BUSY exclusion; disconnect and unplug during blocked SDK work; replug/fresh move; multiple wheels, reversed enumeration and duplicate names; descriptor/open/attach discovery failures with retry; five-device capacity/recovery; repeated requests/lifecycle; connected-shutdown rejection; and queue/registration rollback. Every fixture asserts balanced SDK handles/releases, global locks and USB references and no SDK use after close. Current-position/moving readback, polling, timeout and stop/calibration progress are not applicable because the bundled `gxfw` API exposes no corresponding operation.
9. **Add sanitizer and strict verification — complete for the available host.** The universal macOS x86_64/arm64 archive, dylib and executable build passes; only the baseline linker warnings for vendor x86_64 SDK objects targeting macOS 10.12 versus 10.10 remain. Normal and arm64 ASan+UBSan fake suites both pass 12/12; leak detection is disabled for INDIGO process-global allocations. A `-Wall -Wextra -Werror` syntax build passes with suppressions only for generated/shared unused callback parameters, intentional fake-hook macro redefinitions, generator-owned callback casts, and unused shared harness helpers. Linux was unavailable. Windows was intentionally not attempted because the compatible SDK is absent.
10. **Integrate repository records — complete.** Added the `.driver`, `REFACTOR.md`, and fake-SDK test to their Xcode groups; updated `PROPERTIES.md` for `X_MI_SFW_COMMANDS` and `.driver` ownership; and updated only the `wheel_mi` status columns in `MIGRATION_STATUS.md` to API 3, generated/queued, simulator-retested, and `12 / 0`, preserving its Comment exactly. No README was changed.
11. **Verify generated reproducibility and final diff — complete.** Two consecutive generator runs left identical SHA-256 hashes for `.c`, `.h`, and `_main.c`; all generated behavior traces to the `.driver`. `git diff --check` and `plutil -lint indigo.xcodeproj/project.pbxproj` pass. The final scope contains only `wheel_mi` production/docs, its test/Makefile integration, and required shared project/status/property records; previously completed `ccd_mi` content was preserved.
12. **Reconcile evidence — complete.** Found defects and their regression cases are recorded below; normal and sanitizer suites pass 12/12, hardware is explicitly 0/0 and unavailable, Linux is unavailable, Windows is out of scope, and repository/test artifacts were cleaned.

## Found defects

- **Unchecked mandatory filter count — reproduced and fixed.** Impact: the old driver used an uninitialized or invalid SDK output as slot range and property counts when `FW_GIP_FILTERS` failed or returned a nonpositive/oversized value. Root cause: the query result and output bounds were ignored. Fix: require a successful count in `1..16`, close/unlock transactionally on failure, and permit clean retry. Regression: `connection failures and optional metadata fallbacks`.
- **Move failure overwrote pending intent — reproduced and fixed.** Impact: a failed `gxfw_set_filter()` left the public target different from the accepted current slot and could make stale intent appear accepted. Root cause: the old callback copied the request before the SDK result and did not restore the target. Fix: retain current value during the SDK call and set both value and target only after success, restoring both on failure. Regression: `slot boundaries, no-op, failure and recovery`.
- **Invalid reinit count and latched trigger — reproduced and fixed.** Impact: failed reinitialization left its switch selected; a successful SDK call returning an invalid count could corrupt slot ranges/counts; count changes were not reliably redefined to clients. Root cause: missing count validation and incomplete property/trigger finalization. Fix: validate `1..16`, clear the trigger on every outcome, preserve previous slot metadata on failure, and delete/redefine slot-name/offset vectors on successful count changes. Regression: `reinit count changes, failures and recovery`.
- **Custom property missing required prefix — source-audit defect fixed.** Impact: `MI_SFW_COMMANDS` violated the repository namespace contract for driver-specific properties. Root cause: legacy naming predates the rule. Fix: rename it to `X_MI_SFW_COMMANDS` while preserving item `MI_SFW_REINIT`; update the properties reference. Regression: `metadata and connected property contract`.
- **Hot-plug identity and lifecycle races — reproduced and fixed.** Impact: global last-id plus USB bus/address association could select/detach the wrong SDK wheel, and removal could race active SDK work and handle release. Root cause: discovery, connection, operation and removal had separate synchronization/identity domains. Fix: generated per-driver queue, SDK-id presence matching, discovery retries, device mutex serialization for active SDK calls, and transactional cleanup. Regressions: `multiple devices, reversed identity and duplicate names`, `discovery failures, generated retry and capacity`, `disconnect waits for active SDK call`, and `unplug during active move and replug`.

## Coverage map

| Registered fake-SDK case | Covered behavior |
| --- | --- |
| `metadata and connected property contract` | Driver metadata/version/multi-device flag, wheel interface, pre/post-connect visibility, INFO metadata, slot vectors, ranges/counts, and renamed custom property schema. |
| `connection failures and optional metadata fallbacks` | Global-lock/open/filter-query rollback; invalid counts; optional serial/version failures; balanced retry. |
| `initial homing failure and recovery` | Mandatory initial SDK index 0 command failure, public ALERT, and subsequent successful positioning. |
| `slot boundaries, no-op, failure and recovery` | First/intermediate/last public slots, zero-based SDK mapping, selected-slot no-op, failed move value/target preservation, and retry. |
| `reinit count changes, failures and recovery` | False/no-op trigger, count expansion/contraction, vector redefinition, SDK failure, zero/oversized outputs, cleared trigger, and retry. |
| `move and reinit mutual BUSY exclusion` | Deterministic blocked SDK gates verify both conflict directions and no unintended SDK command. |
| `disconnect waits for active SDK call` | Disconnect serialization behind an active move, close ordering, no call after close, reconnect and fresh move. |
| `unplug during active move and replug` | SDK-based removal during blocked move, release/no invalid handle use, reattachment, reconnect and fresh last-slot move. |
| `multiple devices, reversed identity and duplicate names` | Nontrivial SDK ids, enumeration/event order mismatch, unique public names, independent operations, correct identity removal and surviving sibling. |
| `discovery failures, generated retry and capacity` | Descriptor failure, temporary probe-open and attach failures, generated retry, five-device capacity, removal and waiting-device recovery. |
| `repeated requests, shutdown rejection and lifecycle` | Idempotent connect/disconnect/INIT/SHUTDOWN, connected shutdown refusal with continued operation, reload and fresh positioning. |
| `queue and hotplug registration rollback` | Driver-queue and callback-registration failure cleanup followed by successful init/connect/disconnect. |

Framework-owned slot-name/offset storage is checked for visibility/count integration but not functionally retested. Current position, moving/unknown state, polling, stop, calibration progress and bounded movement timeout are not exposed by `gxfw`; successful positioning therefore means the synchronous SDK command was acknowledged, not that physical optical alignment was measured. Hardware-specific USB/motor behavior remains unverified because no device is available.

## Final test summary

- Simulated/fake-SDK tests run: 12; passed: 12.
- Hardware tests run: 0; passed: 0.

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard in `indigo_wheel_mi.driver` are now declared with the generator's `reject_change` block for `WHEEL_SLOT` and `X_MI_SFW_COMMANDS`. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values.

Both guards set `INDIGO_ALERT_STATE` and published the message inline but never marked the items, so the refused slot stayed visible in the client. The request-scoped `move_pending` / `reinit_pending` bookkeeping stays in `on_change_request`, which the generator runs after the guard.

Covered by the strengthened `move and reinit mutual BUSY exclusion` case in `indigo_test/integration/test_wheel_mi_sdk.c`, which now also asserts the restored slot value and target.

```sh
cd indigo_test && ./build/integration/test_wheel_mi_sdk
```
