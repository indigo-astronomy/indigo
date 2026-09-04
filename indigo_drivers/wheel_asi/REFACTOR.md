# Refactoring plan for INDIGO 3.0 ASI filter wheel driver

Goal: refactor the ASI filter wheel driver into a generator-friendly INDIGO 3.0 structure and then migrate it to `indigo_generator`. Preserve the current SDK/libusb hot-plug behavior, multi-device support, custom ASI properties and public device behavior while moving ordinary wheel boilerplate into the generated driver.

## Reference material

- Use the current `indigo_wheel_asi.c` as both the behavioral reference and the initial source to annotate.
- Use `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` for generator extraction rules, open/close helper naming, annotation blocks and `.driver` ownership.
- Use `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` for INDIGO 3.0 device lifecycle, property state handling and async handler queues.
- Use `indigo_docs/DEVELOPMENT.md` for bus/device/property model details when validating lifecycle changes.
- Use `indigo_drivers/wheel_sx/indigo_wheel_sx.driver` and `indigo_drivers/wheel_atik/indigo_wheel_atik.driver` as compact generated wheel examples.
- Use `indigo_drivers/focuser_fcusb/indigo_focuser_fcusb.driver` as a generated libusb/hotplug example.
- Use `indigo_tools/indigo_generator.c` as the authoritative source for generator hot-plug behavior. Current generator behavior:
  - `libusb { ... }` and `hid { ... }` default to `hotplug = true`;
  - generated libusb hot-plug uses `MAX_DEVICES`, a `devices[]` array, `hotplug_mutex`, `indigo_start_usb_event_handler()` and `libusb_hotplug_register_callback()`;
  - generated libusb hot-plug calls `<driver_name>_match(libusb_device *dev, const char **name)`;
  - generated libusb private data includes `libusb_device *usbdev`;
  - generated connection handling calls `<driver_name>_open()` and `<driver_name>_close()`;
  - generated shutdown deregisters the libusb callback and detaches remaining hot-plug devices.
- Use `indigo_drivers/wheel_asi/README.md` for user-visible platform and hot-plug expectations.
- Use the ZWO EFW SDK header and bundled SDK documentation under `bin_externals/libEFWFilter/` only when current driver behavior is unclear.

## Current public behavior to preserve

- Driver entry point: `indigo_wheel_asi`.
- Driver name: `indigo_wheel_asi`.
- Driver label: `ZWO ASI Filter Wheel`.
- Supported hardware: all ZWO ASI EFW filter wheels reported by `EFWGetProductIDs()`.
- Supported runtime behavior:
  - libusb hot-plug discovery;
  - multiple simultaneously attached wheels, up to `MAX_DEVICES`;
  - stable device naming from SDK model name plus optional device suffix;
  - unique INDIGO device names when needed;
  - per-device connect/disconnect through the ZWO EFW SDK;
  - one logical INDIGO wheel device per physical EFW.
- Standard wheel behavior:
  - `WHEEL_SLOT`;
  - `WHEEL_SLOT_NAME`;
  - `WHEEL_SLOT_OFFSET`;
  - slot count from `EFW_INFO.slotNum`;
  - slot values exposed as one-based INDIGO positions while SDK positions are zero-based.
- Custom/non-standard properties:
  - `X_CALIBRATE` in group `Advanced`, switch, any-of-many, one `START` item;
  - `X_CUSTOM_SUFFIX` in `WHEEL_ADVANCED_GROUP`, text, one `SUFFIX` item, maximum effective length 8 characters.
- Informational behavior:
  - `INFO_PROPERTY->count = 6`;
  - SDK version is copied to `INFO_DEVICE_FW_REVISION_ITEM->text.value`;
  - `INFO_DEVICE_FW_REVISION_ITEM->label` is changed to `SDK version`;
  - SDK model name is copied to `INFO_DEVICE_MODEL_ITEM->text.value`.
- Concurrency behavior:
  - ZWO EFW enumeration/open/close paths are serialized with `indigo_device_enumeration_mutex`;
  - per-device SDK work is serialized by the generated handler queue;
  - the driver-global INDIGO lock is acquired on connect and released on disconnect.

## Target shape

