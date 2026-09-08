# Refactoring plan for the Astroasis Oasis filter wheel driver

Goal: migrate `indigo_wheel_astroasis.c` to `indigo_generator`, following the completed `wheel_asi` and `wheel_playerone` migrations. Preserve the Oasis SDK contract and public properties while replacing manual lifecycle, hot-plug and timer scaffolding with generated handlers.

Status: software migration and hardware-free verification completed, 2026-09-08. Physical hardware validation is deferred because hardware is unavailable.

Baseline: `0d8db791be7ceb298ff3dd27a8f8456c8a533f18`; the working tree was clean before creating this document. Source line references below refer to this baseline.

## Studied sources

- `indigo_wheel_astroasis.c`, `.h`, `_main.c`, `Makefile.inc` and `README.md` in this directory: implementation, public entry point, standalone wrapper and platform constraints.
- `bin_externals/OasisFilterWheel/include/OasisFilterWheel.h`, `VERSION` and `doc/ReleaseNotes.txt`: SDK v1.2.0 contract and release history.
- `../wheel_asi/indigo_wheel_asi.driver` and `../wheel_playerone/indigo_wheel_playerone.driver`, its generated C and `REFACTOR.md`: SDK discovery, generated lifecycle, reference count, delayed completion and `unplug_match` patterns.
- `../../indigo_tools/indigo_generator.c` and `../../indigo_docs/DRIVER_GENERATOR_MIGRATION.md`: actual generator ownership and extension contract.
- `../../indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` and `../../indigo_libs/indigo_wheel_driver.c`: property state, queue and inherited wheel behavior.
- `../../indigo_docs/PROPERTIES.md`, section `wheel_astroasis`; `../../indigo_test/AGENTS.md` and the existing Player One SDK test/Makefile arrangement: documentation mapping and test approach.

These are implementation observations and planning inputs, not an incremental review checkpoint. Formal review findings belong in the repository's `REVIEW.md` workflow; no review baseline is advanced here.

## Existing public contract

- Entry point `indigo_wheel_astroasis`, driver label `Astroasis Oasis Wheel`, version `0x03000003`, original author Rumen G. Bogdanovski.
- One logical wheel per physical wheel. Device name is `Oasis Filter Wheel`, optionally followed by ` #<friendly name>`; SDK ID disambiguates duplicate names. The model is reported in INFO rather than used as the name prefix.
- USB VID `0x338f`, PID `0x0fe0`. Keep this filter.
- Legacy enumeration capacity is SDK `OFW_MAX_NUM = 32`. Migration deliberately uses the generator default of five attached devices; keep a 32-element SDK scan buffer because that is a separate API requirement. Never override `MAX_DEVICES`.
- `INFO_PROPERTY->count = 6`; firmware revision item contains the SDK version and has label `SDK version`. Device firmware is read and cached but not exposed in that item.
- Slot positions are passed directly between SDK and INDIGO. Preserve one-based positions; SDK status position zero means unknown. Do not import Player One's zero-based conversion.
- `WHEEL_SLOT` maximum and both `WHEEL_SLOT_NAME` / `WHEEL_SLOT_OFFSET` counts come from `OFWGetSlotNum()`. Names and offsets use inherited INDIGO behavior and configuration persistence; the driver does not use the SDK slot-name, focus-offset or color APIs.

All custom properties are RW, initially OK, connected-only and already use `X_`. Preserve their exact wire names, including the unusual `_PROPERTY` endings on the Bluetooth properties.

