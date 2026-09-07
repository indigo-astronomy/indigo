# Refactoring plan for INDIGO 3.0 ASI focuser driver

Goal: refactor the ASI EAF focuser driver into a generator-friendly INDIGO 3.0 structure and then migrate it to `indigo_generator`. Preserve current USB SDK/libusb hot-plug behavior, multi-device support, focuser motion semantics, custom EAF properties and public device behavior while moving ordinary focuser boilerplate into the generated driver.

## Reference material

- Use the current `indigo_focuser_asi.c` as the behavioral reference and the initial source to annotate.
- Use `indigo_drivers/wheel_asi/REFACTOR.md` and `indigo_drivers/wheel_asi/indigo_wheel_asi.driver` as the closest completed ASI SDK/hot-plug migration reference.
- Use `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` for generator extraction rules, SDK hot-plug blocks, helper naming, annotation blocks and `.driver` ownership.
- Use `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` and `indigo_docs/DEVELOPMENT.md` for the INDIGO 3.0 lifecycle, property states, handler queues and device/property model.
- Use `indigo_drivers/focuser_fcusb/indigo_focuser_fcusb.driver` and the other generated `indigo_drivers/focuser_*/**.driver` files as compact focuser-property examples.
- Use `indigo_tools/indigo_generator.c` as the authoritative source for generated SDK/libusb hot-plug behavior. In particular, confirm the generated SDK block owns the driver-wide queue, libusb callback, attach/detach storage, connection serialization and shutdown cleanup.
- Use `indigo_drivers/focuser_asi/README.md` and the bundled EAF SDK headers/documentation under `indigo_drivers/focuser_asi/bin_externals/libEAFFocuser/` when current behavior is unclear.
- Use `indigo_docs/PROPERTIES.md` as the property documentation authority; its `focuser_asi` section currently documents the EAF custom properties and standard-property specializations.

## Current public behavior to preserve

- Driver entry point: `indigo_focuser_asi`.
- Driver name: `indigo_focuser_asi`.
- Driver label: `ZWO ASI Focuser`.
- Supported hardware: USB ZWO EAF focusers matching vendor id `0x03c3` and product id `0x1f10`.
- Supported runtime behavior:
  - libusb hot-plug discovery, including startup enumeration;
  - up to `MAX_DEVICES` (10) simultaneously attached USB EAF focusers;
  - one logical INDIGO focuser per physical EAF;
  - names derived from `EAF_INFO.Name`, with an optional parenthesized SDK suffix rendered as `model #suffix`, then made unique with the SDK device id;
  - per-device connect/disconnect through the EAF SDK;
  - serialization of SDK enumeration/open/close with `indigo_device_enumeration_mutex`, plus per-device SDK serialization through `usb_mutex`.
