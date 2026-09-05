# Refactoring plan for INDIGO 3.0 Meade DSI CCD driver

Goal: refactor the Meade DSI CCD driver into a generator-friendly INDIGO 3.0 structure and migrate it to `indigo_generator`. Preserve the current SDK/libusb hot-plug behavior, multiple-device support, DSI firmware handling, image acquisition, temperature polling, gain/offset controls and public CCD behavior while moving ordinary CCD boilerplate into generated code.

## Reference material

- Use the current `indigo_ccd_dsi.c` as the behavioral reference and initial source to annotate.
- Use `indigo_drivers/wheel_asi/REFACTOR.md` and `indigo_drivers/wheel_asi/indigo_wheel_asi.driver` as the closest migration pattern, because both drivers are SDK-based USB hot-plug drivers whose vendor library has its own device identity model.
- Use `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` for generator extraction rules, `sdk { hotplug = true; ... }`, open/close helper naming, annotation blocks and `.driver` ownership.
- Use `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` for INDIGO 3.0 device lifecycle, property state handling and async handler queues.
- Use `indigo_docs/DEVELOPMENT.md` for bus/device/property lifecycle details when validating generated callback behavior.
- Use `TESTING.md` for known historical DSI validation notes, especially the macOS note that opening the camera during hot-plug processing can reset the device and cause duplicate plug/unplug events.
- Use `indigo_drivers/ccd_dsi/README.md` for supported hardware, platform notes and user-visible expectations.
- Use `indigo_tools/indigo_generator.c` as the authoritative source for generator hot-plug behavior and supported `.driver` attributes.
- Use `indigo_drivers/ccd_simulator/indigo_ccd_simulator.c` and existing generated `.driver` files only as secondary references for CCD property shape and generator idioms.

## Current public behavior to preserve

- Driver entry point: `indigo_ccd_dsi`.
- Driver name: `indigo_ccd_dsi`.
- Driver label: `Meade DSI Camera`.
- Supported hardware: Meade DSI Pro/Color, DSI Pro/Color II and DSI Pro/Color III cameras.
- Supported runtime behavior:
  - libusb hot-plug discovery by Meade DSI vendor id `0x156c`;
  - firmware load on macOS through `dsi_load_firmware()`;
  - multiple simultaneously attached cameras, up to the current `MAX_DEVICES`;
  - stable per-camera identity from DSI SID strings;
  - unique INDIGO device names using the SID as suffix when needed;
  - one logical INDIGO CCD device per physical DSI camera.
- Standard CCD behavior:
  - connection and disconnection through `dsi_open_camera()` / `dsi_close_camera()`;
  - exposure start, polling and image download;
  - exposure abort and camera reset;
  - RAW 16-bit image processing with optional Bayer pattern keyword;
  - frame size, pixel size and bits-per-pixel setup from the SDK;
  - optional 1x1 / 2x2 binning depending on `dsi_get_max_binning()`;
  - DSI-specific constraint that horizontal and vertical binning remain equal;
  - `CCD_MODE` count and labels reflecting supported bin modes;
  - `CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM`;
  - `CCD_BIN`, `CCD_TEMPERATURE`, `CCD_GAIN` and `CCD_OFFSET` visibility and permissions based on device capability.
- Informational behavior:
  - `INFO_PROPERTY->count = 8`;
  - serial number from `dsi_get_serial_number()`;
  - model name from `dsi_get_model_name()`;
  - CCD dimensions and pixel size from SDK calls.
- Concurrency behavior:
  - DSI enumeration/open/close paths are serialized with `indigo_device_enumeration_mutex`;
  - per-device SDK calls are currently guarded by `PRIVATE_DATA->usb_mutex`;
  - global INDIGO driver lock is acquired on connect and released on disconnect.

## Target shape

- One generator source file: `indigo_ccd_dsi.driver`, which becomes the source of truth after migration.
- Generated repository outputs: `indigo_ccd_dsi.c`, `indigo_ccd_dsi.h` and `indigo_ccd_dsi_main.c`.
- Hot-plug attach/detach is owned by the generator through an SDK-aware `sdk { hotplug = true; vid = DSI_VENDOR_ID; ... }` block.
- DSI-specific USB/SID discovery is moved into `sdk.plug` / `sdk.unplug`; connection open/close remains in `dsi_open(indigo_device *device)` / `dsi_close(indigo_device *device)`.
- Private data stores only fields needed by CCD behavior and DSI SDK state:
  - DSI SID string;
  - `dsi_camera_t *`;
  - current exposure bin mode;
  - exposure and temperature timers or finalizer state required by generated handlers;
  - image buffer pointer and size;
  - temperature-polling flag;
  - per-device mutex only if queue serialization and SDK behavior still require it after review.
- Low-level DSI SDK access is isolated in small helpers:
  - `dsi_open(indigo_device *device)`;
  - `dsi_close(indigo_device *device)`;
  - exposure start/read/abort helpers;
  - SID scan and name helpers usable from `sdk.plug`.