| Wire name | Type, items and defaults | Group / label / visibility |
| --- | --- | --- |
| `X_CALIBRATE` | Switch, ANY_OF_MANY, `START` / Start = false | `Advanced` / Calibrate filter wheel; visible |
| `X_CUSTOM_SUFFIX` | Text, `SUFFIX` / Suffix = discovered friendly name | `WHEEL_ADVANCED_GROUP` / Device name custom suffix; visible |
| `X_BLUETOOTH_PROPERTY` | Switch, ONE_OF_MANY, `ENABLED` / Enabled = false, `DISABLED` / Disabled = true; refreshed from config on connect | `Advanced` / Bluetooth; hidden |
| `X_BLUETOOTH_NAME_PROPERTY` | Text, `BLUETOOTH_NAME` / Bluetooth name = discovered name | `Advanced` / Bluetooth name; hidden |
| `X_FACTORY_RESET` | Switch, ANY_OF_MANY, `RESET` / Reset = false | `Advanced` / Factory reset; visible |

Preserve the factory-reset item hint `warn_on_set:"Confirm filter wheel factory reset?";` through a generator-owned attach block. Bluetooth is hidden by explicit source assignments with comments about firmware support; release notes do not establish that it is now supported. Do not unhide it as part of migration. The current suffix/name setters accept up to 32 bytes; see the SDK ambiguity below.

## Findings from source study

### SDK discovery and identity

1. `wheel_refresh()` (line 562) rebuilds `gWheels` using `OFWScan()`, retains instances by ID and detaches absent IDs. The header explicitly guarantees stable IDs while connected and a new ID after physical replug. IDs are identities, not bounded array indices.
2. `OFWScan()` requires an array of at least `OFW_MAX_NUM` elements and must precede ID-based calls. It documents only `AO_SUCCESS`, but defensive checks should still validate the return and `0 <= number <= OFW_MAX_NUM` before using results. The current code leaves `number` uninitialized and does not check either condition.
3. `wheel_create()` (line 471) temporarily opens each new ID and queries version, model, friendly name, Bluetooth name and config, then closes it even on metadata failure. The header does not establish that these getters work before open. Preserve a balanced temporary probe; do not copy Player One's open-free discovery assumption.
4. Probe failure currently rejects attachment, including failures of hidden Bluetooth getters. `indigo_attach_device()` return is ignored after allocation. Generated attach cleanup and retry must replace this path without leaking an SDK reservation.
5. There is no SDK API mapping an ID directly to `libusb_device *`. Use existing `sdk.unplug_match` with checked `OFWScan()` membership, so reversed USB/SDK ordering cannot detach the wrong ID. Failed or malformed scans are inconclusive, never evidence of absence. Retain the generator's USB pointer/reference ownership and shutdown bypass through `last_action`.
6. Legacy arrival/removal waits 0.5 seconds before a full scan. The generator's event scheduling and one-device-per-arrival attachment differ. Test simultaneous arrivals, reordered IDs, duplicate events, failed probe retry and delayed SDK readiness. Do not claim immediate discovery or automatic retry without verifying the generated path. Physical SDK readiness remains a hardware question.

### Connection and lifetime

7. `wheel_connect_callback()` (line 246) checks global lock and open, but not slot-count, status, config or Bluetooth-name reads. It can consume uninitialized outputs, assign unchecked counts and mark the connection successful. Required slot/status reads must fail the connection cleanly; validate counts against both allocated slot arrays before assigning them.
8. Connect sets `target_slot` from status but does not initialize `current_slot` immediately, relying on a timer. It ignores `filterStatus`. Initialization must distinguish idle/valid position, active movement/calibration and unknown/invalid position, and never publish an invalid slot as OK.
9. Disconnect closes the SDK without cancelling `wheel_timer`. Calibration and motion share this timer field, while connection timers are not retained. A callback can outlive the connection or race close/detach. Replace these with queued work and generated cancellation before close; test an SDK call already in flight as well as delayed callbacks.
10. Remove `gp_bits` / `is_connected`, manual property pointer fields, `WHEEL_LIST`, enumeration mutex and custom hot-plug callbacks. Use standard integer `count` for open/close references, not `opened`; rename the legacy slot `count` to `slot_count`. For one logical device the helpers own this reference count because the generator only injects it for multi-device templates. Failed open leaves count unchanged; initialization failure releases its successful open exactly once.
11. The `__i386__` branch returns `INDIGO_UNSUPPORTED_ARCH` for INIT/SHUTDOWN while INFO is available. Keep that compatibility requirement explicit: inspect existing generator mechanisms and platform build exclusions before replacing this guard. Do not silently drop it or edit generated C to retain it. Linux links libudev and C++; macOS links C++; vendored SDKs remain untouched.