- Standard focuser behavior:
  - `FOCUSER_POSITION`, `FOCUSER_STEPS`, `FOCUSER_ABORT_MOTION`, `FOCUSER_LIMITS`, `FOCUSER_BACKLASH`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_TEMPERATURE`, `FOCUSER_COMPENSATION` and `FOCUSER_MODE`;
  - position and relative moves are validated against the reported maximum, start `EAFMove()`, remain busy while EAF reports motion, and report the measured final position;
  - abort stops EAF motion and updates position/step state;
  - absolute-position `SYNC` calls `EAFResetPostion()`; `GOTO` calls `EAFMove()`;
  - limits, backlash, reverse direction and beep settings round-trip through the SDK;
  - automatic temperature compensation uses the configured threshold and steps/°C, clamps its target to focuser limits and waits for normal movement completion;
  - no temperature sensor is represented by the EAF sentinel below -270 °C and changes `FOCUSER_TEMPERATURE` to idle;
  - `FOCUSER_SPEED` stays hidden; manual/automatic mode changes retain the current visibility and read/write behavior of the affected standard properties.
- Custom properties:
  - `EAF_BEEP_ON_MOVE`: connected-only, `FOCUSER_ADVANCED_GROUP`, one-of-many switch with `ON` and `OFF`;
  - `EAF_CUSTOM_SUFFIX`: connected-only, `FOCUSER_ADVANCED_GROUP`, writable text with `SUFFIX`, persisted through the existing `CONFIG_SAVE` path and applied using `EAFSetID()`;
  - `EAF_BATTERY_INFO`: connected-only, read-only number property in `Battery`, eight current battery fields; hidden unless `EAFGetBatteryInfo()` succeeds;

Bluetooth support, including `EAF_SCAN_BLUETOOTH`, is intentionally removed from this USB-only refactor version.
- Informational behavior:
  - `INFO_PROPERTY->count = 6`;
  - EAF SDK version is published as `INFO_DEVICE_FW_REVISION` with label `SDK version`;
  - the SDK model portion is published as `INFO_DEVICE_MODEL`.

## Target shape

- One generator source of truth: `indigo_focuser_asi.driver`.
- Generated files: `indigo_focuser_asi.c`, `indigo_focuser_asi.h` and `indigo_focuser_asi_main.c`. Do not hand-edit these after generation.
- The generator owns USB callback registration, SDK-hot-plug attach/detach, device-array ownership, queue serialization and shutdown cleanup through an `sdk { hotplug = true; vid = ASI_VENDOR_ID; pid = EAF_PRODUCT_ID; ... }` block.
- EAF-specific discovery remains in `sdk.plug` / `sdk.unplug`; SDK connection setup and release live in helpers named exactly `asi_open(indigo_device *device)` and `asi_close(indigo_device *device)`.
- Private data retains only the per-device state needed after migration: SDK id, `EAF_INFO`, model/suffix cache, position/target/maximum/backlash/temperature state, sensor capability, timers as required by generated patterns, and custom-property storage. Do not retain legacy `devices[]`, `connected_ids[]`, callback handle or global manual-hot-plug state.
- USB SDK work is split into compact helpers and focuser handlers/finalizers:
  - one motion finalizer that reads `EAFIsMoving()`/`EAFGetPosition()` and completes both absolute and relative properties;
  - a periodic temperature/battery handler that reschedules itself through the generated handler queue;
  - handlers for limits, reverse, backlash, beep, suffix, abort and move/sync;
  - a focused connection initialization helper that reads all EAF state and defines connected-only properties only after success.

## Step-by-step plan

1. Establish the baseline
   - Record the existing build status of `focuser_asi`, bundled EAF SDK availability and all pre-existing working-tree changes.
   - Record the current property inventory, property `count`/`hidden` mutations, connection side effects and manual hot-plug lifecycle from the handwritten C source.
   - Record that physical USB EAF hardware is required for end-to-end hot-plug, motion, battery and temperature verification unless a simulator/SDK mock is added separately.

   Result:
   - Baseline recorded on Darwin. The working tree had no reported pre-existing changes.
   - Bundled EAF SDK artifacts are present for macOS, Linux (x86, x64, ARM and ARM64) and Windows; the macOS build uses `bin_externals/libEAFFocuser/lib/macOS/libEAFFocuser.a` and its public header.
   - `build/drivers/indigo_focuser_asi.a`, `indigo_focuser_asi.dylib` and the standalone executable were already present before rebuilding.
   - `make -C indigo_drivers/focuser_asi -f ../../Makefile.drv` completed successfully, compiling the current handwritten C source and linking the archive, dynamic library and standalone executable.
   - The build emits only bundled-SDK deployment-target warnings: x86_64 EAF objects target macOS 10.15 while the driver links for 10.10; arm64 EAF objects target 15.0 while the driver links for 11.0.
   - Captured the current custom-property allocations and all `count`/`hidden` mutations from the handwritten source. Hardware-backed validation remains deferred until a USB EAF is available.

2. Extract the lifecycle and property inventory
   - Map `focuser_attach()`, `focuser_connect_callback()`, `focuser_change_property()`, `focuser_timer_callback()`, `temperature_timer_callback()` and `focuser_detach()` to generator sections.
   - Inventory every custom allocation and property mutation:
     - `INFO_PROPERTY->count = 6`;
     - visible/hidden mutations for limits, speed, backlash, on-position-set, temperature, reverse, compensation, mode and battery;
     - `FOCUSER_COMPENSATION_PROPERTY->count = 2`;
     - `EAF_BEEP_ON_MOVE`, `EAF_CUSTOM_SUFFIX` and `EAF_BATTERY_INFO`.
   - Verify `indigo_docs/PROPERTIES.md` against the inventory. Update it in the implementation only if a public property, item count, visibility contract or behavior changes.
   - Identify and preserve error-state behavior for every SDK call; do not turn an SDK failure into a successful generated property update.

   Result:
   - Mapped `focuser_attach()` to `focuser.on_attach`, the USB portion of `focuser_connect_callback()` to generated connection handling plus `focuser.on_connect`/`focuser.on_disconnect`, and `focuser_detach()` to `focuser.on_detach`.
   - Mapped `temperature_timer_callback()` to the periodic device timer; mapped `focuser_timer_callback()` to a focuser-specific motion finalizer because it completes a pending movement rather than performing general polling.
   - Mapped `compensate_focus()`, movement/sync/abort helpers and SDK setting helpers to `focuser.code`. The current zero-delay callbacks map to property `on_change` handlers plus queued helper/finalizer calls.
   - Recorded the inherited properties requiring EAF-specific change behavior: `FOCUSER_POSITION`, `FOCUSER_STEPS`, `FOCUSER_ABORT_MOTION`, `FOCUSER_LIMITS`, `FOCUSER_BACKLASH`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_COMPENSATION` and `FOCUSER_MODE`. `FOCUSER_ON_POSITION_SET` is consumed by the position handler; `CONFIG_SAVE` persists the beep setting.
   - Recorded the custom property schemas: two-item one-of-many `EAF_BEEP_ON_MOVE`; one-item writable `EAF_CUSTOM_SUFFIX`; and eight-item read-only `EAF_BATTERY_INFO`, initially hidden and made visible only when `EAFGetBatteryInfo()` succeeds.
   - Recorded attach-time standard-property mutations: `INFO_PROPERTY->count = 6`; visible limits, backlash, on-position-set, temperature, reverse, compensation and mode; hidden speed; compensation count of 2; and ranges derived from `EAF_INFO.MaxStep`.
   - Confirmed the `focuser_asi` entry in `indigo_docs/PROPERTIES.md` covers the current custom properties and all specialized standard properties. No public behavior has changed, so no documentation edit is needed.
   - Recorded error-state boundaries to preserve: connection acquisition/global-lock failure returns `CONNECTION` to alert/disconnected; movement, position, relative move, abort and SDK-setting failures alert their affected standard/custom properties; polling failures alert position/steps or temperature/battery as applicable. Informational reads during successful connection are currently logged even when they fail, rather than rejecting the connection.

