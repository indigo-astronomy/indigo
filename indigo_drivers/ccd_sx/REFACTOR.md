# Refactoring plan for INDIGO 3.0 Starlight Xpress CCD driver

Goal: refactor the Starlight Xpress CCD driver into a generator-friendly INDIGO 3.0 structure and migrate it to `indigo_generator`. Preserve the existing direct-libusb hot-plug behavior, multiple-device support, CCD acquisition, optional cooling and Star2K guider behavior while moving ordinary CCD and guider boilerplate into generated code.

## Reference material

- Use the current `indigo_ccd_sx.c` as the behavioral reference and initial source to annotate.
- Use `indigo_drivers/guider_gpusb/indigo_guider_gpusb.driver` as the closest generated direct-libusb reference for `libusb { hotplug = true; ... }`, `*_match()`, `*_open()`, `*_close()` and timed guider relays.
- Use `indigo_drivers/wheel_asi/REFACTOR.md` and `indigo_drivers/ccd_dsi/REFACTOR.md` as process references for a staged hot-plug-driver migration, not as an implementation template: SX uses a native libusb handle rather than a vendor SDK identity layer.
- Use `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` for extraction rules, generator ownership, open/close helper naming, hot-plug behavior and `.driver` source ownership.
- Use `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` and `indigo_docs/DEVELOPMENT.md` for device lifecycle, property semantics and handler queues.
- Use `indigo_tools/indigo_generator.c` as the authoritative source for the generator's direct-libusb hot-plug and multi-logical-device behavior.
- Use `indigo_drivers/ccd_sx/README.md` and `TESTING.md` for supported hardware and historical hardware validation. CCD SX now has `indigo_test/integration/test_ccd_sx_usb.c`, which tests the production driver against fake libusb. `test_ao_sx_simulator.c` remains a separate AO driver test.

## Current public behavior to preserve

- Driver entry point: `indigo_ccd_sx`.
- Driver name: `indigo_ccd_sx`.
- Driver label: `Starlight Xpress Camera`.
- Supported hardware and product recognition: the current `SX_PRODUCTS` table for vendor id `0x1278`, including SXVF, SXVR, Trius, Lodestar, CoStar, SuperStar, UltraStar, Oculus, LSI9 and HLSI9 models.
- Hot-plug behavior:
  - libusb hot-plug enumeration and arrival/removal handling;
  - multiple simultaneously attached physical cameras, up to the current `MAX_DEVICES` capacity;
  - names based on the product table with the USB path used to make duplicate names unique;
  - a shared private data record and USB handle for the CCD and guider logical devices of one physical camera.
- CCD behavior:
  - camera open, USB interface claim, protocol reset and camera parameter discovery;
  - RAW 16-bit image acquisition, including interlaced and ICX453 readout paths;
  - frame constraints, binning support restricted to equal 1x1, 2x2 and 4x4 modes, and matching `CCD_MODE` labels;
  - optional shutter handling for dark frames;
  - optional cooler/temperature control and periodic temperature polling;
  - exposure abort, image processing and Bayer metadata behavior.
- Guider behavior:
  - Star2K relay control for RA and DEC guide pulses;
  - delayed guide-pulse completion and correct busy/OK state transitions;
  - shared USB access with CCD commands.
- Compatibility constraint to verify rather than silently change:
  - the current plug path creates both CCD and guider logical devices for every recognized product, although `SX_PRODUCTS[].iface` marks guider capability only for some products. Preserve this attachment shape during the initial migration, then decide whether capability-gated guider creation is safe only with hardware confirmation.

## Target shape

- One generator source file: `indigo_ccd_sx.driver`, which becomes the source of truth after migration.
- Generated repository outputs: `indigo_ccd_sx.c`, `indigo_ccd_sx.h` and `indigo_ccd_sx_main.c`.
- Generator-owned direct USB hot-plug through `libusb { hotplug = true; vid = SX_VENDOR_ID; }`.
- An `sx_match(libusb_device *dev, const char **name)` helper retains product-id recognition and the product-table display names.
- `sx_open(indigo_device *device)` and `sx_close(indigo_device *device)` retain only native USB open/claim/reset/setup and release/close behavior. The generator owns connection reference counting for logical devices sharing one physical camera.
- Shared private data retains the native `libusb_device_handle *`, discovered camera parameters, protocol setup buffer, exposure/readout state, cooling state, relay mask and image buffers. It stores no duplicate manual hot-plug arrays, callback handles or driver-local device counters once generator ownership is active.
- The `.driver` contains one CCD logical device and one guider logical device sharing generated private data and the generated master-device queue.
- Exposure completion is modeled as a priority-timed finalizer. It must target the requested completion time accurately and must never block the device queue while merely waiting for exposure completion.
- USB readout is made explicit as a small state/result contract: failed exposure, image not ready yet, or image downloaded. A pending readout reschedules the finalizer; an error performs ordinary CCD failure cleanup; a downloaded image is processed and published once.
- Remove custom mutexes only after queue ownership and CCD/guider USB-command serialization are demonstrated correct. Do not replace required serialization with concurrent libusb access.