- Property change branches are generator-extractable:
  - `CCD_EXPOSURE` starts exposure and schedules download finalizer/poller;
  - `CCD_ABORT_EXPOSURE` aborts, resets hardware and publishes final property states;
  - `CCD_GAIN` applies `dsi_set_amp_gain()`;
  - `CCD_OFFSET` applies `dsi_set_amp_offset()`;
  - `CCD_BIN` keeps X/Y values equal in a synchronous generated handler and explicitly calls the CCD base handler with the adjusted `CCD_BIN_PROPERTY`.
- Slow or blocking work runs on generated handler queues or delayed finalizers, not directly on the bus callback.
- Manual hot-plug scaffolding is removed after generated `sdk` hot-plug equivalence is confirmed.

## Step-by-step plan

1. Check formatting and baseline hygiene
   - Record current workspace status before touching the driver.
   - Run a formatting-only inspection on the driver files `indigo_ccd_dsi.c`, `indigo_ccd_dsi.h` and `indigo_ccd_dsi_main.c`.
   - Identify whitespace, indentation or brace issues that would make later refactor diffs noisy.
   - Fix only formatting issues that are directly needed before refactoring; do not mix formatting churn with behavioral changes.
   - Confirm `.editorconfig` and local C style expectations: tabs, K&R braces, braces on all control bodies, no broad generated/vendor churn.

   Result:
   - Baseline workspace status was recorded before formatting cleanup.
   - Confirmed `.editorconfig` expectations: UTF-8, LF, tabs, indent size 2 and trimmed trailing whitespace.
   - Confirmed the inspected driver files are ASCII text:
     - `indigo_ccd_dsi.c`;
     - `indigo_ccd_dsi.h`;
     - `indigo_ccd_dsi_main.c`.
   - Fixed driver-local formatting issues found by the inspection: trailing whitespace, one obvious macOS firmware-load indentation issue and active one-line `if` bodies.
   - `libdsi.c` and `libdsi.h` are intentionally outside the refactor scope and must not be reformatted or otherwise edited during this driver migration.
   - Left the commented-out legacy `find_device_slot()` body unchanged because it is inactive code and will be revisited during hot-plug cleanup.
   - `uncrustify` is not available in the current environment, so validation used repository style rules plus targeted text checks.
   - Rebuilt `ccd_dsi` with `make -C indigo_drivers/ccd_dsi -f ../../Makefile.drv`; archive, shared library and standalone executable were produced successfully.

2. Establish the behavioral baseline
   - Record current build status for `ccd_dsi`.
   - Confirm whether the driver builds locally on the current platform.
   - Record existing hardware assumptions: real DSI hardware is required for hot-plug, firmware, exposure, temperature and image validation.
   - Capture the current driver entry point, label, version, manual hot-plug design and supported device list.
   - Document known historical constraints from `TESTING.md`, especially macOS hot-plug instability if the camera is opened during plug processing.

   Result:
   - Current workspace status was checked before baseline capture.
   - Narrow `ccd_dsi` build was checked with `make -C indigo_drivers/ccd_dsi -f ../../Makefile.drv`; the build definition lists `indigo_ccd_dsi.c`, `indigo_ccd_dsi_main.c` and `libdsi.c`, and archive, shared library and standalone executable targets are present.
   - Captured current driver identity:
     - entry point `indigo_ccd_dsi`;
     - `DRIVER_NAME` is `indigo_ccd_dsi`;
     - `DRIVER_VERSION` is `0x0300000E`;
     - driver label is `Meade DSI Camera`.
   - Captured current manual hot-plug design:
     - libusb callback registered for vendor id `DSI_VENDOR_ID` / `0x156c`;
     - `MAX_DEVICES` is 32;
     - `devices[]` stores attached logical devices;
     - plug/unplug events are delayed by one second through timer callbacks;
     - `dsi_scan_usb()` discovers SIDs and `indigo_make_name_unique()` uses the SID for unique naming;
     - macOS plug handling calls `dsi_load_firmware()` and avoids model-name open/query during plug processing.
   - Captured supported hardware from the driver README: Meade DSI Pro/Color, DSI Pro/Color II and DSI Pro/Color III.
   - Recorded hardware validation assumptions: real DSI hardware is required for firmware-load, hot-plug, connect/disconnect, exposure, abort, temperature, gain, offset, binning and image-download validation.
   - Recorded historical validation notes from `TESTING.md`:
     - macOS DSI 1 Colour passed with a code change because the camera cannot be opened during hot-plug processing;
     - Linux Meade DSI Pro II passed after a hot-plug crash fix.