3. Reshape the handwritten driver without changing behavior
   - Keep this interim phase behavior-preserving and avoid broad formatting churn.
   - Separate includes/defines, private data, shared EAF SDK helpers, focuser-specific helpers, property handlers/finalizers, temporary manual hot-plug support and the driver entry point.
   - Isolate `split_device_name()`, SDK-id lookup and SDK open/close work from INDIGO callbacks.
   - Replace zero-delay worker timers with `indigo_execute_handler()` where that preserves ordering; retain delayed finalizers/polling using `indigo_execute_handler_in()`.
   - Replace the blocking `indigo_sleep(1)` loop in plug processing with a delayed retry/finalizer before migration. Never block the generated driver-wide hot-plug queue.

   Result:
   - Converted all USB focuser property workers from zero-delay `indigo_set_timer()` calls to `indigo_execute_handler()` calls on the per-device handler queue.
   - Converted focuser movement and temperature polling to `indigo_execute_handler_in()`; removed the obsolete per-device timer handles and use `indigo_cancel_pending_handler()` during disconnect/loss handling.
   - Added disconnected guards to both delayed handlers so queued work cannot access an EAF after connection teardown.
   - Removed both disabled `#if 0 ... #endif` blocks from driver initialization and shutdown.
   - Left the two manual hot-plug dispatch timers in place because they have no logical device queue; the generated `sdk` block replaces them in Step 4.
   - Built `focuser_asi` successfully. Only the known bundled EAF SDK macOS deployment-target warnings remain.