### Movement and calibration

12. `wheel_timer_callback()` (line 132) reads uninitialized `OFWStatus` after a failed SDK call, publishes its position and tests only equality with target. It can poll forever or report completion while the motor is not idle. Use checked local output, valid one-based position and `STATUS_IDLE` for completion. An idle wrong-target reply or SDK error must end in ALERT rather than endless polling.
13. Slot change (line 333) ignores `OFWSetPosition()` failure and truncates fractional input; NaN is not excluded by its comparisons. Check finite integral requests after actual bus validation/clamping, preserve the confirmed value while BUSY and schedule polling only after successful start. Same-slot is a no-op only when no operation is active.
14. `calibrate_callback()` (line 146) blocks with a one-second sleep loop and ignores status errors. Since its zeroed status starts at `STATUS_IDLE`, a failed first read can incorrectly finish calibration and publish position zero. Replace the loop with delayed finalizers; do not block the queue. Preserve `OFWCalibrate(id, 0)`, completion updates to both slot and calibration properties, switch reset and messages.
15. Per-property BUSY filtering does not prevent calibration competing with a slot operation or factory reset. Explicitly handle these cross-property conflicts, including queued requests. Use existing property states unless an additional operation field is demonstrably needed. Do not duplicate the framework's same-property guard.
16. Cover `STATUS_IDLE`, `STATUS_MOVING`, `STATUS_CALIBRATING`, `STATUS_BENCHMARKING`, unknown status values and position zero. The header gives no maximum movement/calibration duration. Choose and document a bounded polling policy during implementation; do not present Player One's 15-second initialization limit as an Oasis guarantee. A software timeout does not imply physical motion stopped; require a checked status before restarting an operation.
17. Evaluate every `if (!IS_CONNECTED)` independently. Ordinary generated handlers already have lifecycle/dispatch guarantees; retain such a guard in a delayed finalizer only when justified. Do not duplicate disconnect resets that are already performed on successful connect.

### Text, configuration and reset

18. Suffix and Bluetooth-name writes update the cache before SDK success, leaving requested values looking like persisted values after failure. Stage writes, keep the last confirmed cache, restore it on failure and publish ALERT. Use correctly named SDK logging; existing suffix logs incorrectly mention `EFWSetID` and log success as ERROR.
19. The header describes `OFW_NAME_LEN = 32` as a buffer length, not an explicit maximum payload. Existing setters permit 32 bytes, and both name/version constants happen to equal 32; Bluetooth validation currently uses the version constant. Keep buffers at least 33 bytes, initialize and explicitly terminate getter buffers, use the name constant, and test lengths 0/1/31/32/33. Preserve current accepted input pending stronger SDK evidence; SDK rejection must be visible. The true 31-versus-32-byte firmware limit remains unverified.
20. `wheel_config()` (line 104) mutates cached config before `OFWSetConfig()` succeeds. Use a local masked config and update the cache only on success. Speed handling exists in this helper but no speed property is exposed; do not add speed, turbo, autorun, temperature, serial-number or device-side slot configuration features.
21. Hidden Bluetooth reads need an explicit optional-feature policy: `AO_ERROR_NOT_IMPLEMENTED` should not prevent use of the visible wheel functions. Avoid publishing invented values as successfully read. Test optional unsupported results separately from required metadata failures and genuine communication loss; record any departure from legacy attach rejection.
22. Factory reset (line 413) remains connected, does not refresh cached config/names/position and does not reject motion/calibration. Preserve reset semantics rather than importing Player One's disconnect behavior. On success, perform checked readback of affected state; handle lost communication without stale OK values. The SDK does not document physical reset/re-enumeration timing, so hardware confirmation remains deferred. A false reset/calibrate switch request must receive a completed OK response without invoking the SDK.
23. INFO caches include unused status/firmware/version data. Retain only fields required by the implemented behavior; preserve the existing SDK-version display. No new INFO items are implied by SDK capabilities.

