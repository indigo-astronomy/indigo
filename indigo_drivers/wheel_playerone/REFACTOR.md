# Refactoring plan for INDIGO 3.0 Player One filter wheel driver

Goal: migrate `indigo_wheel_playerone.c` to `indigo_generator` using the current `wheel_asi` implementation as the structural reference. Preserve Player One device identity, public properties and reset behavior while moving hot-plug, connection and property boilerplate into generated code.

Status: software migration and hardware-free verification completed, 2026-09-08. Results are recorded below each step. Physical hardware is unavailable; hardware validation remains explicitly deferred.

## Reference material

- `indigo_drivers/wheel_asi/indigo_wheel_asi.driver`: primary reference for the final SDK hot-plug structure, open/close helpers, wheel movement finalizers and custom properties.
- `indigo_drivers/wheel_asi/indigo_wheel_asi.c`: verify actual generated queue dispatch, connection failure cleanup, attach/detach and shutdown behavior.
- `indigo_drivers/wheel_asi/REFACTOR.md`: migration history. Its intermediate `libusb`/match-helper and connected-id approaches are historical, not the target design.
- `indigo_drivers/wheel_playerone/indigo_wheel_playerone.c` and `README.md`: current Player One behavior and supported platforms.
- `indigo_drivers/wheel_playerone/bin_externals/libPlayerOnePW/include/PlayerOnePW.h`: bundled SDK contract, handle identity, position return codes and `MAX_NAME_LEN`.
- `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `DRIVER_DEVELOPMENT_BASICS.md` and `DEVELOPMENT.md`: generator ownership, handler queues and device/property lifecycle.
- `README.md`, `TESTING.md`, `indigo_docs/MAKEFILES.md` and the driver's `Makefile.inc`: build and validation conventions.
- `indigo_docs/PROPERTIES.md`, section `wheel_playerone`: authoritative property documentation mapping.
- `indigo_test/integration/test_wheel_asi_sdk.c` and its rules in `indigo_test/Makefile`: hardware-free SDK/libusb test pattern. Read `indigo_test/AGENTS.md` before test work; record coverage and deferred tests in `indigo_test/CHANGES.md`.

## Public behavior to preserve

- Entry point and driver name: `indigo_wheel_playerone`; label: `Player One Filter Wheel`; current version: `0x03000009`.
- One logical wheel per physical device; hot-plug and multiple devices, legacy capacity was ten; the refactored driver uses the generator default of five.
- Current USB filter: vendor `0xa0a0`, product `0xf001`. Do not broaden support as part of migration.
- SDK discovery through `POAGetPWCount()` and `POAGetPWProperties()`, with operations addressed by `PWProperties.Handle`, not enumeration index.
- Model name and optional custom suffix in the attached name, currently converting SDK `model [suffix]` to `model #suffix`; unique names for duplicate models.
- Standard wheel properties and editable slot names/offsets remain inherited from the wheel base class.
- INDIGO slots are one-based; `POAGotoPosition()` and `POAGetCurrentPosition()` use zero-based positions.
- `INFO_PROPERTY->count = 6`, model in `INFO_DEVICE_MODEL_ITEM`, SDK version from `POAGetPWSDKVer()` in the firmware revision item with label `SDK version`.
- SDK version logging at driver initialization and the INDIGO global lock around an open connection.

| Property | Existing contract |
| --- | --- |
| `WHEEL_SLOT` | Maximum from `PWProperties.PositionCount`; movement is BUSY until complete, SDK failures report ALERT. |
| `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET` | Counts set to `PositionCount` on connection. |
| `X_CUSTOM_SUFFIX` | Connected-only RW text, `WHEEL_MAIN_GROUP`, label `Device name custom suffix`, one item `SUFFIX` / `Suffix`; accepts up to 24 bytes as measured by `strlen()`, including empty text to clear. New name applies on replug. |
| `X_RESET` | Connected-only RW switch, `WHEEL_ADVANCED_GROUP`, label `Reset filter wheel`, `INDIGO_ONE_OF_MANY_RULE`, one item `RESET` / `Reset`, initially false. Successful `POAResetPW()` clears the switch and disconnects; failure clears the switch and reports ALERT. |