3. Extract property and lifecycle inventory
   - List every `indigo_init_*_property()` and `indigo_init_*_item()` call relevant to this driver.
   - List every property `count`, `hidden`, `perm`, min/max/step and value mutation.
   - Record connect side effects:
     - `indigo_try_global_lock()`;
     - `dsi_open_camera(PRIVATE_DATA->dev_sid)`;
     - image buffer allocation;
     - CCD info/frame/mode/binning setup;
     - serial/model info updates;
     - gain/offset setup;
     - temperature sensor detection and polling start.
   - Record disconnect side effects:
     - disable temperature checks;
     - cancel temperature timer;
     - close camera;
     - release global lock;
     - free image buffer if ownership stays in close.
   - Compare the inventory with `indigo_docs/PROPERTIES.md` and update docs only if public property behavior changes.

   Result:
   - Confirmed the driver itself does not allocate custom INDIGO properties with `indigo_init_*_property()`; it relies on the CCD base class properties created by `indigo_ccd_attach()`.
   - Confirmed the only driver-local `indigo_init_*_item()` calls rebuild `CCD_MODE` items during connect:
     - `BIN_1x1` always;
     - `BIN_2x2` only when `dsi_get_max_binning()` reports binning support.
   - Recorded attach-time mutation:
     - `INFO_PROPERTY->count = 8`.
   - Recorded connect-time info/frame mutations:
     - `CCD_INFO_WIDTH`, `CCD_INFO_HEIGHT`;
     - `CCD_FRAME_WIDTH`, `CCD_FRAME_HEIGHT`, `CCD_FRAME_LEFT.max`, `CCD_FRAME_TOP.max`;
     - `INFO_DEVICE_SERIAL_NUM`, `INFO_DEVICE_MODEL`;
     - `CCD_INFO_PIXEL_WIDTH`, `CCD_INFO_PIXEL_HEIGHT`, `CCD_INFO_PIXEL_SIZE`;
     - `CCD_INFO_MAX_HORIZONAL_BIN`, `CCD_INFO_MAX_VERTICAL_BIN`;
     - `CCD_INFO_BITS_PER_PIXEL`;
     - `CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM`;
     - `CCD_FRAME_BITS_PER_PIXEL` value/min/max all set to `DEFAULT_BPP`.
   - Recorded connect-time binning and mode mutations:
     - if binning is supported, `CCD_BIN` is visible and writable, `CCD_MODE->count = 2`, and mode labels are based on full and half frame dimensions;
     - if binning is not supported, `CCD_BIN` is hidden and read-only, `CCD_MODE->count = 1`, and only `BIN_1x1` is exposed;
     - horizontal and vertical bin values/minima are set to 1 and maxima are set from `dsi_get_max_binning()`.
   - Recorded connect-time optional property mutations:
     - `CCD_TEMPERATURE` is made visible/read-only with range `MIN_CCD_TEMP` to `MAX_CCD_TEMP` and step 0, then hidden again if `dsi_get_temperature()` reports no sensor;
     - `CCD_GAIN` is made visible/writable with range 0..100 and value from `dsi_get_amp_gain()`;
     - `CCD_OFFSET` is made visible/writable with range 0..100 and value from `dsi_get_amp_offset()`.
   - Recorded connection side effects:
     - `camera_open()` obtains the INDIGO global lock, opens `dsi_open_camera(PRIVATE_DATA->dev_sid)` under enumeration/per-device mutex coverage and allocates the image buffer if needed;
     - successful connect fills CCD metadata, sets `device->is_connected = true`, sets `CONNECTION` OK and starts temperature polling only when a sensor is present;
     - failed connect sets `CONNECTION` alert and restores the disconnected switch.
   - Recorded disconnect side effects:
     - disables temperature checks;
     - cancels `temperature_timer` synchronously;
     - closes the camera;
     - releases the global lock;
     - clears `device->is_connected`;
     - updates `CONNECTION` through the CCD base change handler.
   - Recorded writable property behavior:
     - `CCD_EXPOSURE` ignores overlapping exposure requests, copies values, applies shortest-exposure bias handling, starts DSI exposure, marks upload/image properties busy as needed and schedules `exposure_timer_callback`;
     - `CCD_ABORT_EXPOSURE` aborts and resets the camera only while exposure is busy, then copies the abort request;
     - `CCD_GAIN` copies values, calls `dsi_set_amp_gain()` and updates the property OK;
     - `CCD_OFFSET` copies values, calls `dsi_set_amp_offset()` and updates the property OK;
     - `CCD_BIN` copies values, forces horizontal and vertical binning to remain equal, then delegates to `indigo_ccd_change_property()` with the adjusted `CCD_BIN_PROPERTY`.
   - Recorded polling/readout behavior:
     - `exposure_timer_callback` marks exposure time as 0, reads pixels, adds Bayer FITS metadata when available, processes the image and sets exposure OK or performs CCD failure cleanup and sets alert;
     - `ccd_temperature_callback` updates `CCD_TEMPERATURE` only while connected and `can_check_temperature` is true, then reschedules itself after `TEMP_CHECK_TIME`.
   - Compared the inventory with `indigo_docs/PROPERTIES.md`; no documentation update is needed at this step because the refactor plan preserves existing standard CCD property behavior and introduces no new public properties.