## Deferred Bluetooth support

This section preserves the intended Bluetooth feature for implementation once firmware support is confirmed. It describes configuration of the wheel's Bluetooth radio through the existing USB/SDK connection. The current driver contains no Bluetooth discovery, pairing or Bluetooth transport implementation; enabling the radio must not be presented as adding an INDIGO Bluetooth connection mode.

### Existing design to retain

- `X_BLUETOOTH_PROPERTY`: connected-only RW switch in `Advanced`, label `Bluetooth`, ONE_OF_MANY rule. Items are `ENABLED` / Enabled and `DISABLED` / Disabled. The initial disabled selection is only an allocation default; the actual selection comes from `OFWGetConfig(id, &config)`, with any nonzero `config.bluetoothOn` meaning enabled.
- Setting that switch uses `OFWSetConfig(id, &config)` with `config.mask = MASK_BLUETOOTH` (`0x00000004`) and `config.bluetoothOn = 1` or `0`. Other settings (speed, autorun and turbo) must remain unaffected. Stage a local configuration rather than mutating the confirmed cache before success.
- `X_BLUETOOTH_NAME_PROPERTY`: connected-only RW text in `Advanced`, label `Bluetooth name`, one item `BLUETOOTH_NAME` / Bluetooth name. Read with `OFWGetBluetoothName(id, buffer)` and write with `OFWSetBluetoothName(id, buffer)`.
- The Bluetooth name is independent of `X_CUSTOM_SUFFIX`: the latter uses friendly-name APIs and changes the INDIGO device-name suffix on replug. Do not map either value to the other or rename the attached INDIGO device when changing its Bluetooth name.
- Keep a terminated `bluetooth_name[OFW_NAME_LEN + 1]` cache. Use the name constant, not `OFW_VERSION_LEN`, for validation. The old code permits 32 bytes; the SDK only specifies a 32-byte buffer requirement. Confirm payload length, encoding and whether an empty name is legal before declaring firmware behavior. Test 31/32-byte boundaries and report SDK rejection honestly.
- Both properties are currently explicitly hidden because of missing firmware support. Keep the exact wire names, labels, items and rules above, so the feature can be restored without reconstructing its public contract from legacy C.

### Activation and error handling

1. Establish which firmware/SDK combinations support both operations. `OFWGetVersion()` exposes firmware/protocol versions, but the bundled header defines no Bluetooth capability flag or minimum supported firmware. Do not invent a version threshold or assume that a successful generic config read proves Bluetooth support. Until reliable capability evidence exists, keep both properties hidden.
2. When support is established, represent visibility in the `.driver` source. For mixed firmware populations, determine capability per physical wheel before defining the connected properties. Unsupported Bluetooth must not prevent normal USB wheel operation; distinguish `AO_ERROR_NOT_IMPLEMENTED` from communication failure.
3. Read the radio state and Bluetooth name on connection before reporting them as OK. Execute writes in the existing generated device handlers. Reject invalid text before calling the SDK; on a failed write restore the last confirmed property value and report ALERT. After a successful write, use checked readback to establish the effective value. If readback fails, report uncertainty with ALERT and allow a later refresh; do not pretend the hardware rolled back.
4. Refresh Bluetooth state after factory reset and on reconnect. Whether writes persist across power cycles, require a reboot, change advertising immediately or interrupt another Bluetooth client is not documented here. Verify those behaviors on hardware before promising them in messages. Do not add INDIGO config persistence or automatic reapplication of Bluetooth settings without a deliberate requirement.
5. Once enabled, update `indigo_docs/PROPERTIES.md` visibility/semantics and the automated coverage notes. Implement in `.driver` and regenerate; no generator extension, new mutex or alternative transport is implied.