- One generated source file: `indigo_wheel_asi.c`.
- One generator source file: `indigo_wheel_asi.driver`, which becomes the source of truth after migration.
- Public header and standalone main remain generated or generator-compatible according to repository convention.
- Hot-plug attach/detach is owned by the generator through an SDK-aware `sdk { hotplug = true; ... }` block.
- ASI-specific SDK discovery and naming logic is moved into `sdk.plug` / `sdk.unplug`; connection open/close remains in `asi_open()` / `asi_close()`.
- Private data stores only the fields needed by wheel behavior and ASI SDK state:
  - SDK device id;
  - SDK model/name cache;
  - custom suffix cache;
  - current slot and target slot;
  - slot count.
- Low-level ASI SDK access is isolated in a small helper section:
  - `<driver_name>_open(indigo_device *device)`;
  - `<driver_name>_close(indigo_device *device)`;
  - helpers for parsing SDK model/suffix data.
- INDIGO property change branches are small and generator-extractable:
  - inherited `CONNECTION` behavior handled by generator open/close where practical;
  - inherited `WHEEL_SLOT` owns ASI slot conversion, `EFWSetPosition()` and movement finalizer scheduling;
  - custom `X_CALIBRATE` owns `EFWCalibrate()` and calibration finalizer scheduling;
  - custom `X_CUSTOM_SUFFIX` owns suffix validation and `EFWSetID()`.
- Slow completion waits use finalizers; short SDK start operations run in the generated property handlers.
- Annotation blocks may be used during the intermediate hand-written phase:
  - `//+ include`;
  - `//+ define`;
  - `//+ data`;
  - `//+ code`;
  - `//+ on_init`;
  - `//+ on_shutdown`;
  - `//+ wheel.code`;
  - `//+ wheel.on_attach`;
  - `//+ wheel.on_connect`;
  - `//+ wheel.on_disconnect`;
  - `//+ wheel.on_detach`;
  - `//+ wheel.WHEEL_SLOT.on_change`;
  - `//+ wheel.X_CALIBRATE.on_change`;
  - `//+ wheel.X_CUSTOM_SUFFIX.on_change`.

## Step-by-step plan

1. Establish the baseline
   - Record current build status of `wheel_asi`.
   - Confirm whether the driver can be built locally with the bundled SDK for the current platform.
   - Record any already-dirty workspace files before editing.
   - Save the current property list and custom behavior from `indigo_wheel_asi.c`.
   - Document that hardware-backed validation requires a real ASI EFW unless a simulator or SDK mock is added separately.

   Result:
   - Baseline was recorded on Darwin, with bundled EFW SDK artifacts present under `bin_externals/libEFWFilter/`.
   - `build/drivers/indigo_wheel_asi.a` was already present before behavioral edits.
   - Pre-existing workspace state was noted: `AGENTS.md`, this new `REFACTOR.md`, and an already-modified Xcode project file.
   - Captured the original driver entry point, label, version, manual hot-plug design, multi-device state, one-based INDIGO slot values and zero-based SDK slot calls.
   - Captured custom properties `X_CALIBRATE` and `X_CUSTOM_SUFFIX`, the `INFO_PROPERTY->count = 6` mutation, slot-count mutations and the absence of explicit `hidden` mutations.
   - Hardware validation was explicitly deferred until a real ASI EFW was available.

2. Extract property and lifecycle inventory
   - List every `indigo_init_*_property()` and `indigo_init_*_item()` call.
   - List every property `count` and `hidden` mutation.
   - Record connect and disconnect side effects:
     - device index lookup through `EFWGetNum()` / `EFWGetID()`;
     - `indigo_try_global_lock()`;
     - `EFWOpen()`;
     - `EFWGetProperty()`;
     - `EFWGetPosition()`;
     - custom property definition;
     - polling timer start;
     - custom property deletion;
     - `EFWClose()`;
     - `indigo_global_unlock()`.
   - Compare the property inventory with `indigo_docs/PROPERTIES.md` and update documentation only if public properties or item counts change.

   Result:
   - Confirmed standard wheel properties come from the wheel base class: `WHEEL_SLOT`, `WHEEL_SLOT_NAME` and `WHEEL_SLOT_OFFSET`.
   - Documented connected-only custom properties:
     - `X_CALIBRATE`, switch property in `Advanced`;
     - `X_CUSTOM_SUFFIX`, text property in `WHEEL_ADVANCED_GROUP`.
   - Recorded private data fields for SDK identity, model/suffix cache, slot state, slot count, timer, per-device USB lock and custom property storage.
   - Recorded original connect, disconnect, slot movement, calibration, suffix update and hot-plug lifecycles.
   - No `indigo_docs/PROPERTIES.md` update was needed because public property behavior did not change.