4. Introduce generator-compatible SDK helpers and hot-plug identity mapping
   - Add the target `sdk` block with the EAF USB vendor/product filters.
   - Move the current unmatched-id selection, `EAFOpen()`/`EAFGetProperty()` probe, model/suffix extraction and unique naming into `sdk.plug` while preserving distinct multi-device identities.
   - In `sdk.unplug`, release any EAF id reservation used to prevent a remaining SDK-enumerated device from being matched twice.
   - Add `asi_open()` and `asi_close()` with exactly the generator-required names.
   - Make `asi_open()` perform the current global-lock and EAF SDK connection acquisition, and guarantee lock release on every failure path.
   - Make `asi_close()` stop outstanding motion where appropriate, close the EAF SDK handle and release the global INDIGO lock exactly once.
   - Confirm which data the generator initializes before `sdk.plug`; do not assume `PRIVATE_DATA->usbdev` is sufficient when the EAF SDK uses its own device ids.
   - Retain equivalent locking until generated hot-plug queue serialization demonstrably covers EAF enumeration, attach/detach and connection work. Do not retain redundant locks without documenting why they protect SDK calls outside that queue.

   Result:
   - Added generator-compatible `asi_open(indigo_device *device)` and `asi_close(indigo_device *device)` helpers and routed the USB connection and disconnection paths through them.
   - `asi_open()` serializes SDK id lookup/open with `indigo_device_enumeration_mutex` and `usb_mutex`, acquires the INDIGO global lock only after the device is found, and releases it on every failed-open path.
   - `asi_close()` serializes `EAFStop()`/`EAFClose()`, logs close results and releases the INDIGO global lock exactly once after the SDK handle is closed.
   - Preserved enumeration locking during post-open state discovery, which is separate from the open helper until the generated driver-wide hot-plug queue replaces manual lifecycle serialization.
   - Replaced the blocking hot-plug `EAFGetProperty()` retry loop with a delayed retry. Failed probes now release their reserved SDK id before returning, so an EAF can be retried or matched again.
   - The handwritten libusb callback, device array and hot-plug timers remain temporarily. The actual `sdk { ... }` definition and generator-owned replacement are deferred to the `.driver` extraction/generation steps.
   - Built `focuser_asi` successfully; only the known bundled EAF SDK macOS deployment-target warnings remain.

5. Convert focuser behavior to generator-owned properties and queue-friendly handlers
   - Keep inherited standard properties in the focuser block and add explicit `on_change` blocks only where EAF behavior differs from the base focuser implementation.
   - Implement movement as request handler plus finalizer: validate limits, issue `EAFMove()`, publish busy state, poll EAF state/position, then update `FOCUSER_POSITION` and `FOCUSER_STEPS` together on success or alert.
   - Preserve sync, abort, limits, backlash, reverse, compensation and manual/automatic mode behavior, including all expected property redefinitions and permissions.
   - Make the temperature/battery polling loop a generated `on_timer` or a documented device-level delayed handler; it must reschedule only while connected and safely handle the absent-sensor sentinel.
   - Preserve temperature compensation’s threshold, clamp, movement-state checks and measured-position behavior. Ensure it cannot race a user-initiated motion on the same device queue.
   - Define `EAF_BEEP_ON_MOVE`, `EAF_CUSTOM_SUFFIX` and `EAF_BATTERY_INFO` as generated custom properties, preserving names, groups, permissions, item metadata, connected-only visibility and configuration persistence.
   - Keep `EAF_BATTERY_INFO` hidden until runtime capability probing succeeds; explicitly update its visibility/definition after connection.

   Result:
   - Separated delayed motion completion into `focuser_move_finalizer()` and reserved `focuser_timer_callback()` for the periodic temperature/battery polling loop expected by the generated focuser lifecycle.
   - Updated connection, disconnect, abort and motion paths to schedule or cancel the correct finalizer/polling handler.
   - Kept existing property schemas, SDK calls, ranges, busy/alert states and runtime battery-capability behavior unchanged pending their transfer to `indigo_focuser_asi.driver`.
   - Removed Bluetooth support and `EAF_SCAN_BLUETOOTH` from the USB-only driver version; synchronized `indigo_docs/PROPERTIES.md`.
   - Removed the obsolete `is_connected` alias. Connection checks use `IS_CONNECTED` for the established state and `CONNECTION_CONNECTED_ITEM->sw.value` for the requested transition.
   - Built `focuser_asi` successfully; only the known bundled EAF SDK macOS deployment-target warnings remain.