## Step-by-step plan

1. Check formatting and baseline hygiene
   - Record workspace status before touching the driver.
   - Inspect `indigo_ccd_sx.c`, `indigo_ccd_sx.h` and `indigo_ccd_sx_main.c` for repository-local formatting issues.
   - Confirm `.editorconfig` expectations: UTF-8, LF, tabs, K&R braces and trimmed trailing whitespace.
   - Fix only formatting issues needed to make later diffs reviewable; do not mix broad formatting churn with behavior changes.
   - Run the narrow SX build before refactoring and record whether all driver targets build.

   Result:
   - Recorded the baseline workspace state before SX source changes. The only pre-existing unrelated change is `indigo.xcodeproj/project.pbxproj`; it remains outside this refactor. `REFACTOR.md` is the only new SX-path file.
   - Confirmed `.editorconfig` requirements: UTF-8, LF line endings, tabs, indent size 2 and trimmed trailing whitespace.
   - Confirmed `indigo_ccd_sx.c`, `indigo_ccd_sx.h` and `indigo_ccd_sx_main.c` are ASCII text and have no trailing whitespace.
   - `uncrustify` is not available in the current environment, so formatting validation used the repository rules and targeted whitespace checks.
   - Identified historical single-line control bodies in the handwritten source. They will be converted only while their surrounding handlers are refactored, avoiding a standalone broad formatting diff before behavior is inventoried.
   - No SX source formatting edit was needed at this stage.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded: archive, shared library and standalone executable were built without reported warnings.

2. Establish the behavioral and property baseline
   - Capture current entry point, label, version, supported product ids and the manual libusb hot-plug model.
   - Inventory all CCD and guider property mutations: permissions, visibility, counts, ranges, values, mode labels and connection-time setup.
   - Record all `indigo_init_*_property()` / `indigo_init_*_item()` calls and compare the public property surface with `indigo_docs/PROPERTIES.md`.
   - Record the exact behavior of connection sharing, temperature polling, exposure start/readout/abort, interlaced/ICX453 image paths and guide relays.
   - Document hardware validation requirements and the lack of a CCD SX simulator.

   Result:
   - Captured current driver identity:
     - entry point `indigo_ccd_sx`;
     - `DRIVER_NAME` is `indigo_ccd_sx`;
     - `DRIVER_VERSION` is `0x0300000F`;
     - driver label is `Starlight Xpress Camera`.
   - Captured manual hot-plug behavior:
     - libusb callback registration uses vendor id `SX_VENDOR_ID` / `0x1278` and all product ids are filtered by `SX_PRODUCTS`;
     - `MAX_DEVICES` is 10 and the array stores both logical devices, so it currently permits at most five CCD-plus-guider pairs;
     - the attached CCD name comes from `SX_PRODUCTS` and both device names gain the USB-path suffix through `indigo_make_name_unique()`;
     - one `sx_private_data` record is allocated per physical camera and is shared by its CCD master and guider slave;
     - the driver always creates the guider logical device for a matched product, while the product table's interface flags are not used by the plug path.
   - Confirmed no driver-local property is allocated with `indigo_init_*_property()`. `indigo_ccd_attach()` and `indigo_guider_attach()` create the public property surface. The driver only rebuilds the three standard `CCD_MODE` items at connect time:
     - `BIN_1x1`, `BIN_2x2` and `BIN_4x4` with dynamic RAW 16 dimensions;
     - `CCD_MODE` is made writable and its count is set to 3.
   - Recorded CCD attach-time mutations:
     - `CCD_BIN` is made writable;
     - horizontal and vertical bin maxima and `CCD_INFO` maximum-bin values are set to 4;
     - `CCD_INFO_BITS_PER_PIXEL` is set to 16;
     - the shared USB mutex is initialized.
   - Recorded CCD successful-connect mutations and side effects:
     - the first of the CCD/guider pair acquires the INDIGO global lock and calls `sx_open()`; the shared `device_count` prevents a second open;
     - `CCD_INFO` and `CCD_FRAME` width/height/limits are populated from the discovered sensor dimensions;
     - pixel width, height and size are populated from the discovered camera parameters;
     - optional `CCD_COOLER` and `CCD_TEMPERATURE` are made visible only when `CAPS_COOLER` is present, initial target temperature is 0 and a five-second polling timer is started;
     - successful connect sets `can_check_temperature`, `device->is_connected` and `CONNECTION` OK; failure decrements the shared count, sets connection alert and restores the disconnected switch.
   - Recorded CCD writable-property behavior:
     - `CCD_EXPOSURE` rejects overlap, copies values, applies bias shortest-exposure handling, starts the USB exposure, marks output properties busy and schedules either the early register-clear timer or the exposure-readout timer;
     - `CCD_ABORT_EXPOSURE` cancels the exposure timer when possible, aborts the hardware and re-enables temperature checks;
     - `CCD_FRAME` rounds interlaced dimensions to even values, rounds dimensions to the selected bin factors, clamps an overrun and reports alert on clamp;
     - `CCD_BIN` is intended to allow only equal 1x1, 2x2 and 4x4 binning; its current validation reads the values before copying the incoming property, so the exact resulting behavior must be preserved and separately verified before correcting it;
     - `CCD_COOLER` is marked busy on a connected cooled camera and the periodic callback performs the USB command;
     - `CCD_TEMPERATURE` stores the target, restores the displayed current temperature, enables the cooler if needed and marks temperature busy.
   - Recorded exposure/readout and cooling behavior:
     - a successful exposure timer sets exposure time to 0, calls `sx_read_pixels()`, processes the image and publishes `CCD_EXPOSURE` OK; failure calls `indigo_ccd_failure_cleanup()` and publishes alert;
     - a long exposure clears registers at target minus three seconds, disables temperature checks and schedules final readout three seconds later; a short exposure disables temperature checks immediately and schedules readout at the target;
     - the temperature callback calls `sx_set_cooler()` only while connected and `can_check_temperature` is true, updates both cooler and temperature states, then repeats after five seconds.
   - Recorded guider behavior:
     - guider connection shares the same global lock/open/close count as CCD connection and asserts `CAPS_STAR2K` before resetting relays;
     - `GUIDER_GUIDE_DEC` and `GUIDER_GUIDE_RA` cancel the shared guider timer, update their half of `relay_mask`, send relays and set busy only for a nonzero pulse;
     - the single timer callback sends relay mask 0, clears both completed axis properties as applicable and then resets the shared relay mask.
   - Recorded teardown behavior:
     - CCD disconnect synchronously cancels temperature polling; guider disconnect synchronously cancels the guider timer;
     - the final logical disconnect calls `sx_close()`, releases the global lock and clears the connection state;
     - `sx_close()` closes the libusb handle and frees image buffers, while hot unplug detaches both logical devices and frees the shared data and referenced libusb device.
   - Compared this inventory with `indigo_docs/PROPERTIES.md`. The driver only modifies documented standard CCD and guider properties; no documentation update is needed unless a later step deliberately changes their public contract.
   - Validation remains hardware-dependent: the repository records a historical successful macOS Lodestar test, while the driver README also lists Lodestar X2 and H694. Automated coverage is now provided by the fake USB suite below; the historical hardware results are unchanged.