3. Reshape the hand-written driver into generator-friendly sections
   - Keep behavior unchanged in this phase.
   - Reorder the file into clear sections:
     - includes;
     - defines and property macros;
     - private data;
     - temporary legacy hot-plug state;
     - low-level ASI SDK helpers;
     - wheel callbacks and property handlers;
     - temporary legacy hot-plug attach/detach support;
     - driver entry point.
   - Move SDK helper code out of INDIGO callbacks where it can be named and reused without changing behavior.
   - Keep the license header and version history intact.
   - Avoid broad formatting churn while moving code.

   Result:
   - Reordered `indigo_wheel_asi.c` into explicit sections for definitions, includes, private data, temporary hot-plug state, ASI SDK helpers, wheel implementation and legacy hot-plug support.
   - Moved SDK identity helpers ahead of INDIGO callbacks without intended behavior changes.
   - Kept the legacy plug/unplug processing isolated for later replacement by generated hot-plug code.
   - Built `wheel_asi` successfully; only the existing bundled `libEFWFilter.a` macOS deployment-target warnings remained.

4. Introduce generator-compatible match/open/close helpers
   - Add `libusb { hotplug = true; vid = ASI_VENDOR_ID; }` to the target `.driver` shape.
   - Add `asi_match(libusb_device *dev, const char **name)` in the shared code block.
   - Make `asi_match()` filter by ZWO vendor id and the SDK product id list from `EFWGetProductIDs()`.
   - Make `asi_match()` establish the display-name seed currently produced by `process_plug_event()`, including SDK model name and custom suffix handling where possible.
   - Decide whether `asi_match()` can safely open/query/close the SDK device during hot-plug matching; if not, keep only lightweight libusb filtering in `asi_match()` and move SDK id/model resolution into `asi_open()`.
   - Add `asi_open(indigo_device *device)` and `asi_close(indigo_device *device)` or the exact helper names expected by the selected generator definition.
   - Make the open helper contain the connection branch currently embedded in `wheel_connection_handler()`.
   - Make the close helper contain SDK close, property cleanup prerequisites and global unlock behavior currently embedded in `wheel_connection_handler()`.
   - Preserve `indigo_device_enumeration_mutex` coverage around all SDK enumeration/open/close sequences.
   - Preserve `PRIVATE_DATA->usb_mutex` coverage around per-device SDK calls.
   - Verify failed opens restore the connection switch to disconnected and do not leak the global lock.

   Result:
   - Added interim generator-shaped `asi_match()`, `asi_open()` and `asi_close()` helpers.
   - Routed the handwritten connection handler through `asi_open()` and `asi_close()` while leaving the rest of the successful-connect initialization in place.
   - Reused `asi_match()` in the legacy hot-plug callback for vendor/product filtering.
   - Preserved enumeration/global/per-device locking in that intermediate handwritten shape.
   - Built `wheel_asi` successfully; only the known bundled-SDK macOS linker warnings remained.

