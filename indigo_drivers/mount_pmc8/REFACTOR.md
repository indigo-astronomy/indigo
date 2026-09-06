# Refactoring plan for INDIGO 3.0 PMC-Eight mount driver

Goal: refactor the PMC-Eight mount driver into a generator-friendly INDIGO 3.0 structure, rebuild transport on `indigo_uni_io`, and migrate it to `indigo_generator`. Preserve the current serial, UDP and TCP connection behavior, mount/guider logical-device pairing, PMC-Eight protocol behavior, model selection/detection and simulator-backed public behavior while moving ordinary mount and guider boilerplate into generated code.

## Reference material

- Use the current `indigo_mount_pmc8.c` as the behavioral reference and initial source to annotate.
- Use `indigo_drivers/mount_synscan/REFACTOR.md` as the closest mount + guider migration plan, especially for master-device queue serialization and simulator-backed validation.
- Use `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` for generator extraction rules, open/close helper naming, annotation blocks, inherited-property behavior and `.driver` ownership.
- Use `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` for INDIGO 3.0 device lifecycle, `indigo_uni_io`, property state handling and async handler queues.
- Use `indigo_docs/DEVELOPMENT.md` for bus/device/property lifecycle details when validating generated callback behavior.
- Use `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` and `indigo_test/AGENTS.md` for the existing host-side serial simulator contract and integration-test conventions.
- Use `indigo_drivers/mount_pmc8/README.md` for supported hardware, user-visible connection modes and platform expectations.
- Use `PMC_Eight_ProgrammersReferenceManual_Release2_2019_March_07.pdf` as the protocol reference whenever current-driver behavior is unclear.
- Use `indigo_test/integration/test_mount_pmc8_simulator.c` and `indigo_drivers/mount_pmc8/mount_pmc8_simulator/` as the main hardware-free validation path.

## Current public behavior to preserve

- Driver entry point: `indigo_mount_pmc8`.
- Driver name: `indigo_mount_pmc8`.
- Driver label: `PMC Eight Mount`.
- Logical devices:
  - `Mount PMC Eight`;
  - `Mount PMC Eight (guider)`.
- Supported hardware from the README:
  - Explore Scientific iEXOS-100, iEXOS-300 and EXOS2-GT;
  - Losmandy G11 and Titan with PMC-Eight controller.
- Supported connection behavior:
  - serial port at 115200 baud;
  - serial mode with optional DTR clear through `SERIAL_DTR`;
  - UDP URL, defaulting to `udp://192.168.47.1` and port `54372`;
  - TCP URL, defaulting to `tcp://192.168.47.1` and port `54372`;
  - runtime switching between UDP/TCP/serial where the current driver allows it;
  - no USB hot-plug support;
  - one mount device present at startup with additional instances supported.
- Standard mount behavior:
  - `MOUNT_INFO` vendor/model/firmware updates;
  - `MOUNT_ON_COORDINATES_SET` limited to track and sync behavior by setting `count = 2`;
  - visible `DEVICE_PORT`, `DEVICE_PORTS` and `MOUNT_SIDE_OF_PIER`;
  - hidden base `MOUNT_GUIDE_RATE`;
  - coordinate polling, side-of-pier update and `indigo_update_coordinates()`;
  - goto/sync through PMC-Eight raw HA/DEC counts;
  - tracking on/off and sidereal/lunar/solar rates;
  - manual RA/DEC motion;
  - abort motion;
  - park to raw zero position and unpark state handling.
- Custom/non-standard properties:
  - `CONNECTION_MODE`, switch, main group, one-of-many:
    - `UDP`;
    - `TCP`;
    - `SERIAL`;
    - `SERIAL_DTR`.
  - `MOUNT_TYPE`, switch, main group, one-of-many:
    - `G11`;
    - `TITAN`;
    - `EXOS-2`;
    - `iEXOS-100`.
- Mount model behavior:
  - firmware query with `ESGv!`;
  - optional model query with `ESGi!` for newer firmware;
  - selected/detected model drives model label and RA/DEC count constants;
  - `MOUNT_TYPE` is writable while disconnected and redefined read-only while connected.
- Guider behavior:
  - guider exposes `INDIGO_INTERFACE_GUIDER`;
  - guider shares the mount/master connection and increments/decrements the shared open count;
  - `GUIDER_RATE` is visible;
  - `GUIDER_GUIDE_RA` uses temporary tracking-rate offsets;
  - `GUIDER_GUIDE_DEC` uses PMC-Eight motion commands.
- Concurrency behavior:
  - mount and guider currently serialize shared transport through `indigo_lock_master_device()` plus `PRIVATE_DATA->port_mutex`;
  - mount and guider share one `pmc8_private_data` instance and one transport handle;
  - direct timer workers are currently used for connect/disconnect, motion, goto, tracking, park and guider pulses.

## Target shape

- One generator source file: `indigo_mount_pmc8.driver`, which becomes the source of truth after migration.
- Generated repository outputs: `indigo_mount_pmc8.c`, `indigo_mount_pmc8.h` and `indigo_mount_pmc8_main.c`.
- Public header and standalone main remain generated or generator-compatible according to repository convention.
- Connection type starts as generator `serial;` plus custom connection-mode handling for UDP/TCP URL values, unless generator URL support is expanded or an existing pattern proves a cleaner representation.
- Private data stores only fields needed by the mount/guider behavior and PMC-Eight state:
  - `indigo_uni_handle *handle`;
  - mount model type;
  - computed sidereal/lunar/solar rates;
  - connection mode state;
  - shared open count;
  - firmware version;
  - park-in-progress state;
  - guide-pulse timers or deadline state required by generated handlers.
- Low-level PMC-Eight communication is isolated in small helpers:
  - `pmc8_open(indigo_device *device)`;
  - `pmc8_close(indigo_device *device)`;
  - command helper using `indigo_uni_io`;
  - model detection and rate computation helpers;
  - raw count and coordinate conversion helpers where that reduces duplicated handler code.
