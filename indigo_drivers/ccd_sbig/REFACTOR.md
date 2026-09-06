# Refactoring plan for INDIGO 3.0 SBIG CCD driver

Goal: refactor the hand-written SBIG driver to INDIGO 3.0 APIs, handler queues and Windows-capable platform boundaries while preserving its current SBIG-specific behavior. The result should follow the same file structure, naming style, lifecycle flow and property-change conventions as generated INDIGO 3.0 drivers, but the source remains hand-written. This driver must not be migrated to `indigo_generator`: it combines the SBIG Universal Driver handle model, USB and Ethernet discovery, primary CCD, guider CCD, ST-4 guider, filter wheel and adaptive optics devices in ways that are too specific for generator ownership.

## Reference material

- Use `indigo_drivers/ccd_sbig/indigo_ccd_sbig.c` as the behavioral reference.
- Use `indigo_drivers/ccd_sbig/README.md` for supported devices, platform notes, legacy wheel/AO behavior and simulator references.
- Use `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` and `indigo_docs/DEVELOPMENT.md` for INDIGO 3.0 handler queues, property states, lifecycle and non-blocking driver entry points.
- Use `indigo_docs/MAKEFILES.md` and the existing Windows project patterns in nearby drivers when adding Windows build support.
- Use `indigo_docs/PROPERTIES.md` only if the public property surface changes. Property additions/removals must be documented in the same change.
- Use generated C outputs such as `indigo_drivers/wheel_asi/indigo_wheel_asi.c` and the refactored `indigo_drivers/ccd_sx/indigo_ccd_sx.c` as style references for section order, handler names, connection handlers and change-property conventions.
- Use `indigo_drivers/ccd_sx/REFACTOR.md` only as a process reference for staged queue migration and validation discipline, not as an instruction to generate this driver.
- Use the bundled SBIG Universal Driver headers and documentation under `bin_externals/sbigudrv/` as the SDK command reference.

## Current public behavior to preserve

- Driver identity:
  - entry point `indigo_ccd_sbig`;
  - `DRIVER_NAME` value `indigo_ccd_sbig`;
  - label `SBIG Camera`;
  - hot-plug flag in `SET_DRIVER_INFO`.
- Physical camera discovery:
  - USB discovery through the SBIG Universal Driver `CC_QUERY_USB2` result and libusb hot-plug events for vendor id `0x0d97`;
  - Ethernet device placeholder named `SBIG Ethernet Device`;
  - manual Ethernet connection through `DEVICE_PORT` as an IP address or hostname;
  - multiple physical cameras through the existing `devices[]` registry and per-camera shared private data.
- Logical device topology per physical camera:
  - primary CCD device;
  - guider/ST-4 logical device;
  - secondary guider CCD device when `CCD_INFO_TRACKING` is available;
  - optional filter wheel device, auto-detected when possible;
  - optional AO device, auto-detected when possible;
  - legacy CFW6A/CFW8 and AO-7 devices exposed through `SBIG_ADD_WHEEL` and `SBIG_ADD_AO`.
- Shared-handle semantics:
  - one SBIG physical camera private-data object is shared by all logical devices belonging to that camera;
  - SBIG driver/device open is reference-counted by `count_open`;
  - `driver_handle` is set before every SDK command that targets a specific physical device;
  - `global_handle` is used only for enumeration and initial plug probing.
- CCD behavior:
  - RAW 16-bit imaging;
  - primary and secondary CCD exposure paths;
  - secondary CCD shutter handling while a primary exposure is active;
  - wait-for-secondary logic before starting a primary exposure so the secondary CCD cannot close the shutter during the primary exposure;
  - explicit `CC_END_EXPOSURE`, `CC_START_READOUT`, per-line `CC_READOUT_LINE` and `CC_END_READOUT`;
  - Bayer metadata behavior for color cameras at 1x1 binning;
  - frame constraints: equal binning modes, width alignment to 8 pixels, height alignment to 2 pixels, and minimum 64x64 binned readout;
  - exposure abort and cleanup behavior;
  - cooler, temperature and TEC-freeze behavior.
- Guider behavior:
  - relay map read/modify/write behavior;
  - independent RA and DEC pulse durations;
  - `RELAY_MAX_PULSE` limit and current relay bit mapping.
- Filter wheel behavior:
  - auto wheel open/query/close probe;
  - legacy wheel manual add/remove properties;
  - filter count and current-slot handling, including wheels that report position 0;
  - polling until motion becomes idle.
- AO behavior:
  - auto AO probe through CCD capability bits;
  - manual AO add/remove for non-auto-detected AO;
  - `CC_AO_TIP_TILT` and `CC_AO_CENTER` behavior.