4. Reshape the hand-written driver into generator-friendly sections
   - Keep behavior unchanged in this phase.
   - Reorder the source into clear sections:
     - includes;
     - defines;
     - private data;
     - DSI SDK helpers;
     - CCD handlers/finalizers;
     - temporary legacy hot-plug support;
     - driver entry point.
   - Preserve license header and version history.
   - Move SDK helper code out of INDIGO callbacks where it can be named and reused without changing behavior.
   - Keep the manual hot-plug path isolated so it can later be replaced by generated `sdk` hot-plug.

   Result:
   - Reorganized `indigo_ccd_dsi.c` into explicit top-level sections:
     - driver metadata;
     - includes;
     - constants and macros;
     - private data;
     - DSI SDK helpers;
     - INDIGO CCD callbacks and handlers;
     - temporary manual hot-plug support;
     - driver entry point.
   - Moved the driver-local `MAX_DEVICES` and `NOT_FOUND` definitions next to the other constants so the temporary hot-plug section only contains state and behavior.
   - Renamed the misleading legacy section label from `FLI USB interface implementation` to `DSI SDK helpers`.
   - Kept the existing helper/function order otherwise unchanged to avoid behavior churn before the generator migration.
   - Kept manual hot-plug support isolated for later replacement by generated `sdk` hot-plug.
   - Confirmed `libdsi.c` and `libdsi.h` still have no diff.
   - Rebuilt `ccd_dsi` with `make -C indigo_drivers/ccd_dsi -f ../../Makefile.drv`; archive, shared library and standalone executable were produced successfully.

5. Introduce generator-compatible SDK identity and open/close helpers
   - Add target `.driver` shape with `sdk { hotplug = true; vid = DSI_VENDOR_ID; }`.
   - Move DSI SID discovery into a helper suitable for `sdk.plug`.
   - In `sdk.plug`, use `dsi_scan_usb()` to find SIDs not already assigned to generated devices.
   - Fill `private_data->dev_sid` and generated `name` without unsafe camera open on macOS.
   - On non-macOS, preserve the current optional model-name lookup through a temporary `dsi_open_camera()` only if it remains safe.
   - Use `sdk.unplug` or generated detach context to release any SID reservation.
   - Add `dsi_open(indigo_device *device)` and `dsi_close(indigo_device *device)` with the exact names expected by the generator for driver `dsi`.
   - Preserve enumeration/global-lock coverage around SDK scan/open/close sequences.
   - Verify failed opens restore connection state and do not leak the global lock or image buffer.

   Result:
   - Renamed the connection helpers to the generator-compatible names:
     - `camera_open()` -> `dsi_open()`;
     - `camera_close()` -> `dsi_close()`.
   - Renamed the remaining driver-local low-level camera helpers to use the DSI driver prefix without colliding with exported `libdsi` API names:
     - `camera_start_exposure()` -> `dsi_camera_start_exposure()`;
     - `camera_read_pixels()` -> `dsi_camera_read_pixels()`;
     - `camera_abort_exposure()` -> `dsi_camera_abort_exposure()`.
   - Updated the handwritten connection, detach and shutdown cleanup paths to call `dsi_open()` / `dsi_close()` without changing the current behavior.
   - Renamed the plug SID scan helper to `dsi_find_plugged_device_sid()` to make its role explicit for the later `sdk.plug` migration.
   - Extracted `dsi_get_plugged_device_name()` from `process_plug_event()` so the current platform-specific plug naming behavior is isolated:
     - macOS still uses the safe `"Meade DSI"` fallback and does not open the camera during plug processing;
     - non-macOS still opens the camera temporarily to read the model name and closes it immediately.
   - Left manual libusb hot-plug registration, `devices[]` and timer-delayed plug/unplug processing in place for now; generator-owned `sdk { hotplug = true; ... }` will replace that in later steps.
   - Preserved enumeration/global-lock and per-device mutex coverage in the open/close paths.
   - Confirmed `libdsi.c` and `libdsi.h` still have no diff.
   - Rebuilt `ccd_dsi` with `make -C indigo_drivers/ccd_dsi -f ../../Makefile.drv`; archive, shared library and standalone executable were produced successfully.

6. Convert connection setup into generator-owned lifecycle blocks
   - Move successful-connect camera setup into `ccd.on_connect`.
   - Move disconnect cleanup into `ccd.on_disconnect`.
   - Keep `INFO_PROPERTY->count = 8` and initial static property customization in `ccd.on_attach`.
   - Let generated connection handling own the standard connection property prologue/epilogue, open/close calls, property definition/deletion and pending-handler cleanup.
   - Avoid direct `return` from `on_connect` / `on_disconnect` blocks; use generator-provided flow variables as documented.

   Result:
   - Split the handwritten connection callback into generator-friendly pieces without changing behavior:
     - `ccd_on_connect()` now contains the successful-connect CCD metadata, frame, mode, binning, temperature, gain and offset setup;
     - `ccd_on_disconnect()` now contains the temperature shutdown, camera close, `device->is_connected` clear and `CONNECTION` OK state update.
   - Kept `dsi_open()` and `dsi_close()` as the generator-compatible open/close helpers called by the current handwritten connection callback.
   - Left `ccd_connect_callback()` in place as the temporary handwritten connection dispatcher; later `.driver` conversion will move the helper bodies into `ccd.on_connect` and `ccd.on_disconnect` and let the generator own the standard connection prologue/epilogue.
   - Kept `INFO_PROPERTY->count = 8` in `ccd_attach()` for now; it is ready to become `ccd.on_attach` content during annotation/extraction.
   - Confirmed there are no direct early `return` statements inside the extracted connect/disconnect helper bodies that would conflict with generator-owned lifecycle flow.
   - Confirmed `libdsi.c` and `libdsi.h` still have no diff.
   - Rebuilt `ccd_dsi` with `make -C indigo_drivers/ccd_dsi -f ../../Makefile.drv`; archive, shared library and standalone executable were produced successfully.