5. Convert callback work into queue-friendly handlers
   - Replace zero-delay timers used as worker dispatch with `indigo_execute_handler()` where generated driver patterns expect handlers.
   - Keep delayed polling/finalizers with `indigo_execute_handler_in()` or timers only where generator examples use them for wheel motion polling.
   - Ensure slot movement reports the current one-based slot, keeps `WHEEL_SLOT` busy while moving and returns to OK only when the target slot is reached.
   - Ensure calibration keeps both `X_CALIBRATE` and `WHEEL_SLOT` busy while the SDK reports moving.
   - Ensure suffix update validates the 8-character SDK limit before calling `EFWSetID()`.

   Result:
   - Replaced zero-delay worker timers with generator-style handler queue dispatch.
   - Renamed worker functions to generator-aligned names ending in `_handler`; delayed completion polling uses `_finalizer`.
   - Represented long-running movement and calibration as handler + finalizer, with `indigo_execute_handler_in()` rescheduling the delayed poll.
   - Removed the blocking sleep loop from legacy plug processing and converted SDK-moving retry to a delayed retry.
   - Preserved slot validation, calibration busy guards and suffix length validation.
   - Built `wheel_asi` successfully; only the known bundled-SDK macOS linker warnings remained.

6. Add migration annotations
   - Mark additional includes and custom defines with `//+ include` and `//+ define`.
   - Mark private fields with `//+ data`.
   - Mark shared SDK helpers, including `asi_match()`, `asi_open()` and `asi_close()`, with `//+ code`.
   - Do not annotate or preserve manual libusb callback registration in the final `.driver`; the generator emits that from the `libusb` block.
   - Mark wheel-specific helpers with `//+ wheel.code`.
   - Mark `INFO_PROPERTY` and custom property setup with `//+ wheel.on_attach`.
   - Mark connection initialization with `//+ wheel.on_connect`.
   - Mark disconnect cleanup with `//+ wheel.on_disconnect`.
   - Mark inherited `WHEEL_SLOT` behavior and custom property behavior with property `on_change` annotations.

   Result:
   - Added extraction annotations for include, define, data, shared code, wheel code, attach, connect, disconnect and property `on_change` blocks.
   - Annotated SDK startup work in `on_init`.
   - Intentionally left manual hot-plug callback registration, legacy plug/unplug timers, `devices[]` ownership and shutdown cleanup outside the final generated-driver blocks.
   - Checked annotation pairs and built `wheel_asi` successfully; only the known bundled-SDK macOS linker warnings remained.

7. Generate the initial `.driver`
   - Run `indigo_generator -c indigo_wheel_asi.driver` from `indigo_drivers/wheel_asi`.
   - Do not pass the `.c` file name to `-c`; extraction writes the requested `.driver` target and a `.c` target name can overwrite the source file.
   - Inspect the extracted `indigo_wheel_asi.driver` by hand.
   - Fix any extraction gaps in the annotated C source or directly in the `.driver`, according to whichever path produces the least churn.
   - Confirm the `.driver` contains:
     - driver metadata;
     - `libusb { hotplug = true; vid = ASI_VENDOR_ID; }` configuration;
     - custom includes for `EFW_filter.h` and any other SDK headers not emitted by the generator;
     - ASI constants and property names;
     - private data fields;
     - `asi_match()`, `asi_open()` and `asi_close()`;
     - wheel device block;
     - inherited `WHEEL_SLOT`;
     - custom `X_CALIBRATE`;
     - custom `X_CUSTOM_SUFFIX`;
     - any ASI SDK product-id initialization still needed by `asi_match()`.

   Result:
   - Added `indigo_wheel_asi.driver` as the first generator source definition.
   - Kept driver metadata, `libusb { hotplug = true; vid = ASI_VENDOR_ID; }`, the EFW SDK include, ASI constants, private wheel fields, `asi_match()`, `asi_open()` and `asi_close()`.
   - Converted extracted custom properties from inherited placeholders to generated custom property declarations:
     - `switch X_CALIBRATE`;
     - `text X_CUSTOM_SUFFIX`.
   - Adjusted extracted property-change early exits from `return INDIGO_OK;` to plain `return;`, because generator property handlers are `void`.
   - Renamed custom worker handlers that would collide with generated property handler names:
     - SDK slot movement worker: `wheel_slot_move_handler`;
     - SDK suffix update worker: `wheel_custom_suffix_handler`.
   - Verified the `.driver` parses by generating temporary output in `/private/tmp/wheel_asi_driver_check`; no repository C/H/main files were regenerated in this step.
   - Open issue before Step 8: generated libusb hot-plug stores only `PRIVATE_DATA->usbdev`, while the ASI SDK operates on SDK ids and model/suffix data. `asi_open()` or a generator SDK-hotplug extension still needs to map the matched `libusb_device *` to the correct SDK id before generated output can preserve multi-wheel identity semantics.

