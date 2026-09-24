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

## Step 12 results - AZ-GTi hardware validation

Date: 2026-09-20

### Hardware test decision

Hardware testing was performed. The device is a Sky-Watcher AZ-GTi reached over the network:

- model code `0xA5`, reported as `AZGTi`;
- motor controller firmware `3.16`;
- vendor string `Sky-Watcher SynScan`;
- transport: SynScan UDP on port 11880, at `synscan://192.168.111.139:11880`;
- the address was found by the driver's own UDP broadcast autodetection, started from `synscan://`.

Controller capabilities as reported by the extended feature inquiry on this unit:

- auxiliary encoders: supported, `MOUNT_USE_ENCODERS` defined;
- AZ/EQ operating mode: supported, `MOUNT_OPERATING_MODE` defined;
- snap port: supported, `Mount SynScan (aux)` defines `CCD_EXPOSURE` and `CCD_ABORT_EXPOSURE`;
- polarscope LED: not supported, `POLARSCOPE` stays hidden;
- PPEC: not supported, `MOUNT_PEC` and `MOUNT_PEC_TRAINING` stay hidden;
- home indexer: not supported on either axis, `MOUNT_AUTOHOME` and `MOUNT_AUTOHOME_SETTINGS` stay hidden.

The capability gating was therefore exercised in both directions on one physical unit: four optional
properties defined and four correctly withheld.

### Test infrastructure added

- `indigo_test/hardware/test_mount_synscan_hw.c` holds the opt-in acceptance suite. It is named
  after the driver, as required, and registers one case per acceptance scenario.
- `indigo_test/Makefile` gained the `test-mount-synscan-hw` target plus the driver object and test
  binary rules. The driver object is compiled with `indigo_uni_config_folder` redirected to a
  temporary directory, the same override the simulator test uses, so a run never writes a park
  file into the user's own INDIGO configuration folder.
- The test source is registered in `indigo.xcodeproj/project.pbxproj`.
- The suite is opt-in and is not reachable from `test`, `test-unit` or `test-integration`.

Environment variables: `SYNSCAN_HW_URL` (default `synscan://`), `SYNSCAN_HW_LATITUDE` and
`SYNSCAN_HW_LONGITUDE` (default 48 / 17), `SYNSCAN_HW_DECLINATION` (default 60),
`SYNSCAN_HW_AUTOHOME` and `SYNSCAN_HW_DEBUG`.

The suite physically moves both axes. Slews are relative to the pointing found at start-up, the
park and home positions are set to the current pointing so those workflows run without a large
rotation, and the starting pointing, tracking state, guide rates, park position, home position,
slew rate, track rate and coordinate-set action are restored before the session disconnects.

### Scenario to test mapping

The mount hardware acceptance checklist of `indigo_test/DRIVER_TESTING_RULES.md` maps onto the
registered cases as follows.

| Acceptance scenario | Test case |
| --- | --- |
| Discovery and connection | `synscan_discovers_and_connects` |
| Model, firmware and capability readback | `synscan_reports_identity_and_capabilities` |
| Coordinate and status readback | `synscan_reads_coordinates_and_state` |
| Tracking on/off, track rates, guide rates | `synscan_tracking_and_rates` |
| Manual motion in both axes, stop, abort, fresh command | `synscan_manual_motion` |
| SYNC without motion, small reachable slew, post-slew tracking | `synscan_syncs_and_slews` |
| Abort during a slew followed by a fresh command | `synscan_aborts_slew_and_recovers` |
| Park, parked-request rejection, unpark | `synscan_parks_and_unparks` |
| Home and post-home tracking state | `synscan_homes` |
| Controller autohome procedure | `synscan_runs_autohome` |
| Snap port shutter and abort | `synscan_controls_snap_port` |
| Guide pulses, rejection, extension, opposite-direction cancel | `synscan_guides` |
| Guide pulse duration accuracy | `synscan_guide_pulse_accuracy` |
| Disconnect/reconnect, both connection orders, shared ownership | `synscan_reconnects` |
| Unreachable mount refused cleanly, then recovery | `synscan_reports_unreachable_mount` |
| Driver INIT/SHUTDOWN cycle and fresh operation | `synscan_reinitializes` |

Scenarios that are not applicable or not covered on this unit, with the reason:

- Autohome could not be exercised: this AZ-GTi reports no home indexer on either axis, so the
  driver correctly withholds `MOUNT_AUTOHOME`. The case detects that and reports it as not
  applicable rather than passing silently. Autohome remains covered against the simulator by
  `synscan_mount_autohome_finds_home_index`.
- PPEC and PPEC training could not be exercised for the same reason: the controller reports no
  PPEC support, so `MOUNT_PEC` and `MOUNT_PEC_TRAINING` stay hidden. Simulator coverage exists.
- Polarscope brightness could not be exercised: no polarscope LED on this controller.
- Side-of-pier transitions were not forced. The AZ-GTi used here is a single-arm mount and the
  driver reports a constant side; provoking a meridian flip needs an equatorial head such as the
  EQ6 the simulator models.
- Transport loss during active work was not injected. The SynScan network transport is UDP and has
  no session to drop, and the driver deliberately treats a UDP timeout as a temporary failure
  rather than a disconnect. The reachable failure mode, an unreachable endpoint, is covered by
  `synscan_reports_unreachable_mount`; cable loss on the serial transport is covered against the
  simulator by `synscan_mount_disconnects_after_serial_loss`.
- Multiple physical mounts were not exercised; only one unit was available.

## Found defects

### D1 - MOUNT_INFO.FIRMWARE published with a leading space