The current driver has no explicit property `hidden` assignments. Custom property names use the `X_` prefix: rename `POA_CUSTOM_SUFFIX` to `X_CUSTOM_SUFFIX` and `POA_RESET` to `X_RESET`, as requested. Keep groups, permissions, rules and item counts unchanged. Do not import ASI's `X_CALIBRATE` or eight-byte suffix limit.

## Target structure

- Add `indigo_wheel_playerone.driver` with `driver playerone`, the existing label/author and an appropriate version increment when implementation is made. Helpers must be `playerone_open(indigo_device *device)` and `playerone_close(indigo_device *device)`.
- Use `sdk { hotplug = true; vid = PONE_VENDOR_ID; pid = 0xf001; plug { ... } }`. Add `unplug` only if Player One-specific reservations require cleanup.
- Let the generator own libusb registration, USB references, `devices[]`, capacity checks, attach/detach, driver queue and shutdown. Keep only Player One enumeration and naming inside `sdk.plug` or shared helpers.
- Keep private data for SDK handle, model, suffix cache, current/target slot, slot count and any required initialization deadline. Rename the old `count` field to `slot_count`: `PRIVATE_DATA->count` is reserved for open/close reference counting. For this single logical device, the helpers maintain it; the generator provides that accounting only for multi-device templates.
- Use a single `wheel` block with `on_attach`, `on_connect`, inherited `WHEEL_SLOT`, text `X_CUSTOM_SUFFIX` and switch `X_RESET`.
- Use generated property storage and connection state. Remove hand-written property pointer fields, enumeration/release code and the `gp_bits` / `is_connected` alias once replaced.
- Use queued handlers for SDK operations and delayed finalizers through `indigo_execute_handler_in()` for motion. Remove legacy zero-delay worker timers and `wheel_timer` after equivalent cancellation is verified.
- Preserve the license and version history, extend the copyright year to 2026 and append the required OpenAI Codex refactoring notice after the license header.

## Player One-specific decisions

### Discovery and identity

The bundled SDK explicitly allows `POAGetPWProperties()` before opening the wheel. Prefer that enumeration metadata for attach-time handle/model discovery instead of copying ASI's temporary open/query/close sequence. Confirm naming against the SDK and hardware before removing the old probe, which opens the wheel and retries `POAGetPWPropertiesByHandle()` while moving.

Deduplicate handles against attached devices, following the final ASI structure. Avoid `connected_handles[PONE_HANDLE_MAX]`: the current driver indexes this 24-element array directly with SDK handles, whereas the header describes unique handles without establishing that array bound. Reserve identity at attach, retain it across ordinary disconnect/reconnect and make failed attach retryable.

Do not assume enumeration order maps a USB arrival to the same SDK device. Verify the generated USB-device-to-SDK-handle association and correct unplug behavior with two wheels, reordered enumeration and different connect orders. The header exposes a serial number but no direct libusb mapping API. The implemented `unplug_match` extension therefore checks SDK-handle presence on removal instead of relying on arrival-order correlation.

### Connection and initial movement

Split SDK open/global-lock ownership into `playerone_open()` / `playerone_close()`. Validate `POAGetPWPropertiesByHandle()` before using its output, then validate `PositionCount` against both slot-property allocations before changing counts. Check position and suffix reads and publish only valid SDK output.

Replace the current connect-time sleep loop of up to 15 seconds with delayed polling. Following ASI, the intended behavior is a connected device with `WHEEL_SLOT` BUSY during initial positioning, followed by a valid one-based value/target or ALERT. Preserve a bounded initialization wait and a useful timeout message. Record this timing change explicitly: the old implementation waits before completing connection.

Use `connection_result` in `on_connect`; do not return from connection lifecycle blocks. Inspect the generated failure path and ensure successful SDK open is paired with exactly one close/global unlock when subsequent initialization fails. Do not duplicate cleanup already owned by the generator.

### Movement, reset and suffix

