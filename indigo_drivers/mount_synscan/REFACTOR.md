# Greenfield plan for INDIGO 3.0 SynScan mount driver

Goal: write a new SynScan mount driver in INDIGO 3.0 style, using `indigo_uni_io`, device async handler queues and a code structure that can be migrated to `indigo_generator` in the next step. The final hand-written driver should live in `indigo_mount_synscan.c`.

## Reference material

- Treat the old SynScan driver as a behavioral reference and source of protocol examples, not as code to mechanically refactor.
- Old split source files may be renamed or moved aside during development so they do not interfere with the new single-file driver.
- Use `skywatcher_motor_controller_command_set.pdf` as the authoritative reference for SynScan motor-controller commands whenever old-driver behavior is unclear or incomplete.
- Use `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` for INDIGO 3.0 driver structure, `indigo_uni_io` and async handler queues.
- Use `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` for generator-friendly structure and future `.driver` migration.
- The existing simulator and integration test are valuable and must remain the main hardware-free validation path.
- `indigo_mount_ioptron` is a good nearby model for generated 3.0 mount + guider structure, `indigo_uni_handle *handle`, `indigo_execute_handler*()` usage and `//+ ... //- ...` migration annotations.

## Target shape

- One source file: `indigo_mount_synscan.c`.
- Public header and standalone main remain only if required by the build convention.
- Private data stores `indigo_uni_handle *handle`, mount state, protocol cache, custom properties and guide pulse state.
- Hardware communication is isolated in a small low-level section:
  - open/close helpers named for future generator migration, e.g. `synscan_open()` and `synscan_close()`;
  - command helpers using `indigo_uni_discard()`, `indigo_uni_write()` or `indigo_uni_printf()`, `indigo_uni_read_section*()`, `indigo_uni_open_serial_with_speed()`, `indigo_uni_open_url()` or `indigo_uni_client_udp_socket()` as appropriate;
  - no direct `read()`, `write()`, `send()`, `recv()`, `select()`, `open()` or platform socket setup in driver communication paths.
- UDP autodetection remains supported in the new driver, not deferred to a later patch.
- INDIGO requests do not perform slow work on the bus callback. They copy values, set properties BUSY when needed, update clients and schedule queue handlers.
- Polling, connect/disconnect, slews, parking, autohome, tracking updates and guide-pulse finalizers run through `indigo_execute_handler()`, `indigo_execute_handler_in()` or priority variants.
- Mount and guider work is serialized through the master mount device queue.
- Urgent operations such as abort and guide pulse finalizers use priority handlers where that changes observable behavior.
- The code is organized into sections matching generator extraction:
  - includes;
  - common definitions;
  - property definitions;
  - private data;
  - low-level protocol code;
  - mount device code;
  - guider device code;
  - driver entry point.
- Optional migration annotations can be added while refactoring:
  - `//+ define`, `//+ data`, `//+ code`;
  - `//+ mount.on_attach`, `//+ mount.on_connect`, `//+ mount.on_disconnect`, `//+ mount.on_timer`, property `on_change` blocks;
  - matching guider blocks.

## Step-by-step plan

1. Define the new-driver contract
   - Record current build status of `mount_synscan`.
   - Run the existing SynScan simulator-backed integration test, or document why it cannot be run locally.
   - Save a list of currently defined driver properties and non-standard behavior so the new driver preserves the public surface intentionally.
   - Define the minimum supported feature set for the first greenfield version: connect/disconnect, mount model/capability detection, coordinate readout, goto, sync, tracking, abort, manual motion, park/unpark, autohome where supported, guider pulses, serial connection and UDP connection/autodetection.

2. Extract requirements from references
   - Read the old driver only to identify expected behavior, edge cases and existing property names/items.
   - Read `skywatcher_motor_controller_command_set.pdf` for command syntax, axis commands, reply format, feature bits, error replies and timing expectations.
   - Map old source files to reference topics:
     - `_protocol.c`: examples of SynScan command encoding/decoding.
     - `_driver.c`: current connection behavior, UDP autodetect and park file semantics.
     - `_mount.c`: current mount property behavior and state transitions.
     - `_guider.c`: current guide-pulse behavior.
     - `_private.h`: current state cache and custom property names.
   - List all `indigo_init_*_property()`, `indigo_init_*_item()`, property `count` and `hidden` changes.
   - Compare that list with `indigo_docs/PROPERTIES.md` and update documentation only if property behavior changes.

3. Build the greenfield skeleton
   - Start from a clean `indigo_mount_synscan.c` layout, not by folding old files together.
   - Define includes, constants, property handles, private data, low-level protocol helpers, mount handlers, guider handlers and driver entry point in generator-friendly order.
   - Keep old files available during development as reference material until the new source is complete and building. They may be renamed with an `old_` prefix or moved into a clearly named reference/archive location.
   - Keep public declarations in `indigo_mount_synscan.h` and standalone main support according to repository build conventions.

4. Implement transport with `indigo_uni_io`
   - Store `indigo_uni_handle *handle` in private data from the start.
   - Implement serial open with `indigo_uni_open_serial_with_speed()` or `indigo_uni_open_serial_with_config()` according to the selected new-driver contract.
   - Implement network/URL open with `indigo_uni_open_url()` or the appropriate UDP uni I/O helper while preserving both explicit `synscan://host:port` and `synscan://` UDP autodetection behavior.
   - Implement flush/read/write helpers with `indigo_uni_discard()`, `indigo_uni_write()` / `indigo_uni_printf()` and `indigo_uni_read_section2()`.
   - Use `indigo_uni_close()` in every disconnect/error path and clear stale handles consistently.
   - Convert park-position persistence to `indigo_uni_open_file()`, `indigo_uni_create_file()`, `indigo_uni_read()` / `indigo_uni_write()` and `indigo_uni_close()`.
   - Keep one serialized protocol path through the master queue. Avoid introducing command-level locking unless a temporary bring-up issue proves it is needed.

5. Implement async device behavior
   - Implement connection and disconnection as queue handlers from the beginning.
   - Implement mount polling with `indigo_execute_handler_in()`.
   - Implement slew, tracking-rate update, manual slew, park, unpark, home and autohome actions as queue handlers.
   - Implement property-change branches with `INDIGO_COPY_VALUES_PROCESS_CHANGE*()` or equivalent explicit queue scheduling.
   - Ensure every handler has a single responsibility and leaves the affected property in `OK`, `ALERT`, or rescheduled `BUSY` state.
   - Use priority queue calls for abort/motion-stop and guide pulse finalizers.