### Validation before enabling

Hardware-free cases must cover supported and unsupported firmware responses, hidden/visible enumeration, radio on/off, exclusive switch selection, correct configuration mask and preservation of unrelated fields, independent friendly/Bluetooth names, text boundaries, failed writes/readback, reconnect/reset refresh and disconnect during queued work. Verify actual bus handling of requests to hidden properties rather than treating `hidden` as an access-control mechanism.

Physical validation must establish actual radio enable/disable and advertised-name changes, supported firmware versions, legal name lengths/encoding, persistence across power cycles, reset effects and continued USB operation while a Bluetooth client is connected. Pairing and control from that client are hardware interoperability checks, not functionality implemented by this USB driver.

Result: the existing Bluetooth property/API design and future activation requirements are documented. Bluetooth remains deferred; no driver source or property visibility changed in this documentation step.

## Target generator structure

Use `driver astroasis`, one `wheel` block and `sdk { hotplug = true; vid = 0x338f; pid = 0x0fe0; plug { ... } unplug_match { ... } }`. Implement `astroasis_open(indigo_device *device)` and `astroasis_close(indigo_device *device)` in shared code. Keep discovery/probe logic there or in the SDK blocks, and use property `on_change` blocks plus delayed finalizers for hardware operations.

The generator owns property allocation/definition/deletion, connection dispatch, queues, USB references and attach/detach/shutdown. Use `connection_result` in `on_connect`; never return from connection lifecycle blocks. Respect generated property prologues/epilogues and use explicit updates only for messages, early returns or asynchronous completion. Keep hidden attributes declarative. Inherit slot names/offsets and configuration handling.

Reuse the existing `unplug_match`. The user explicitly approved one generator extension: optional `supported_architecture`, with documented Intel/Apple Silicon conditions using INDIGO_MACOS; Astroasis itself only excludes __i386__ because it supports both macOS CPUs. Attribute omission preserves existing output and unrestricted architecture support. Do not add a mutex, condition variable, shutdown flags or alternative queue framework. Use generator defaults unless the user explicitly requests otherwise. Preserve license/history, extend the copyright to 2026 and add the required Codex refactoring notice. Hand-written blocks use tabs, K&R braces, one-line calls and no blank lines inside functions. Never hand-format generated output. Never edit README without a separate request.

## Atomic implementation steps

Each step has one concrete outcome and a verification gate. Record actual commands, results, intentional behavior changes and deferred checks under its `Result:` before proceeding. Steps are logical changes, not authorization to commit automatically.

1. **Capture executable baseline and compatibility decisions.** Read the remaining relevant build/test references, build the original driver with `make -C indigo_drivers/wheel_astroasis -f ../../Makefile.drv`, record warnings and platform limits. Resolve the generator representation of the i386 fallback and item hint, document optional Bluetooth handling and proposed polling bounds. Preserve the baseline public-property table above.

   Result: baseline narrow make passed (existing targets were up to date). The initial dirty Xcode change was the user’s REFACTOR reference and is preserved. Optional Bluetooth NOT_IMPLEMENTED results do not fail connection; communication errors do. A 240 × 0.5-second software poll budget (120 seconds) bounds initialization, movement and calibration; this is a driver policy, not a hardware timing guarantee. The item warning is preserved in property on_attach. Architecture compatibility uses the explicitly approved generator attribute.

2. **Introduce generator source and equivalent lifecycle.** Add `.driver` with metadata/version increment, custom property declarations and required hints. Implement SDK scan/probe/deduplication, `unplug_match`, count-based open/close and checked connect. Replace manual scaffolding by regenerated `.c`, `.h`, `_main.c`; preserve exact names/labels/groups/rules/visibility. Keep the SDK scan array independent of the default five-device attach capacity. Add `.driver` to relevant project groups, including Xcode; inspect existing Windows project conventions without expanding platform support.

   Result: added .driver and regenerated C/header/main; universal x86_64/arm64 compilation and linking passed. SDK probes close on all query failures; hidden Bluetooth reads moved from discovery to connection so unsupported optional functions cannot suppress attachment. Default capacity is five, SDK scan buffer remains 32. Count-based helpers and SDK-membership unplug matching replace the legacy framework. Xcode source/test references preserve the user’s document reference. Windows projects continue compiling the same generated C; no new compilation unit is required.