- Read position into a local SDK variable. On `PW_ERROR_IS_MOVING`, keep the last valid public position, remain BUSY and reschedule. Convert to one-based position only on `PW_OK`; other errors finish in ALERT. An idle result at the wrong target must not poll forever.
- Preserve bus range clamping, fractional/NaN rejection and the same-slot no-op. Verify BUSY guards before request values are overwritten and prevent overlapping movement/reset operations.
- Keep reset distinct from calibration. After successful `POAResetPW()`, publish reset completion and request the generated disconnect lifecycle on the driver queue. Do not invoke the removed legacy connection callback or close/free a device directly from a property handler. Verify reset completion updates occur before property deletion and no generated epilogue updates a deleted property.
- Clear the reset switch on completion/failure and restore clean state for reconnect. A reset request with `RESET = false` must settle without issuing an SDK reset.
- Move `POASetPWCustomName()` out of the bus callback into the generated handler. Use the SDK's 24-byte limit and terminated buffers with space for the full suffix. Check read/write failures and keep cached suffix state consistent with successful writes.
- Replace the existing 16-byte suffix parsing buffers with bounded parsing supporting the full SDK limit. Handle absent or malformed brackets and long model names without unterminated strings. Preserve the displayed `#suffix` convention and replug messages.

### Concurrency and cleanup

The generator uses a driver-wide queue for hot-plug and connection work, plus per-device queues for ordinary properties/finalizers. Verify how generated disconnect drains or cancels device work before SDK close. Remove the old enumeration mutex and `usb_mutex` only after every SDK call path has an equivalent serialization/lifetime guarantee; do not assume two different queues serialize each other.

No sleep/retry loop may monopolize the driver queue. If SDK discovery needs a readiness retry, use a bounded delayed mechanism compatible with generated ownership. Verify disconnect, unplug and shutdown leave no callback able to touch a closed handle or freed private data.

## Step-by-step implementation plan

1. **Establish the baseline.** Record commit, dirty files, SDK version and narrow driver build result. Capture property definitions/count mutations and current connect, move, reset, suffix and multi-device behavior. Distinguish real hardware observations from source assumptions.

   Result:
   - Baseline commit: `c252256d4d9b411cf353264b3f1214d5403f684f` on macOS.
   - Pre-existing changes: staged `REFACTOR.md` and the user's Xcode references to that file; preserved.
   - Bundled SDK version file: `bPlayerOnePW v.1.2.3`.
   - Narrow build and forced recompilation of both C sources passed, producing the archive, dylib and executable. Existing warning: bundled SDK targets macOS 10.15 while the driver targets 10.10.
   - Property inventory and source-observed workflows are recorded above. Hardware is unavailable; no physical behavior is claimed verified.

2. **Prepare the generator input.** Write `.driver` directly from the current Player One source using the final ASI layout; this avoids recreating superseded ASI migration stages. If annotation/extraction is used instead, run `../../build/bin/indigo_generator -c indigo_wheel_playerone.driver` from the driver directory, never with `.c` as the `-c` output target.

   Result:
   - Added `indigo_wheel_playerone.driver` directly from the Player One source using the final ASI structure. Metadata uses `driver playerone`, version 10 (`0x0300000A`), the original author and the required 2026 refactoring notice.
   - All custom implementation lives in generator-owned blocks. The old slot `count` became `slot_count`; generated property storage and lifecycle replace the hand-written boilerplate.

3. **Implement SDK discovery and lifecycle.** Add handle deduplication, bounded naming, `playerone_open()` / `playerone_close()` and checked connection initialization. Remove manual hot-plug scaffolding only when generated paths cover attach failure, capacity, unplug and shutdown.

   Result:
   - SDK discovery uses metadata enumeration without a temporary open or a blocking readiness loop. Attached handles are deduplicated by comparison, including handles greater than 23; duplicate USB arrival is rejected.
   - Added bounded model/suffix parsing and unique naming, preserving full 24-byte suffixes. No new properties were introduced.
   - Added `playerone_open()` / `playerone_close()` with checked open, balanced global locks and idempotent close. As requested, standard integer `count` replaces `opened`: successful opens acquire a reference, failed opens leave it unchanged, and the last close releases the SDK handle and global lock; connection validates SDK metadata, slot capacity, position and suffix reads.
   - After replacing `opened` with `count`, regeneration, the universal macOS build and all 12 hardware-free SDK cases passed, including failed connection cleanup, repeated connect/disconnect, reset and unplug. `git diff --check` passed.
   - Removed the local `MAX_DEVICES` override as requested. The driver uses the generator default of five; capacity overflow/recovery tests use that limit. Regeneration, the universal macOS driver build and all 12 hardware-free SDK cases passed after removing the override. `AGENTS.md` now requires generator defaults during refactoring unless the user explicitly requests an override.
   - The driver queue serializes enumeration and connection operations. Framework `indigo_queue_remove()` waits for a running device handler before generated disconnect closes the SDK handle; dedicated legacy mutexes are unnecessary.
   - Added the explicitly approved optional `sdk.unplug_match` generator block. Player One determines absence by SDK handle, so reversed USB/SDK arrival order cannot detach the wrong logical wheel. Failed enumeration is inconclusive and does not detach devices.
   - Generated removal frees each matched private-data allocation and its retained USB reference once, separately from the event reference. Shutdown bypasses SDK presence checks using the existing `last_action`. No new mutex, condition variable, queue-drain protocol or other lifecycle change was introduced.