- Unsupported Apple Silicon behavior:
  - current `INDIGO_UNSUPPORTED_ARCH` path for `__APPLE__ && __arm64__` must remain unless the SBIG SDK support story changes.

## Target shape

- Keep a hand-written `indigo_ccd_sbig.c`, `indigo_ccd_sbig.h` and `indigo_ccd_sbig_main.c`; do not introduce `indigo_ccd_sbig.driver`.
- Make `indigo_ccd_sbig.c` look like a generated INDIGO 3.0 driver even though it is maintained manually:
  - license header and version history;
  - `#pragma mark - Includes`;
  - `#pragma mark - Common definitions`;
  - `#pragma mark - Property definitions`;
  - `#pragma mark - Private data definition`;
  - `#pragma mark - Low level code`;
  - `#pragma mark - High level code (ccd)`;
  - `#pragma mark - Device API (ccd)`;
  - matching high-level and device-API sections for guider, Ethernet placeholder, wheel and AO;
  - `#pragma mark - Hot-plug and driver registry`;
  - `#pragma mark - Driver entry point`.
- Use generated-driver naming conventions throughout:
  - `DRIVER_VERSION`, `DRIVER_NAME`, `DRIVER_LABEL` and device-name format macros near the top;
  - `PRIVATE_DATA` and property/item macros grouped under property definitions;
  - low-level helpers named `sbig_open(indigo_device *device)` and `sbig_close(indigo_device *device)` for the physical camera connection;
  - logical-device handlers named `ccd_connection_handler`, `ccd_exposure_handler`, `ccd_exposure_finalizer`, `ccd_temperature_poll_handler`, `guider_connection_handler`, `guider_guide_ra_handler`, `wheel_connection_handler`, `wheel_slot_handler`, `ao_connection_handler`, etc.;
  - device callbacks named `ccd_attach`, `ccd_enumerate_properties`, `ccd_change_property`, `ccd_detach`, and likewise for `guider`, `wheel`, `ao` and `eth`.
- Route property changes to INDIGO 3.0 handler queues:
  - change callbacks copy/validate properties, update BUSY state when appropriate, and dispatch work;
  - ordinary asynchronous branches use `INDIGO_COPY_VALUES_PROCESS_CHANGE()` or an equivalent hand-written generated-style sequence;
  - synchronous validation branches use `INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE()` or the same generated-style pattern;
  - time-critical branches use `INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE()` or explicit `indigo_execute_priority_handler*()` calls;
  - hardware work runs on the device queue or on a deliberately documented driver-level queue;
  - time-sensitive finalizers use priority handlers;
  - abort and guide relay stop work use urgent or time-priority handlers.
- Keep change callbacks small, like generated code: they should not contain SBIG SDK calls, long loops, attach/detach work, readout work or blocking waits.
- Treat generated-driver lifecycle as the manual template:
  - attach allocates/initializes properties and immediately enumerates;
  - enumerate defines only matching connected-time custom properties, then delegates to the base class;
  - connection handlers own open/close, define/delete of custom connected properties, messages and final connection-state update;
  - disconnect begins with `indigo_cancel_pending_handlers(device)` plus targeted cancellation of finalizers that must not survive close;
  - detach forces disconnect when needed, releases driver-owned properties, logs and delegates to the base detach.
- Keep custom code visually separated with generated-like block comments such as `//+ ccd.on_connect` / `//- ccd.on_connect` where that helps future comparison with generator conventions. These comments are organizational only; no generator will consume them.
- Preserve one serialized SBIG SDK command path per physical camera. The queue migration must not introduce concurrent SDK access to a shared SBIG handle.
- Replace POSIX-only dependencies with portable INDIGO or platform-wrapped helpers:
  - direct sleeps become `indigo_usleep()`;
  - raw networking includes and address resolution are isolated behind a portable helper;
  - `pthread` use is reduced to INDIGO-provided synchronization or kept behind a small compatibility layer only where unavoidable;
  - Windows builds do not see Unix-only headers such as `unistd.h`, `sys/time.h`, `netdb.h`, `sys/socket.h` or `arpa/inet.h`.
- Add Windows build integration only after the source is compile-clean behind `INDIGO_WINDOWS`.
- Keep driver entry point fast. SBIG SDK initialization/probing that can block must be queued or otherwise kept out of long synchronous bus paths.

## Step-by-step plan