6. Add migration annotations and extract the initial definition
   - Add only the annotations required for the final generated shape:
     - `//+ include`, `//+ define`, `//+ data`, `//+ code`;
     - `//+ on_init`;
     - `//+ focuser.code`, `//+ focuser.on_attach`, `//+ focuser.on_timer`, `//+ focuser.on_connect`, `//+ focuser.on_disconnect`, `//+ focuser.on_detach`;
     - property `on_change` annotations for each inherited EAF-specialized property and each custom property.
   - Do not annotate manual libusb callbacks, global `devices[]`, legacy attach/detach timers, `callback_handle` or manual shutdown cleanup: the SDK block replaces them.
   - Run `indigo_generator -c indigo_focuser_asi.driver` from `indigo_drivers/focuser_asi`. Never pass the existing `.c` path to `-c`.
   - Inspect the extracted file and correct extraction gaps in the `.driver` (or, before generation, in narrowly scoped annotations) rather than hand-editing generated output.

   Result:
   - Added extraction annotations for the EAF SDK include/defines/private data, shared open/close helpers, focuser helpers, initialization log, attach, connect, disconnect, timer and all EAF-specialized property changes.
   - Kept the handwritten libusb callback, device array, callback handle and manual shutdown code outside generator-owned annotations.
   - Verified `indigo_generator -c indigo_focuser_asi.driver` in an isolated directory. The extracted draft contains the expected focuser lifecycle and property blocks and no manual hot-plug references.
   - The extractor also creates an empty `eaf` logical-device block from the `EAF_*` custom-property macro prefix. This is an extraction artifact; Step 7 will fold those three custom properties into `focuser` and remove the empty block in the hand-maintained `.driver` source.
   - Built the interim handwritten driver successfully and verified `git diff --check`. Only the known bundled EAF SDK macOS deployment-target warnings remain.

7. Complete and validate `indigo_focuser_asi.driver`
   - Confirm it includes driver metadata, EAF SDK headers, SDK hot-plug configuration, all required constants, private fields, `asi_open()`/`asi_close()`, the USB identity-mapping helpers and a focuser device block.
   - Confirm the generator definition contains the full custom-property schema, custom item metadata and persistent suffix behavior.
   - Confirm standard inherited properties have the correct generator attributes (`hidden`, `persistent`, `asynchronous_change`, `preserve_values` or `pass_through_change`) instead of copied manual boilerplate where generator semantics already own it.
   - Check generator semantics carefully for `FOCUSER_POSITION`, `FOCUSER_STEPS`, `FOCUSER_ABORT_MOTION`, `FOCUSER_MODE` and `CONFIG`: property handlers are generated as `void`, standard prologue/epilogue updates may be inserted, and manual early returns/final updates must be adapted accordingly.
   - Generate into a temporary directory first if any remaining generator behavior is uncertain, then inspect the generated C before replacing repository sources.

   Result:
   - Created `indigo_focuser_asi.driver` as the generator source of truth, with the EAF SDK headers, required constants and private per-device state.
   - Replaced the extracted `libusb` declaration with `sdk { hotplug = true; vid = ASI_VENDOR_ID; pid = EAF_PRODUCT_ID; ... }`. Its `plug` block enumerates EAF SDK ids, probes the device, reserves its id, preserves model/suffix naming and makes the INDIGO name unique; its `unplug` block releases that reservation.
   - Moved all three EAF custom properties into the single `focuser` block and removed the empty extraction-generated `eaf` device.
   - Retained `asi_open()` and `asi_close()` as generator-required helpers, now operating on the SDK id established by `sdk.plug` rather than on the removed manual device array and enumeration helpers.
   - Verified generator-owned property-handler semantics in a temporary generated C source. Removed the duplicate initial timer scheduling from `on_connect`, because the generated connection handler already starts the device timer.
   - Corrected generator-source details discovered by syntax checking: `<stdbool.h>` precedes the EAF header, the suffix item name is defined, `CONFIG` uses its copied switch value, and the focuser template uses the SDK-provided name.
   - Generated into a temporary directory and checked the generated C with `clang -fsyntax-only`; it completed without diagnostics before repository regeneration in Step 8.