- Observable impact: on the AZ-GTi the driver published `MOUNT_INFO.FIRMWARE` as `" 3.16"`. Clients
  show the value verbatim, so the firmware appeared indented in every UI and any exact string
  comparison against `"3.16"` failed.
- Reproduced on hardware, not only by source audit: the first hardware run printed
  `Sky-Watcher SynScan AZGTi, firmware  3.16`.
- Root cause: `synscan_set_model_info()` formatted the version with `"%2d.%02d"`. The width of 2
  pads any single-digit release number with a space. The behaviour was inherited from the INDIGO
  2.0 driver, which used `"%2d.%02d.%02d"`, so it is not a regression of this refactoring, but it
  is still wrong.
- Fix: format with `"%d.%02d"` in `indigo_mount_synscan.driver` and regenerate. Driver version
  bumped from 1 to 2.
- Regression test: `synscan_reports_identity_and_capabilities` asserts the model, vendor and
  firmware items are non-empty and prints them; the hardware run now reports `firmware 3.16`.
  The simulator suite covers the same code path through
  `synscan_mount_passes_serial_compliance_checks` and `synscan_mount_reports_new_model_codes`.

### D2 - a request refused because the mount is parked kept the refused values

- Observable impact: park the mount, then request `MOUNT_TRACKING.ON`. The driver correctly
  answered `INDIGO_ALERT_STATE` with the message `Mount is parked!`, but `MOUNT_TRACKING` kept
  `ON = true` and `OFF = false` afterwards, and nothing later corrected it. The RA axis was
  verifiably stopped at that moment: the reported right ascension ran away at the sidereal rate,
  0.046 degrees over 12 s against an expected 0.050. A parked mount therefore published itself as
  tracking. The same applied to `MOUNT_MOTION_RA` and `MOUNT_MOTION_DEC`, whose direction item
  stayed set, and to `MOUNT_EQUATORIAL_COORDINATES`, whose target stayed at the refused target.
- Reproduced on hardware in the third hardware run, after the peer session that had been sharing
  the mount was stopped, so it is not an artefact of concurrent access.
- This violates the rule in `indigo_test/DRIVER_TESTING_RULES.md` that a failed request must not
  overwrite accepted values with invalid output. It matters operationally because agents read
  these switches as mount state.
- Root cause: the park guard was emitted only at the top of the generated handler. By the time the
  handler ran, `INDIGO_COPY_VALUES_PROCESS_CHANGE` had already copied the client's requested values
  into the driver's property, set `INDIGO_BUSY_STATE` and published it. The guard published
  `INDIGO_ALERT_STATE` and returned without undoing that copy.
- The defect was generator-owned, not specific to this driver, so the fix was proposed and
  explicitly approved by the user before being implemented, as required by the root `AGENTS.md`.
- Fix, in `indigo_tools/indigo_generator.c`: the guard is now emitted in two places.
  - An admission check in the `change_property` branch, before the values are copied. It marks every
    item for update, sets `INDIGO_ALERT_STATE`, publishes with the `Mount is parked!` message and
    returns without scheduling the handler. This reuses the shape the generator already emits for
    `reject_change` conditions, so a refused request leaves the driver's own state intact and no
    spurious `INDIGO_BUSY_STATE` is published.
  - The original check at the top of the handler is retained as a safety net. A request admitted
    while the mount was still unparked can reach the hardware after a park request has been
    accepted, because the park switch flips when the park request is copied in its own change
    branch, before the park slew starts. Without the handler check that admitted request would
    start an axis on a mount the user had just told to park. Both checks are required; neither is
    redundant.
- Affected drivers, all regenerated with their `version` incremented: `mount_synscan`,
  `mount_ioptron`, `mount_lx200`, `mount_nexstar`, `mount_nexstaraux`, `mount_pmc8`,
  `mount_rainbow`, `mount_simulator`, `mount_starbook`, `mount_temma`. `mount_mxhd` is
  hand-written and already performed the check in `change_property` before copying, so it was not
  affected and was not changed.
- Regression test: `synscan_mount_keeps_state_when_parked_request_is_refused` in
  `indigo_test/integration/test_mount_synscan_simulator.c`. It parks the mount, confirms tracking
  is reported off, then requests tracking on and both motion directions, and asserts that each is
  refused with `INDIGO_ALERT_STATE` while the refused items keep the driver's values. The test was
  verified to fail against a build with the admission check removed and to pass with it in place.
  On hardware, `synscan_parks_and_unparks` checks the published tracking state immediately after
  the park completes and then exercises the same three refusals.
- Documentation updated: `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` now describes both guards and
  why each is needed, and `indigo_test/DRIVER_TESTING_RULES.md` states that a parked-request
  refusal must leave the refused property holding the driver's own state.

## Guide pulse duration accuracy

Required by `indigo_drivers/AGENTS.override.md` for every driver exposing a guider interface.

Measurement method: the test issues a pulse through `GUIDER_GUIDE_RA` or `GUIDER_GUIDE_DEC` and
measures wall-clock time from the moment the change request is submitted to the bus until the
property is republished in `INDIGO_OK_STATE` by the driver's finalizer. Timing is taken from
`indigo_monotonic_time()` inside the test. Five samples per duration, on both axes, with the mount
tracking at the sidereal rate and the guide rate set to 50 %. The workload is the idle driver: the
mount poll handler runs on the same queue, no slew is in progress.

**This measures public-property completion timing over the UDP transport and the driver's
finalizer. It is not an electrical measurement of the ST4 relay output and must not be presented
as one.** The transport itself contributes: a measured round trip to this mount over Wi-Fi was
9.1 ms minimum, 10.4 ms median, 21.2 ms maximum, with 1 packet lost out of 60.

Results are recorded in the final test summary below, together with the root cause of the
systematic overshoot.