1. Establish the baseline
   - Record workspace status before touching files.
   - Build the current `ccd_sbig` target on the current platform and record compiler warnings.
   - Capture current source line count, direct POSIX includes, all `pthread_*` uses, all timers, all `sbig_command()` call sites and all device registry mutations.
   - Inventory every `indigo_init_*_property()`, `indigo_init_*_item()`, `PROPERTY->count` and `PROPERTY->hidden` mutation and compare it with `indigo_docs/PROPERTIES.md`.
   - Confirm no `.driver` source exists and record explicitly that this refactor stays hand-written.

2. Write a behavior map before changing code
   - Document all logical devices created from one physical SBIG camera and their shared private-data fields.
   - Map the SBIG SDK command sequences for:
     - driver open/close;
     - device open/establish-link/close;
     - USB enumeration;
     - Ethernet plug;
     - primary exposure;
     - secondary CCD exposure;
     - readout;
     - abort;
     - cooling and temperature polling;
     - relay guide pulses;
     - filter wheel open/query/goto/close;
     - AO center and tip/tilt.
   - Mark which sequences must be serialized with each other and which can be delayed or cancelled.
   - Record known quirks such as the `CC_CLOSE_DEVICE` workaround, legacy wheel/AO manual add properties, and secondary CCD shutter protection.

3. Restructure the source without behavioral changes
   - Preserve the license header and update the copyright year range only if code is actually refactored.
   - Add a short "Refactored by Codex" notice after the license header when the first automated refactor patch lands.
   - Reorder the file into generated-driver-style `#pragma mark` sections:
     - includes;
     - common definitions;
     - property definitions;
     - private data definition;
     - low-level code;
     - high-level code per logical device;
     - device API per logical device;
     - hot-plug/registry;
     - driver entry point.
   - Introduce `DRIVER_LABEL` and device-name format macros near `DRIVER_NAME`, matching generated output style.
   - Move property and item macros together, including `SBIG_FREEZE_TEC`, `SBIG_ABG`, `SBIG_ADD_WHEEL` and `SBIG_ADD_AO`.
   - Put all `INDIGO_DEVICE_INITIALIZER()` templates near the matching generated-style device API section.
   - Rename confusing comments and local helper names only where it moves the code toward generated conventions.
   - Fix spelling in user-facing strings only if intentionally accepted as a behavior change; otherwise leave strings stable during this step.
   - Build after this purely mechanical restructuring.

4. Introduce a small SBIG SDK wrapper layer
   - Centralize `set_sbig_handle()` plus `sbig_command()` into helpers that take the target private data or explicit handle.
   - Return both SDK result codes and printable error strings consistently.
   - Make ownership explicit for `global_handle` versus per-camera `driver_handle`.
   - Keep the current recursive locking behavior until queue serialization has replaced the call paths that require it.
   - Add assertions and defensive checks for `PRIVATE_DATA`, `driver_handle`, `DEVICE_CONNECTED` and disconnected/unplugged states.
   - Keep wrapper calls out of change callbacks; wrappers should be used only from low-level helpers, lifecycle handlers and property handlers.

5. Define queue ownership and lock boundaries
   - Use the master CCD device queue as the serialized hardware queue for all logical devices attached to the same physical camera.
   - Keep a driver-level queue or mutex only for the global device registry and hot-plug attach/detach operations.
   - If a driver-level queue is introduced, name it `driver_queue` and protect scheduling with `driver_queue_mutex`, matching generated hot-plug drivers.
   - Replace `indigo_set_timer(device, 0, ...)` connection dispatch with `indigo_execute_handler(device, ...)`.
   - Replace timed operation timers with `indigo_execute_handler_in()` or `indigo_execute_priority_handler_in()`.
   - Do not remove `driver_mutex` until every SDK command path is proven to run on the intended queue or under a documented registry lock.
   - Prefer generated-style queue dispatch for first physical open from a hot-plug/registry queue, and ordinary device-queue dispatch for already-open shared logical devices.

6. Convert CCD connection lifecycle
   - Rename `ccd_connect_callback()` to `ccd_connection_handler()` and make it follow generated connection-handler shape.
   - Keep `indigo_lock_master_device()` around shared logical-device connect/disconnect transitions until queue behavior is verified.
   - On connect, preserve all camera-info queries, dynamic `CCD_MODE` construction, frame/binner limits, cooler visibility, INFO fields and image-buffer allocation.
   - On disconnect, cancel queued exposure/readout/temperature work before closing the SBIG device.
   - Convert connection-property update code to the INDIGO 3.0 pattern: copy request, mark BUSY, queue handler, terminal update from the handler.
   - Publish generated-style connection messages with `indigo_send_message(device, OK_PROPERTY, ...)` or `ALERT_PROPERTY` where appropriate.
   - Keep custom connected-time property definition/deletion inside the connection handler, as generated drivers do.
   - Verify that failed connection restores the disconnected switch and does not leak the shared open count.