7. Convert exposure and polling work to queue-friendly handlers
   - Convert `CCD_EXPOSURE` to a generated `on_change` handler that starts the exposure on the device queue.
   - Use a delayed finalizer/poller for image readiness and download.
   - Ensure image download does not block other urgent operations longer than necessary; reassess the current `camera_read_pixels()` loop that sleeps while holding `usb_mutex`.
   - Preserve FITS/Bayer keyword behavior and image dimensions under current bin mode.
   - Ensure failure paths call `indigo_ccd_failure_cleanup()`, reset hardware when needed and publish alert state.
   - Convert temperature polling into `ccd.on_timer` or a named delayed handler consistent with generated driver behavior.

   Result:
   - Converted `CCD_EXPOSURE` handling to queue-friendly shape in the handwritten driver:
     - the change branch now copies and normalizes exposure values, marks `CCD_EXPOSURE` busy immediately and queues `ccd_exposure_handler()`;
     - `ccd_exposure_handler()` performs the SDK exposure start and schedules delayed image readout with `indigo_execute_priority_handler_in(..., INDIGO_TASK_PRIORITY_TIME, ...)` for more precise exposure timing;
     - image readout now lives in `ccd_exposure_finalizer()`, preserving current FITS/Bayer keyword and image-size behavior.
   - Converted temperature polling from raw timer handles to generated-style handler polling:
     - `ccd_timer_callback()` updates `CCD_TEMPERATURE` when allowed and reschedules itself with `indigo_execute_handler_in()`;
     - successful connect starts the polling handler only when the camera reports a usable temperature sensor.
   - Removed the driver-private exposure and temperature timer handles because queued handlers now own the delayed work.
   - Disconnect now cancels pending device handlers before closing the camera, matching the generated connection-handler model.
   - Abort now cancels a pending exposure finalizer before calling the SDK abort/reset helper.
   - Reassessed the `dsi_camera_read_pixels()` wait loop and changed it so `usb_mutex` is not held while sleeping for SDK image readiness; the loop now rechecks connection/exposure state after reacquiring the mutex before continuing.
   - Kept `CCD_ABORT_EXPOSURE`, `CCD_GAIN`, `CCD_OFFSET` and `CCD_BIN` custom property conversion for Step 8.
   - Confirmed `libdsi.c` and `libdsi.h` still have no diff.
   - Rebuilt `ccd_dsi` with `make -C indigo_drivers/ccd_dsi -f ../../Makefile.drv`; archive, shared library and standalone executable were produced successfully.

8. Convert custom writable property handling
   - Convert `CCD_ABORT_EXPOSURE` handling to an urgent or high-priority generated change path if the generator supports the required dispatch; otherwise document why it remains direct.
   - Convert `CCD_GAIN` to an `on_change` block that applies `dsi_set_amp_gain()` and updates property state.
   - Convert `CCD_OFFSET` to an `on_change` block that applies `dsi_set_amp_offset()` and updates property state.
   - Convert `CCD_BIN` to an inherited property with an `on_change` block that equalizes horizontal and vertical values.
   - Keep `CCD_BIN` synchronous and call `indigo_ccd_change_property(device, NULL, CCD_BIN_PROPERTY)` from the handler after equalizing horizontal and vertical values.
   - Verify generated handler naming does not collide with helper/finalizer names.

   Result:
   - Converted `CCD_ABORT_EXPOSURE` to a queue-friendly urgent path:
     - the change branch now copies the abort request and queues `ccd_abort_exposure_handler()` with `INDIGO_TASK_PRIORITY_URGENT`;
     - the handler cancels any pending exposure finalizer, calls the DSI abort/reset helper while exposure is busy and then publishes the standard CCD abort cleanup state.
   - Converted `CCD_GAIN` to a generated-style apply handler:
     - the change branch copies values, marks `CCD_GAIN` busy and queues `ccd_gain_handler()`;
     - the handler applies `dsi_set_amp_gain()` under the per-device USB mutex and publishes `CCD_GAIN` OK.
   - Converted `CCD_OFFSET` the same way through `ccd_offset_handler()`, applying `dsi_set_amp_offset()` under the USB mutex and publishing `CCD_OFFSET` OK.
   - Left `CCD_BIN` as a synchronous local-base-handler shape for generator migration:
     - the branch copies values, equalizes horizontal and vertical binning according to the DSI constraint and delegates to `indigo_ccd_change_property()` with the adjusted `CCD_BIN_PROPERTY`;
     - this maps to an inherited `CCD_BIN` `on_change` block with `asynchronous_change = false` and an explicit base CCD handler call, without using `pass_through_change`.
   - Verified the new handler names do not collide with existing DSI SDK helpers or generated lifecycle helper names.
   - Confirmed `libdsi.c` and `libdsi.h` still have no diff.
   - Rebuilt `ccd_dsi` with `make -C indigo_drivers/ccd_dsi -f ../../Makefile.drv`; archive, shared library and standalone executable were produced successfully.