8. Regenerate and remove legacy scaffolding
   - Run `indigo_generator indigo_focuser_asi.driver` to regenerate the repository `.c`, `.h` and `_main.c` files.
   - Make all subsequent source changes in `.driver`, then regenerate. Do not hand-edit the generated C.
   - Remove only the manual hot-plug code now replaced by the generator: callback registration/deregistration, callback handle, `devices[]`, manual id/device-slot searches, plug/unplug callbacks and manual shutdown removal.
   - Keep the Xcode project entries and Makefile integration synchronized with the repository convention; add the `.driver` file to project metadata if existing generated-driver conventions require it.
   - Inspect `git diff` to ensure every generated difference corresponds to an intended lifecycle, property or queue-semantics change.

   Result:
   - Regenerated `indigo_focuser_asi.c`, `indigo_focuser_asi.h` and `indigo_focuser_asi_main.c` from `indigo_focuser_asi.driver`; the generator added the `.driver` file to the `focuser_asi` Xcode group.
   - The generated `sdk` lifecycle now owns hot-plug callback registration, driver-wide queueing, device storage, attach/detach processing and shutdown cleanup. The handwritten device array, callback handle, id/slot lookup helpers and manual plug/unplug lifecycle are absent from the regenerated source.
   - Removed redundant handwritten custom-property pointers from the `.driver`; the generator supplies `eaf_beep_property`, `eaf_custom_suffix_property` and `eaf_battery_info_property` for the declared custom properties.
   - Removed the per-device `usb_mutex` and all manual mutex operations. Generated device and driver SDK queues serialize the lifecycle and per-device handlers.
   - Normalized generator-source formatting: one blank line between functions, no blank lines inside function bodies, and a space after `if`.
   - Built the regenerated driver successfully. Only the known bundled EAF SDK macOS deployment-target linker warnings remain.

9. Verify and document the migration
   - Build the narrow driver target with `make -C indigo_drivers/focuser_asi -f ../../Makefile.drv` and resolve source/generator failures in `.driver`.
   - Run the narrowest available hardware-free checks. If no EAF simulator or mock exists, record the limitation rather than claiming hardware coverage.
   - With physical hardware, verify startup enumeration, plug/unplug, repeated connect/disconnect, simultaneous EAF devices, name/suffix uniqueness, motion, sync, abort, limits, backlash, reverse, beep, battery capability visibility, sensor absence and temperature compensation.
   - Verify failed SDK open/probe/move paths restore connection/property states and do not leak EAF id reservations, global locks, timers or dynamically allocated properties.
   - Update `indigo_docs/PROPERTIES.md` only if the public property inventory or documented behavior changed. Update `README.md` only if platform, supported-device or operational behavior changed.

   Result:
   - Built the generated driver with `make -C indigo_drivers/focuser_asi -f ../../Makefile.drv`; the archive, dynamic library and standalone executable were produced successfully.
   - `git diff --check` is clean. The generator emits only visibility notices for inherited properties; the linker emits the existing bundled-EAF-SDK macOS deployment-target warnings.
   - No EAF simulator, mock or automated `focuser_asi` test is available. `TESTING.md` records an historical physical ZWO EAF test only, so USB enumeration, plug/unplug, repeated connection, motion, sync, abort, limits, settings, battery capability and temperature compensation remain hardware-validation items.
   - Confirmed `indigo_docs/PROPERTIES.md` documents exactly `EAF_BATTERY_INFO`, `EAF_BEEP_ON_MOVE` and `EAF_CUSTOM_SUFFIX` for this driver. The driver README contains no Bluetooth behaviour to update; no further documentation change was needed.