The overshoot is systematic rather than random, and it differs per axis because the two finalizers
do different work at the deadline:

- `guider_guide_dec_finalizer()` calls `synscan_stop_axis_and_wait()`, which sends `:K2` and then
  polls `:f2` until the axis reports stopped. `synscan_wait_axis_stopped()` sleeps 100 ms between
  status queries, so the completion is quantised to that poll interval plus the axis deceleration.
- `guider_guide_ra_finalizer()` calls `synscan_slew_axis_at_rate()` to restore the tracking rate.
  Because the guide rate and the tracking rate differ, the cached axis configuration does not
  match, so that path stops the axis with `synscan_stop_axis_and_wait()` first, then issues `:G1`,
  `:I1` and `:J1`. It therefore pays the same 100 ms-quantised stop-and-wait as DEC plus three
  further command round trips.

This is a property of the SynScan protocol as the driver currently uses it: the motor controller
requires a stopped axis before the step period is changed, and the driver must confirm the stop.
Reducing it would mean changing the manner in which the driver issues protocol commands, which
`indigo_drivers/AGENTS.override.md` gates behind a dedicated characterization test and an
original-driver reference-trace comparison. That work is not part of this hardware validation and
is recorded here as a known characteristic, not as a fixed defect.

Practical consequence for autoguiding, so the number is not left uninterpreted: at the 50 % guide
rate used here an RA overshoot of about 300 ms adds roughly 300 ms x 0.5 x sidereal, about 2.2
arcseconds, of extra correction beyond the requested pulse. For short pulses that is a large
relative error, and a guiding agent tuning against this driver should be aware of it.

### D3 - an accepted coordinate slew could be reported as finished before it started

- Observable impact: immediately after a goto request was accepted, `MOUNT_EQUATORIAL_COORDINATES`
  could go `INDIGO_BUSY_STATE` and then back to `INDIGO_OK_STATE` while still carrying the old
  pointing, before the mount had moved at all. A client that treats the property leaving BUSY as
  arrival, which is how the INDIGO mount agent sequences a slew, would conclude the slew finished
  instantly and move on while the mount was only starting to turn.
- Reproduced on hardware, intermittently: it occurred in the third and sixth hardware runs and not
  in the others, which matches a race whose window is the duration of one poll cycle once per
  second. In both failing runs the mount was still at the starting declination after the property
  had reported OK.
- Root cause: `synscan_update_mount_coordinates()` derives the coordinate property state from
  `PRIVATE_DATA->global_mode`, and `global_mode` only became `SYNSCAN_GLOBAL_SLEWING` inside
  `mount_equatorial_coordinates_handler()`. The generated change branch publishes BUSY and queues
  that handler, but the mount poll performs several UDP transactions per cycle, so a poll already
  executing when the request arrived finished afterwards, still saw `SYNSCAN_GLOBAL_IDLE`, and
  republished the property as OK over the BUSY the change branch had just published.
- Fix: claim the slew synchronously in the change branch through an `on_change_request` block in
  `indigo_mount_synscan.driver`, which the generator emits before the values are copied and before
  the handler is scheduled. The block sets `global_mode` to `SYNSCAN_GLOBAL_SLEWING` under exactly
  the condition in which the generated branch will schedule the handler: the coordinate-set action
  is not SYNC, the global mode is idle, and the property is not already BUSY. A SYNC performs no
  motion and must not claim a slew, and the idle and not-BUSY conditions keep a request arriving
  during a park or home from relabelling that operation as a slew. The handler continues to set the
  mode itself, so the assignment is idempotent.
- Regression test: `synscan_syncs_and_slews` on hardware now runs the slew through
  `slew_reports_busy_until_arrival()`, which samples the property across several poll cycles after
  the request is accepted and fails if it reports OK while the mount is not yet at the target
  declination. The ordinary arrival waits in the suite were also changed to require the reported
  pointing to match the request rather than trusting the property state alone, so no other
  scenario can be satisfied by a stale OK.
- Not reproducible against the serial simulator: the simulator answers fast enough that the poll
  cycle does not overlap the request in the way the networked mount's latency produces, so this
  defect is covered by the hardware suite only. That is recorded here rather than worked around.

### D4 - overlapping guide pulses were silently discarded

- Observable impact: a guide pulse request arriving while another pulse on the same axis was still
  running never reached the driver. The client got no update, no message and no alert, and the
  running pulse ended on its original deadline. `README.md` described overlapping pulses as
  extending the active pulse and an opposite-direction pulse as cancelling it; neither happened.
- Reproduced on hardware: a 1500 ms north pulse followed after 700 ms by a second 1500 ms north
  pulse finished in 1658 ms, which is the first pulse's own 1500 ms plus the usual DEC finalizer
  overhead. The second request had no effect at all.
- Root cause: the generated change branch dispatched `GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC`
  through `INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE`, which is guarded by
  `if (p->state != INDIGO_BUSY_STATE)`. An active pulse holds the property in `INDIGO_BUSY_STATE`,
  so the second request was never copied and the handler was never scheduled. The deadline
  arithmetic in the `.driver` `on_change` blocks was therefore unreachable in exactly the
  situation it was written for.
- Why the existing simulator coverage did not catch it: the guider case in
  `indigo_test/integration/test_mount_synscan_simulator.c` issued a 200 ms pulse followed
  immediately by a 400 ms pulse and then only waited for the item to return to zero. It never
  measured how long the pulse actually lasted, so it passed whether the second request took effect
  or was dropped. Step 9 of this document recorded that case as covering "same-direction DEC guide
  pulse extension"; that claim was too strong and is corrected here.