6. Implement guide pulse handling
   - On pulse request, start motion in the guider handler and schedule an urgent finalizer after the requested duration.
   - Extend overlapping guide pulses deterministically instead of replacing them or returning BUSY/ALERT.
   - Track per-axis guide-pulse deadlines so a new pulse can extend the active pulse and stale finalizers cannot stop a later extended pulse.
   - Keep mount and guider access serialized through the master device queue, so both logical devices cannot interleave SynScan commands.

7. Complete the single-file implementation
   - Keep all new implementation code in `indigo_mount_synscan.c`.
   - Use old `_protocol.c`, `_driver.c`, `_mount.c`, `_guider.c` and `_private.h` only as references while recreating needed behavior.
   - Remove private/internal headers from the build once the new driver no longer depends on them.
   - Preserve license header, driver entry point, driver label and supported device behavior.
   - Update makefile source lists if the folder currently builds multiple SynScan object files.

8. Prepare for generator migration
   - Align helper naming with generator expectations: `<driver_name>_open` / `<driver_name>_close` if the generator step will consume this file directly.
   - Keep custom constants in a contiguous `define` block.
   - Keep extra private fields in a contiguous `data` block.
   - Keep shared protocol helpers independent of attach/change-handler boilerplate.
   - Keep each property's custom attach and change code small enough to become a `.driver` `on_attach` or `on_change` block.
   - Avoid cross-file macros and hidden dependencies that the generator cannot extract.

9. Validation
   - Build `indigo_mount_synscan` after each major phase.
   - Run simulator-backed tests against serial PTY mode.
   - Manually validate URL mode, including explicit `synscan://host:port` and `synscan://` UDP autodetection.
   - Exercise connection/disconnection cycles, failed connection, abort during slew, parking/unparking, persisted park position read/write, manual slews, tracking rate changes and guide pulses.
   - Exercise overlapping guide pulses and verify that the active pulse is extended and stopped only at the latest requested deadline.
   - Check for stuck BUSY properties after failed protocol commands.
   - Check that unload/shutdown leaves no queued work, open handle, timer or thread behind.

10. Cleanup
   - Remove obsolete object/source references and unused includes.
   - Remove or archive old split SynScan source files after the new single-file driver builds and tests pass.
   - Remove avoidable direct POSIX I/O from the driver source.
   - Keep simulator sources and tests separate; do not fold them into the driver.
   - Update `README.md` only if user-visible connection behavior changes.
   - Update `indigo_docs/PROPERTIES.md` only if properties are added, removed, renamed, hidden/unhidden or item counts change.

## Suggested milestones

1. Clean 3.0 skeleton in `indigo_mount_synscan.c` with attach/detach and property definitions.
2. Transport layer implemented with `indigo_uni_io`, including serial, explicit UDP URL and UDP autodetection.
3. Mount behavior implemented on the master queue.
4. Guider behavior implemented on the master queue with extendable guide pulses.
5. Park-position persistence implemented with `indigo_uni_io`.
6. Generator-ready layout and annotations.
7. Simulator-backed regression pass.

## Decisions

- Preserve UDP autodetection in the new driver.
- Use the master mount device queue to serialize mount and guider work.
- Extend overlapping guide pulses.
- Migrate park-position persistence to `indigo_uni_io` as part of the refactor.

## Step 1 results

Date: 2026-09-01

Baseline build:

- `build/drivers/indigo_mount_synscan.a` is present and up to date.
- `make build/drivers/indigo_mount_synscan.a` completed successfully with no rebuild required.
- Existing local workspace note: `indigo.xcodeproj/project.pbxproj` was already modified before this step and was not touched.

Simulator-backed baseline:

- The SynScan simulator and integration test binary built successfully with `make -C indigo_test build/integration/test_mount_synscan_simulator`.
- Running the test from the repository root fails because the test executable expects `build/integration/mount_synscan_simulator` relative to `indigo_test`.
- Running from `indigo_test` passes:
  - `synscan_mount_passes_serial_compliance_checks`
  - `synscan_guider_passes_serial_compliance_checks`

Public behavior to preserve in the first greenfield version:

- Driver entry point: `indigo_mount_synscan`.
- Driver name: `indigo_mount_synscan`.
- Logical devices:
  - `Mount SynScan`
  - `Mount SynScan (guider)`
- Supported connection modes:
  - serial port through `DEVICE_PORT`;
  - configured baudrate through `DEVICE_BAUDRATE`;
  - explicit network URL `synscan://host:port`;
  - UDP autodetection with `synscan://`.
- Mount base/interface behavior:
  - expose `INDIGO_INTERFACE_MOUNT`;
  - unhide `DEVICE_PORT`, `DEVICE_PORTS` and `DEVICE_BAUDRATE` on the master mount device;
  - support additional instances according to current driver behavior;
  - use two alignment modes in `MOUNT_ALIGNMENT_MODE`;
  - expose alignment point selection/deletion;
  - expose raw coordinates and side-of-pier.
- Mount properties covered by the current simulator test:
  - `MOUNT_INFO` with `MODEL`, `VENDOR`, `FIRMWARE`;
  - `GEOGRAPHIC_COORDINATES`;
  - `MOUNT_SLEW_RATE`;
  - `MOUNT_MOTION_DEC`;
  - `MOUNT_MOTION_RA`;
  - `MOUNT_TRACK_RATE`;
  - `MOUNT_TRACKING` with `ON`, `OFF`;
  - `MOUNT_GUIDE_RATE` with `RA`, `DEC`;
  - `MOUNT_ON_COORDINATES_SET`;
  - `MOUNT_EQUATORIAL_COORDINATES` with `RA`, `DEC`;
  - `MOUNT_HORIZONTAL_COORDINATES`;
  - `MOUNT_ABORT_MOTION`;
  - `MOUNT_PEC`;
  - `MOUNT_PEC_TRAINING`;
  - `POLARSCOPE` with `BRIGHTNESS`;
  - `MOUNT_USE_ENCODERS` with `RA`, `DEC`;
  - `MOUNT_AUTOHOME` with `AUTOHOME`;
  - `MOUNT_AUTOHOME_SETTINGS` with `DEC_OFFSET`.
- Capability-dependent visibility to preserve:
  - `MOUNT_OPERATING_MODE` visible only for AZ/EQ-capable mounts, with `POLAR` and `ALTAZ`;
  - `MOUNT_USE_ENCODERS` visible only when axis encoders are supported;
  - `MOUNT_PEC` and `MOUNT_PEC_TRAINING` visible only when PPEC is supported;
  - `MOUNT_AUTOHOME` and `MOUNT_AUTOHOME_SETTINGS` visible only when home indexers are supported;
  - `POLARSCOPE` visible only when polarscope brightness control is supported.