9. Add migration annotations
   - Mark additional includes with `//+ include`.
   - Mark constants and property-related macros with `//+ define`.
   - Mark private data fields with `//+ data`.
   - Mark shared DSI SDK helpers with `//+ code`.
   - Mark CCD-specific helper/finalizer code with `//+ ccd.code`.
   - Mark `INFO_PROPERTY` setup with `//+ ccd.on_attach`.
   - Mark connection setup with `//+ ccd.on_connect`.
   - Mark disconnect cleanup with `//+ ccd.on_disconnect`.
   - Mark exposure, abort, gain, offset and binning behavior with property `on_change` annotations.
   - Do not annotate manual `devices[]`, libusb callback registration, `callback_handle`, `process_plug_event()`, `process_unplug_event()` or `remove_all_devices()` for final generated ownership.

   Result:
   - Added extraction annotations to `indigo_ccd_dsi.c` for:
     - additional includes required by the migrated driver;
     - DSI constants and driver-specific macros;
     - private data fields;
     - shared DSI SDK helpers, including `dsi_open()`, `dsi_close()` and low-level camera helpers;
     - `ccd.code` for the exposure finalizer;
     - `ccd.on_timer` for temperature polling;
     - `ccd.on_attach` for `INFO_PROPERTY->count = 8`;
     - `ccd.on_connect` for CCD metadata, frame, mode, binning, temperature, gain and offset setup;
     - `ccd.on_disconnect` for driver-specific polling shutdown;
     - property `on_change` bodies for `CCD_EXPOSURE`, `CCD_ABORT_EXPOSURE`, `CCD_GAIN`, `CCD_OFFSET` and `CCD_BIN`.
   - Kept generator-owned connection boilerplate out of `ccd.on_connect` / `ccd.on_disconnect` annotations:
     - no annotated `device->is_connected` assignment;
     - no annotated `CONNECTION_PROPERTY` final state assignment;
     - no annotated generated-equivalent pending-handler cancellation or close call.
   - Kept the manual hot-plug framework outside extraction annotations:
     - `devices[]`;
     - manual libusb callback registration and `callback_handle`;
     - `process_plug_event()`, `process_unplug_event()` and `remove_all_devices()`.
   - Annotated only the reusable DSI plug name helper from the hot-plug area; SDK plug/unplug SID mapping will be completed in the `.driver` during Step 10.
   - Checked annotation pairs and `git diff --check`; both passed.
   - Confirmed `libdsi.c` and `libdsi.h` still have no diff.
   - Rebuilt `ccd_dsi` with `make -C indigo_drivers/ccd_dsi -f ../../Makefile.drv`; archive, shared library and standalone executable were produced successfully.

10. Generate the initial `.driver`
   - Run `indigo_generator -c indigo_ccd_dsi.driver` from `indigo_drivers/ccd_dsi`.
   - Do not pass the `.c` file name to `-c`.
   - Inspect the extracted `.driver` by hand.
   - Fix extraction gaps in the annotated C or directly in the `.driver`, whichever gives the least churn.
   - Confirm the `.driver` contains:
     - driver metadata;
     - `sdk { hotplug = true; vid = DSI_VENDOR_ID; ... }`;
     - DSI includes and constants;
     - private data fields;
     - `dsi_open()` and `dsi_close()`;
     - `ccd` device block;
     - inherited `CCD_EXPOSURE`;
     - inherited `CCD_ABORT_EXPOSURE`;
     - inherited `CCD_GAIN`;
     - inherited `CCD_OFFSET`;
     - inherited synchronous `CCD_BIN` with explicit base CCD handler delegation;
     - temperature polling code;
     - SDK plug/unplug SID mapping.

   Result:
   - Extracted the initial generator source with `indigo_generator -c indigo_ccd_dsi.driver`.
   - Created `indigo_ccd_dsi.driver` and manually completed the extracted skeleton:
     - changed the extracted `libusb` transport to `sdk { hotplug = true; vid = DSI_VENDOR_ID; ... }`;
     - added `sdk.plug` SID discovery through `dsi_scan_usb()`;
     - added `sdk.unplug` SID release;
     - preserved the macOS firmware-load path and the rule that plug processing must not open the camera on macOS;
     - preserved generated device naming through the SDK-discovered model name plus SID uniqueness suffix;
     - preserved the historical 32-device DSI limit by overriding generated `MAX_DEVICES`.
   - Added driver-level SID reservation helpers in `code` so generated SDK hot-plug can map libusb arrivals to DSI SIDs without reusing an already attached SID.
   - Adjusted generated-compatible open/close assumptions in the `.driver`:
     - `dsi_open()` no longer rejects based on handwritten `device->is_connected`;
     - `dsi_close()` no longer depends on handwritten `device->is_connected`, because generated connection teardown owns the connected state.
   - Completed the CCD device block:
     - `name = "%s"`;
     - `on_attach` initializes `usb_mutex` and preserves `INFO_PROPERTY->count = 8`;
     - `on_connect`, `on_disconnect`, `on_timer`, exposure finalizer and inherited CCD property blocks are present.
   - Marked synchronous generated changes where needed:
     - `CCD_ABORT_EXPOSURE`, `CCD_GAIN`, `CCD_OFFSET` and `CCD_BIN` use `asynchronous_change = false`.
   - Kept `CCD_BIN` local to the DSI `.driver` instead of changing generator pass-through semantics:
     - no `pass_through_change` is used;
     - the synchronous `CCD_BIN` handler calls `indigo_ccd_change_property(device, NULL, CCD_BIN_PROPERTY)` after equalizing X/Y values so the CCD base handler sees the adjusted property.
   - Verified the `.driver` by generating temporary C/H/main files in `/private/tmp/dsi_driver_check` without overwriting repository generated files.
   - Verified the temporary generated C with `clang -fsyntax-only`.
   - Did not run the normal `ccd_dsi` build in this step because the new `.driver` would cause make to regenerate repository C/H/main files; that is Step 11.
   - Confirmed `libdsi.c` and `libdsi.h` still have no diff.