3. Reshape the hand-written driver into generator-friendly sections
   - Preserve behavior while separating driver metadata, constants, shared private data, low-level SX USB helpers, CCD handlers, guider handlers and temporary manual hot-plug support.
   - Preserve the license header and version history.
   - Isolate manual hot-plug code so it can later be removed as one coherent generator-owned replacement.
   - Keep formatting local and remove empty lines inside function bodies only where this does not obscure logical sections.

   Result:
   - Corrected the source-file documentation spelling from `StarlighXpress` to `Starlight Xpress`.
   - Renamed the former broad `SX USB interface implementation` section to `SX protocol definitions and shared state`.
   - Moved `SX_VENDOR_ID` and `MAX_DEVICES` next to the other driver constants, so the temporary hot-plug section now contains only hot-plug state and behavior.
   - Added explicit boundaries for low-level SX USB helpers and temporary manual hot-plug support.
   - Did not change USB protocol commands, private-data layout, CCD/guider callback logic, timer behavior or manual hot-plug behavior.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded for the archive, shared library and standalone executable without reported warnings.

4. Isolate direct-libUSB identity and connection helpers
   - Extract `sx_match(libusb_device *dev, const char **name)` from the product table and preserve all accepted product ids and display names.
   - Adapt `sx_open(indigo_device *device)` and `sx_close(indigo_device *device)` to the exact generator naming convention and generator-owned `PRIVATE_DATA->usbdev` device reference.
   - Preserve interface-claim, kernel-driver detach where supported, reset/setup command flow, parameter discovery and buffer allocation/frees.
   - Audit every libusb transfer timeout and error path for portable behavior, especially on Windows.
   - Keep direct protocol functions consistently prefixed `sx_` and make their ownership and return status explicit.

   Result:
   - Renamed the shared native device field from `dev` to `usbdev` and updated open, unplug and shutdown paths. This matches the generator's direct-libusb private-data convention without changing ownership or reference counting.
   - Added `sx_match(libusb_device *dev, const char **name)` as the single product-id recognition helper. It reads the USB descriptor, retains the current first-match order of `sx_products` and returns the existing display-name seed.
   - Updated the temporary manual plug path to use `sx_match()`. A descriptor-read failure now rejects the plug event instead of continuing with an invalid descriptor; all successful product filtering and names remain unchanged.
   - Renamed the static product table to `sx_products` and kept every product id, order, name and interface flag unchanged.
   - Kept `sx_open()` / `sx_close()` as the generator-compatible connection helper names and preserved native libusb open, optional kernel-driver detach, interface claim, reset, parameter discovery and buffer allocation/free behavior.
   - Audited direct transfers: command exchanges retain `BULK_COMMAND_TIMEOUT` of 2000 ms and image chunks retain `BULK_DATA_TIMEOUT` of 10000 ms. The deliberately blocking image-transfer design will be replaced only in the scheduled finalizer/readout step.
   - Corrected libusb diagnostic messages to identify bulk transfers and to use `%d` for the `int transferred` result, avoiding an incorrect variadic format on Windows and 64-bit platforms.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded for the archive, shared library and standalone executable without reported warnings.