- Custom/non-standard behavior:
  - non-standard `POLARSCOPE` brightness property;
  - non-standard `MOUNT_USE_ENCODERS` property;
  - non-standard `MOUNT_AUTOHOME` and `MOUNT_AUTOHOME_SETTINGS` properties;
  - current simulator expects `MOUNT_CUSTOM_TRACKING_RATE` not to be defined.
- Guider base/interface behavior:
  - expose `INDIGO_INTERFACE_GUIDER`;
  - guider shares the mount/master connection rather than opening its own serial port;
  - `GUIDER_RATE` is visible and has two items in the current driver.
- Guider properties covered by the current simulator test:
  - `GUIDER_GUIDE_DEC` with `NORTH`, `SOUTH`;
  - `GUIDER_GUIDE_RA` with `EAST`, `WEST`;
  - `GUIDER_RATE`.
- Minimum feature set for the greenfield driver:
  - connect/disconnect;
  - mount model/capability detection;
  - coordinate polling;
  - goto and sync;
  - tracking enable/disable and tracking-rate changes;
  - abort/motion stop;
  - manual motion;
  - park/unpark and persisted park position;
  - autohome where supported;
  - serial connection;
  - explicit UDP URL connection;
  - UDP autodetection;
  - guide pulses with overlapping pulses extended.

## Step 2 results

Date: 2026-09-01

References inspected:

- Old SynScan sources:
  - `indigo_mount_synscan.c`
  - `indigo_mount_synscan_driver.c`
  - `indigo_mount_synscan_protocol.c`
  - `indigo_mount_synscan_protocol.h`
  - `indigo_mount_synscan_mount.c`
  - `indigo_mount_synscan_guider.c`
  - `indigo_mount_synscan_private.h`
- Current simulator test:
  - `indigo_test/integration/test_mount_synscan_simulator.c`
- Protocol PDF:
  - `skywatcher_motor_controller_command_set.pdf`

PDF protocol requirements:

- Commands start with `:` and end with carriage return `0x0D`.
- A second `:` before carriage return restarts command parsing on the controller side.
- Normal replies start with `=` and end with carriage return.
- Error replies start with `!`, include a two-hex-digit error code, and end with carriage return.
- All command and response data is ASCII hex.
- Channel words are `1` for RA/Az, `2` for Dec/Alt and, for selected commands, `3` for both axes.
- 24-bit values are transmitted low-byte first by byte pairs. Example: `0x123456` is encoded as `563412`.
- 16-bit values are transmitted low-byte first by byte pairs. Example: `0x1234` is encoded as `3412`.
- Position values are offset by `0x800000`; true position `0x000012` is sent as `0x800012`.
- Firmware/hardware serial default is 9600 bps, 8N1.
- Wi-Fi uses the same protocol over UDP port 11880. Each command and response is one UDP packet.

Protocol commands required by the greenfield driver:

- Inquiry:
  - `:e<axis>`: inquire motor board version / firmware model data.
  - `:a<axis>`: inquire counts per revolution.
  - `:s<axis>`: inquire PEC/worm period.
  - `:b1`: inquire timer interrupt frequency.
  - `:g<axis>`: inquire high-speed ratio.
  - `:f<axis>`: inquire status.
  - `:j<axis>`: inquire current position.
  - `:q<axis><id>`: extended inquiry, including home indexer position and extended status/features.
- Axis setup and motion:
  - `:E<axis><pos>`: set axis reference position.
  - `:F<axis>`: initialization done / energize axis.
  - `:G<axis><mode><direction>`: set motion mode.
  - `:H<axis><steps>`: set goto target increment.
  - `:S<axis><target>`: set goto target.
  - `:M<axis><steps>`: set brake point increment.
  - `:I<axis><period>`: set step period / T1 preset.
  - `:J<axis>`: start motion.
  - `:K<axis>`: stop motion.
  - `:L<axis>`: instant stop.
- Optional/capability-dependent control:
  - `:V1<brightness>`: set polar scope LED brightness.
  - `:P<axis><rate>`: set ST4 guide rate.
  - `:W<axis><id>`: extended setting, including PPEC, dual encoders, low-speed current and home index reset.

Error handling requirements from the PDF:

- Surface protocol error replies as failed commands, not as malformed success replies.
- Decode at least the documented error classes for diagnostics: unknown command, command length, motor not stopped, invalid character, not initialized, driver sleeping, PEC training running and no valid PEC data.
- For commands documented as requiring a fully stopped motor, the new driver must stop/wait before issuing them or return an INDIGO property error before sending an unsafe command.

Feature and capability bits to preserve:

- `kHasEncoder = 0x0001`
- `kHasPPEC = 0x0002`
- `kHasHomeIndexer = 0x0004`
- `kIsAZEQ = 0x0008`
- `kInPPECTraining = 0x0010`
- `kInPPEC = 0x0020`
- `kHasPolarLED = 0x1000`
- `kHasCommonSlewStart = 0x2000`
- `kHasHalfCurrentTracking = 0x4000`

Mount model mapping to preserve from the old driver:

- `0x00`: `EQ6`
- `0x01`: `HEQ5`
- `0x02`: `EQ5`
- `0x03`: `EQ3`
- `0x04`: `EQ8`
- `0x05`: `AZEQ6`
- `0x06`: `AZEQ5`
- `0x0A`: `Star Adventurer`
- `0x0C`: `Star Adventurer GTi`
- `0x20`: `EQ8-R Pro`
- `0x22`: `AZEQ6 Pro`
- `0x23`: `EQ6-R Pro`
- `0x24`: `EQ6 Pro`
- `0x25`: `CQ350 Pro`
- `0x31`: `EQ5 Pro`
- `0x80`: `GT`
- `0x81`: `MF`
- `0x82`: `114GT`
- `0x83`: `StarSeek`
- `0x90`: `DOB`
- `0xA2`: `AZ-GTe`
- `0xA5`: `AZGTi`
- `0x44`: `Wave 100i`
- `0x45`: `Wave 150i`
- unknown values: `CUSTOM (%02x)`

Connection requirements extracted from old behavior:

- The master mount owns the physical connection.
- The guider shares the master's connection and increments/decrements shared connection ownership state.
- On serial connect, if initial configuration fails, the old driver retries once with the alternate configured baudrate between `9600-8N1` and `115200-8N1`; decide during implementation whether this remains part of the greenfield contract or becomes an explicit compatibility option.
- UDP autodetection sends `:e1\r` to broadcast port 11880 up to three times, accepts a reply beginning with `=`, then updates `DEVICE_PORT` with the detected host.
- The new implementation must recreate UDP autodetection with `indigo_uni_io` rather than direct sockets.