4. **Implement properties and finalizers.** Migrate slot movement, asynchronous initialization, reset/disconnect and suffix handling. Check generated prologues/epilogues: an `on_change` block containing `_finalizer` suppresses the final automatic property update, so every completion/error/no-op path must publish the required state.

   Result:
   - Migrated movement, reset and suffix changes to generated handlers. Initial positioning uses a 30-poll delayed finalizer instead of blocking connection for up to 15 seconds.
   - Position replies are converted only on success. BUSY retains the last valid position; wrong-target, invalid-position and read failures settle in ALERT. Fixed the start update so clients receive the current position after the request is copied.
   - The bus clamps out-of-range numeric requests before the handler, in both the old and new implementation. Tests cover that actual contract; fractional and NaN slots are rejected without SDK movement.
   - Removed the redundant `on_disconnect` reset initialization: the property is deleted on disconnect and initialized on successful connect; the reset handler clears its switch itself.
   - Reset preserves successful-disconnect behavior through a public connection request and early return before the generated property epilogue. Failure/no-op/busy rejection and reconnect are tested.
   - Suffix failures retain the last accepted value. Full-length suffixes survive reconnect and replug.
   - Removed redundant `!IS_CONNECTED` guards from the three property handlers: undefined properties are rejected by `indigo_property_match_changeable()` and queued work is drained on disconnect. Retained the guard only in the delayed movement finalizer, which can be rescheduled by a running poll while disconnect is waiting for it.

5. **Generate and integrate.** Run `../../build/bin/indigo_generator indigo_wheel_playerone.driver` from the driver directory. Keep `.driver`, generated `.c`, `.h` and `_main.c` synchronized. Add `.driver` to the `wheel_playerone` Xcode group and other relevant project listings without changing unrelated build settings. Check that a second generation produces no diff.

   Result:
   - Generated `.c`, `.h` and `_main.c` from `.driver` and added the generator input and SDK test to the existing Xcode groups, preserving the user's pre-existing REFACTOR references.
   - No generated C was hand-edited. Hand-written `.driver` and test functions use repository whitespace/braces/call formatting.
   - A second generator run produced byte-identical C/header/main files. Old/new generator output is byte-identical for `wheel_asi`, `focuser_asi`, `rotator_asi`, `ccd_dsi` and `wheel_sx`. Xcode project syntax passed `plutil -lint`. The generator extension is limited to `unplug_match` and its required matching/cleanup behavior; drivers that omit it retain their previous generated output.

6. **Build and inspect the migration diff.** Run `make -C indigo_drivers/wheel_playerone -f ../../Makefile.drv` from the repository root. Verify the archive, shared library and standalone target using the bundled SDK. Explain each behavioral difference, especially initialization timing and error handling; fix generated behavior in `.driver`, not generated C. Do not modify vendored SDKs or commit build artifacts.

   Result:
   - Narrow universal macOS build passed for both x86_64 and arm64, including archive, dylib and executable. No compiler warnings remain; the pre-existing SDK deployment-target linker warning remains.
   - Compared the generated lifecycle, property definitions and removed legacy callbacks with the original. Intentional changes are checked errors, safe suffix parsing, asynchronous initial positioning, BUSY current-position reporting, reset busy rejection and generator queue ownership.
   - Generated driver metadata uses the same `multi_device_support = false` convention as `wheel_asi` for a single logical wheel template; multiple physical wheels remain supported and tested up to the default five.