5. Convert CCD execution to handler-queue work
   - Move connection work, exposure start, abort, frame/bin validation and cooler updates into generator-extractable CCD handlers.
   - Use `indigo_ccd_exposure_setup(device)` after a successfully started hardware exposure so standard base-class exposure setup is retained.
   - Keep the equal horizontal/vertical 1x1, 2x2 and 4x4 binning rule in a synchronous handler. When the adjusted bin property must be processed by the CCD base class, explicitly call `indigo_ccd_change_property(device, NULL, CCD_BIN_PROPERTY)` rather than changing generator-wide dispatch behavior.
   - Keep connection failure, image upload state and base-class property updates semantically equivalent to the current driver.

   Result:
   - Replaced the CCD connection timer dispatch with `ccd_connection_handler()` on the device handler queue. The existing shared-device locking and open/close behavior remain in that handler for now.
   - Added queue handlers for exposure start, urgent exposure abort, frame validation, binning, cooler changes and target-temperature changes; `ccd_change_property()` now only copies/acknowledges properties and dispatches the appropriate handler.
   - `ccd_exposure_handler()` starts the hardware exposure first, then calls `indigo_ccd_exposure_setup(device)` to set standard upload/countdown state. A start failure now immediately performs `indigo_ccd_failure_cleanup()` and reports `CCD_EXPOSURE` alert rather than waiting for the old readout timer to fail.
   - Kept the existing timer-based register-clear/readout scheduling temporarily. The timing-sensitive finalizer and blocking pixel-read state contract are deferred to step 6.
   - `CCD_ABORT_EXPOSURE` now executes as an urgent queue task and uses `indigo_ccd_abort_exposure_cleanup()` after cancelling the pending timer and issuing the hardware abort.
   - `CCD_FRAME` and `CCD_BIN` use synchronous handlers. `CCD_BIN` now validates the copied incoming values, retains the equal 1x1, 2x2 or 4x4 constraint, and delegates a valid property to `indigo_ccd_change_property(device, NULL, CCD_BIN_PROPERTY)`. This fixes the previous pre-copy validation defect.
   - Cooler and target-temperature property changes are dispatched to the device queue without marking either property busy when the camera is disconnected or has no cooler, preserving the previous condition for state changes.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded for the archive, shared library and standalone executable without reported warnings.

6. Redesign exposure completion and image readout
   - Replace timer callbacks with priority-timed handlers using `INDIGO_TASK_PRIORITY_TIME` so the finalizer is scheduled at the intended exposure completion time.
   - Separate waiting from data transfer: no sleep/poll loop may occupy the handler queue merely to wait for exposure end.
   - Change the low-level pixel-read path to return an explicit result: exposure failed, image not available yet, or image downloaded.
   - On pending readout, reschedule the finalizer promptly; on failure, perform CCD failure cleanup and publish alert; on success, process and publish the image exactly once.
   - Preserve the current early register-clear behavior for long exposures only if its hardware timing remains required, and schedule it as an independent timed queue task.
   - Give this step high priority because finalizer timing and queue latency affect measurement accuracy and all subsequent device tasks.

   Result:
   - Replaced the exposure and early-register-clear timers with `ccd_exposure_finalizer()` and `ccd_clear_registers_handler()` scheduled through `indigo_execute_priority_handler_in()` at `INDIGO_TASK_PRIORITY_TIME`.
   - Added the explicit `sx_image_result` contract: `SX_IMAGE_FAILED`, `SX_IMAGE_PENDING` and `SX_IMAGE_DOWNLOADED`.
   - Added `sx_get_timer()` using the documented `CCD_GET_TIMER` four-byte USB command. SX hardware-timed exposures (below one second) now return pending while the camera timer is nonzero; the finalizer reschedules after 50 ms without beginning an image transfer.
   - On a downloaded image, the finalizer clears the exposure countdown, processes and publishes the image once. On failure, it performs `indigo_ccd_failure_cleanup()` and reports an exposure alert.
   - Long exposures retain their existing clear-register hardware sequence, but both its clear task and its finalizer are now time-priority queue tasks rather than generic timers.
   - Abort cancels both pending priority handlers before issuing the hardware abort and base-class abort cleanup.
   - Removed the unused `exposure_timer` private-data field.
   - The actual bulk image download remains synchronous once pixels are ready. This is intentional for this step: the finalizer no longer waits for exposure completion, but hardware validation is still required to measure readout duration for large sensors and unplug/error behavior.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded for the archive, shared library and standalone executable without reported warnings.