- Transport code uses `indigo_uni_io` instead of direct `select()`, `read()`, `write()`, `send()`, `recv()`, `close()`, socket-specific flushing or direct serial open calls.
- No final generated or generator-source transport path may keep direct POSIX/socket I/O when an `indigo_uni_io` helper can express the same behavior.
- Slow or blocking behavior runs on generated handler queues:
  - connect/disconnect;
  - coordinate polling;
  - goto/sync;
  - park;
  - abort;
  - manual motion;
  - tracking and track-rate changes;
  - connection-mode switching;
  - guide-pulse start and finalizer work.
- Mount and guider work is serialized through the master mount device queue. Keep an explicit transport mutex only if queue serialization is not sufficient for the final generated shape.
- Property change branches are generator-extractable and map cleanly to mount/guider `on_change` blocks.
- Annotation blocks may be used during the intermediate hand-written phase:
  - `//+ include`;
  - `//+ define`;
  - `//+ data`;
  - `//+ code`;
  - `//+ mount.code`;
  - `//+ mount.on_attach`;
  - `//+ mount.on_connect`;
  - `//+ mount.on_disconnect`;
  - `//+ mount.on_timer`;
  - `//+ mount.CONNECTION_MODE.on_change`;
  - `//+ mount.MOUNT_TYPE.on_change`;
  - `//+ mount.MOUNT_PARK.on_change`;
  - `//+ mount.MOUNT_EQUATORIAL_COORDINATES.on_change`;
  - `//+ mount.MOUNT_ABORT_MOTION.on_change`;
  - `//+ mount.MOUNT_MOTION_DEC.on_change`;
  - `//+ mount.MOUNT_MOTION_RA.on_change`;
  - `//+ mount.MOUNT_TRACKING.on_change`;
  - `//+ mount.MOUNT_TRACK_RATE.on_change`;
  - `//+ guider.on_attach`;
  - `//+ guider.on_connect`;
  - `//+ guider.on_disconnect`;
  - `//+ guider.GUIDER_GUIDE_DEC.on_change`;
  - `//+ guider.GUIDER_GUIDE_RA.on_change`;
  - `//+ guider.GUIDER_RATE.on_change`.

## Step-by-step plan

1. Establish the baseline
   - Record current workspace status before touching the driver.
   - Build `mount_pmc8` with the narrow driver build target.
   - Build the simulator-backed integration test with `make -C indigo_test build/integration/test_mount_pmc8_simulator`.
   - Run the PMC-Eight simulator integration test from the correct working directory and record results for both logical devices.
   - Record any local limitations for UDP/TCP validation and real-hardware validation.
   - Save the current public property list and custom behavior from `indigo_mount_pmc8.c`.

   Result:
   - Baseline was recorded on 2026-09-06.
   - Workspace status was clean before starting step 1.
   - `build/drivers/indigo_mount_pmc8.a` was already present and up to date.
   - `make build/drivers/indigo_mount_pmc8.a` completed successfully with no rebuild required.
   - `make -C indigo_test build/integration/test_mount_pmc8_simulator` built both:
     - `indigo_test/build/integration/mount_pmc8_simulator`;
     - `indigo_test/build/integration/test_mount_pmc8_simulator`.
   - Running `build/integration/test_mount_pmc8_simulator` from `indigo_test` passed:
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.
   - Current driver identity to preserve:
     - entry point `indigo_mount_pmc8`;
     - driver name `indigo_mount_pmc8`;
     - driver label `PMC Eight Mount`;
     - logical device names `Mount PMC Eight` and `Mount PMC Eight (guider)`.
   - Current connection behavior to preserve:
     - serial connection at 115200 baud;
     - optional `SERIAL_DTR` mode that clears DTR after opening the serial port;
     - UDP URL mode with default device port `udp://192.168.47.1`;
     - TCP URL mode with default device port `tcp://192.168.47.1`;
     - default network port `54372`;
     - runtime `CONNECTION_MODE` switching across the currently supported transitions.
   - Current mount public surface to preserve:
     - `MOUNT_INFO` with vendor/model/firmware values;
     - `MOUNT_ON_COORDINATES_SET` with two active items;
     - visible `DEVICE_PORT`, `DEVICE_PORTS` and `MOUNT_SIDE_OF_PIER`;
     - hidden base `MOUNT_GUIDE_RATE`;
     - `MOUNT_TRACKING`, `MOUNT_TRACK_RATE`, `MOUNT_ABORT_MOTION`, `MOUNT_MOTION_RA`, `MOUNT_MOTION_DEC`, `MOUNT_PARK` and `MOUNT_EQUATORIAL_COORDINATES`;
     - custom `CONNECTION_MODE` with `UDP`, `TCP`, `SERIAL` and `SERIAL_DTR`;
     - custom `MOUNT_TYPE` with `G11`, `TITAN`, `EXOS-2` and `iEXOS-100`.
   - Current guider public surface to preserve:
     - `GUIDER_GUIDE_DEC` with `NORTH` and `SOUTH`;
     - `GUIDER_GUIDE_RA` with `EAST` and `WEST`;
     - visible `GUIDER_RATE`;
     - shared connection through the master mount device.
   - Simulator-backed validation currently covers serial mode only. UDP/TCP mode switching and real PMC-Eight hardware behavior still require manual or hardware-backed validation.