8. Regenerate source from `.driver`
   - Run `indigo_generator indigo_wheel_asi.driver`.
   - Treat the generated `.c`, `.h` and `_main.c` as generated output from this point forward.
   - Build the driver and address generator or compiler errors in the `.driver`, not by hand-editing generated C.
   - Inspect the generated diff and verify every behavior change is intentional.

   Result:
   - Regenerated repository files from `indigo_wheel_asi.driver`:
     - `indigo_wheel_asi.c`;
     - `indigo_wheel_asi.h`;
     - `indigo_wheel_asi_main.c`.
   - Added `#include <stdbool.h>` to the `.driver` include block because the EFW SDK header uses `bool` before the generated source includes INDIGO headers.
   - Built `wheel_asi` with `make -C indigo_drivers/wheel_asi -f ../../Makefile.drv`; the driver, shared library and standalone executable all compile/link.
   - The only build output warnings are the existing macOS deployment-target warnings from the bundled `libEFWFilter.a`.
   - Generated source now has the expected generator marker, generated hot-plug framework, generated custom property allocation and generated public header/main.
   - Open issue carried into Step 9: the generated attach path names devices from `asi_match()` and stores `PRIVATE_DATA->usbdev`, but the ASI SDK connection path still needs robust SDK id/model/suffix mapping before behavior can be considered equivalent to the old handwritten hot-plug code.

9. Reconcile ASI identity semantics with generator hot-plug
   - Let the generator own libusb callback registration, plug attach, unplug detach, `devices[]` storage and shutdown cleanup.
   - Remove handwritten `process_plug_event()`, `process_unplug_event()`, `hotplug_callback()`, `remove_all_devices()`, `devices[]` and `callback_handle` after equivalent generated behavior is confirmed.
   - Keep only ASI-specific product filtering, SDK id lookup and display-name suffix logic in helper functions.
   - Re-evaluate whether `connected_ids[]` is still required once generated hot-plug tracks devices by `libusb_device *`.
   - If SDK ids are still needed because `EFWOpen()` works by SDK id rather than `libusb_device *`, map `PRIVATE_DATA->usbdev` to the correct SDK id in `asi_open()` under the SDK enumeration mutex.
   - Preserve delayed SDK query behavior only if hardware testing or existing SDK notes show that immediate `EFWGetProperty()` after libusb arrival is unreliable.

   Result:
   - Kept generator-owned libusb hot-plug as the only attach/detach framework in the generated C output.
   - Added `NO_DEVICE` and initialized `PRIVATE_DATA->dev_id` to it during wheel attach.
   - Reworked `asi_open()` so the generated connection handler still calls the generator-expected helper, but ASI-specific SDK identity is assigned inside that helper:
     - serialize with `indigo_device_enumeration_mutex`;
     - take the INDIGO global lock;
     - scan `EFWGetNum()` / `EFWGetID()`;
     - skip ids already marked in `connected_ids[]`;
     - open the selected SDK id;
     - read `EFWGetProperty()`;
     - cache model and custom suffix in `PRIVATE_DATA`.
   - Reworked `asi_close()` to close the SDK id, clear `connected_ids[id]`, reset `PRIVATE_DATA->dev_id` to `NO_DEVICE` and release the INDIGO global lock.
   - Added `split_device_name()` to preserve the old model/suffix split from the SDK name.
   - Updated `on_connect` to refresh `INFO_DEVICE_MODEL_ITEM` and `X_CUSTOM_SUFFIX_ITEM` after `asi_open()` has populated SDK identity data.
   - Regenerated `indigo_wheel_asi.c`, `indigo_wheel_asi.h` and `indigo_wheel_asi_main.c` from the `.driver`.
   - Built `wheel_asi` successfully with `make -C indigo_drivers/wheel_asi -f ../../Makefile.drv`; only the known bundled-SDK macOS deployment-target warnings remain.
   - Residual behavior gap: device names at generated hot-plug attach time are still seeded by `asi_match()` as `ASI EFW`, because current generator hooks do not let `asi_match()` pass SDK id/model/suffix into the freshly allocated `PRIVATE_DATA`. Full old naming equivalence probably needs the SDK-hotplug generator extension considered in Step 10.