7. Convert cooling and guider work to generated handlers
   - Replace temperature and guider timers with handler-queue based periodic/timed work while preserving their property state transitions.
   - Ensure cooler changes and temperature reads cannot race with exposure/readout commands on the shared device queue.
   - Model RA and DEC guide-pulse finishes as independent priority-timed handlers, matching the established generated direct-libUSB guider pattern.
   - Review whether a guider connect on non-Star2K hardware should retain the current behavior or be capability-gated; do not make that public behavior change without hardware validation.

   Result:
   - Replaced the temperature timer with `ccd_temperature_poll_handler()` on the device queue. It reschedules itself after five seconds and is cancelled through `indigo_cancel_pending_handler()` during CCD disconnect.
   - Replaced the shared guider timer with independent `guider_guide_dec_finalizer()` and `guider_guide_ra_finalizer()` time-priority handlers.
   - Each guider finalizer clears only its own relay bits, sends the remaining shared `relay_mask` to the hardware and completes only its matching property. A DEC pulse therefore no longer stops an active RA pulse, and vice versa.
   - Moved guide-pulse start commands into `guider_guide_dec_handler()` and `guider_guide_ra_handler()` dispatched at time priority, so relay USB commands serialize on the shared device queue with CCD work.
   - Removed `temperture_timer` and `guider_timer` from private data; no driver timer pointer remains for temperature, exposure or guider operation.
   - Retained the current guider connection assertion for `CAPS_STAR2K` and did not capability-gate guider attachment or connection, pending hardware validation of the existing public behavior.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded for the archive, shared library and standalone executable without reported warnings.

8. Prepare lifecycle for `.driver` blocks
   - Move static property setup into `ccd.on_attach` and `guider.on_attach`.
   - Move successful CCD setup, dynamic modes and optional cooler visibility into `ccd.on_connect`; move timer/finalizer cancellation and cleanup into `ccd.on_disconnect`.
   - Move guider setup and relay reset into its generated lifecycle blocks.
   - Let generated connection code own connection-property prologue/epilogue, shared open/close count, define/delete behavior and pending-handler cleanup.
   - Do not return directly from generated `on_connect` or `on_disconnect` blocks.

   Result:
   - Confirmed from the generator template and existing generated drivers that one direct-libusb driver can declare both `ccd` and `guider` blocks with independent `on_attach`, `on_connect` and `on_disconnect` code while sharing private data and the master-device queue.
   - Converted the remaining guider connection timer callback to `guider_connection_handler()` dispatched on the device queue, matching the CCD connection model.
   - Kept the current shared connection count, global lock, native open/close calls and connection-state behavior intact. These form the exact code to move into generated lifecycle blocks.
   - Deferred creation of a partial `.driver` source intentionally: after a `.driver` exists it becomes source of truth, so the lifecycle transfer must be performed together with the complete extraction/generation in step 11 rather than leaving two competing implementations.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded for the archive, shared library and standalone executable without reported warnings.