2. Extract property and lifecycle inventory
   - List every `indigo_init_*_property()` and `indigo_init_*_item()` call.
   - List every property `count`, `hidden` and `perm` mutation.
   - Record attach-time behavior:
     - default `DEVICE_PORT`;
     - `MOUNT_INFO_VENDOR`;
     - `MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2`;
     - visible `DEVICE_PORT`, `DEVICE_PORTS` and `MOUNT_SIDE_OF_PIER`;
     - hidden `MOUNT_GUIDE_RATE`;
     - custom `CONNECTION_MODE`;
     - custom `MOUNT_TYPE`;
     - additional-instance visibility.
   - Record connect side effects:
     - open serial/UDP/TCP transport;
     - optional DTR clear;
     - firmware/model query;
     - model/rate computation;
     - tracking-rate query;
     - park state reset;
     - coordinate polling start;
     - `MOUNT_TYPE` delete/redefine as read-only.
   - Record disconnect side effects:
     - coordinate polling cancellation;
     - shared open-count decrement;
     - transport close;
     - `MOUNT_TYPE` delete/redefine as writable.
   - Compare the property inventory with `indigo_docs/PROPERTIES.md` and update documentation only if public properties or item counts change.

   Result:
   - Confirmed the driver creates two custom mount properties:
     - `CONNECTION_MODE`, switch, `MAIN_GROUP`, label `Connection mode`, `INDIGO_RW_PERM`, `INDIGO_ONE_OF_MANY_RULE`, 4 items;
     - `MOUNT_TYPE`, switch, `MAIN_GROUP`, label `Mount type`, `INDIGO_RW_PERM` while disconnected, `INDIGO_ONE_OF_MANY_RULE`, 4 items.
   - Confirmed `CONNECTION_MODE` items and defaults:
     - `UDP`, label `UDP`, default selected;
     - `TCP`, label `TCP`;
     - `SERIAL`, label `Serial`;
     - `SERIAL_DTR`, label `Serial (clear DTR)`.
   - Confirmed `MOUNT_TYPE` items:
     - `G11`, label `Losmandy G-11`;
     - `TITAN`, label `Losmandy Titan`;
     - `EXOS-2`, label `Explore Scientific EXOS II`;
     - `iEXOS-100`, label `Explore Scientific iEXOS-100`.
   - Confirmed attach-time base-property mutations:
     - `DEVICE_PORT` default value is `udp://192.168.47.1` and state is set to `INDIGO_OK_STATE`;
     - `MOUNT_INFO_VENDOR` is set to `Explore Scientific`;
     - `MOUNT_ON_COORDINATES_SET_PROPERTY->count = 2`;
     - `DEVICE_PORT_PROPERTY->hidden = false`;
     - `DEVICE_PORTS_PROPERTY->hidden = false`, followed by `indigo_enumerate_serial_ports()`;
     - `MOUNT_GUIDE_RATE_PROPERTY->hidden = true`;
     - `MOUNT_SIDE_OF_PIER_PROPERTY->hidden = false`;
     - `ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL`.
   - Confirmed mount enumeration explicitly defines `CONNECTION_MODE` and `MOUNT_TYPE` before delegating to the mount base enumerator.
   - `CONNECTION_MODE` and `MOUNT_TYPE` are defined on the disconnected mount device as part of normal enumeration. The generator migration must keep them available before connection, not move them to connected-only definition.
   - Confirmed mount connect side effects:
     - lock the master device;
     - call `pmc8_open(device)`;
     - open serial at 115200 baud unless `DEVICE_PORT` is a PMC8 URL;
     - clear DTR when `SERIAL_DTR` is selected;
     - open network transport with default port `54372` for URL connections;
     - query firmware with `ESGv!`;
     - for old firmware, infer model from the firmware string tokens `G11`, `TITAN`, `EXOS2` or `ES1A`;
     - for newer firmware, query `ESGi!` and map type ids 4..7 to G11, 8..11 to EXOS2 and the remaining known case to iEXOS-100;
     - update `MOUNT_INFO_FIRMWARE`, `MOUNT_INFO_MODEL` and the selected `MOUNT_TYPE` item;
     - compute sidereal, lunar and solar rates from the selected model counts;
     - force `MOUNT_PARK` to unparked;
     - query current tracking rate and update `MOUNT_TRACKING` / `MOUNT_TRACK_RATE` state;
     - start position polling;
     - delete and redefine `MOUNT_TYPE` as read-only;
     - set `CONNECTION` to OK or ALERT and restore disconnected switch on failure.
   - Confirmed mount disconnect side effects:
     - cancel position polling synchronously;
     - decrement the shared open count and close the transport only when it reaches zero;
     - delete and redefine `MOUNT_TYPE` as writable;
     - set `CONNECTION` to OK;
     - unlock the master device after delegating the final connection update to the mount base handler.
   - Confirmed mount change handlers with custom behavior:
     - `CONNECTION` copies values, marks BUSY and schedules connect/disconnect work;
     - `MOUNT_PARK` schedules park only on unparked-to-parked transition and publishes unchanged unpark requests;
     - `MOUNT_EQUATORIAL_COORDINATES` rejects changes while parked, preserves current values while copying targets and schedules goto/sync work;
     - `MOUNT_ABORT_MOTION` rejects while parked, otherwise stops both axes and alerts active coordinate slews;
     - `MOUNT_MOTION_DEC` and `MOUNT_MOTION_RA` reject while parked and otherwise schedule manual motion;
     - `MOUNT_TRACKING` and `MOUNT_TRACK_RATE` copy values, mark BUSY and apply the selected PMC-Eight tracking rate;
     - `CONNECTION_MODE` updates defaults while disconnected or schedules mode-switch work while connected;
     - `MOUNT_TYPE` accepts the selected model while disconnected;
     - `CONFIG_SAVE` persists `CONNECTION_MODE` and `MOUNT_TYPE`.
   - Confirmed connection-mode switching behavior while connected:
     - UDP to TCP sends `ESY!` and expects `ESY0`;
     - TCP to UDP sends `ESY!` and expects `ESY1`;
     - TCP to serial sends `ESX!` and expects `ESX0`;
     - serial to TCP sends `ESX!` and expects `ESX1`;
     - UDP to serial and serial to UDP are rejected with user messages and the previous mode restored.
   - Confirmed position polling behavior:
     - reads raw HA/DEC with `ESGp0!` and `ESGp1!`;
     - converts raw counts to RA/DEC using selected model counts, hemisphere, longitude/LST and epoch conversion;
     - updates `MOUNT_SIDE_OF_PIER`, coordinates and `MOUNT_UTC_TIME`;
     - polls every 1 second normally and every 0.5 seconds while `MOUNT_EQUATORIAL_COORDINATES` is BUSY;
     - completes park when raw HA/DEC are near zero while tracking is off and private park state is set.
   - Confirmed guider attach/change behavior:
     - `GUIDER_RATE_PROPERTY->hidden = false`;
     - guider `CONNECTION` shares the master mount transport through `pmc8_open(device->master_device)` and `pmc8_close(device->master_device)`;
     - `GUIDER_GUIDE_RA` cancels any pending RA guide timer, copies values, marks BUSY and starts a blocking timer worker;
     - RA guiding applies a temporary tracking-rate offset, sleeps for the requested duration, restores tracking rate and clears RA guide values;
     - `GUIDER_GUIDE_DEC` cancels any pending DEC guide timer, copies values, marks BUSY and starts a blocking timer worker;
     - DEC guiding starts PMC-Eight axis motion, sleeps for the requested duration, stops that axis and clears DEC guide values;
     - `GUIDER_RATE` copies values and relies on the base property state.
   - Confirmed private/shared state fields to preserve or replace deliberately:
     - raw transport handle;
     - selected model type;
     - three computed tracking rates;
     - position and guider timers;
     - custom property pointers;
     - transport mutex;
     - network protocol marker;
     - private park flag;
     - connection-mode booleans;
     - shared open count;
     - firmware version.
   - Confirmed `indigo_docs/PROPERTIES.md` already records `mount_pmc8` custom properties `CONNECTION_MODE` and `MOUNT_TYPE` plus the currently documented driver-specific existing-property use. No `PROPERTIES.md` update was needed in this step because no source behavior or public property shape changed.