- Chosen behaviour, decided by the user: an overlapping pulse **replaces** the running one. The
  axis runs for the new duration in the new direction, measured from the new request. Replacement
  is simpler than the extension rule the driver previously attempted to implement and never
  achieved, and it is what a guiding agent issuing a correction actually wants.
- Fix, in three parts:
  - `indigo_libs/indigo/indigo_bus.h` gained `INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME`,
    the unguarded counterpart of the existing priority dispatch, alongside the unguarded
    `INDIGO_COPY_VALUES_PROCESS_CHANGE_ANYTIME` that `MOUNT_MOTION_*` already uses for the same
    reason.
  - `indigo_tools/indigo_generator.c` gained an opt-in property attribute `accept_while_busy`. A
    `GUIDER_GUIDE_RA` or `GUIDER_GUIDE_DEC` property that sets it is dispatched through the
    unguarded macro; every other property, and every driver that does not opt in, keeps the
    previous BUSY-guarded dispatch. The flag is off by default deliberately: 21 generated drivers
    dispatch guide-pulse properties and their handlers were not written to be re-entered, so a
    global change would have altered the 20 that could not be tested here. `mount_synscan` is the
    only driver that opts in; spot-regenerating `guider_asi`, `ccd_asi`, `mount_ioptron` and
    `mount_lx200` produced no diff, confirming the attribute is inert when unset.
  - `indigo_mount_synscan.driver` opts both pulse properties in and implements replacement: the
    handler cancels the pending finalizer of the pulse it supersedes and sets the deadline to the
    new one unconditionally, instead of keeping the later of the two deadlines.
- Regression tests, both of which were verified to fail against a build with the opt-in removed:
  - `synscan_guider_passes_serial_compliance_checks` in the simulator suite now measures the
    elapsed time of two overlapping sequences. A 2000 ms north pulse replaced after 500 ms by a
    600 ms north pulse must finish in about 1100 ms; without the fix it took 2057 ms. A 2000 ms
    north pulse replaced after 500 ms by a 300 ms south pulse must finish in about 900 ms.
  - `synscan_guides` in the hardware suite runs the same two sequences against the mount. The
    second pulse is deliberately shorter than the remainder of the first, so replacing and keeping
    the superseded deadline cannot produce the same elapsed time.
- Driver version bumped to 3.

### D5 - a single lost UDP reply silenced the mount for the rest of the session

- Observable impact: one datagram lost on the way back from the mount permanently disabled the
  connection. Every later command failed, the mount stopped responding to anything, and the driver
  kept reporting itself connected the whole time. Only an explicit disconnect and reconnect
  restored it. On the Wi-Fi link to this AZ-GTi, where a measurement of 60 probes showed roughly
  two percent loss, this happened within a few minutes of use.
- Reproduced on hardware repeatedly: it struck three of the nine hardware runs made during this
  validation, each time with the same signature. A single
  `Failed to read (Resource temporarily unavailable)` line appeared, after which every scenario
  failed with the affected property in `INDIGO_ALERT_STATE`, and the run recovered only at the
  reconnect scenario near the end. The first two occurrences were initially misread as flaky
  Wi-Fi; the third made the pattern unambiguous, because a link that had genuinely dropped would
  not have come back at the exact moment the driver reopened its socket.
- Root cause: `read_data()` in `indigo_libs/indigo_uni_io.c` returns -1 immediately when
  `handle->last_error` is set, and it sets `last_error` from `errno` whenever the underlying
  `read()` fails. A UDP socket carrying a receive timeout returns `EAGAIN` on a timeout, so a
  missing reply latched `EAGAIN` onto the handle. From that point every read *and* write on that
  handle failed without touching the socket. The driver's own UDP retry, three attempts per
  command, could not help: attempts two and three hit the latch rather than the network.
- The same defect was found independently in `mount_nexstaraux` while this validation was running
  and fixed there in commit `aaee12f9e`. The remedy used here is the same one.
- Fix: in `synscan_read_response()` the UDP branch now calls `indigo_uni_wait_for_data()` with the
  command timeout before it reads. A reply that never arrives is detected without issuing a read,
  so nothing latches on the handle and the existing retry loop can actually retry. The serial
  branch already used `indigo_uni_read_section2()` with explicit timeouts and was never affected.
- Driver version bumped to 4.
- Regression test: `synscan_mount_survives_lost_udp_replies` in
  `indigo_test/integration/test_mount_synscan_simulator.c`. The SynScan simulator gained a
  `--drop-nth-reply <n>` option that executes a command normally but withholds its reply, which is
  what a datagram lost on the way back looks like to the driver. The test connects over UDP to a
  simulator dropping every seventh reply, a far worse rate than the real link, and then runs
  several further transactions. It was verified to fail against a build with the
  `indigo_uni_wait_for_data()` call removed, reproducing the exact hardware signature: the
  connection itself fails after the first `Failed to read (Resource temporarily unavailable)`.
- Worth noting for other drivers: any driver that reads from a `indigo_uni_handle` with a socket
  receive timeout through a bare `indigo_uni_read()` has this defect. Two have now been found.

## Final test summary

Date: 2026-09-20. Driver version at completion: 4.

Simulated tests, hardware free, `indigo_test/integration/test_mount_synscan_simulator.c`:

- total run: 18
- passed: 18
- failed: 0

Hardware tests, physical Sky-Watcher AZ-GTi reached over UDP at
`synscan://192.168.111.139:11880`, `indigo_test/hardware/test_mount_synscan_hw.c`:

- total run: 16
- passed: 16
- failed: 0

Guide pulse duration accuracy, hardware, 5 samples per duration per axis, mount tracking at the
sidereal rate with a 50 % guide rate. Public-property completion timing over the UDP transport, not
an electrical measurement of the ST4 output:

| Axis | Requested | Mean error | Mean absolute error | Min | Max |
| --- | --- | --- | --- | --- | --- |
| RA | 100 ms | +317.2 ms | 317.2 ms | +290.0 ms | +356.6 ms |
| RA | 250 ms | +339.9 ms | 339.9 ms | +310.0 ms | +385.1 ms |
| RA | 500 ms | +343.9 ms | 343.9 ms | +311.6 ms | +364.4 ms |
| RA | 1000 ms | +324.2 ms | 324.2 ms | +288.7 ms | +370.0 ms |
| RA | 2000 ms | +344.0 ms | 344.0 ms | +329.1 ms | +364.0 ms |
| DEC | 100 ms | +183.2 ms | 183.2 ms | +174.2 ms | +191.1 ms |
| DEC | 250 ms | +180.8 ms | 180.8 ms | +150.0 ms | +214.4 ms |
| DEC | 500 ms | +170.1 ms | 170.1 ms | +149.8 ms | +193.3 ms |
| DEC | 1000 ms | +169.8 ms | 169.8 ms | +161.7 ms | +177.6 ms |
| DEC | 2000 ms | +179.6 ms | 179.6 ms | +149.8 ms | +207.8 ms |

The error is a systematic overrun, never an early finish, and is independent of the requested
duration, which is consistent with the root cause recorded in the guide pulse accuracy section
above: the finalizer must stop or re-rate the axis at the deadline, and that costs a
100 ms-quantised stop-and-wait plus, on RA, three further command round trips.

Defects found and fixed during this validation: D1, D2, D3, D4, D5. None are left open.

D2 and D4 required changes to `indigo_tools/indigo_generator.c`, both proposed to the user and
explicitly approved before implementation, as the root `AGENTS.md` requires. D2 also changed nine
other generated mount drivers, all regenerated with their version incremented. D4 was made an
opt-in generator attribute precisely so that the twenty other drivers sharing the same dispatch
were left untouched.

## Step 12 validation results

Date: 2026-09-20

Hardware acceptance run against the AZ-GTi, final run of the session, all cases passing:

```
make -C indigo_test test-mount-synscan-hw
```

Measured values from that run, for the record:

- Identity: `Sky-Watcher SynScan`, `AZGTi`, firmware `3.16`.
- Tracking holds the reported right ascension to 0.0014 degrees over 12 s; a stopped axis lets it
  run away 0.0501 degrees over the same interval, against a sidereal expectation of 0.050.
- Manual motion at the centring rate over 3 s: RA west -0.395 degrees, RA east +0.425, DEC north
  +0.411, DEC south -0.407. Both axes move in both directions with the correct sign.
- SYNC changed the reported coordinates without moving either axis; the raw declination was
  identical before and after.
- A three-degree slew arrived at RA 14.17953 DEC 57.00000 against a target of RA 14.17936 DEC
  57.00000, and the requested post-slew tracking was started.
- A slew aborted at the moment it was accepted left the mount at its starting declination, 40
  degrees short of the abandoned target, reported `INDIGO_ALERT_STATE`, and accepted a fresh slew
  immediately afterwards.
- Park refused tracking, both motion axes and a goto, stopped the RA axis, and unparked cleanly.
- Home reached the configured home position, stopped tracking and lit `MOUNT_STATE.HOME`.
- Guide pulse travel, eight one-second pulses per direction at the 50 % guide rate, differential
  between the two directions -0.0347 degrees against an expectation of about -0.033.
- Guide pulse replacement: a 2000 ms north pulse replaced after 500 ms by a 600 ms north pulse
  finished in 1239 ms; a 2500 ms north pulse replaced after 500 ms by a 500 ms south pulse
  finished in 1231 ms; a 3000 ms north pulse replaced after 500 ms by a 500 ms north pulse
  finished in 1168 ms. All three follow the new request, not the superseded one.
- An unreachable endpoint was refused in 3.1 s, which is the three UDP attempts at one second each,
  and the real mount was usable immediately afterwards.
- No `Failed to read` occurred anywhere in the run, against three occurrences in the nine runs made
  before D5 was fixed.

Guide pulse duration accuracy, five samples per duration, mount tracking, guide rate 50 %:

| Axis | Requested | Mean error | Mean absolute error | Min | Max |
| --- | --- | --- | --- | --- | --- |
| RA | 100 ms | +314.0 ms | 314.0 ms | +300.5 ms | +344.6 ms |
| RA | 250 ms | +312.9 ms | 312.9 ms | +307.8 ms | +316.6 ms |
| RA | 500 ms | +339.7 ms | 339.7 ms | +317.1 ms | +395.5 ms |
| RA | 1000 ms | +310.8 ms | 310.8 ms | +292.1 ms | +327.9 ms |
| RA | 2000 ms | +310.0 ms | 310.0 ms | +286.5 ms | +343.0 ms |
| DEC | 100 ms | +173.1 ms | 173.1 ms | +138.8 ms | +186.8 ms |
| DEC | 250 ms | +182.6 ms | 182.6 ms | +152.4 ms | +201.1 ms |
| DEC | 500 ms | +183.5 ms | 183.5 ms | +167.8 ms | +207.9 ms |
| DEC | 1000 ms | +183.6 ms | 183.6 ms | +151.9 ms | +208.9 ms |
| DEC | 2000 ms | +166.7 ms | 166.7 ms | +143.2 ms | +198.7 ms |

The error is a systematic overshoot, never an early finish, and it is independent of the requested
duration, which is consistent with the fixed cost of the stop-and-restore work each finalizer does
at the deadline. See the guide pulse accuracy section above for the mechanism. This is
public-property completion timing, not an electrical measurement of the ST4 output.

## Final test summary