## Bluetooth support reference (intentionally not implemented)

This section captures the Bluetooth implementation removed from the pre-refactor driver. It is a recovery reference for a later, separately scoped generated-driver change; it is not a request to restore Bluetooth in the current USB-only driver.

### Previous public surface

- Bluetooth was exposed as a separate logical focuser named `EAF Pro (Bluetooth)`, rather than as a transport selector on a USB EAF instance.
- It added a connected-capable custom switch property named `EAF_SCAN_BLUETOOTH` in the `Main` group, with `INDIGO_AT_MOST_ONE_RULE`.
- Item zero was `RESCAN` / `Rescan`; each following item represented one scanned BLE device. Its item name was the concatenation `<device name>-<address>` and its label was the device name.
- The selection was stored per INDIGO device in a `.bt` configuration file, as `selected_bt_device=<item-name>`. The old code read the file while rebuilding the property and preselected the matching item.
- The existing EAF beep, custom suffix and battery properties remained available. The standard focuser properties reused the normal motion/settings handlers after BLE pairing.

### Previous SDK sequence

- At Bluetooth-device attach, the driver called `EAFBLEScan(1000, devices, 64, &found)`, retained up to `MAX_BLE_DEVICES` (64) `BLE_DEVICE_INFO_T` values and rebuilt `EAF_SCAN_BLUETOOTH`.
- A user rescan called `EAFBLEScan(3500, ...)`; it deleted and released the old dynamic scan property, rebuilt its items and redefined it. Changing selection while connected set the scan property to alert.
- On CONNECT, the selected `<name>-<address>` value was split at its final `-`, then passed to `EAFBLEConnect(name, address, &id)`. Successful connection was followed by `EAFBLEPair(id)`; the user was prompted to press IN or OUT if the focuser beeped.
- After pairing, the SDK id became the logical device id and the driver registered `EAFBLERegConnStateCallback(id, ble_connection_state_callback)`.
- Bluetooth initialization used `EAFBLEgetAllInfo(id, &all_info)` to populate maximum steps, current position, backlash, reverse state and buzzer state. This differed from USB initialization, which used the individual `EAFGet*` calls. The old source explicitly noted this distinction as unresolved.
- On normal disconnect it called `EAFStop(id)` and `EAFBLEDisconnect(id)`. On a failed BLE connect/pair it attempted to disconnect and reset the id to `-1`.

### Lost-connection handling and migration requirements

- The old global `ble_connection_state_callback(bool state)` logged state transitions. On loss, it manipulated the Bluetooth device directly: switched CONNECTION to disconnected, cancelled movement/temperature handlers, deleted connected-only custom properties, released the global lock and invoked the focuser connection path.
- Do not copy that callback implementation: it used the obsolete `device->is_connected`/`gp_bits` state, direct property mutation from an SDK callback and manual mutexes. A new version must marshal the event onto the generated driver/device queue, use `CONNECTION_CONNECTED_ITEM->sw.value` for the requested transition and let the generator own CONNECTION completion and property cleanup.
- The 1 s / 3.5 s scans are blocking SDK operations. They must execute as queued asynchronous work and publish busy/complete/alert scan-property states without blocking a bus callback or the generator's hot-plug queue.
- The selection encoding is ambiguous when a BLE device name itself ends in `-...`. Preserve the existing format only for compatibility if necessary; otherwise add a migration strategy before changing persisted `.bt` data.
- Reintroduce Bluetooth as a separate generated logical device or an explicitly supported generator construct. It must not be folded into the USB `sdk` block, because BLE discovery does not originate from a libusb device event.
- Add hardware-backed tests before enabling it: initial scan, rescan, persisted selection, pairing confirmation, normal disconnect, unexpected loss callback, failed connect/pair, and coexistence with one or more USB EAF devices.