3. Reshape the hand-written driver into generator-friendly sections
   - Keep behavior unchanged in this phase.
   - Reorder the file into clear sections:
     - includes;
     - defines and property macros;
     - model constants;
     - private data;
     - low-level PMC-Eight transport/protocol helpers;
     - mount helpers and handlers;
     - guider helpers and handlers;
     - driver entry point.
   - Move firmware/model parsing and rate computation out of `pmc8_open()` where practical.
   - Preserve the license header, driver entry point, driver label and supported logical device names.
   - Avoid broad formatting churn while moving code.

   Result:
   - Updated the source copyright range to include 2026 and added the repository-required Codex refactor notice after the license header.
   - Confirmed the existing source order was already close to the target generator-friendly shape, so the edit intentionally stayed small and did not move behavior.
   - Added explicit section markers for:
     - driver constants and property macros;
     - model data;
     - private data;
     - PMC-Eight transport and protocol helpers;
     - INDIGO mount device implementation;
     - INDIGO guider device implementation;
     - driver entry point.
   - Left firmware/model parsing and rate computation in `pmc8_open()` for now to avoid mixing code motion with the sectioning-only step; extracting those helpers is still a good follow-up before or during annotation.
   - Built the driver with `make -C indigo_drivers/mount_pmc8 -f ../../Makefile.drv`.
   - Built and ran the PMC-Eight simulator integration test after the source edit. Both test cases passed:
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.

4. Introduce portable transport with `indigo_uni_io`
   - Replace `int handle` with `indigo_uni_handle *handle`.
   - Replace `indigo_open_serial_with_speed()` with the appropriate `indigo_uni_open_serial_*()` helper.
   - Replace `indigo_open_network_device()` usage with `indigo_uni_open_url()` or a local uni-I/O-compatible helper that preserves the existing UDP/TCP URL behavior and default port `54372`.
   - Replace flush/write/read paths in `pmc8_command()` with `indigo_uni_discard()`, `indigo_uni_write()` / `indigo_uni_printf()` and `indigo_uni_read_section*()`.
   - Preserve PMC-Eight response terminators `!`, `%` and `#`.
   - Preserve command retries and the optional post-write microsecond sleep.
   - Use `indigo_uni_close()` in every disconnect/error path and clear stale handles consistently.
   - Keep serial DTR clearing behavior if an equivalent portable hook exists; otherwise isolate the platform-specific part and document the remaining limitation.

   Result:
   - Replaced the raw integer transport handle with `indigo_uni_handle *handle`.
   - Replaced serial open with `indigo_uni_open_serial_with_speed(..., 115200, INDIGO_LOG_DEBUG)`.
   - Preserved `SERIAL_DTR` behavior with `indigo_uni_set_dtr(handle, false)`.
   - Replaced network open with `indigo_uni_open_url(..., 54372, INDIGO_UDP_HANDLE, INDIGO_LOG_DEBUG)`.
   - Preserved existing URL semantics:
     - `tcp://` opens TCP;
     - `udp://` opens UDP;
     - `pmc8://` uses the UDP protocol hint.
   - Added socket read/write timeouts for uni I/O network handles.
   - Reworked `pmc8_command()` in the same style as the iOptron low-level helpers:
     - validate the handle first;
     - discard pending input;
     - format and write the command through varargs with `indigo_uni_vprintf()`;
     - read UDP replies with `indigo_uni_read()`;
     - read serial/TCP replies with `indigo_uni_read_section2()`;
     - accept PMC-Eight terminators `!`, `%` and `#`;
     - store replies in the shared `PRIVATE_DATA->response` buffer;
     - use one cleanup path to release the transport mutex.
   - Removed temporary command-formatting buffers from low-level helpers; formatted commands now use `pmc8_command(device, "...", ...)`.
   - Preserved the existing 10 retry attempts for commands expecting a reply.
   - Replaced transport close with `indigo_uni_close(&PRIVATE_DATA->handle)`.
   - Removed direct transport dependencies and calls from `indigo_mount_pmc8.c`: no remaining direct `select()`, `read()`, `write()`, `send()`, `recv()`, `close()`, `ioctl()`, `indigo_open_serial_with_speed()`, `indigo_open_network_device()` or `indigo_write()` calls.
   - Built the driver with `make -C indigo_drivers/mount_pmc8 -f ../../Makefile.drv`.
   - Built and ran the PMC-Eight simulator integration test. Both test cases passed:
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.
   - Simulator-backed validation still covers serial mode only. UDP/TCP open, command response behavior and runtime connection-mode switching still require manual or hardware-backed validation.