9. Migrate property changes into `.driver` declarations
   - Define inherited CCD and guider properties with only the required `on_change`, `asynchronous_change`, visibility and persistence attributes.
   - Move `CCD_EXPOSURE`, `CCD_ABORT_EXPOSURE`, `CCD_FRAME`, `CCD_BIN`, `CCD_COOLER`, `CCD_TEMPERATURE`, `GUIDER_GUIDE_DEC` and `GUIDER_GUIDE_RA` behavior into generator-owned property branches.
   - Keep custom updates only for intentionally self-managed asynchronous handlers and early error paths; otherwise let generated property epilogues publish the update.
   - Verify no custom branch accidentally bypasses the appropriate CCD or guider base handler.

   Result:
   - Mapped every current handwritten change branch to its generator declaration; no partial `.driver` was created because that would make it a competing source of truth before the complete extraction in step 11.
   - `CCD_EXPOSURE` will use an inherited asynchronous `on_change` branch containing the exposure-start handler. Its finalizer publishes completion or failure, so the generated branch must not perform a second terminal update.
   - `CCD_ABORT_EXPOSURE` will use an inherited asynchronous `on_change` branch that dispatches the existing urgent abort handler. `CCD_FRAME` and `CCD_BIN` will use inherited synchronous branches (`asynchronous_change = false`); the binning branch retains the direct `indigo_ccd_change_property(device, NULL, CCD_BIN_PROPERTY)` call after validation.
   - `CCD_COOLER` and `CCD_TEMPERATURE` will use inherited asynchronous branches. `GUIDER_GUIDE_DEC` and `GUIDER_GUIDE_RA` will use inherited asynchronous branches, retaining their priority-timed finalizers.
   - The custom `X_CCD_FLOOD_LED` property will be declared in the CCD block with an asynchronous `on_change` branch. Its visibility remains controlled by the discovered model capability during connection and disconnection.
   - `pass_through_change` is deliberately not used. Apart from the explicit synchronous binning handoff, each driver-owned property is terminally handled by the generated branch; inherited properties without a SX-specific branch continue to the standard CCD or guider base handler.
   - Confirmed the syntax and generated semantics against `indigo_drivers/guider_gpusb/indigo_guider_gpusb.driver`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` and `indigo_tools/indigo_generator.c`.
   - The concrete declarations and removal of handwritten dispatch will be performed atomically with the complete `.driver` extraction and generated-output review in step 11.

10. Prepare generator-owned direct-libUSB support
   - Add the `libusb` block and use `sx_match()` for product filtering and base names.
   - Confirm generated plug/unplug code gives each physical SX device one shared private-data allocation, native `usbdev` reference and the same CCD/guider master-device relationship.
   - Remove manual `devices[]`, `device_mutex`, hot-plug callback registration, plug/unplug functions and shutdown enumeration only after generated behavior has been inspected.
   - Verify detach releases buffers and USB references once, even when both logical devices are present or a device is unplugged during an exposure.

   Result:
   - Verified `indigo_generator` direct-libUSB hot-plug output: arrival and removal events are serialized on a generated driver queue, each accepted physical device receives one shared private-data allocation, and every declared logical device receives that same allocation. The guider is assigned the CCD master device, so its property handlers execute on the shared CCD queue.
   - The generated `devices[]` array stores every logical device, as does the handwritten driver. Retaining `MAX_DEVICES = 10` therefore preserves the existing limit of five CCD-plus-guider pairs rather than silently raising the number of supported physical cameras.
   - The generator transfers the hot-plug callback's libusb reference to the shared private data and releases it exactly once after detaching all matching logical devices. `sx_close()` remains responsible for an open native handle and image buffers; generated unplug owns only the device reference and private-data allocation.
   - Generated direct-libUSB hot-plug formats names from the `name` returned by `sx_match()` but does not call `indigo_make_name_unique()`. During complete extraction, `sx_match()` will build the existing product-name-plus-USB-path seed from its `libusb_device *`; the CCD declaration will use it directly and the guider declaration will append ` (guider)`. This preserves current visible names and duplicate-name disambiguation.
   - The physical `libusb` block and deletion of the handwritten callback, device array and mutex are deferred to the same atomic `.driver` extraction as step 11. Until then, the handwritten implementation remains the sole active lifecycle owner.
   - Confirmed against the generator's `write_c_hotplug_section()` and connection-handler output; the planned generated behavior preserves the current shared handle and master-queue relationship.

11. Extract and generate the driver source
   - Create `indigo_ccd_sx.driver` by annotation/extraction into a new generator source, preserving the existing C source until generated output is reviewed.
   - Generate `indigo_ccd_sx.c`, `indigo_ccd_sx.h` and `indigo_ccd_sx_main.c` with `indigo_generator`.
   - Add the `.driver` source to `indigo_ccd_sx.vcxproj` and `indigo_ccd_sx.vcxproj.filters` without unnecessary XML reformatting.
   - Treat the `.driver` as the only source of truth after generation; do not hand-edit the generated outputs.

   Result:
   - Created `indigo_ccd_sx.driver` by annotating the existing driver and extracting its shared state, low-level SX protocol helpers, CCD/guider lifecycle blocks and property handlers with `indigo_generator -c`.
   - Replaced the handwritten `indigo_ccd_sx.c`, `indigo_ccd_sx.h` and `indigo_ccd_sx_main.c` with outputs generated from the new `.driver` source. The `.driver` is now the source of truth.
   - Migrated direct-libUSB hot-plug ownership to the generator. The handwritten device array, hot-plug callback, callback handle, plug/unplug functions and device mutex are no longer present in generated source.
   - The generated CCD and guider templates share one private-data record; the guider is attached as the CCD slave and generator-owned `PRIVATE_DATA->count` now owns native open/close reference counting.
   - Preserved product recognition in `sx_match()` and changed its name seed to include the USB path before generator formatting. CCD names remain `<product> #<USB path>` and guider names remain `<product> #<USB path> (guider)`.
   - Declared `X_CCD_FLOOD_LED` in the CCD block and retained its model-gated visibility and on-change command. Hardware validation is still required for the undocumented command and for visibility on both supported and unsupported models.
   - Added `indigo_ccd_sx.driver` to the Visual Studio project and filters without reformatting the project files.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded for the archive, shared library and standalone executable without compiler warnings. The generator reports its existing informational warnings when conditional cooler and temperature properties are made visible at connection.