- Simulated tests run: 18. Passed: 18.
  - `indigo_test/integration/test_mount_synscan_simulator.c`, run with
    `./build/integration/test_mount_synscan_simulator` from `indigo_test`.
  - Includes the three regression tests added by this validation:
    `synscan_mount_keeps_state_when_parked_request_is_refused` for D2, the duration measurements in
    `synscan_guider_passes_serial_compliance_checks` for D4, and
    `synscan_mount_survives_lost_udp_replies` for D5.
- Hardware tests run: 16. Passed: 16.
  - `indigo_test/hardware/test_mount_synscan_hw.c`, run with
    `make -C indigo_test test-mount-synscan-hw` against a Sky-Watcher AZ-GTi, firmware 3.16,
    reached over UDP at `synscan://192.168.111.139:11880` through the driver's own broadcast
    autodetection.
  - Of the 16, one case, `synscan_runs_autohome`, reports the controller autohome procedure as not
    applicable because this AZ-GTi has no home indexer on either axis. It is counted as passed
    because the capability gate is what it verifies on this unit; the procedure itself remains
    covered against the simulator.
- Defects found during hardware validation: 5. Fixed: 5. Open: 0.
  - D1 firmware string, D2 parked-request state, D3 slew reported finished before it started,
    D4 overlapping guide pulses discarded, D5 a lost UDP reply silencing the session.
- Driver version: 1 before this validation, 4 after.

## Overlapping guide pulses, unified pattern (2026-09-20)

The replacement behaviour introduced with `accept_while_busy` is unchanged, but the way the
superseded item is dropped now matches the other twenty INDIGO guiders. `on_change` used to work out
which item was stale from `PRIVATE_DATA->guide_ra_direction` / `guide_dec_direction`; both axis items
are now zeroed in `on_change_request` before `indigo_property_copy_values()` runs, so only the
requested direction is set when the handler starts. The direction fields are still maintained and
used for the axis rate and the resume decision, they are simply no longer consulted to disambiguate
the request.

`synscan_guider_passes_serial_compliance_checks` is unchanged and still measures both replacements:
1123 ms for a 2000 ms pulse replaced after 500 ms by a 600 ms pulse in the same direction and 911 ms
for the same pulse replaced by a 300 ms pulse in the opposite direction.

Re-validated on the simulator only. The AZ-GTi hardware run recorded earlier was not repeated.

## EQMOD MountSim acceptance (2026-09-24)

This is a separate macOS run through MountSim 2.3's `EQMOD` SkyWatcher motor-controller
profile, not the SynScan hand-controller profile or a physical mount. The existing generated
driver (version 5) and MountSim app both built before behavior changes. The unchanged
driver failed its first connection against the unchanged EQMOD profile: the raw relay trace
ended at `:s1\r` without an RX frame, followed by `CONNECTION` ALERT. The manufacturer
command set specifies a success frame or a `!00\r` unknown-command error for that inquiry.
The original raw trace excerpt was `324379.760974 TX 3a 73 31 0d` (`:s1\r`)
with no following RX; the replay after the profile correction showed an error frame and a
successful connection. The initial per-case capture was replaced by the subsequent launcher
run, so this excerpt is the retained baseline evidence. Both runs used `run_mountsim.py`
with the per-user flock, isolated app instance, `--mount EQMOD`, and `--terminator 0d`.

The independent references are the bundled SkyWatcher motor-controller command-set PDF,
INDI's `skywatcherAPI.cpp`/`.h`, and AstroEQ's `commands.c`/`synta.c`. They agree that `s`
is a PEC-period inquiry, `E` sets a stopped motor counter, `j` reads it, errors use `!` plus
two hex digits, and ST4 code 4 means 0.125 sidereal. The PDF calls `q` an inquiry, so it
must not clear axis initialization. Raw EQMOD tests exercise `s`, `q`, `E`/`j`, status
preservation, and actual elapsed-time encoder motion. The old profile ACKed `E` but still
returned `=000000\r` to `j` for both programmed axes. A separate GOTO trace showed the
counter remained fixed while the status was running: EQMotor's motion method was not
reached by the base timer. The corrected profile keeps physical pose continuous under `E`
with an independent 24-bit reported counter and advances motor motion from elapsed timer ticks;
the counter no longer jumps when the mechanical position wraps at one revolution. The EQMOD
parser also used a truncated `!0` or silence for malformed/unknown commands; it now returns
the command-set's full `!00` unknown, `!01` bad-length and `!03` invalid-character frames.

The driver-side guide-rate defect was separate: `MOUNT_GUIDE_RATE` is specified and initialized
by INDIGO in whole percent, but `synscan_set_st4_guide_rate()` compared its input with fractions.
A 75% request emitted `:P10\r` instead of `:P11\r` in the baseline relay trace. Version 6
converted to a fraction before mapping the documented P codes; the post-change case covers
100, 75, 50, 25 and 12.5 percent. The long-motion cases then reproduced a second driver
defect: coordinate, park and home handlers waited synchronously for both axes, blocking abort
behind them. Version 7 starts each operation and polls with named finalizers, with cancellation
on abort/disconnect and terminal state updates after completion or failure. The generated
`INDIGO_COPY_*_PROCESS_CHANGE` guards own initial BUSY and reject a second same-property target;
the abort handler explicitly publishes OK because its finalizer cancellation references suppress
the generator's normal epilogue. SLEW completion clears the stale ON tracking property after the
motor has stopped tracking. Abort and failed motion also publish tracking OFF; the generated
disconnect path already cancels all pending handlers, so no separate finalizer cancellation is
needed there. On successful completion the finalizer refreshes motor and translated coordinate
readback before publishing terminal OK, so that event contains the arrived position rather than
the previous periodic sample.