Initialization/configuration requirements:

- On connect, read firmware/model/vendor and fill `MOUNT_INFO`.
- Query RA and DEC motor status.
- If an axis is active during configuration, stop it.
- If an axis is not initialized, initialize it with `:F<axis>` and restore or set its persisted position.
- Query and cache per-axis total steps, worm steps, timer frequency and high-speed factor.
- Query extended feature bits for both axes and use them for property visibility.
- Derive home and zero positions from total steps:
  - RA home position starts at `0x800000`.
  - DEC home position starts at `0x800000 + decTotalSteps / 4`.
  - RA zero position is `raHomePosition - raTotalSteps / 4`.
  - DEC zero position is `decHomePosition - decTotalSteps / 4`.
- Invalidate cached axis motion configuration after successful connect.
- Read current coordinates before declaring the mount configured.

Property attach/enumeration requirements:

- Mount attach must unhide:
  - `MOUNT_PARK_SET`
  - `MOUNT_PARK_POSITION`
  - `MOUNT_HOME`
  - `MOUNT_HOME_SET`
  - `MOUNT_HOME_POSITION`
  - `MOUNT_RAW_COORDINATES`
  - `DEVICE_PORTS`
  - `DEVICE_PORT`
  - `DEVICE_BAUDRATE`
  - `MOUNT_ALIGNMENT_MODE`
  - `MOUNT_ALIGNMENT_SELECT_POINTS`
  - `MOUNT_ALIGNMENT_DELETE_POINTS`
  - `MOUNT_SIDE_OF_PIER`
- Mount attach must set `MOUNT_ALIGNMENT_MODE->count = 2`.
- Additional instances are visible only on the base device, matching current behavior.
- Custom properties are defined only after connection and only when not hidden by capability detection.
- Config save must persist:
  - `POLARSCOPE`
  - `MOUNT_OPERATING_MODE`
  - `MOUNT_USE_ENCODERS`
  - `MOUNT_AUTOHOME_SETTINGS`
  - guider `GUIDER_RATE`

Custom property requirements:

- `POLARSCOPE`
  - Type: number.
  - Group: `MOUNT_MAIN_GROUP`.
  - Label: `Polarscope`.
  - Permission: read/write.
  - Default hidden.
  - Item `BRIGHTNESS`, label `Polarscope Brightness`, range `0..255`.
- `MOUNT_OPERATING_MODE`
  - Type: switch.
  - Rule: one of many.
  - Group: `MOUNT_MAIN_GROUP`.
  - Label: `Operating mode`.
  - Permission: read/write.
  - Default hidden.
  - Items: `POLAR` default on, `ALTAZ` default off.
- `MOUNT_USE_ENCODERS`
  - Type: switch.
  - Rule: any of many.
  - Group: `MOUNT_MAIN_GROUP`.
  - Label: `Use encoders`.
  - Permission: read/write.
  - Default hidden.
  - Items: `RA`, `DEC`.
- `MOUNT_AUTOHOME`
  - Type: switch.
  - Rule: any of many.
  - Group: `MOUNT_MAIN_GROUP`.
  - Label: `Auto home`.
  - Permission: read/write.
  - Default hidden.
  - Item: `AUTOHOME`.
- `MOUNT_AUTOHOME_SETTINGS`
  - Type: number.
  - Group: `MOUNT_MAIN_GROUP`.
  - Label: `Auto home settings`.
  - Permission: read/write.
  - Default hidden.
  - Item: `DEC_OFFSET`, range `-90..90`.

Mount change-handler requirements:

- Reject goto, tracking, manual motion and autohome requests while parked.
- Reject goto/sync while the mount global mode is not idle.
- For sync, delegate through the base mount alignment flow when idle.
- For goto, preserve current coordinate values while copying requested targets, then schedule the slew handler.
- Implement handlers for:
  - connection;
  - park/unpark;
  - home;
  - equatorial goto/sync;
  - tracking rate;
  - tracking on/off;
  - manual RA/DEC motion;
  - abort;
  - ST4 guide rate;
  - polarscope brightness;
  - encoder enable/disable;
  - PPEC enable/disable;
  - PPEC training start/stop;
  - autohome;
  - autohome settings;
  - operating mode.

Polling and coordinate requirements:

- Poll current raw axis positions while connected.
- Convert encoder positions to raw HA/DEC and side-of-pier.
- Convert raw coordinates to aligned/transformed equatorial coordinates with current LST.
- Update horizontal coordinates and LST when geographic coordinates are available.
- Preserve northern/southern hemisphere handling for RA direction and pier-side conversion.
- Use a queue-rescheduled polling handler instead of an INDIGO timer.

Motion/rate requirements:

- Use sidereal, solar and lunar rates from the old driver unless a later PDF/source check proves a better value:
  - sidereal: `(360.0 * 3600.0) / 86164.090530833`
  - lunar: `14.511415`
  - solar: `15.0`
- Preserve manual slew rate mapping:
  - guide -> `1`
  - centering -> `4`
  - find -> `6`
  - max -> `9`
- Preserve RA and DEC manual rate tables:
  - RA: `1.25, 2, 8, 16, 32, 70, 100, 625, 725, 825`
  - DEC: `0.5, 1, 8, 16, 32, 70, 100, 625, 725, 825`
- Compute T1 preset values from timer frequency, total axis steps and requested angular rate as described in the PDF.
- Use high-speed mode above `128x` sidereal and apply the per-axis high-speed ratio.
- Invalidate cached axis configuration whenever a command sequence fails.

Guide pulse requirements:

- RA guide pulses require tracking to be enabled; otherwise the driver returns an alert for the RA guide property.
- DEC guide pulses do not require mount tracking.
- The greenfield implementation must not busy-wait for pulse duration.
- On a new guide pulse, start or update motion on the master queue and schedule an urgent finalizer.
- Overlapping pulses on the same axis extend the active deadline.
- A stale finalizer must not stop an axis if a later pulse extended the deadline.
- RA pulse completion resumes the previous tracking rate.
- DEC pulse completion instant-stops the DEC axis.

Persistence requirements:

- Park position persistence stores RA and DEC axis positions.
- The new implementation must use `indigo_uni_io` file helpers for both reading and writing.
- The persisted position path and remove-after-restore behavior should remain compatible with the old driver unless explicitly changed later.

Generator-readiness requirements:

- Keep protocol helpers as shared top-level code.
- Keep custom property definitions and handlers small and contiguous.
- Keep private fields in a compact block that can become `.driver` `data`.
- Prefer function names and section boundaries that can map directly to generator `on_attach`, `on_connect`, `on_disconnect`, `on_timer` and property `on_change` blocks.

## Step 3 results

Date: 2026-09-01

Greenfield skeleton created:

- `indigo_mount_synscan.c` is now a clean INDIGO 3.0 scaffold rather than the old split-driver entry file.
- The source is organized into generator-friendly sections:
  - includes;
  - common definitions;
  - property definitions;
  - private data;
  - low-level protocol code;
  - mount handlers;
  - mount device API;
  - guider handlers;
  - guider device API;
  - driver entry point.
- The source already includes `indigo_uni_io.h` and private data already stores `indigo_uni_handle *handle`.
- `synscan_open()` and `synscan_close()` exist as low-level transport entry points for step 4. `synscan_open()` is intentionally a stub until the transport layer is implemented.
- Mount and guider logical devices are attached from the single source file and share one `synscan_private_data` instance.
- Guider operations are scheduled onto the master mount device queue, matching the design decision for mount/guider serialization.
- Custom SynScan properties are defined in the new source:
  - `POLARSCOPE`
  - `MOUNT_OPERATING_MODE`
  - `MOUNT_USE_ENCODERS`
  - `MOUNT_AUTOHOME`
  - `MOUNT_AUTOHOME_SETTINGS`
- The public header `indigo_mount_synscan.h` was simplified to match the style of other modern drivers: it exposes the driver entry point and device names without pulling in mount/guider implementation headers.

Old-source handling:

- Old split implementation files were moved to `old_split_driver/` during implementation as temporary reference material:
  - old `indigo_mount_synscan.c`
  - `indigo_mount_synscan_driver.c/.h`
  - `indigo_mount_synscan_protocol.c/.h`
  - `indigo_mount_synscan_mount.c/.h`
  - `indigo_mount_synscan_guider.c/.h`
  - `indigo_mount_synscan_private.h`
- `indigo_mount_synscan_main.c`, `indigo_mount_synscan.h`, simulator sources and the protocol PDF remain in their normal locations.

Build result:

- `make -f ../../Makefile.drv` from `indigo_drivers/mount_synscan` succeeds.
- The driver makefile now sees only:
  - `indigo_mount_synscan.c`
  - `indigo_mount_synscan_main.c`
- The produced archive, dynamic library and executable build successfully.

Known skeleton limitations before step 4:

- Connection intentionally fails because `synscan_open()` has not been implemented yet.
- Mount motion, coordinate polling, parking, protocol commands and guide pulses are placeholders.
- The simulator-backed integration test is expected to fail until transport and enough connection/configuration behavior are implemented.
- `indigo.xcodeproj/project.pbxproj` still contains pre-existing SynScan file references and also had unrelated local modifications before this work; update it later when the final file layout is stable.

## Step 4 results

Date: 2026-09-01

Transport layer implemented:

- `synscan_open()` now opens serial connections with `indigo_uni_open_serial_with_config()`.
- Explicit network URLs are converted from public `synscan://host[:port]` syntax to a uni I/O UDP URL and opened with `indigo_uni_open_url(..., INDIGO_UDP_HANDLE, ...)`.
- `synscan://` UDP autodetection is implemented as a first pass through a uni I/O UDP handle connected to `255.255.255.255:11880`.
- UDP and serial handles use `indigo_uni_set_socket_read_timeout()` / `indigo_uni_set_socket_write_timeout()` where applicable.
- `synscan_close()` closes all open transport handles with `indigo_uni_close()`.
- `synscan_command()` writes complete command packets with `indigo_uni_write()`.
- Serial responses are read with `indigo_uni_read_section2()`.
- UDP responses are read with `indigo_uni_read()`, matching the PDF requirement that one command and one response fit in single UDP packets.
- Response parsing now handles normal `=` replies and protocol `!xx` error replies.
- The 24-bit SynScan hex byte-pair order described in the PDF is implemented in helper functions.

Park-position I/O implemented:

- Park-position path generation now uses `indigo_uni_config_folder()`.
- The config directory is created with `indigo_uni_mkdir()`.
- Park-position writing uses `indigo_uni_create_file()`, `indigo_uni_write()` and `indigo_uni_close()`.
- Park-position reading uses `indigo_uni_open_file()`, `indigo_uni_read()` and `indigo_uni_close()`.
- Remove-after-restore uses `indigo_uni_remove()`.

Build and validation:

- `make -f ../../Makefile.drv` from `indigo_drivers/mount_synscan` succeeds with no compiler warnings.
- The simulator integration binary builds.
- The simulator integration test is still expected to fail because configuration, capability detection, property visibility updates and behavior handlers are not implemented yet.
- Current simulator-test failure mode confirms that the next required work is configuration/property-definition behavior: connected enumeration does not yet expose capability-dependent properties such as `MOUNT_PEC`, `MOUNT_PEC_TRAINING`, `POLARSCOPE`, `MOUNT_USE_ENCODERS`, `MOUNT_AUTOHOME` and `MOUNT_AUTOHOME_SETTINGS`.

Important follow-up for UDP autodetection:

- This follow-up was completed after step 10.
- `indigo_uni_io` now provides an active UDP discovery helper for probe-and-reply discovery protocols.
- The SynScan driver no longer owns platform-specific UDP discovery code; it calls the uni I/O helper for the broadcast probe and then opens the detected responder through normal uni I/O UDP transport.

## Step 5 results

Date: 2026-09-01

Async connection/configuration behavior implemented:

- Mount connection now runs on the mount device queue.
- Guider connection requests are scheduled onto the master mount queue.
- The master mount remains the owner of the physical connection and shared `synscan_private_data`.
- The shared connection counter is preserved for mount + guider lifecycles.
- Successful first connection now runs `synscan_configure()` before reporting `CONNECTION` as OK.
- Disconnection cancels pending handlers on the relevant queue and closes the uni I/O handle when the shared connection count reaches zero.

Configuration behavior implemented:

- Firmware/model/vendor data is read with `:e1` and mapped into `MOUNT_INFO`.
- RA and DEC motor status are read with `:f1` and `:f2`.
- Running axes are stopped before configuration continues.
- Total steps, worm steps, timer frequency and high-speed ratio are queried for both axes.
- Extended feature bits are queried for both axes with `:q<axis>000100`.
- Capability-dependent property visibility is updated from feature bits:
  - AZ/EQ mode;
  - encoders;
  - PPEC and PPEC training;
  - autohome and autohome settings.