12. Eliminate redundant synchronization carefully
   - Review the generated per-driver hot-plug queue and shared master-device handler queue against every low-level USB call.
   - Remove `pthread_mutex_t usb_mutex` and any remaining driver-local locking only where queue serialization covers all CCD and guider commands.
   - Keep a minimal synchronization primitive only if a proven cross-queue or libusb callback race remains; document why it is necessary.
   - Verify connect/disconnect, abort, guider pulse and hot unplug cannot access a closed handle or freed private data.

   Result:
   - Removed `pthread_mutex_t usb_mutex`, its initialization and every low-level USB lock/unlock pair from the `.driver` source and regenerated C output.
   - All CCD and guider commands now serialize through the shared CCD master-device handler queue. Generated direct-libUSB hot-plug serializes arrival, removal and initial connection handling through its separate driver queue, matching `wheel_asi` and the refactored `ccd_dsi` pattern.
   - Retained only the generator-owned `driver_queue_mutex`, which protects driver-queue scheduling from libusb callback threads. It is not held around USB protocol commands and is not driver-private synchronization to remove.
   - Retained the INDIGO global lock in `sx_open()` / `sx_close()` so the migration does not drop the prior cross-driver exclusivity behavior.
   - Generated unplug detaches logical devices through the driver queue, while disconnect cancels pending handlers before closing the shared native handle. This is the established generated direct-libUSB lifetime model; hardware unplug during image transfer remains a required validation case.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded for the archive, shared library and standalone executable without compiler warnings.

13. Build and validation
   - Run `indigo_generator` and inspect the generated diff; every semantic difference must be deliberate and explained by generator behavior.
   - Build the generator, core libraries and narrow `ccd_sx` target on the current platform.
   - Run whitespace checks and inspect warnings, especially format-specifier and Windows portability warnings.
   - Exercise real hardware where available: plug/unplug, reconnect, CCD-only and guider-capable models, 1x1/2x2/4x4 binning, subframes, dark/light exposure, abort, cooler/temperature and simultaneous guider pulses.
   - Record validation limits explicitly when hardware is unavailable. Do not claim Windows runtime validation merely because the Visual Studio project builds.

   Result:
   - Rebuilt `build/bin/indigo_generator` from `indigo_tools/indigo_generator.c` before validation.
   - Ran the generator twice against `indigo_ccd_sx.driver`; both runs produced identical SHA-1 checksums for `indigo_ccd_sx.c`, `indigo_ccd_sx.h` and `indigo_ccd_sx_main.c`, confirming reproducible generated output.
   - Forced rebuild with `make -B -C indigo_drivers/ccd_sx -f ../../Makefile.drv` succeeded for the archive, shared library and standalone executable without compiler warnings.
   - `git diff --check` passed. The generator emits informational messages for the intentional connection-time unhide of `CCD_COOLER` and `CCD_TEMPERATURE`; these are not compiler warnings or build failures.
   - Reviewed the `.driver` for Windows-portability regressions. It uses INDIGO and libusb APIs only; the only short wait is `indigo_usleep()`, the portable INDIGO helper. No direct POSIX handle, thread, time or format-string dependency was introduced.
   - No SX CCD simulator or compatible camera is available in this environment. Hardware validation remains required for hot plug/unplug during image transfer, reconnect, 1x1/2x2/4x4 binning, subframes, light/dark/bias exposure, abort, cooler/temperature, simultaneous Star2K guide pulses and the model-gated flood LED command. `TESTING.md` records a historical Lodestar success only; it is not validation of this migration or of Windows runtime behavior.