5. Convert worker timers into handler queues
   - Replace zero-delay worker timers with `indigo_execute_handler()` where generated drivers expect queued handlers.
   - Use `indigo_execute_handler_in()` for periodic polling and delayed guide-pulse finalizers.
   - Keep polling behavior equivalent to the current `position_timer_callback`, including faster polling while coordinates are busy.
   - Use priority queue calls for abort and guide-pulse stop/finalizer paths if that improves observable behavior.
   - Ensure each handler sets affected properties to `OK`, `ALERT` or deliberately keeps them `BUSY`.
   - Ensure failed protocol commands cannot leave `CONNECTION`, `MOUNT_EQUATORIAL_COORDINATES`, motion, tracking or guider properties stuck in `BUSY`.

   Result:
   - Replaced immediate mount worker timers with handler-queue dispatch:
     - `CONNECTION` uses `indigo_execute_handler()`;
     - `MOUNT_PARK` uses `indigo_execute_handler()`;
     - `MOUNT_EQUATORIAL_COORDINATES` uses `indigo_execute_handler()`;
     - `MOUNT_MOTION_DEC` and `MOUNT_MOTION_RA` use `indigo_execute_handler()`;
     - `MOUNT_TRACKING` and `MOUNT_TRACK_RATE` use `indigo_execute_handler()`;
     - connected `CONNECTION_MODE` switching uses `indigo_execute_handler()`.
   - `MOUNT_ABORT_MOTION` now uses `indigo_execute_priority_handler(..., INDIGO_TASK_PRIORITY_URGENT, ...)`.
   - Replaced mount position polling timer storage with queued polling:
     - successful mount connect starts polling with `indigo_execute_handler()`;
     - the polling callback reschedules itself with `indigo_execute_handler_in()`;
     - disconnect cancels pending queued mount handlers with `indigo_cancel_pending_handlers()`.
   - Removed `position_timer` from private data.
   - Removed manual `indigo_lock_master_device()` / `indigo_unlock_master_device()` from mount and guider connection handlers because generated-style handler queues already run under the device/master queue mutex. Keeping both caused the simulator-backed connection test to hang.
   - Replaced the immediate guider `CONNECTION` timer with `indigo_execute_handler()`.
   - Left `GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` on their existing zero-delay timers for this step. They still implement blocking pulse sleeps and cancellation/replacement semantics, so they are intentionally deferred to the dedicated guider-pulse conversion step.
   - Built the driver with `make -C indigo_drivers/mount_pmc8 -f ../../Makefile.drv`.
   - Built and ran a clean PMC-Eight simulator integration test. Both test cases passed:
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.

6. Preserve mount behavior in generator-ready handlers
   - Implement `CONNECTION` through generator-compatible `pmc8_open()` and `pmc8_close()`.
   - Implement `CONNECTION_MODE` switching with the same allowed transitions and messages:
     - UDP to TCP through `ESY!`;
     - TCP to UDP through `ESY!`;
     - TCP to serial through `ESX!`;
     - serial to TCP through `ESX!`;
     - reject direct UDP to serial and serial to UDP.
   - Preserve default `DEVICE_PORT` changes when connection mode changes while disconnected.
   - Preserve `MOUNT_TYPE` manual selection while disconnected and read-only behavior while connected.
   - Preserve coordinate conversion across hemisphere, LST and side-of-pier logic.
   - Preserve goto/sync behavior, including stop-tracking before point command and tracking restart after completion.
   - Preserve park-to-zero behavior and automatic transition back to unparked when raw position is near zero with tracking off.
   - Preserve manual motion rates for guide, centering, find and max.
   - Preserve tracking rate computation from selected model counts.

   Result:
   - Split transport open from mount configuration:
     - `pmc8_open()` now only opens serial/UDP/TCP transport and preserves shared open-count behavior;
     - new `pmc8_configure_mount()` performs the firmware/model query retry loop;
     - new `pmc8_detect_mount_type()` owns `ESGv!` / `ESGi!` model detection;
     - new `pmc8_update_mount_model()` owns `MOUNT_INFO_MODEL`, private model type and tracking-rate computation.
   - Preserved failed-connect cleanup semantics:
     - failed transport open still decrements the shared open count inside `pmc8_open()`;
     - failed mount configuration after a successful open closes the transport through `pmc8_close()`.
   - Added a private `configured` flag so either the mount or guider connection path can configure the shared master mount exactly when the transport is first opened.
   - Added `pmc8_update_mount_type_perm()` to keep the disconnected writable / connected read-only `MOUNT_TYPE` lifecycle in one generator-extractable helper.
   - Added `pmc8_update_connection_port()` and reused it for disconnected `CONNECTION_MODE` changes and successful connected mode switches.
   - Preserved user-visible default ports:
     - UDP selects `udp://192.168.47.1`;
     - TCP selects `tcp://192.168.47.1`;
     - serial selects the first enumerated serial port after the base port item when available, otherwise an empty port value.
   - Preserved `CONNECTION_MODE` transition commands and rejection messages.
   - Preserved coordinate conversion, goto/sync, park, manual motion and tracking-rate behavior; this step only isolated mount setup/default helpers.
   - Built the driver with `make -C indigo_drivers/mount_pmc8 -f ../../Makefile.drv`.
   - Built and ran a clean PMC-Eight simulator integration test. Both test cases passed:
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.
   - UDP/TCP connection-mode switching remains a manual or hardware-backed validation item.