10. Evaluate a generator extension for SDK-based hot-plug
   - Consider extending `indigo_generator` before migrating `wheel_asi` if the ASI id mapping cannot be expressed cleanly with the current `libusb` match/open hooks.
   - Keep the extension generic, not ASI-specific.
   - Candidate shape:
     - add an optional SDK-style hot-plug mode to the `libusb` block;
     - allow generated plug/unplug handlers to call driver-provided no-argument SDK scan helpers instead of attaching exactly the `libusb_device *` passed by libusb;
     - support a configurable plug delay for SDKs that need the USB stack to settle before enumeration;
     - let helper code return one or more discovered SDK device records with stable id, display name and optional suffix;
     - generate common `connected_ids[]` / device-slot attach-detach bookkeeping for SDK ids;
     - still let normal libusb/HID drivers use the current direct `libusb_device *` path unchanged.
   - Candidate syntax to prototype after reviewing generator constraints:
     - `libusb { hotplug = sdk; vid = ASI_VENDOR_ID; delay = 0.5; }`, or
     - `libusb { hotplug = true; sdk_enumeration = true; delay = 0.5; }`.
   - Use ASI wheel as the first consumer only if the generated code also looks reusable for ASI focuser, ASI rotator, ASI guider, ASI CCD, Player One and SVBONY-style SDK drivers.
   - Defer the generator extension if `asi_match()` plus `asi_open()` can preserve behavior with small helper code and without duplicating the old manual hot-plug framework.

   Result:
   - Added a generic `sdk { hotplug = true; vid = ...; pid = ...; plug { ... } unplug { ... } }` transport block to `indigo_generator`.
   - The generated SDK hot-plug path still uses libusb hot-plug registration and generated `MAX_DEVICES` / `devices[]` bookkeeping, but delegates SDK-specific identity work to inline blocks:
     - `plug` runs with `libusb_device *dev`, `<driver>_private_data *private_data`, `char name[INDIGO_NAME_SIZE]` and `bool plug_result`;
     - set `plug_result = false` to reject a USB event after SDK inspection;
     - fill `private_data` and `name` before generated attach;
     - `unplug` runs with `indigo_device *device`, `<driver>_private_data *private_data` and `libusb_device *dev` before generated detach/free.
   - Existing `libusb` and `hid` generated hot-plug paths remain unchanged.
   - Updated `AGENTS.md` and `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` with the new SDK-hotplug contract.
   - Built both `indigo_tools/indigo_generator` and `build/bin/indigo_generator`.
   - Verified the new syntax with a temporary `.driver` file in `/private/tmp/sdk_generator_check`; generated C contains the expected `sdk.plug` / `sdk.unplug` blocks and libusb callback registration.
   - Migrated `indigo_wheel_asi.driver` from `libusb { hotplug = true; ... }` to the new `sdk { hotplug = true; ... }` transport.
   - Moved ASI SDK identity assignment from `asi_open()` into `sdk.plug`:
     - filter USB events by VID and SDK product-id list;
     - scan `EFWGetNum()` / `EFWGetID()`;
     - skip SDK ids already reserved in `connected_ids[]`;
     - briefly `EFWOpen()` the candidate id, read `EFWGetProperty()`, then `EFWClose()`;
     - cache `dev_id`, model and custom suffix in `private_data`;
     - seed the generated device name from the SDK `info.Name`.
   - Moved SDK id reservation release into `sdk.unplug`.
   - Simplified `asi_open()` / `asi_close()` back to connection-only SDK open/close for the already assigned `PRIVATE_DATA->dev_id`; disconnect no longer clears the hot-plug reservation.
   - Regenerated `indigo_wheel_asi.c`, `indigo_wheel_asi.h` and `indigo_wheel_asi_main.c` from the `.driver`.
   - Built `wheel_asi` successfully with `make -C indigo_drivers/wheel_asi -f ../../Makefile.drv`; only the known bundled-SDK macOS deployment-target warnings remain.
   - The previous generated-name gap is resolved in shape: generated attach now receives the SDK-provided `name`. Hardware validation is still needed for SDK timing immediately after USB arrival, especially the `EFW_ERROR_MOVING` case documented by the SDK.