7. Convert exposure start, completion and abort
   - Rename exposure start code to `ccd_exposure_handler()` and keep the change branch to generated-style dispatch only.
   - Preserve `indigo_use_shortest_exposure_if_bias()`, dark shutter rules and the primary/secondary CCD shutter interlock.
   - Replace the blocking `while (!sbig_exposure_complete()) usleep(...)` loop with a non-blocking finalizer contract:
     - image pending;
     - image downloaded;
     - exposure/readout failed.
   - Schedule exposure completion with `indigo_execute_priority_handler_in(..., INDIGO_TASK_PRIORITY_TIME, ...)`.
   - If the SBIG API has no reliable timer query for all models, poll `CC_QUERY_COMMAND_STATUS` from a short delayed finalizer instead of sleeping on the queue.
   - Keep long readout itself serialized, but measure whether large sensors block urgent abort/guide commands for too long.
   - Rename abort work to `ccd_abort_exposure_handler()` and dispatch it with urgent priority; it must cancel pending finalizers and call the standard CCD abort cleanup.
   - Ensure TEC freeze is always reverted after readout failure paths.

8. Convert cooling and temperature polling
   - Replace temperature timers with a periodic queue handler.
   - Preserve the current `TEMP_CHECK_TIME`, target/current temperature behavior and cooler-power updates.
   - Keep temperature polling disabled during exposure/readout where the current driver suppresses it.
   - Make cooler and target-temperature property changes queue work instead of touching hardware in the change callback.
   - Use generated-style names `ccd_cooler_handler()` and `ccd_temperature_handler()`.
   - On disconnect, cancel pending temperature handlers before releasing buffers or closing the SBIG handle.

9. Convert guider relay control
   - Move RA and DEC guide-pulse start work into `guider_guide_ra_handler()` and `guider_guide_dec_handler()`.
   - Use independent `guider_guide_ra_finalizer()` and `guider_guide_dec_finalizer()` so one axis ending cannot accidentally stop the other axis.
   - Schedule guide-pulse stops with time-priority or urgent priority.
   - Preserve the existing relay map semantics and `RELAY_MAX_PULSE`.
   - Fix any timer-pointer mistakes found during the migration only with explicit notes in the commit or result log.
   - Verify that guide pulses still serialize with CCD readout and wheel/AO commands on the shared physical camera.

10. Convert filter wheel work
   - Rename wheel connect/disconnect work to `wheel_connection_handler()`.
   - Move `WHEEL_SLOT` changes into `wheel_slot_handler()`.
   - Replace wheel polling timer with delayed queue polling in `wheel_slot_finalizer()` or `wheel_move_finalizer()`.
   - Preserve legacy CFW6A/CFW8 manual add/remove behavior and the current filter counts.
   - Keep the position-0 fallback for wheels that do not report their current filter.
   - Ensure wheel disconnect closes the CFW device before the shared SBIG camera handle is closed.

11. Convert AO work
   - Rename AO connection work to `ao_connection_handler()`.
   - Move AO guide and reset commands into `ao_guide_ra_handler()`, `ao_guide_dec_handler()` and `ao_reset_handler()`.
   - Preserve manual AO add/remove behavior for non-auto-detected AO devices.
   - Preserve `ao_x_deflection` and `ao_y_deflection` state across RA and DEC requests.
   - Reset both deflection values after center/unjam as the current driver does.
   - Confirm whether AO commands need the SBIG handle set explicitly before every command; if yes, route them through the SDK wrapper.

12. Make hot-plug and device registry robust
   - Serialize libusb arrival/removal, `devices[]` changes and shared private-data frees with a driver-level queue or a small registry mutex.
   - Remove the currently unused/commented `hotplug_mutex` only after registry serialization is explicit.
   - Avoid detaching/freeing logical devices while queued work can still run against their private data.
   - On unplug, cancel queued work for all logical devices of the physical camera before freeing image buffers and private data.
   - Preserve USB and Ethernet removal paths separately, because Ethernet is user-triggered through the placeholder device rather than libusb hot-plug.
   - Make the hot-plug warning path use the INDIGO 3.0 `indigo_send_message()` signature with an explicit state property.
   - Keep manual hot-plug code organized like generated hot-plug output: registry state first, match/probe helpers next, plug/unplug helpers next, callback/driver-queue dispatch last.