- Polarscope support is probed with `:V1xx`.
- PPEC and PPEC training base properties are initialized from feature state.
- Home and zero positions are derived from total axis steps.
- Uninitialized axes are initialized with `:F<axis>` and restored through the uni I/O park-position helper.
- Current raw axis positions are read with `:j<axis>`.
- The mount starts in idle, unparked, tracking-off state after configuration.

Queue handlers implemented or improved:

- `MOUNT_ABORT_MOTION` runs on the queue and sends instant stop commands to both axes.
- `MOUNT_GUIDE_RATE` runs on the queue and sends ST4 guide-rate commands to both axes.
- `POLARSCOPE` runs on the queue and sends brightness commands.
- `MOUNT_USE_ENCODERS` runs on the queue and sends extended encoder enable/disable commands.
- `MOUNT_AUTOHOME_SETTINGS` and `MOUNT_OPERATING_MODE` now complete as OK queue updates.
- Periodic mount polling runs with `indigo_execute_handler_in()` and updates raw/equatorial coordinates from SynScan axis positions.
- `MOUNT_PARK` stops both axes, waits for idle state, saves the current park position through uni I/O file helpers and updates tracking/park state.
- `MOUNT_HOME` slews both axes to the derived home positions and waits for completion.
- `MOUNT_EQUATORIAL_COORDINATES` handles sync immediately and goto through queued SynScan axis moves.
- `MOUNT_TRACKING` starts/stops RA tracking through queued rate commands.
- `MOUNT_TRACK_RATE` reapplies the current tracking rate when tracking is enabled.
- `MOUNT_MOTION_RA` and `MOUNT_MOTION_DEC` run as anytime queue handlers for manual slew start/stop.
- `MOUNT_PEC` enables/disables PPEC with SynScan extended setting commands.
- `MOUNT_PEC_TRAINING` starts/stops PPEC training with SynScan extended setting commands.
- `MOUNT_AUTOHOME` has a first greenfield queue implementation which resets home indexer state and initializes the RA/DEC home positions, including configured DEC offset.

Step 5 continuation:

- Absolute axis slews now stop the affected axis before programming the goto move.
- `MOUNT_PARK` now slews to the configured `MOUNT_PARK_POSITION` HA/DEC before saving the parked SynScan raw position through uni I/O.
- `MOUNT_HOME` now slews to the configured `MOUNT_HOME_POSITION` HA/DEC instead of only using derived internal home constants.
- `MOUNT_PARK_SET`, `MOUNT_PARK_POSITION`, `MOUNT_HOME_SET` and `MOUNT_HOME_POSITION` remain handled by the common INDIGO mount driver, matching the documented base-driver contract for these properties.
- New goto/home/park operations reset the pending abort flag before waiting for axis completion.
- Abort clears `MOUNT_ABORT_MOTION.ABORT_MOTION` after sending stop commands.
- Manual RA/DEC motion now rejects motion while parked.
- Polling now updates `MOUNT_SIDE_OF_PIER` from the current hour-angle sign.
- The old driver's encoder conversion model was ported into the new single-file implementation:
  - SynScan raw RA/DEC encoder fractions are converted to raw HA/DEC and side of pier with northern/southern hemisphere handling.
  - Raw coordinates are translated through `indigo_raw_to_translated_with_lst()`.
  - Goto, park and home targets use the two-solution encoder conversion and select the same normal/CW-down solution rule used by the old driver.
  - Horizontal coordinates and LST are updated from the same LST sample as equatorial polling, preserving the old driver's intent to avoid derived-coordinate jitter.

Validation:

- `make -f ../../Makefile.drv` from `indigo_drivers/mount_synscan` succeeds with no compiler warnings.
- `make build/integration/test_mount_synscan_simulator && ./build/integration/test_mount_synscan_simulator` from `indigo_test` passes:
  - `synscan_mount_passes_serial_compliance_checks`
  - `synscan_guider_passes_serial_compliance_checks`
- Test build artifacts were removed with `make test-clean`.

Validation deferred beyond step 5:

- Validate coordinate precision, side-of-pier behavior and goto completion against real SynScan hardware.
- Validate whether the first greenfield autohome implementation is sufficient, or whether the full legacy home-index search procedure should be restored after hardware testing.
- Validate explicit UDP URL mode and `synscan://` UDP autodetection against real hardware or a future UDP simulator; the current simulator only provides serial PTY mode.

## Step 6 results

Date: 2026-09-01

Guide pulse behavior implemented:

- `GUIDER_GUIDE_RA` now runs through the master mount queue with urgent priority.
- RA guide pulses require mount tracking to be enabled, matching the old driver behavior.
- RA guiding temporarily changes the RA axis rate to tracking rate plus/minus the configured RA guide percentage.
- RA guide finalization restores the RA tracking rate instead of stopping the RA axis.
- `GUIDER_GUIDE_DEC` now runs through the master mount queue with urgent priority.
- DEC guiding starts a temporary DEC axis rate based on the configured DEC guide percentage.
- DEC guide finalization stops the DEC axis.
- Guide pulse finalizers are scheduled with urgent priority through the master queue, so mount and guider SynScan commands remain serialized.
- Overlapping guide pulses in the same direction extend per-axis deadlines. A stale finalizer checks the latest deadline and reschedules itself instead of stopping an extended pulse.
- Overlapping guide pulses in the opposite direction on the same axis cancel the active pulse deadline and start a new pulse in the new direction.
- Guide properties clear their numeric pulse items and return to OK only when the effective pulse deadline has finished.

Validation:

- `make -f ../../Makefile.drv` from `indigo_drivers/mount_synscan` succeeds with no compiler warnings.
- The serial simulator integration test passes after extending the guider test to issue same-direction overlapping DEC guide pulses and opposite-direction DEC cancellation:
  - `synscan_mount_passes_serial_compliance_checks`
  - `synscan_guider_passes_serial_compliance_checks`
- Test build artifacts were removed with `make test-clean`.

Validation deferred beyond step 6:

- Validate RA guiding rate changes against real hardware while tracking is enabled.
- Validate same-axis opposite-direction pulse cancellation against real hardware.

## Step 7 results

Date: 2026-09-01

Single-file implementation cleanup completed:

- The active SynScan implementation remains in `indigo_mount_synscan.c`.
- The public driver interface remains in `indigo_mount_synscan.h`, matching the usual driver-header pattern used by other INDIGO drivers.
- `indigo_mount_synscan_main.c` remains as the small standalone executable entry point.
- The old split implementation files were kept under `old_split_driver/` as temporary reference material through step 9:
  - `indigo_mount_synscan_driver.c/.h`
  - `indigo_mount_synscan_protocol.c/.h`
  - `indigo_mount_synscan_mount.c/.h`
  - `indigo_mount_synscan_guider.c/.h`
  - `indigo_mount_synscan_private.h`
- The active Xcode project no longer references the old split source/header files in file references, group children, headers phases or sources phases.
- The active Makefile driver build uses only `indigo_mount_synscan.c` and `indigo_mount_synscan_main.c`.
- Stale object files from the previous split implementation were removed from the driver folder.
- No new implementation code depends on the old private/internal SynScan headers.

Validation:

- `make -f ../../Makefile.drv clean` from `indigo_drivers/mount_synscan` removes the current single-file driver build outputs.
- `make -f ../../Makefile.drv` from `indigo_drivers/mount_synscan` succeeds with no compiler warnings and reports only:
  - `indigo_mount_synscan.c`
  - `indigo_mount_synscan_main.c`
- `make build/integration/test_mount_synscan_simulator` from `indigo_test` builds the SynScan simulator integration test against the new driver archive.
- `./build/integration/test_mount_synscan_simulator` passes:
  - `synscan_mount_passes_serial_compliance_checks`
  - `synscan_guider_passes_serial_compliance_checks`
- Test build artifacts were removed with `make test-clean`.

Validation deferred beyond step 7:

- Validate explicit UDP URL mode and `synscan://` UDP autodetection against real hardware or a future UDP simulator.
- Validate real hardware mount behavior for coordinate precision, side-of-pier transitions, goto/home/park completion, autohome and PPEC.
- Validate real hardware guider behavior for RA rate restoration and same-axis opposite-direction pulse cancellation.

## Step 8 results

Date: 2026-09-01

Generator migration preparation completed:

- The single-file driver now has generator extraction annotations for:
  - `include`
  - `define`
  - `data`
  - top-level shared `code`
  - `mount.code`
  - `mount.on_attach`
  - `mount.on_connect`
  - `mount.on_disconnect`
  - `mount.on_timer`
  - mount inherited property `on_change` blocks
  - `guider.code`
  - `guider.on_attach`
  - `guider.on_connect`
  - `guider.on_disconnect`
  - guider inherited property `on_change` blocks
- The low-level SynScan protocol, UDP autodetection, uni I/O transport, encoder conversion and park-file helpers remain grouped in the top-level `code` block.
- The mount manual-slew helper is isolated in `mount.code`.
- RA/DEC guide-pulse finalizers are isolated in `guider.code`.
- `synscan_open()` and `synscan_close()` already match the expected generator naming convention for a future `driver synscan { ... }` definition.
- A temporary generator extraction smoke test was run outside the driver folder with:
  - source copy: `/private/tmp/synscan_generator_check/indigo_mount_synscan.c`
  - command: `indigo_generator -c indigo_mount_synscan.driver`
- The smoke test produced a temporary `driver synscan` skeleton containing the expected include, define, data, code, mount and guider sections. No `.driver` file was added to the active driver folder in this step.

Validation:

- `make -f ../../Makefile.drv` from `indigo_drivers/mount_synscan` succeeds with no compiler warnings after adding generator annotations.
- `make build/integration/test_mount_synscan_simulator` from `indigo_test` builds the SynScan simulator integration test against the annotated single-file driver.
- `./build/integration/test_mount_synscan_simulator` passes:
  - `synscan_mount_passes_serial_compliance_checks`
  - `synscan_guider_passes_serial_compliance_checks`
- Test build artifacts were removed with `make test-clean`.

Remaining work for the actual generator migration step:

- Create the real `indigo_mount_synscan.driver` in the driver folder.
- Fill in the driver metadata, especially `author` and the final `version`.
- Review the extracted property skeleton manually; inherited mount/guider properties are recognized, but custom property declarations may need hand-polished labels, groups, ranges, defaults, persistence flags and `always_defined` flags.
- Decide whether the generated driver should model the shared mount/guider connection counter directly or whether this driver needs a generator extension for two logical devices sharing one transport handle and master queue.
- Regenerate `indigo_mount_synscan.c`, `indigo_mount_synscan.h` and `indigo_mount_synscan_main.c` only after the `.driver` definition is reviewed.

Validation deferred beyond step 8:

- Validate the generated output in a future migration step by comparing the generated driver behavior against the current hand-written single-file implementation.
- Validate explicit UDP URL mode, UDP autodetection and real hardware guider/mount behavior as listed in the previous deferred-validation sections.

## Step 9 results

Date: 2026-09-01

Automated validation completed:

- The driver builds through `Makefile.drv` with the active source list limited to:
  - `indigo_mount_synscan.c`
  - `indigo_mount_synscan_main.c`
- The serial simulator integration test was expanded to cover:
  - mount and guider connection/disconnection through the shared driver lifecycle
  - mandatory mount property presence and item shape
  - custom SynScan property presence and item shape
  - ST4 guide-rate changes
  - polarscope brightness changes
  - encoder enable changes
  - abort motion
  - tracking on/off and tracking-rate update handlers
  - manual RA and DEC motion start/stop handlers
  - guider rate changes
  - RA guide pulse rejection while mount tracking is disabled
  - same-direction DEC guide pulse extension
  - same-axis opposite-direction DEC guide pulse cancellation
  - failed serial connection without leaving the connection property BUSY
- The current serial simulator test passes:
  - `synscan_mount_passes_serial_compliance_checks`
  - `synscan_guider_passes_serial_compliance_checks`
  - `synscan_mount_reports_failed_serial_connection`
- The generator extraction smoke test still succeeds on a temporary copy with `indigo_generator -c indigo_mount_synscan.driver`.
- Test artifacts were removed with `make test-clean`.
- Driver build artifacts were removed with `make -f ../../Makefile.drv clean`.

Validation deliberately not automated in step 9:

- Home and park slews are not asserted in the short serial simulator test because the current simulator and default home/park positions can require longer motion windows than the generic test timeout. Property presence and handler wiring are covered; physical completion remains a hardware/manual validation item.
- UDP URL mode and UDP autodetection were not covered by the serial PTY simulator in step 9; this gap was resolved after step 10 by adding UDP mode to the SynScan simulator.

Validation deferred beyond step 9:

- Validate real hardware mount behavior for coordinate precision, side-of-pier transitions, goto completion, home completion, park/unpark completion, persisted park restore, autohome and PPEC.
- Validate real hardware guider behavior for RA rate restoration, same-direction pulse extension and same-axis opposite-direction pulse cancellation.
- Validate shutdown/unload after long-running real slews and guide pulses to confirm no queued work, timers, open handles or threads remain.

## Step 10 results

Date: 2026-09-01

Cleanup completed:

- Active build inputs remain limited to the single implementation file and standalone entry point:
  - `indigo_mount_synscan.c`
  - `indigo_mount_synscan_main.c`
- The old split implementation was removed from the active driver folder after serving as temporary reference material, and is not referenced by the active Makefile or Xcode project.
- Active driver code has no direct POSIX file I/O. Park persistence, serial transport, UDP autodetection, connected UDP transport and command reads/writes go through `indigo_uni_io`.
- UDP autodetection uses `indigo_perform_active_discovery()` so the driver can receive the responder address without owning platform-specific socket code, then opens the detected mount through the normal uni I/O UDP transport.
- Simulator sources and integration tests remain separate from the driver implementation.
- `README.md` was updated for the current UDP autodetection wording, shared mount/guider connection model and guide-pulse overlap semantics.
- `indigo_docs/PROPERTIES.md` was updated with the SynScan-specific optional properties:
  - `POLARSCOPE`
  - `MOUNT_OPERATING_MODE`
  - `MOUNT_USE_ENCODERS`
  - `MOUNT_AUTOHOME`
  - `MOUNT_AUTOHOME_SETTINGS`

Validation:

- `make -f ../../Makefile.drv` from `indigo_drivers/mount_synscan` succeeds with no compiler warnings.
- `make build/integration/test_mount_synscan_simulator` from `indigo_test` succeeds.
- `./build/integration/test_mount_synscan_simulator` passes:
  - `synscan_mount_passes_serial_compliance_checks`
  - `synscan_guider_passes_serial_compliance_checks`
  - `synscan_mount_reports_failed_serial_connection`
  - `synscan_mount_connects_with_explicit_udp_url`
  - `synscan_mount_connects_with_udp_autodetection`
- `indigo_generator -c indigo_mount_synscan.driver` succeeds on a temporary copy of the annotated single-file source.
- Test artifacts were removed with `make test-clean`.
- Driver build artifacts were removed with `make -f ../../Makefile.drv clean`.

Additional UDP simulator validation:

- The SynScan simulator supports `--udp-port <port>` and writes a `synscan://host:port` ready value for explicit UDP URL tests.
- Explicit UDP URL mode is covered by the simulator using an ephemeral UDP port.
- UDP autodetection is covered by the simulator on the default SynScan UDP port 11880.
- The autodetection implementation records the actual responder address instead of keeping the public device port set to the broadcast address.
- The active UDP probe implementation lives in `indigo_uni_io` and is available for other probe-and-reply UDP discovery protocols.

Remaining deferred validation after cleanup:

- Validate real hardware mount behavior for coordinate precision, side-of-pier transitions, goto completion, home completion, park/unpark completion, persisted park restore, autohome and PPEC.
- Validate real hardware guider behavior for RA rate restoration, same-direction pulse extension and same-axis opposite-direction pulse cancellation.
- Validate shutdown/unload after long-running real slews and guide pulses to confirm no queued work, timers, open handles or threads remain.

## Step 11 results

Date: 2026-09-01

Generator migration completed:

- `indigo_mount_synscan.driver` is now the source of truth for the SynScan driver.
- `indigo_mount_synscan.c`, `indigo_mount_synscan.h` and `indigo_mount_synscan_main.c` were regenerated from the `.driver` definition.
- The `.driver` definition contains the shared low-level protocol code, mount device block, guider device block, custom SynScan property definitions and all custom property handlers.
- The mount and guider devices keep the shared transport/state model through the generator-managed shared connection counter and common private data fields:
  - `mount_device`
  - `guider_device`
- Mount and guider `on_connect` blocks use the generator's `connection_result` variable; the generator owns open/close counting, connection state updates and property definition.
- Driver `on_connect` and `on_disconnect` blocks do not return out of the generated connection handler body, so generator-owned failure cleanup, reference counting and final connection property updates always run.
- Generated change handlers pass the logical device to `indigo_execute_handler*()`; INDIGO then queues the work on `device->master_device` when one is present while still invoking the callback with the original logical device. This preserves shared mount/guider queue serialization without custom generated dispatch code.
- Property `on_change` blocks rely on the generator's initial `PROPERTY->state = INDIGO_OK_STATE` insertion where possible, and only set the property state explicitly for actual results, alerts or busy guide-pulse states.
- The generated park guard is used as the only parked-mount check for `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_TRACKING`, `MOUNT_MOTION_RA` and `MOUNT_MOTION_DEC`; the `.driver` source does not duplicate these generator-owned exceptions.
- RA/DEC guide pulse handlers intentionally use `_finalizer` callbacks so the generator does not append a terminal property update; these handlers publish their own busy/start update and delayed completion update.
- Custom SynScan properties are now generator-owned property blocks instead of manual `indigo_init_*_property()` allocations inside `mount.on_attach`.
- The generated driver keeps UDP autodetection on `indigo_perform_active_discovery()` and connected UDP/serial communication on `indigo_uni_io`.
- The generated public header follows the standard generated-driver style and exposes only `indigo_mount_synscan()`.
- The integration test now uses the generated device names directly instead of private name macros that are not emitted by the generator.
- The Xcode project includes the new `.driver` file next to the generated SynScan sources.

Validation:

- `../../build/bin/indigo_generator indigo_mount_synscan.driver` succeeds.
- `make -f ../../Makefile.drv` from `indigo_drivers/mount_synscan` succeeds with the generated sources.
- `make build/integration/test_mount_synscan_simulator` from `indigo_test` succeeds.
- `./build/integration/test_mount_synscan_simulator` passes:
  - `synscan_mount_passes_serial_compliance_checks`
  - `synscan_guider_passes_serial_compliance_checks`
  - `synscan_mount_reports_failed_serial_connection`
  - `synscan_mount_connects_with_explicit_udp_url`
  - `synscan_mount_connects_with_udp_autodetection`

Remaining deferred validation after generator migration:

- Validate real hardware mount behavior for coordinate precision, side-of-pier transitions, goto completion, home completion, park/unpark completion, persisted park restore, autohome and PPEC.
- Validate real hardware guider behavior for RA rate restoration, same-direction pulse extension and same-axis opposite-direction pulse cancellation.
- Validate shutdown/unload against real hardware during long-running slews and guide pulses; simulator-backed connect/disconnect and UDP cleanup are covered by integration tests.