11. Validation
   - Build `build/drivers/indigo_wheel_asi.a`.
   - Build the standalone `indigo_wheel_asi` target if available.
   - Load the driver in `indigo_server indigo_wheel_asi` on a host with the SDK library available.
   - With hardware, test:
     - initial hot-plug enumeration;
     - plug while server is running;
     - unplug while disconnected;
     - refused unplug while connected;
     - connect/disconnect cycles;
     - slot count detection;
     - slot movement in both directions;
     - calibration success and failure paths;
     - suffix set and suffix clear;
     - multiple wheels with unique names.
   - Without hardware, at minimum validate build output and inspect generated source for preserved property definitions and SDK call ordering.

   Result:
   - Re-ran the narrow `wheel_asi` build after migrating to `sdk`; `Makefile.drv` reports the generated sources, archive, shared library and standalone executable are up to date.
   - Verified generated `wheel_asi` source contains the new `sdk.plug` / `sdk.unplug` blocks and no longer depends on the `libusb`-specific `asi_match()` hook.
   - Verified the SDK identity flow in generated C:
     - `sdk.plug` reserves an SDK id, fills `private_data->dev_id`, model and custom suffix, and seeds the generated device name from SDK `info.Name`;
     - `asi_open()` opens the already reserved SDK id during INDIGO connect;
     - `asi_close()` closes the SDK id during INDIGO disconnect;
     - `sdk.unplug` releases the reservation when the physical USB device is detached.
   - Generated compatibility samples for an existing `libusb` driver and an existing `hid` driver in `/private/tmp/generator_compat_check`; both kept their original hot-plug shapes, with no accidental `sdk` path.
   - Hardware validation remains deferred: the current SDK plug block still depends on `EFWGetProperty()` succeeding shortly after USB arrival, so a real ASI EFW test should cover fresh plug-in and any `EFW_ERROR_MOVING` timing behavior.

   Hardware follow-up:
   - First real-hardware connect showed hot-plug discovery succeeded through `EFWGetNum()`, `EFWGetID()`, `EFWOpen()` and `EFWGetProperty()`, and the generated device attached with the SDK name.
   - The connect attempt then crashed in a later queue allocation because `split_device_name()` used `INDIGO_COPY_VALUE()` to copy into `PRIVATE_DATA->model[64]`; that macro writes `INDIGO_VALUE_SIZE` bytes and corrupted the heap.
   - Fixed `split_device_name()` to copy the SDK name with `snprintf(model, 64, "%s", name)`.
   - Also replaced suffix cache `memcpy()` with bounded zero-fill plus `strncpy()`.
   - Regenerated `indigo_wheel_asi.c` from `.driver` and rebuilt successfully.
   - Second real-hardware connect log showed `sdk.plug` correctly discovered SDK id `0`, but `on_attach` reset `PRIVATE_DATA->dev_id` back to `NO_DEVICE`, so `asi_open()` attempted `EFWOpen(-1)`.
   - Removed the `PRIVATE_DATA->dev_id = NO_DEVICE` initialization from `on_attach`; `sdk.plug` owns that initialization before SDK scan.
   - Regenerated and rebuilt successfully; generated `on_attach` no longer overwrites the SDK id assigned by `sdk.plug`.
   - Real-hardware suffix update returned `EFWSetID(0, "aa") = 8`, which is `EFW_ERROR_NOT_SUPPORTED` according to the SDK header.
   - Updated `wheel_custom_suffix_handler()` so failed `EFWSetID()` calls do not update `PRIVATE_DATA->custom_suffix`, restore the property text to the previous cached suffix, and report a specific message when firmware does not support custom suffix aliases.
   - Changed successful `EFWSetID()` logging from error to debug.
   - Regenerated and rebuilt successfully.
   - Removed the per-device `usb_mutex`; SDK work for an attached wheel is serialized by the generated handler queue, while SDK enumeration/open/close and hot-plug id reservation remain protected by `indigo_device_enumeration_mutex`.
   - Regenerated and rebuilt successfully; `usb_mutex` no longer appears in the `.driver` or generated C source.
   - Moved `#include <stdbool.h>` ownership into `indigo_generator`; generated C now includes it before custom driver include blocks, so SDK headers can use `bool` without every `.driver` repeating the include.
   - Removed the explicit `stdbool.h` include from `indigo_wheel_asi.driver`, regenerated and rebuilt successfully.
   - Removed the `wheel_move_finalizer` scheduling from `on_connect`; connect now reads the current SDK position, converts it to one-based INDIGO value, and initializes both `WHEEL_SLOT` value and target without starting a movement poller.
   - Regenerated and rebuilt successfully; `wheel_move_finalizer` is now scheduled only after actual slot movement starts or while it is polling its own delayed completion.
   - Removed the extra start-only worker handlers:
     - `wheel_slot_move_handler`;
     - `wheel_calibrate_handler`;
     - `wheel_custom_suffix_handler`.
   - Moved `EFWSetPosition()` directly into the generated `wheel_slot_handler` on-change block and kept `wheel_move_finalizer` only for delayed movement completion polling.
   - Moved `EFWCalibrate()` directly into the generated `wheel_x_calibrate_handler` on-change block and kept `wheel_calibrate_finalizer` only for delayed calibration completion polling.
   - Moved `EFWSetID()` directly into the generated `wheel_x_custom_suffix_handler` on-change block.
   - Regenerated and rebuilt successfully; helper naming is now aligned with generated property handlers plus `_finalizer` functions for long-running completion.