13. Add Windows portability in the source
   - Guard or remove Unix-only includes from the Windows compilation path.
   - Replace `usleep()` with `indigo_usleep()` everywhere.
   - Replace raw hostname resolution with a helper that builds on INDIGO portable I/O or a minimal `INDIGO_WINDOWS`/POSIX split.
   - Confirm `INVALID_HANDLE_VALUE`, SBIG handle types and format specifiers compile cleanly on 32-bit and 64-bit Windows.
   - Ensure libusb hot-plug code is compiled only where the Windows dependency is present and linked.
   - Keep the Apple Silicon unsupported branch independent from Windows support.

14. Add Windows build integration
   - Add `ccd_sbig` Visual Studio project files only when the SBIG SDK import library/header layout is known.
   - Include the driver in `indigo_windows.sln` and any relevant Windows build lists without broad project-file reformatting.
   - Decide whether the SBIG SDK DLL/import library should live under `bin_externals/sbigudrv/lib/Windows/` or be documented as an external prerequisite.
   - Document missing proprietary Windows SDK artifacts clearly if they cannot be committed.
   - Build with `msbuild indigo_windows.sln /t:Build /p:Configuration=Release` when a Windows environment is available.

15. Validation matrix
   - Current-platform build:
     - narrow `ccd_sbig` driver build;
     - full driver build if the narrow build passes.
   - Static checks:
     - `git diff --check`;
     - warnings for format strings, unreachable branches and unused variables;
     - search for remaining `usleep`, raw Unix networking includes and direct hardware work in change callbacks.
   - Simulator validation:
     - SBIG Ethernet simulator `EthSim2.exe` from the SBIG Windows Dev Kit, using the driver README reference;
     - Ethernet connect/disconnect/reconnect;
     - light, dark and bias exposures;
     - abort during exposure and during pending readout where possible.
   - Hardware validation:
     - USB plug/unplug while idle;
     - USB unplug while connected;
     - USB unplug during exposure/readout;
     - primary CCD exposure while secondary CCD activity is pending;
     - secondary guider CCD exposure while primary exposure is running;
     - 1x1, 2x2, 3x3 and 9x9 modes where supported;
     - subframes and minimum-frame clamping;
     - cooler on/off, target temperature and TEC freeze;
     - ST-4 RA and DEC guide pulses, including simultaneous/overlapping axes;
     - auto-detected wheel and legacy CFW6A/CFW8 manual add/remove;
     - wheel move, position-0 wheel behavior and disconnect during motion;
     - auto-detected AO and manual AO-7 add/remove;
     - AO guide, center and unjam.
   - Windows validation:
     - compile with `INDIGO_WINDOWS`;
     - load the driver in a Windows INDIGO server build;
     - connect to `EthSim2.exe`;
     - verify DLL deployment and runtime SDK discovery.

16. Documentation and cleanup
   - Update `indigo_drivers/ccd_sbig/README.md` only with verified support changes or new Windows setup requirements.
   - Update `indigo_docs/PROPERTIES.md` only if properties are added, removed, renamed or moved between source files.
   - Remove obsolete timer fields, dead mutexes, commented-out lock code and unused helper prototypes after the queue migration.
   - Keep temporary diagnostic logging out of the final diff unless it is useful operational logging.
   - Run a final style audit against generated-driver conventions:
     - section order;
     - handler naming;
     - small change callbacks;
     - connection handler shape;
     - pending-handler cancellation on disconnect;
     - property macros grouped near the top;
     - no hardware calls in change callbacks.
   - Finish with a focused diff review and a clear list of unvalidated hardware assumptions.

## Open questions to answer during implementation

- Does the current SBIG Windows SDK expose the same `SBIGUnivDrvCommand` ABI and header names as the bundled Unix/macOS SDK?
- Can the Windows SDK redistributables be committed to `bin_externals`, or must they remain documented external prerequisites?
- Is libusb hot-plug still the right trigger on Windows, or should Windows rely on SBIG SDK enumeration plus manual rescan?
- Which SBIG models support a reliable exposure-status query that lets the finalizer avoid any blocking wait?
- Should `SBIG_ADD_WHEEL` and `SBIG_ADD_AO` remain visible while disconnected, or only on the connected primary CCD as today?
- Are 9x9 modes still valid for all models where the existing `sbig_get_bin_mode()` maps them, or should mode construction be entirely SDK-info driven?

## Non-goals

- Do not migrate this driver to `indigo_generator`.
- Do not create or maintain `indigo_ccd_sbig.driver`; generator-like structure is a manual coding convention for this driver.
- Do not remove Ethernet support.
- Do not remove legacy CFW6A/CFW8 or AO-7 manual add support.
- Do not redesign the public device topology.
- Do not change property names, item names, labels or persistence unless required by a proven bug fix.
- Do not claim Windows runtime support until the driver is built and exercised on Windows with the SBIG SDK.