7. Preserve guider behavior in generator-ready handlers
   - Keep guider connection shared with the master mount and shared open-count semantics.
   - Keep `GUIDER_RATE` visible and accepted.
   - Convert RA guide pulses from blocking sleeps to start + delayed finalizer while preserving temporary tracking-rate offset behavior.
   - Convert DEC guide pulses from blocking sleeps to start + delayed finalizer while preserving PMC-Eight motion direction mapping.
   - Define deterministic behavior for overlapping pulses:
     - either preserve current cancellation/replacement semantics exactly;
     - or intentionally upgrade to deadline extension, document it, and cover it with a test.
   - Keep guide-pulse command execution serialized through the master device queue.

   Result:
   - Removed the old `indigo_timer *guider_timer_ra` and `indigo_timer *guider_timer_dec` private fields.
   - Replaced blocking guide timer callbacks with generator-ready queued handlers:
     - `guider_guide_ra_handler()` applies the temporary tracking-rate offset and schedules `guider_guide_ra_finalizer()`;
     - `guider_guide_ra_finalizer()` restores the selected tracking rate, clears `EAST` / `WEST` and marks `GUIDER_GUIDE_RA` OK;
     - `guider_guide_dec_handler()` applies north/south PMC-Eight motion and schedules `guider_guide_dec_finalizer()`;
     - `guider_guide_dec_finalizer()` stops DEC motion, clears `NORTH` / `SOUTH` and marks `GUIDER_GUIDE_DEC` OK.
   - New guide pulses cancel any pending start handler for the same axis, and each start handler cancels any pending finalizer for that axis before scheduling the replacement finalizer. This preserves the effective replacement semantics of the old timer-pointer implementation without sleeping on a worker thread.
   - Guider disconnect now cancels pending queued guider work and targeted guide finalizers before closing the shared master transport.
   - `GUIDER_RATE` remains visible and accepted without changing the public property shape.
   - Confirmed no remaining `indigo_set_timer()` / `indigo_cancel_timer()` / `guider_timer_*` use in `indigo_mount_pmc8.c`.
   - `make build/drivers/indigo_mount_pmc8.a` and `make build/drivers/indigo_mount_pmc8` reported the existing targets as up to date, so the edited source was additionally compiled directly to a temporary object with the repository driver include/compile flags; compilation passed.
   - Built the PMC-Eight simulator integration test with `make -C indigo_test build/integration/test_mount_pmc8_simulator`.
   - A first test run from the repository root failed only because the simulator executable path is relative to `indigo_test`.
   - Running `build/integration/test_mount_pmc8_simulator` from `indigo_test` passed:
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.
   - UDP/TCP guide behavior remains covered only by manual or hardware-backed validation because the automated simulator path is serial-only.

8. Add migration annotations
   - Mark additional includes and constants with `//+ include` and `//+ define`.
   - Mark private fields with `//+ data`.
   - Mark shared transport/protocol helpers with `//+ code`.
   - Mark mount-specific helpers with `//+ mount.code`.
   - Mark attach/connect/disconnect/timer bodies with the corresponding mount annotations.
   - Mark custom property setup and all non-empty property changes with property `on_change` annotations.
   - Mark guider attach/connect/disconnect and guide/rate changes with guider annotations.
   - Keep generated mount/guider boilerplate out of annotated custom blocks.

   Result:
   - Added balanced `//+` / `//-` annotation pairs to `indigo_mount_pmc8.c`.
   - Marked additional includes with `include`.
   - Marked driver-private constants, property macros, model types and model table with `define`.
   - Marked shared private-data fields with `data`.
   - Marked shared PMC-Eight transport/protocol/model helpers with `code`.
   - Marked the helper needed by connected `CONNECTION_MODE` switching with `mount.code`.
   - Marked mount lifecycle blocks:
     - `mount.on_attach`;
     - `mount.on_timer`;
     - `mount.on_connect`;
     - `mount.on_disconnect`.
   - Marked mount property custom behavior:
     - `mount.MOUNT_PARK.on_change`;
     - `mount.MOUNT_EQUATORIAL_COORDINATES.on_change`;
     - `mount.MOUNT_ABORT_MOTION.on_change`;
     - `mount.MOUNT_MOTION_DEC.on_change`;
     - `mount.MOUNT_MOTION_RA.on_change`;
     - `mount.MOUNT_TRACKING.on_change`;
     - `mount.MOUNT_TRACK_RATE.on_change`;
     - `mount.CONNECTION_MODE.on_change`;
     - `mount.MOUNT_TYPE.on_change`.
   - `mount.CONNECTION_MODE.on_change` deliberately wraps the full change branch, including disconnected default-port updates, so `CONNECTION_MODE` remains a disconnected-device property during extraction.
   - `mount.MOUNT_TYPE.on_change` deliberately wraps the disconnected model selection branch; connected read-only redefinition remains in `mount.on_connect` / `mount.on_disconnect`.
   - Marked guider lifecycle and helper blocks:
     - `guider.on_attach`;
     - `guider.on_connect`;
     - `guider.on_disconnect`;
     - `guider.code` for guide finalizers.
   - Marked guider property custom behavior:
     - `guider.GUIDER_GUIDE_RA.on_change`;
     - `guider.GUIDER_GUIDE_DEC.on_change`;
     - `guider.GUIDER_RATE.on_change`.
   - Verified annotation balance with a marker count check: 25 open markers and 25 close markers.
   - Rebuilt the driver with `make -C indigo_drivers/mount_pmc8 -f ../../Makefile.drv`.
   - Also compiled the annotated source directly to a temporary object with the repository driver compile flags; compilation passed.
   - Built and ran the PMC-Eight simulator integration test. Both test cases passed:
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.
   - Cleaned `indigo_test` build artifacts with `make -C indigo_test test-clean`.