11. Regenerate source from `.driver`
   - Run `indigo_generator indigo_ccd_dsi.driver`.
   - Treat generated `.c`, `.h` and `_main.c` as generated output from this point forward.
   - Build the driver and address generator or compiler errors in the `.driver`, not by hand-editing generated C.
   - Inspect the generated diff and verify every behavior change is intentional.

   Result:
   - Regenerated repository outputs from `indigo_ccd_dsi.driver`:
     - `indigo_ccd_dsi.c`;
     - `indigo_ccd_dsi.h`;
     - `indigo_ccd_dsi_main.c`.
   - The regenerated files are now marked as generated from `indigo_ccd_dsi.driver`.
   - Built `ccd_dsi` with `make -C indigo_drivers/ccd_dsi -f ../../Makefile.drv`; the driver archive, shared library and standalone executable build completed successfully.
   - The generator reported warnings that `CCD_GAIN_PROPERTY->hidden` and `CCD_OFFSET_PROPERTY->hidden` are set to false; this matches the current connect-time behavior that exposes gain and offset after opening the camera.
   - Because the makefile regenerated sources during the build, separately compiled the regenerated `indigo_ccd_dsi.c` and `indigo_ccd_dsi_main.c` into temporary objects to confirm the final generated source state compiles.
   - Verified the generated CCD property paths:
     - `CCD_EXPOSURE` uses a generated async handler and schedules `ccd_exposure_finalizer()` with `INDIGO_TASK_PRIORITY_TIME`;
     - `CCD_ABORT_EXPOSURE`, `CCD_GAIN` and `CCD_OFFSET` use asynchronous generated change dispatch; `CCD_BIN` remains synchronous;
     - `CCD_BIN` does not use `pass_through_change`; its synchronous handler equalizes X/Y values and calls `indigo_ccd_change_property(device, NULL, CCD_BIN_PROPERTY)`.
   - Refined exposure readout after queue review:
     - `dsi_camera_read_pixels()` returns `DSI_IMAGE_READY`, `DSI_IMAGE_PENDING` or `DSI_IMAGE_FAILED` and never waits in a handler;
     - pending readout schedules `ccd_exposure_finalizer()` again with the SDK-reported remaining exposure time;
     - all driver-owned mutexes were removed: generated driver/device queues serialize DSI access, and disconnect waits for active device tasks before `dsi_close()`.
   - Verified generated SDK hot-plug code contains the DSI `sdk.plug` and `sdk.unplug` blocks from the `.driver`.
   - Deferred the generated SDK unplug reference-release pattern to Step 12 review.
   - Confirmed `libdsi.c` and `libdsi.h` still have no diff.