3. **Make positioning and calibration asynchronous and checked.** Implement valid-state initialization, slot requests and delayed movement/calibration completion, failure and timeout handling, cross-property guards and disconnect cancellation. Remove all legacy operation timers/sleep loops and validate each remaining connection guard.

   Result: replaced timers and calibration sleep loop with one delayed operation finalizer. Checked status/position, SDK start failures, cross-property conflicts, timeouts and idle preflight are covered by the initial passing test suite. Generated handlers containing a finalizer suppress their standard state/update code, so those two handlers explicitly own their OK/start/final updates. Only the delayed finalizer retains an IS_CONNECTED guard; no redundant disconnect block or mutex was added.

4. **Finalize custom writes and reset consistency.** Implement transactional suffix/Bluetooth writes, optional-feature error handling, false-switch completion, factory-reset conflict handling and checked readback. Preserve hidden properties, limits subject to documented SDK ambiguity, naming-on-replug and reset confirmation hint. Avoid unrelated features or redundant disconnect initialization.

   Result: suffix and hidden Bluetooth writes preserve confirmed cache on failure. Factory reset remains connected, refreshes names/config/position and reports failed readback as ALERT; false action switches complete without an SDK call. Both Bluetooth properties remain hidden. Calibration start failure leaves the confirmed slot intact and reports calibration ALERT. Narrow build and all ten SDK groups passed, including the added boundary cases. Bluetooth setters remain dormant while the properties are hidden/undefined; direct public requests are verified to be ignored. Firmware activation validation remains deferred in the Bluetooth section above.

5. **Add hardware-free public-bus tests.** Add `indigo_test/integration/test_wheel_astroasis_sdk.c` and its Makefile target using the Player One arrangement: separately compiled generated driver, SDK/libusb stubs, real bus/base handlers and deterministic multi-device property cache. Exercise the matrix below, all exposed properties and hidden-property behavior. Keep configuration in a temporary test directory and clean it. Record coverage/deferred work in `indigo_test/CHANGES.md`.

   Result: added the separately compiled Oasis SDK/libusb fixture and Makefile target with ten groups. All passed; assertions cover public states/values, SDK calls, configuration roundtrip, cross-property workflows, scan/probe failure and balanced teardown. Names/offsets retain base-class persistence. Added a generic C architecture-generator test using test_runner.h, a synthetic AUX definition and temporary files; it runs both in the normal integration suite and through test-generator-architecture. All three C test groups passed after replacing the Python version; coverage and explicitly deferred work are recorded in indigo_test/CHANGES.md.

6. **Verify the completed migration.** Regenerate twice and compare generated files, build the production driver, run the complete Astroasis SDK suite and a native ASan/UBSan variant with instrumentation scope recorded. Run the existing ASI and Player One SDK suites as focused regressions. Check project syntax, formatting and `git diff --check`; remove test artifacts with `make -C indigo_test test-clean`.

   Result: production universal x86_64/arm64 build passed without compiler/linker warnings. All ten Astroasis SDK groups passed normally and under ASan/UBSan; instrumentation includes the test/driver/test-local framework/base, not shared libindigo. The native sanitizer build reports existing framework sprintf deprecation warnings. ASI (5) and Player One (12) regression cases passed. Architecture tests passed nine platform/CPU combinations, three reverse-extracted expressions, compiled fallback entry-point checks and the omitted-attribute case. Astroasis generation is reproducible; old/new generator C/header/main output is identical for wheel_asi, wheel_playerone, focuser_asi, rotator_asi, ccd_dsi and wheel_sx. Xcode plutil lint and git diff --check passed. Hand-written blocks/tests were checked for tabs, brace/call formatting, function separation and no interior blank lines; generated output was not hand-edited. The temporary handler simplification was reverted at the user’s request; the original finalizer pattern and explicit updates are retained. The restored version passed the production build and all ten hardware-free groups. AGENTS.md retains the minimal-custom-code rule with an explicit exception for _finalizer handlers. Test artifacts were removed with `make -C indigo_test test-clean`.