The opt-in `indigo_test/mountsim/test_mount_synscan_mountsim.c` suite maps the mount class
requirements to named cases: raw controller contract, identity/init/reconnect, shared
mount/guider lifetime, SYNC and actual GOTO arrival including wrap-relative target,
BUSY/abort/recovery, all advertised manual directions and rates, tracking rates and ST4
mapping, signed park/home arrival and interruption, guider direction/replacement/overlap,
idle/motion/pulse transport loss, and two guide-timing workloads. The timing groups use
20/100/500 ms in four directions, one discarded warmup and three measured repetitions per
duration and workload. They report J-to-K transport-command timing and RA restore timing
separately. This is software/PTY scheduling evidence; it does not measure physical ST4
electrical output, mechanical pointing accuracy or behavior on an actual EQ6.

Two exploratory harness oracles were corrected after checking the relay trace and property
lifecycles. RA EAST and WEST guide pulses both select the same motor direction (`G110`),
while their `I` step periods differ around sidereal tracking; the test now checks both
periods. A guider-originated transport loss publishes ALERT on the active guider pulse and
disconnects the shared mount, but idle mount coordinates are deleted without first becoming
ALERT. The pulse-loss case therefore checks the guider ALERT and mount disconnect; the
separate moving-mount loss case checks coordinate ALERT.

The finished macOS arm64 MountSim 2.3 EQMOD run passed 15/15 named cases with a fresh app
instance per case and the launcher lock. It includes real elapsed counter motion, nontrivial
GOTO/arrival, signed park/home and interruption, reconnection after three transport-loss
states, all manual directions/rates, tracking/ST4 command mapping, and guider replacement,
overlap and completion. Each timing workload discarded one warmup per direction and measured
three repeats of 20, 100 and 500 ms per direction (36 samples per workload). Forwarded J-to-K
signed error was 0.346/6.257/17.714 ms (min/median/max) under tracking with idle polling,
and 0.353/5.053/10.324 ms under tracking, other-axis guiding and polling. RA tracking
restoration was measured from K to the next J separately. These are host/PTY command edges,
including scheduler delay, rather than electrical ST4 or mechanical movement timing. No
physical EQDIR mount was used; mechanical pointing, guide electrical output, exact physical
GOTO rates, and macOS x86_64/Linux/Windows behavior remain unverified.

The existing portable SynScan simulator suite also passed 18/18 in its normal
macOS arm64 build after the EQDIR changes. A separate ASAN/UBSAN build completed
17/18 without a sanitizer diagnostic: only UDP broadcast autodetection timed
out. Its explicit UDP URL and dropped-reply cases passed. The isolated
autodetection case also timed out under sanitizer, while a standalone UDP
socket received the simulator's `=020304\r` reply via both loopback and
broadcast. An isolated `indigo_perform_active_discovery()` probe timed out in
both normal and instrumented builds, even though the normal full suite passed
autodetection. This leaves an intermittent host/framework broadcast-discovery
limitation outside the EQMOD relay path, not an established memory error.

### UDP autodetection follow-up (2026-09-24)

The portable test used the shared five-second connection wait, but one call to
`indigo_perform_active_discovery()` can make five one-second receive attempts,
and the driver permits eight such calls with pauses. A lost first reply could
therefore make the test tear down the driver before its documented retry path
finished. The UDP simulator can now discard its first five replies on request;
the autodetection case uses this deterministic failure and waits through the
driver's bounded retry budget. The focused normal case passed after those five
losses, and the full portable suite passed **18/18**. The production driver and
shared discovery helper are unchanged.

The complete ASan/UBSan rerun passed **17/18**; its isolated and full-suite
autodetection cases still timed out without a sanitizer diagnostic. Socket
instrumentation showed all C-process broadcasts
were sent successfully and every receive timed out. A plain C UDP probe showed
the same behavior, while a Python socket received the simulator's reply and the
unchanged INDIGO discovery helper also succeeded when called from Python via
`ctypes` against the same running simulator. This confines the remaining failure
to local broadcast delivery for these test executables on this macOS host;
the exact host permission or routing policy is not established. Direct loopback
UDP and the ordinary portable executable work. The sanitizer's UDP broadcast
case remains **unverified**, rather than counted as passed or diagnosed as a
driver memory defect.

## Found defect — SYNSCAN-D01, a guide pulse killed the process (FIXED, version 8)

Found by the first hardware run of this driver against a physical AstroEQ 8.25 ESP32-S3 controller
on 2026-09-24, reached over serial from a Raspberry Pi 5 (Debian 12, aarch64).

Observable impact: the whole driver process died with SIGSEGV on the first DEC guide pulse that
reached the stop-and-wait path. Backtrace, driver at `c5a329c20`:

```
Thread 4 received signal SIGSEGV
#0  synscan_read_mount_coordinates (device=0x5555555be760) at indigo_mount_synscan.c:978
#2  synscan_update_mount_coordinates (device=0x5555555be760) at indigo_mount_synscan.c:1008
#3  synscan_wait_axis_stopped (device=..., axis=SYNSCAN_AXIS_DEC, abort=0x0) at indigo_mount_synscan.c:595
#4  synscan_stop_axis_and_wait (abort=0x0, axis=SYNSCAN_AXIS_DEC, device=...) at indigo_mount_synscan.c:507
#5  guider_guide_dec_finalizer (device=...) at indigo_mount_synscan.c:1627
#7  queue_func (...) at indigo_timer.c:945
```