9. Generate the initial `.driver`
   - Run `indigo_generator -c indigo_mount_pmc8.driver` from `indigo_drivers/mount_pmc8`.
   - Do not pass the `.c` file as the `-c` output argument.
   - Inspect the extracted `indigo_mount_pmc8.driver` by hand.
   - Confirm the `.driver` contains:
     - driver metadata and version;
     - serial connection declaration or the chosen transport representation;
     - custom `CONNECTION_MODE` and `MOUNT_TYPE` definitions;
     - private data fields;
     - `pmc8_open()` and `pmc8_close()`;
     - shared protocol helpers;
     - mount and guider device blocks;
     - all custom property `on_change` blocks.
   - Fix extraction gaps either in annotations or directly in the `.driver`, whichever produces the least churn.

   Result:
   - Extracted the first `indigo_mount_pmc8.driver` with:
     - `../../build/bin/indigo_generator -c indigo_mount_pmc8.driver`;
     - run from `indigo_drivers/mount_pmc8`.
   - Confirmed the extracted file contains driver metadata, version, common data, `indigo_uni_io` transport helpers, `pmc8_open()`, `pmc8_close()`, `pmc8_command(..., ...)`, shared PMC-Eight protocol helpers, mount block and guider block.
   - Reconciled extraction gaps directly in the `.driver`:
     - set mount name to `Mount PMC Eight`;
     - set guider name to `Mount PMC Eight (guider)`;
     - converted extracted `inherited CONNECTION_MODE` to a driver-defined `switch CONNECTION_MODE`;
     - converted extracted `inherited MOUNT_TYPE` to a driver-defined `switch MOUNT_TYPE`;
     - marked both custom switch properties `always_defined = true` so they are enumerated on the disconnected mount device;
     - removed manual `CONNECTION_MODE` / `MOUNT_TYPE` property initialization from `mount.on_attach`, leaving creation to the generator property blocks;
     - removed duplicate custom-property macros and private property pointers from `define` / `data`, leaving those to generated property definitions;
     - removed the hand-written `PRIVATE_DATA` macro from `define`, leaving it to the generator;
     - removed `indigo_property_copy_values(..., property, false)` from generated `on_change` blocks, because generated handlers copy changed values before dispatch;
     - fixed `MOUNT_TYPE` item handles to preserve existing code macros `MOUNT_TYPE_G11`, `MOUNT_TYPE_TITAN`, `MOUNT_TYPE_EXOS2` and `MOUNT_TYPE_IEXOS100`;
     - added missing slew-rate calculation to both `MOUNT_MOTION_DEC` and `MOUNT_MOTION_RA` `on_change` blocks;
     - replaced old `position_timer_callback` references with generated `mount_timer_callback`;
     - replaced old `mount_connect_handler` references with generated `mount_connection_handler`;
     - added a local forward declaration for `mount_connection_handler()` before `mount_switch_connection_handler()`.
   - Validated the `.driver` without overwriting repository generated outputs by copying it to `/private/tmp/pmc8_driver_check` and running `indigo_generator` there.
   - Compiled the temporarily generated `indigo_mount_pmc8.c` to an object with the repository driver compile flags; compilation passed.
   - Verified in the temporarily generated source that `CONNECTION_MODE` and `MOUNT_TYPE` are generated as custom switch properties and enumerated outside the `IS_CONNECTED` guard.
   - The extractor added `indigo_mount_pmc8.driver` to the PMC8 group in `indigo.xcodeproj/project.pbxproj`; this is kept as relevant project-file wiring for the new generator source.
   - No repository `.c`, `.h` or `_main.c` generated outputs were overwritten in this step; that is left for step 10.

10. Regenerate and reconcile outputs
   - Run `indigo_generator indigo_mount_pmc8.driver`.
   - Inspect generated `indigo_mount_pmc8.c`, `.h` and `_main.c`.
   - Compare generated behavior against the hand-written baseline.
   - Remove manual boilerplate no longer owned by the driver source.
   - Keep the checked-in generated outputs synchronized with the `.driver`.
   - Update project/build files only if needed by repository convention.

   Result:
   - Regenerated repository outputs from `indigo_mount_pmc8.driver`:
     - `indigo_mount_pmc8.c`;
     - `indigo_mount_pmc8.h`;
     - `indigo_mount_pmc8_main.c`.
   - Changed the `.driver` connection declaration from the temporary `serial { no_ports = true; }` shape to plain `serial;`.
   - Removed duplicate `DEVICE_PORT_PROPERTY->hidden`, `DEVICE_PORTS_PROPERTY->hidden` and `indigo_enumerate_serial_ports()` code from `mount.on_attach`; the generator now owns the normal serial port visibility and enumeration.
   - Kept the PMC-Eight-specific default `DEVICE_PORT` value `udp://192.168.47.1` in `mount.on_attach`.
   - Reconciled generator connection semantics:
     - `pmc8_open()` / `pmc8_close()` are now called by generated mount/guider connection handlers;
     - `mount.on_connect` and `guider.on_connect` use the generator-provided `connection_result`;
     - `mount.on_disconnect` and `guider.on_disconnect` no longer call `pmc8_close()` directly;
     - `pmc8_close()` normalizes slave guider calls to the master device before touching shared transport state.
   - Removed the obsolete nested `count_open` reference counter. Shared open/close ownership now lives only in the generated `PRIVATE_DATA->count` lifecycle, while `pmc8_open()` and `pmc8_close()` perform only the physical transport open/close.
   - Updated the PMC-Eight simulator integration test to use explicit generated device names instead of removed private header macros `MOUNT_PMC8_NAME` and `MOUNT_PMC8_GUIDER_NAME`.
   - Confirmed the generated public header exposes only `indigo_mount_pmc8()`, matching generated-driver convention.
   - Confirmed generated `CONNECTION_MODE` and `MOUNT_TYPE` remain custom switch properties and are enumerated without an `IS_CONNECTED` guard.
   - Built the generated driver with `make -C indigo_drivers/mount_pmc8 -f ../../Makefile.drv`.
   - Built and ran the PMC-Eight simulator integration test. Both test cases passed:
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.
   - The generator warning `GUIDER_RATE_PROPERTY->hidden set to false` is expected because the driver intentionally keeps `GUIDER_RATE` visible.