7. **Reconcile documentation and mark software completion.** Update `indigo_docs/PROPERTIES.md` source mapping and any intentional semantic changes, complete this document's results and automated coverage notes. Record unresolved SDK/hardware assumptions explicitly. Hardware testing is deferred, never marked passed; do not advance unrelated review records.

   Result: updated PROPERTIES.md contract/source mapping, generator architecture documentation and indigo_test/CHANGES.md. README, vendored SDK and unrelated review records are unchanged. The user’s pre-existing Xcode REFACTOR entry and subsequent MIGRATION_STATUS edits are preserved. All software steps are complete; physical operation, initial SDK discovery readiness and future Bluetooth activation remain explicitly unverified. Generator scheduling is unchanged: a failed discovery probe can be retried by a later arrival event, without claiming automatic retry.

## Hardware-free validation matrix

| Area | Required scenarios |
| --- | --- |
| Public contract | INIT/INFO/SHUTDOWN, exact driver/device naming, interface/version, INFO count/model/SDK label, all standard properties, five custom wire names/items/rules, hidden Bluetooth visibility, factory-reset hint, connected/disconnected enumeration. |
| Inherited workflows | Slot names/offsets, CONFIG save/load/remove, PROFILE and PROFILE_NAME roundtrip; temporary storage only, no accidental SDK slot-name/offset calls. |
| Discovery | Empty scan, first scan before ID APIs, SDK ID above array bounds, duplicate IDs/events, reversed USB/SDK order, simultaneous arrivals, new ID after replug, model/suffix boundaries, every probe failure and balanced probe close, attach failure/retry. |
| Capacity and removal | Default five-device capacity plus overflow/recovery; SDK buffer supports 32 IDs; invalid scan counts/errors, correct SDK membership removal, multiple removals from one event, unrelated VID/PID, per-pointer USB reference balance. |
| Connection | Global-lock/open failures, bad slot counts including allocation overflow, status/config/name errors, optional NOT_IMPLEMENTED, idle/moving/calibrating/benchmarking/unknown states, zero/invalid positions, repeated connect/disconnect, close/unlock balance and shutdown while connected. |
| Motion | First/last/same slot, bus range clamping, fractional/NaN input, start failure, BUSY confirmed value, transient positions, idle at wrong target, status read failure, bounded timeout and checked recovery, duplicate and competing requests. |
| Calibration | START=false, start/read failure, busy-to-idle success, invalid final position, timeout/retry, move/calibration/reset overlap, switch reset and exactly one final completion update. |
| Suffix and Bluetooth | Empty/1/31/32/33-byte values, getter termination, failed setters preserve confirmed cache, reconnect/replug naming, masked config preserves other fields, unsupported hidden features and direct hidden-property requests according to actual framework policy. |
| Factory reset | RESET=false, success/failure, busy rejection, readback and readback failure, preserved confirmation hint, property consistency while connected; reset-induced communication loss without fabricated success. |
| Teardown | Disconnect/unplug during movement and calibration, held SDK read before close, cancellation of delayed work, no updates after property deletion, ignored disconnected requests, normal shutdown/reinit and balanced SDK/global-lock/USB ownership. |

Stub tests establish software behavior under modeled SDK replies. They do not prove thread safety inside the proprietary SDK, USB discovery readiness, maximum operation durations, payload limits or physical calibration/reset/replug behavior. Do not infer arbitrary concurrent hot-plug/shutdown safety from ordinary teardown cases.