12. Reconcile DSI hot-plug semantics with generated `sdk` hot-plug
   - Let the generator own libusb callback registration, plug attach, unplug detach, `devices[]` storage and shutdown cleanup.
   - Remove handwritten `process_plug_event()`, `process_unplug_event()`, `hotplug_callback()`, `remove_all_devices()`, `devices[]` and `callback_handle` after generated behavior is confirmed.
   - Keep DSI-specific SID discovery, model-name lookup and firmware handling in `.driver` helper blocks.
   - Preserve the macOS rule that plug processing must not open the camera if that triggers reset and duplicate hot-plug events.
   - Confirm unplug behavior matches the current driver when a connected camera is physically removed.
   - Verify generated shutdown refuses connected devices with `VERIFY_NOT_CONNECTED` or equivalent generated behavior.

   Result:
   - Confirmed that the generator owns libusb callback registration, driver-queue serialization, the `devices[]` table, attach/detach and shutdown cleanup.
   - Confirmed `sdk.plug` preserves DSI-specific behavior: macOS firmware loading remains before DSI enumeration, SID discovery is serialized by the generated driver queue, and macOS still avoids opening the camera for model-name lookup.
   - Confirmed `sdk.unplug` releases the DSI SID before generated detach; detach synchronously disconnects the CCD device, cancels pending device handlers and closes the camera.
   - Confirmed generated shutdown first applies `VERIFY_NOT_CONNECTED` and then detaches remaining disconnected devices.
   - Reviewed the two `libusb_unref_device(dev)` calls in generated unplug handling. They release the separate retained references for the original arrival device and the current removal or shutdown event, so no generator change is needed.
   - Confirmed no manual hot-plug framework remains in the DSI source of truth; all hot-plug behavior is expressed through `sdk { hotplug = true; ... }`.

13. Validation
   - Build `ccd_dsi` with the narrowest available driver build command.
   - Build the standalone `indigo_ccd_dsi` target if available.
   - Run generator compatibility checks with temporary output before overwriting repository files when useful.
   - Without hardware, verify generated source contains the expected SDK plug/unplug blocks, open/close helpers and property handlers.
   - With hardware, test:
     - initial hot-plug enumeration;
     - plug while server is running;
     - unplug while disconnected;
     - unplug while connected;
     - connect/disconnect cycles;
     - firmware load path on macOS;
     - exposure success;
     - exposure abort;
     - image download and Bayer keyword behavior;
     - no-temperature-sensor path;
     - temperature polling path;
     - gain and offset changes;
     - 1x1/2x2 binning on a camera that supports binning;
     - multiple cameras with unique names and stable SID assignment.

   Result:
   - Regenerated `indigo_ccd_dsi.c`, `indigo_ccd_dsi.h` and `indigo_ccd_dsi_main.c` from `indigo_ccd_dsi.driver`.
   - Built `indigo_libs`, then built the DSI archive, shared driver and standalone `indigo_ccd_dsi` executable successfully.
   - Verified `git diff --check`, generated SDK plug/unplug blocks, generated asynchronous exposure/abort/gain/offset dispatch, synchronous DSI bin handling, the CCD exposure setup call and the three readout-result states.
   - Confirmed `libdsi.c` and `libdsi.h` have no refactor diff.
   - No DSI simulator or hardware-free integration test exists. Initial enumeration, hot-plug, connect/disconnect, exposure/readout/abort, temperature, binning and multi-camera validation remain pending real DSI hardware.

14. Cleanup
   - Keep only the `.driver` plus generated `.c`, `.h` and `_main.c` as driver source files.
   - Keep `libdsi.c`, `libdsi.h`, firmware files, udev rules, project files and README untouched unless migration requires a build-system or user-visible update.
   - Remove stale handwritten-only declarations and manual hot-plug helpers.
   - Do not edit vendored or firmware artifacts.
   - Update `README.md` only if user-visible behavior changes.
   - Update `indigo_docs/PROPERTIES.md` only if properties, item counts, labels, visibility or persistence change.

   Result:
   - Kept `indigo_ccd_dsi.driver` as the source of truth together with its generated C, header and main outputs.
   - Added the `.driver` source to the Visual Studio project and filters as a non-build source item.
   - Confirmed no stale handwritten hot-plug or connection helper remains in the driver source of truth.
   - Left `libdsi`, firmware, udev rules and README untouched; this refactor introduces no public property or user-visible behavior requiring documentation changes.

## Suggested milestones

1. Formatting and baseline status recorded.
2. Property and lifecycle inventory completed.
3. Hand-written `indigo_ccd_dsi.c` reorganized into generator-friendly sections with no intended behavior change.
4. DSI SDK SID mapping and open/close helpers isolated behind generator-compatible names.
5. Exposure, abort, gain, offset, binning and temperature handling converted to generator-friendly handlers.
6. Migration annotations added and reviewed.
7. `indigo_ccd_dsi.driver` extracted or written.
8. `.c`, `.h` and `_main.c` regenerated from `.driver`.
9. Driver builds from generated output.
10. Hardware validation pass completed or explicitly documented as deferred.

## Decisions

- Use generator-native `sdk` hot-plug for the migrated driver.
- Keep DSI SID assignment in `sdk.plug` / `sdk.unplug`, not in a preserved manual libusb callback framework.
- Preserve macOS plug behavior by avoiding camera open during plug processing unless hardware testing proves it is safe.
- Preserve `INFO_PROPERTY->count = 8`.
- Preserve DSI equal-X/Y binning semantics with a synchronous `CCD_BIN` handler that explicitly calls the CCD base handler with `CCD_BIN_PROPERTY`.
- Preserve global SDK enumeration/open/close serialization with `indigo_device_enumeration_mutex`.
- Re-evaluate the per-device `usb_mutex` after queue conversion; keep it only if SDK calls still need protection outside generated queue serialization.