7. **Add hardware-free SDK tests.** Follow the ASI test arrangement: compile the driver separately with test hooks and provide Player One SDK/libusb stubs, exercise the public entry point and bus APIs. Add a dedicated test target under `indigo_test`, document coverage/deferred work in `CHANGES.md` and clean test outputs according to its instructions.

   Result:
   - Added `indigo_test/integration/test_wheel_playerone_sdk.c` with 12 fixture-isolated test cases, covering every exposed property and the workflows listed in `indigo_test/CHANGES.md`.
   - Tests compile the generated driver separately, substitute SDK/USB calls and keep actual bus, wheel properties, configuration and queue behavior. Configuration uses a temporary test directory; SDK handles are deliberately outside the old array's range.
   - All 12 cases passed normally and with AddressSanitizer/UndefinedBehaviorSanitizer after the final generator simplification. The initialization-timeout case accelerates dispatch, not the production poll limit.
   - Regression checks passed: 5 ASI wheel SDK cases and 86 timer/queue cases. The additional removal test checks reversed USB/SDK arrival order, inconclusive SDK enumeration, several removals in one event and per-USB-reference balance. Test outputs were removed with `make -C indigo_test test-clean`.

8. **Validate hardware and documentation.** Exercise a real Phoenix wheel and two-wheel hot-plug/identity when available. Reconcile `indigo_docs/PROPERTIES.md` if properties/items are added or removed, or documented behavior changes. Do not change README without an explicit user request. Record actual build/test results here as steps complete; keep review findings in the applicable `REVIEW.md` workflow and automated-test coverage in `indigo_test/CHANGES.md`.

   Result:
   - Updated the property reference/source mapping. Renamed `POA_CUSTOM_SUFFIX` to `X_CUSTOM_SUFFIX` and `POA_RESET` to `X_RESET`, as requested; items, groups and permissions remain unchanged. Added the mandatory custom-property `X_` prefix check to `AGENTS.md`. README is unchanged, per user instruction.
   - After the custom-property rename, regenerated sources, rebuilt the universal macOS driver and reran all 12 hardware-free SDK cases successfully. `git diff --check` passed.
   - Recorded automated coverage and deferred work in `indigo_test/CHANGES.md`; no incremental review baseline was advanced.
   - Physical tests were not run because the user confirmed hardware is unavailable. Reversed USB/SDK order and multiple removals are covered by stubs; real SDK readiness and physical reset/replug timing remain unverified.
   - Software implementation is complete; hardware validation is explicitly deferred rather than marked passed.

## Validation matrix

| Area | Required scenarios |
| --- | --- |
| Lifecycle | INIT/INFO/SHUTDOWN, attach-time properties, connected-only custom properties, repeated connect/disconnect, global-lock failure, SDK open failure, initialization failure after open, balanced close/unlock. |
| Initialization | Invalid handle/property query, zero or excessive slot count, initial motion then idle, initialization timeout, state/position/suffix read errors, disconnect during initial motion. |
| Slots | First/last position, zero-based SDK conversion, same-slot no-op, out-of-range request, repeated moving replies, idle at wrong target, SDK start/read failures, overlapping requests. |
| Reset | Success followed by disconnect, failure remaining connected with ALERT, false-switch no-op, reset while moving, reconnect after reset, no stale finalizer or property update after disconnect. |
| Suffix | Empty, 1-byte and 24-byte values, reject 25 bytes, read/write failure, full-length attach name, malformed brackets, replug naming, duplicate model names. |
| Hot-plug | Two or more wheels, reordered SDK enumeration, connect in reverse order, unplug the correct wheel while idle/moving, failed attach retry, capacity overflow/recovery, repeated arrival, unrelated USB product, shutdown with pending work. |

Completion requires reproducible generation, a successful narrow build, passing SDK-backed integration scenarios and an explained generated diff. Stub tests cannot establish real SDK readiness timing, physical USB-to-handle mapping or reset/replug behavior; record any unavailable hardware checks as deferred rather than claiming full validation.