14. Final cleanup and documentation
   - Remove obsolete manual callbacks, timer fields, dead compatibility code and comments made redundant by generation.
   - Keep generated source/header/main and the `.driver` synchronized.
   - Update `indigo_docs/PROPERTIES.md` only if the observable property contract changes.
   - Update `README.md` or `TESTING.md` with verified support/limitations when validation establishes new facts.
   - Finish with a focused diff review, workspace-status check and a record of hardware-dependent residual risk.

   Result:
   - Removed empty generator placeholder comments from `indigo_ccd_sx.driver`; the generated C, header and main remain fully derived from this single source.
   - Confirmed obsolete manual hot-plug and USB synchronization artifacts are absent: no `device_count`, device mutex, USB mutex, handwritten plug/unplug callback or manual device array remains in the SX source.
   - Updated `indigo_docs/PROPERTIES.md` so `X_CCD_FLOOD_LED` points to `indigo_ccd_sx.driver`, the source of truth, rather than generated C output.
   - Rebuilt the driver after cleanup. Archive, shared library and standalone executable succeeded without compiler warnings; `git diff --check` passed.
   - Residual hardware risk is intentionally recorded rather than assumed away: this migration still requires validation with supported SX hardware, including unplug during readout, reconnect, exposure modes, binning/subframes, abort, cooler, Star2K pulses and the undocumented flood-LED command. No Windows runtime validation was performed.

## Fake USB acceptance (2026-09-08)

`indigo_test/integration/test_ccd_sx_usb.c` has 24 named groups. Its Makefile target compiles the generated production driver with test-local libusb, discovery and queue instrumentation. Images use deterministic generated noise; there is no vendor SDK or simulator photograph dependency. All source fixes are in `indigo_ccd_sx.driver`; generated C is synchronized. Allocation/free uses INDIGO helpers.

| Standard area | SX scenarios |
| --- | --- |
| Metadata and properties | Driver info/version, CCD interface, RAW16 handoff, mode/bin synchronization, flood LED names/items/type/permissions; cooler and streaming visibility. |
| Open and shared ownership | Global lock, open, descriptor/claim, reset/model/parameter commands, read and short-packet failures with retry and balanced locks/handles/config descriptors. Both CCD/guider open/close orders, guider-only use, unavailable Star2K capability and rejected shutdown. |
| Discovery | Unknown PID/descriptor failure, distinct USB paths for identical cameras, survivor acquisition, duplicate arrivals, failed master/slave attachment and replug recovery, capacity overflow/refill, queue/registration rollback and shutdown with active/queued discovery. |
| Cooling | Cooled/uncooled profiles, Celsius-to-protocol target conversion, independent measured temperature, ON/OFF, settling, command/read/short-reply failures and recovery; slow initialization and cancellation on disconnect. SX has no cooler-power readback. |
| Images and acquisition | Progressive, interlaced and ICX453 layout, 1/2/4 bins with short/long readout, ROI offsets/bounds, invalid bins, all five frame types, bias and milliseconds, overlap rejection, short/zero/error USB transfers, long-exposure clear sequence and clear failure, zero-signal interlaced normalization, fresh acquisition after failures/abort/reconnect. |
| Controlled races | Barriers at exposure command and pixel read force abort before/after frame completion, disconnect and removal during readout, and guide-request coalescing while readout occupies the queue. Active exposure/pulse removal and subsequent reacquisition/guiding are exercised. USB instrumentation rejects overlapping per-handle calls, direct calls from the client thread and calls after close. |
| Guider | Four direction masks, zero/stop, same-axis replacement/reversal, simultaneous axes with independent completion, ON/OFF failures and recovery, cancellation without closing a CCD sibling, guider-only reconnect. Normal completion must reset pulse values to zero. |
| Timing | 80 measured pulses (20/50/100/250/500 ms, two repeats per direction in each workload), plus discarded warm-ups. Monotonic timestamps at fake USB relay ON/OFF entry; idle and during acquisition. Per-pulse signed/percentage errors and min/mean/median/p95/p99/max/stddev/max-absolute statistics, without a machine-dependent accuracy threshold. |

Non-applicable standard rows: SX has no streaming implementation, gain/offset/fan/heater controls, wheel/focuser/rotator, guide-rate command, SDK callbacks/discovery ids, suffix/name writes, or driver-specific persistent settings. Its readout uses synchronous USB transfers and host scheduling, without hardware-timer polling or SDK-ready retry/watchdog logic. USB transfer failures are tested at that actual boundary. Framework codecs, upload destinations and generic persistence are excluded.

Validation: all 24 groups passed in the normal build and under ASan/UBSan (test and driver instrumented; prebuilt framework/dependencies excluded). Final metadata and guide-zero assertions passed targeted reruns. The generator regression suite covers all 86 existing attribute/block cases and the new libusb lifecycle invariants. No hardware tests were repeated. This is coverage of applicable standard scenarios, not a claim of measured 100% line/branch coverage.

The user approved generator changes for duplicate USB arrivals, failed-master rollback, failed queue/registration INIT rollback, and draining accepted USB events before SHUTDOWN detach/free. Shutdown also serializes its connected-device check against an in-progress attach. The generated allocation cleanup uses `indigo_safe_free()` as requested.

Shutdown synchronization now uses the shared `indigo_queue_drain()` API in `indigo_timer.c`; the generator no longer emits a per-driver drain callback, condition variable or completion flag. The connected-device check remains serialized against attach.