11. Validation
   - Build `mount_pmc8` after each major phase.
   - Build and run `test_mount_pmc8_simulator`.
   - Exercise both registered simulator tests:
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.
   - Add simulator coverage if the refactor changes risk around:
     - `CONNECTION_MODE`;
     - `MOUNT_TYPE`;
     - goto/sync;
     - park/unpark;
     - manual motion;
     - RA guide pulses;
     - overlapping guide pulses if semantics are intentionally changed.
   - Validate direct UDP and TCP connection paths with the optional loopback simulator endpoints.
   - Manually validate real-hardware UDP/TCP mode switching because the simulator can cover protocol and transport paths, not the mount's physical WiFi/serial reconfiguration behavior.
   - Document real-hardware assumptions for PMC-Eight-specific behavior not covered by the simulator.
   - Check unload/shutdown for leftover timers, queued work, open handles and shared open-count leaks.

   Result:
   - Removed duplicate connected/disconnected transport log reporting from `pmc8_open()` and `pmc8_close()`. The generated connection handlers now remain the only code reporting normal connection and disconnection outcomes.
   - Kept the protocol initialization failure log in `pmc8_configure_mount()`, because it reports a PMC-Eight initialization failure after the transport is already open.
   - Added `pmc8_mount_defines_custom_properties_while_disconnected` to the PMC-Eight simulator integration test.
   - Confirmed the disconnected mount device defines custom `CONNECTION_MODE` and `MOUNT_TYPE` properties with their expected items before any device connection is attempted.
   - Updated `indigo_docs/PROPERTIES.md` to point the `mount_pmc8` property source at `indigo_mount_pmc8.driver`, with generated output in `indigo_mount_pmc8.c`.
   - Regenerated `indigo_mount_pmc8.c`, `indigo_mount_pmc8.h` and `indigo_mount_pmc8_main.c` from `indigo_mount_pmc8.driver`.
   - Built the generated driver with `make -C indigo_drivers/mount_pmc8 -f ../../Makefile.drv`.
   - Built and ran the PMC-Eight simulator integration test. The initial three compliance-oriented test cases passed:
     - `pmc8_mount_defines_custom_properties_while_disconnected`;
     - `pmc8_mount_passes_serial_compliance_checks`;
     - `pmc8_guider_passes_serial_compliance_checks`.
   - The generator warning `GUIDER_RATE_PROPERTY->hidden set to false` remains expected because the driver intentionally keeps `GUIDER_RATE` visible.
   - Expanded PMC-Eight simulator coverage beyond compliance smoke checks:
     - disconnected `CONNECTION_MODE` changes;
     - connected direct `SERIAL` to `UDP` rejection without dropping the serial connection;
     - direct TCP connection through the loopback PMC-Eight simulator endpoint;
     - direct UDP connection through the loopback PMC-Eight simulator endpoint;
     - `MOUNT_TYPE=AUTO` autodetection and manual `MOUNT_TYPE` override without autodetection;
     - basic mount workflow through unpark, sync, `TRACK` coordinate move, manual RA/DEC motion, abort and park;
     - abort command while the coordinate tracking property is observed in `INDIGO_BUSY_STATE`;
     - guider pulses on both RA and DEC axes.
   - The PMC-Eight driver currently exposes `TRACK` and `SYNC` in `MOUNT_ON_COORDINATES_SET`. A pure `SLEW` item is not part of the current public property shape, so the automated operation test does not claim pure slew coverage.

12. Cleanup
   - Remove obsolete direct POSIX/socket I/O from the driver source where `indigo_uni_io` covers the same behavior.
   - Remove unused includes such as socket, file-descriptor and pthread headers once no longer needed.
   - Keep the simulator and tests separate from driver implementation changes.
   - Update `README.md` only if user-visible connection behavior changes.
   - Update `indigo_docs/PROPERTIES.md` only if properties are added, removed, renamed, hidden/unhidden, permission-changed or item counts change.
   - Run the narrowest relevant cleanup target for generated test artifacts after validation, unless build artifacts are intentionally kept.

   Result:
   - Reduced the custom generator `include` block to `stdarg.h`; the remaining standard and INDIGO headers are supplied by the generator.
   - Confirmed there are no remaining direct POSIX/socket transport calls or obsolete `count_open` / `no_ports` references in the PMC-Eight `.driver` or generated `.c` output.
   - Regenerated `indigo_mount_pmc8.c`, `indigo_mount_pmc8.h` and `indigo_mount_pmc8_main.c` from `indigo_mount_pmc8.driver`.
   - Built the generated driver with `make -C indigo_drivers/mount_pmc8 -f ../../Makefile.drv`.
   - Built and ran the PMC-Eight simulator integration test; all eleven test cases passed.

13. Mount type mode cleanup
   - Added `AUTO` as the default `MOUNT_TYPE` item so the `INDIGO_ONE_OF_MANY_RULE` property has a valid disconnected state.
   - `AUTO` means the driver performs PMC-Eight model autodetection during connection and applies the detected model to `MOUNT_INFO_MODEL`, internal encoder counts and tracking rates.
   - A concrete `MOUNT_TYPE` selection means the driver reads firmware to validate communication, skips model autodetection and uses the selected model for `MOUNT_INFO_MODEL`, internal encoder counts and tracking rates.
   - The property remains writable while disconnected and read-only while connected.
   - Updated `indigo_docs/PROPERTIES.md` to document the new `AUTO` item and changed item count.
   - Added simulator coverage for disconnected default `MOUNT_TYPE=AUTO`, AUTO-driven EXOS2 detection that leaves the AUTO item selected, and manual `MOUNT_TYPE=G11` against a simulator reporting EXOS2 to prove the manual selection is not overwritten by autodetection.

## Suggested milestones

1. Baseline build and simulator test results recorded.
2. Property/lifecycle inventory completed.
3. Hand-written driver reordered into generator-friendly sections with no behavior change.
4. Transport migrated to `indigo_uni_io` while preserving serial, UDP and TCP behavior.
5. Mount work moved to handler queues.
6. Guider pulses moved to start/finalizer handlers.
7. Migration annotations added and verified.
8. `indigo_mount_pmc8.driver` extracted and reviewed.
9. Generated `.c`, `.h` and `_main.c` build cleanly.
10. Simulator-backed regression pass completed, with UDP/TCP/manual hardware assumptions documented.

## Decisions

- Preserve serial, UDP and TCP as first-class user-visible connection modes.
- Preserve the current `CONNECTION_MODE` property instead of folding it into `DEVICE_PORT` alone.
- Preserve the current `MOUNT_TYPE` property and its disconnected writable / connected read-only lifecycle.
- Use the master mount device queue to serialize mount and guider work.
- Migrate direct transport I/O to `indigo_uni_io` as part of the refactor.
- Treat `indigo_uni_io` as the target transport layer for serial, UDP and TCP paths, not just as a cleanup after generator migration.
- Keep the existing simulator-backed integration test as the primary hardware-free regression check.