Root cause: `guider_guide_dec_finalizer` passes its own logical device down that chain, and
`synscan_read_mount_coordinates` / `synscan_update_mount_coordinates` read and publish `MOUNT_*`
properties. Every `MOUNT_*` macro resolves through `MOUNT_CONTEXT`, that is `device->device_context`,
which on the guider is an `indigo_guider_context`. The read was type-confused. Publishing there would
also have sent mount properties under the guider's name, so the wrong device was wrong in two ways.

Fix: both helpers resolve the master device themselves, which is what the root `AGENTS.md` prescribes
for a helper that needs another logical device's property context. The guider's RA finalizer is not
affected - `synscan_slew_axis_at_rate` touches no `MOUNT_*` property - and `synscan_update_mount_state`
is only reached through `synscan_update_mount_coordinates`, so it inherits the resolved device.

Why no hardware-free test caught it: `synscan_wait_axis_stopped` only updates coordinates from inside
its polling loop, and the simulator cleared `RUNNING` the instant `:K` was acknowledged, so the wait
returned on its first iteration and never got there. The bench controller reported `=210`, still
running, for minutes after the stop. The simulator now models that deceleration behind a new
`--stop-lag <n>` option, which keeps an axis reporting `RUNNING` for `n` status queries after a stop.

Regression test: `synscan_guider_finishes_a_dec_pulse_while_the_axis_decelerates` in
`indigo_test/integration/test_mount_synscan_simulator.c`, which runs the simulator with
`--stop-lag 3`, connects the guider through its master, issues a DEC pulse and requires both the pulse
to complete and the driver to still answer afterwards. Verified A/B on macOS arm64: against the
pre-fix driver the suite dies with SIGSEGV in exactly that case (exit 139, no verdict printed, the
case is the last one entered); against the fixed driver the suite is 19/19, exit 0.

## Hardware run — AstroEQ 8.25 on ESP32-S3, Linux arm64 (2026-09-24)

`SYNSCAN_HW_URL=/dev/ttyACM0 make -C indigo_test test-mount-synscan-hw`, 5 of 16 cases passed, so
**this is not a passing hardware run**. The crash above is gone - `synscan_guides` now runs to a
verdict instead of taking the process down - and identity, coordinate and state readback, tracking and
rates, and the two cases that do not apply to this controller all pass.

Two things still stand in the way, neither of them the guider defect:

* **A commanded GoTo creeps.** The driver sends the whole sequence (`:K`, `:f`, `:j`, `:G`, `:H`, `:M`,
  `:J`) and the controller does step - RA went from `0x800051` to `0x7F90DB`, 3958 counts, in the two
  minutes before the timeout - but that is about 33 counts per second, roughly six times sidereal on a
  460800-count axis, where a 96076-count slew needs to finish in seconds. The driver asked for period
  600 with `:I` (the configured sidereal `IVal`) and selected motion mode with `:G101` / `:G121`.
  Attributing this needs `skywatcher_motor_controller_command_set.pdf`: either the driver is selecting
  a low-speed GoTo where it wants high speed, or this AstroEQ port is not honouring the high-speed
  selection. It was not resolved here and no change was made for it. Three cases fail on the timeout
  directly and the later cases cascade from `prepare_mount()` failing.
* `synscan_discovers_and_connects` asserts that `DEVICE_PORT` was rewritten to a `synscan://`
  address, which only holds for the UDP transport the suite was written for. It cannot pass over a
  serial port as written; that is a limitation of the test, not a defect.

The axes were left stopped (`:K1`, `:K2`, both then reporting `=100`). No motors or sensors were
attached, so nothing here validates physical motion.

### The portable suite did not run on this host - diagnosed and fixed in the framework

`test_mount_synscan_simulator` failed every serial case on the Pi with `No SynScan response from
/dev/pts/N` at connect, 4 of 19, while the same commit passed 18/18 on macOS arm64. Reverting every
driver change of this session reproduced it, so it was not the guider fix. The cause was in the
framework, not in this driver or its suite.

`indigo_uni_is_valid()` in `indigo_libs/indigo_uni_io.c` treated a failing `ioctl(TIOCMGET)` as a lost
connection. A pseudo-terminal slave has no modem lines, so that ioctl fails on one - measured as
`errno 25`, `ENOTTY`, on both Linux and macOS. The function carried a **macOS-only** exemption for
names starting with `/dev/ttys`, which is what macOS calls its pseudo-terminal slaves; Linux calls
them `/dev/pts/N` and had no equivalent, so on Linux every PTY-backed handle was reported as
disconnected at the first command. The timing in the trace makes it unambiguous: the connection was
declared lost 7 microseconds after the port was opened, before any command was written.

```
10:58:23.590904  0 <- // /dev/pts/1 opened
10:58:23.590911  0 <- // Lost connection
```

It only hit the serial cases because `synscan_validate_handle()` returns early for a UDP handle, which
is exactly the four cases that passed.

Fixed by exempting `ENOTTY` instead of matching device names, which covers both platforms and retires
the macOS special case. An earlier attempt also exempted `EINVAL` on the assumption that it meant the
same thing; it did not, and it broke `ioptron_guider_directions_overlap_and_timing` on macOS, so the
exemption is limited to the one errno that was actually measured.

Verified on both platforms after the change: `mount_synscan` 19/19 on Linux arm64 and 19/19 on macOS
arm64, `mount_nexstaraux` 40/40 on both, `mount_lx200` 99/99 and `mount_ioptron` 105/105 on macOS as
regression controls - `mount_ioptron` was 105/105 before the change too, and 104/1 with the `EINVAL`
version, which is how that regression was caught. Note that `mount_lx200_simulator.c` and
`mount_ioptron_simulator.c` do not compile at all with gcc on Linux, failing
`-Werror=format-truncation` and `-Werror=stringop-truncation`, so those two suites could not be run
there; that is a separate matter from this fix.