12. Cleanup
   - Keep only the `.driver` plus generated `.c`, `.h` and `_main.c` as source files for the driver.
   - Keep SDK externals, project files and README untouched unless the migration requires a build-system or user-visible update.
   - Remove stale handwritten-only helper declarations after generation.
   - Do not edit vendored SDK files or binary libraries.
   - Update `README.md` only if user-visible behavior changes.
   - Update `indigo_docs/PROPERTIES.md` only if properties, item counts, labels, visibility or persistence change.

   Result:
   - Updated the final target description in this plan from the earlier `libusb`/`asi_match()` migration shape to the actual final `sdk` hot-plug shape.
   - Confirmed `indigo_wheel_asi.driver` is now the source of truth and generated `indigo_wheel_asi.c` is marked as generated from it.
   - Confirmed the driver source no longer contains `usb_mutex`, `asi_match()`, `wheel_slot_move_handler()`, `wheel_calibrate_handler()` or `wheel_custom_suffix_handler()`.
   - Normalized this document so every step records its outcome in an inline `Result:` block instead of mixing inline results with separate historical result chapters.
   - No `README.md` or `indigo_docs/PROPERTIES.md` update is required: public properties and user-visible feature set are unchanged, aside from more explicit alert messaging when firmware does not support custom suffix aliases.
   - Build validation remains successful with `make -C indigo_drivers/wheel_asi -f ../../Makefile.drv`; only known bundled-SDK macOS linker warnings remain.

## Suggested milestones

1. Baseline build and behavior inventory recorded.
2. Hand-written `indigo_wheel_asi.c` reorganized into generator-friendly sections with no intended behavior change.
3. Open/close and SDK helpers isolated behind generator-compatible function names.
4. Property handlers converted into compact generator-extractable blocks.
5. Migration annotations added and reviewed.
6. `indigo_wheel_asi.driver` extracted or written.
7. `.c`, `.h` and `_main.c` regenerated from `.driver`.
8. Driver builds from generated output.
9. Hardware validation pass completed or documented as deferred.

## Decisions

- Use generator-native `sdk` hot-plug for the migrated driver.
- Keep ASI-specific SDK product filtering, SDK id mapping and custom suffix naming in the `.driver` `sdk.plug` / `sdk.unplug` blocks and small shared helpers.
- Preserve `X_CALIBRATE` and `X_CUSTOM_SUFFIX`.
- Preserve one-based INDIGO slot values and zero-based SDK calls.
- Preserve mutex coverage around SDK global enumeration/open/close and hot-plug id reservation paths.
- Prefer generator ownership of wheel attach/change/detach boilerplate after migration.
