# Driver Testing Rules

This document is a reference for automated driver compliance tests. It summarizes the default property visibility from the base driver implementations in `../indigo_libs/` and the compliance scenarios from `../indigo_tests/*_compliance.sh` plus the in-process simulator tests in `integration/`.

Concrete drivers may unhide optional properties, reduce item counts, add driver-specific properties, or expose multiple logical devices. Tests should assert the base contract first, then assert the concrete driver extensions explicitly.

## Common Device Rules

Source: `../indigo_libs/indigo_driver.c`.

Before connection, every normal device should expose the common device properties that are not hidden by the concrete driver:

- `INFO`
- `CONFIG`
- `PROFILE_NAME`
- `PROFILE`
- `CONNECTION`

Common properties available but hidden by default:

- `SIMULATION` unless the device is a simulator.
- `DEVICE_PORT`
- `DEVICE_PORTS`
- `AUTHENTICATION`
- `ADDITIONAL_INSTANCES`

After connection, the common visible properties remain visible and the class-specific base driver properties are defined by the corresponding `indigo_<class>_enumerate_properties()` function.

Common compliance scenarios:

- Verify `INFO.DEVICE_INTERFACE` contains the expected interface bit.
- Enumerate before connection and assert the expected common visible properties.
- Connect through `CONNECTION.CONNECTED=ON` and wait for `CONNECTION` to reach `INDIGO_OK_STATE`.
- Enumerate after connection and assert class-specific visible properties.
- Exercise representative writable properties through public bus APIs.
- Disconnect through `CONNECTION.DISCONNECTED=ON` and wait for `CONNECTION` to reach `INDIGO_OK_STATE`.
- Disconnect again while already disconnected when the driver class should tolerate it.
- Shut down with no connected devices.

## Test scope shared by all driver classes

The class matrices below specify required coverage where the driver implements the capability; they do not claim that existing tests already cover it. Use the property inventories as visibility references, not as instructions to retest framework behavior.

- Test the production driver through public bus requests, with a fake SDK/USB boundary or a protocol simulator for serial/network devices. Check commands, argument units, order, readback and property transitions together. Keep real queues and timers; do not replace the driver with a model of its implementation.
- Test only driver-owned behavior. Generic property copying, range clamping, configuration storage, coordinate/alignment math and image encoding belong in framework tests. Check their integration only when the driver supplies parameters, overrides behavior or translates requests into device commands. An inherited property needs a visibility check, not another exhaustive framework test.
- Framework-validated request values are the driver input contract. Do not inject generic NaN, fractional integer-step or out-of-range property requests into driver tests, and do not add driver guards for them. Test device replies, hardware-specific restrictions and operational conflicts such as BUSY instead.
- Cover metadata/capability discovery, connect/disconnect/reconnect, INIT/SHUTDOWN, initialization rollback and balanced resources. For hot-plug implementations cover identity, duplicate events, failed attachment, capacity and removal. For shared devices cover each logical interface alone, both connection orders, sibling survival and last-close ownership.
- Inject failures at distinct driver decision points: open, command/write, readback/poll and stop. Include short/malformed replies, invalid SDK outputs and bounded timeout recovery where the protocol applies. Failed requests must not silently become OK or overwrite accepted values with invalid output.
- Check queued operations, completion versus abort/disconnect races, recurring polling cancellation and absence of calls after close or updates after detach. Use deterministic gates and bounded waits, not repeated long sleeps. Assert fresh property revisions so stale OK cannot pass a test.
- Test driver-specific settings and persistence by confirming restored device commands/readback. Restore original hardware settings; keep automated configuration in temporary directories. For persistent names/suffixes test boundaries and failure handling with the fake, and one write/reconnect/restore cycle on applicable hardware.
- Keep default tests hardware-free. Run real transport acceptance only when explicitly requested and with available devices. Record unavailable capabilities; do not require external measurement equipment. Do not rerun hardware tests for documentation or test-harness-only edits.
- Use a small representative matrix for distinct driver branches, not a Cartesian product. Run only affected cases after test-only changes; broaden validation for changed production behavior or unresolved failures. Extended timing/load runs are separate opt-in work.
- Record scenario-to-test mappings, automated gaps and physical evidence in the relevant driver's `REFACTOR.md`, and follow cleanup/project-file rules in `AGENTS.md`. Existing source examples are starting points, not evidence that another driver is covered.

For every class, hardware acceptance includes discovery/connection, the class-specific workflow below, disconnect/reconnect and fresh operation. Test actual transport loss during idle and active work where the transport supports it; verify recovery and no hung teardown. Distinguish INIT/SHUTDOWN from actual library unload/reload. Exercise multiple physical devices only when available.

## CCD Drivers

Source: `../indigo_libs/indigo_ccd_driver.c`.

Visible before connection:

- Common visible properties only.

Visible after connection by default:

- `CCD_INFO`
- `CCD_LENS`
- `CCD_LOCAL_MODE`
- `CCD_IMAGE_FILE`
- `CCD_MODE`
- `CCD_EXPOSURE`
- `CCD_FPS`
- `CCD_ABORT_EXPOSURE`
- `CCD_FRAME`
- `CCD_BIN`
- `CCD_FRAME_TYPE`
- `CCD_IMAGE_FORMAT`
- `CCD_UPLOAD_MODE`
- `CCD_PREVIEW`
- `CCD_IMAGE`
- `CCD_FITS_HEADERS`
- `CCD_SET_FITS_HEADER`
- `CCD_REMOVE_FITS_HEADER`
- `CCD_JPEG_SETTINGS`
- `CCD_JPEG_STRETCH_PRESETS`

Available but hidden by default or driver-dependent:

- `CCD_READ_MODE`
- `CCD_STREAMING`
- `CCD_STREAMING_SETTINGS`
- `CCD_OFFSET`
- `CCD_GAIN`
- `CCD_EGAIN`
- `CCD_GAMMA`
- `CCD_PREVIEW_IMAGE`
- `CCD_PREVIEW_HISTOGRAM`
- `CCD_COOLER`
- `CCD_COOLER_POWER`
- `CCD_TEMPERATURE`
- `CCD_RBI_FLUSH_ENABLE`
- `CCD_RBI_FLUSH`

Compliance scenarios:

- Verify CCD interface bit.
- Assert `CCD_INFO.WIDTH` and `CCD_INFO.HEIGHT`.
- Assert `CCD_EXPOSURE.EXPOSURE`, `CCD_ABORT_EXPOSURE.ABORT_EXPOSURE`, `CCD_FRAME.WIDTH`, and `CCD_FRAME.HEIGHT`.
- If `CCD_BIN` is visible, assert `HORIZONTAL` and `VERTICAL`.
- Assert `CCD_IMAGE_FORMAT.RAW` where the driver supports raw format.
- Assert `CCD_UPLOAD_MODE.CLIENT`.
- Assert `CCD_IMAGE`.
- Validate numeric ranges for exposure and frame size.
- Exercise a short exposure where simulator or fake I/O can complete deterministically.
- Exercise abort handling only when an exposure can be placed into `BUSY`.
- Treat cooler, temperature, streaming, preview, read mode, gain, offset, gamma, and RBI properties as optional unless the concrete driver documents them as visible.

### Camera Driver Test Standard: Fake SDK/USB and Hardware

Apply this standard when implementing, migrating or extending CCD driver tests. It is the union of the scenarios in `indigo_test/integration/test_ccd_touptek_sdk.c`, `indigo_test/integration/test_ccd_playerone_sdk.c`, their corresponding `indigo_test/hardware/test_ccd_*_hw.c` programs, and the SX-specific validation requirements in `indigo_drivers/ccd_sx/REFACTOR.md`. There is currently no dedicated CCD SX automated test in the repository; `test_ao_sx_simulator.c` tests a different driver. SX requirements below are required coverage, not evidence of completed tests.

#### Scope and evidence

- Test only behavior implemented by the driver: SDK/USB arguments, raw image handoff, driver-owned property behavior, acquisition, guiding and lifecycle. Framework codecs, output containers, upload destinations and generic property/configuration mechanics belong in framework tests. Do not multiply driver acquisitions across JPEG/TIFF/FITS/XISF/SER/AVI encodings. Test configuration only where it restores driver-specific SDK settings.

- Maintain two separate layers: hardware-free integration tests prove driver logic with a fake SDK or USB transport; opt-in hardware tests prove behavior with the real transport, vendor library and camera. Neither substitutes for the other.
- Apply each scenario to capabilities the driver exposes. Test both supported and unsupported capability profiles with the fake; on hardware, record unavailable capabilities/models as deferred with a reason. Do not require a guider, cooler, streaming, wheel or focuser on a camera that does not provide it.
- Inventory every published property and writable item, including inherited base properties and every logical device. Map each applicable scenario below to a named test and its assertions. Record missing coverage in the relevant driver's `REFACTOR.md`; a reference driver's passing suite does not establish compliance for another driver.
- Keep the normal integration target free of physical USB access, vendor SDK discovery, servers and network sockets. Place real-camera programs under `indigo_test/hardware/`, require explicit opt-in, and exclude them from normal `test`/`test-integration` targets. Add new source files to relevant project groups, including Xcode.

#### Required fake SDK or fake USB coverage

Compile the production driver separately and exercise its public entry point and bus properties with the real framework, handler queues and image processing. Replace the SDK/USB boundary; for direct-libusb drivers such as SX, emulate the actual protocol commands and transfers rather than inventing an SDK. Keep injection hooks local to test builds.

Generate deterministic noise for fake camera SDK/USB image data and file-camera test inputs; do not use or link the image arrays from `ccd_simulator` or other photographs. Use a fixed seed or coordinate/channel-addressable generator so tests can verify ROI/bin mapping, channel order, bit depth and payload integrity reproducibly. When testing `ccd_simulator` itself, exercise its production image generation without duplicating its built-in images as expected-data fixtures.

| Area | Required scenarios and assertions |
| --- | --- |
| Identity and lifecycle | Driver metadata and interface bits; INIT, enumeration, connect/disconnect and SHUTDOWN; repeated INIT/SHUTDOWN; rejection of shutdown while connected without losing active work. Exercise CCD-only, guider-only, both connection and disconnection orders, and each additional logical interface. Verify one shared open/init, no premature close while a sibling remains connected, and one final close/unlock. |
| Discovery and ownership | Mono/color and optional-interface profiles; combined camera/guider/wheel/focuser discovery where supported; duplicate and burst arrivals/removals; failed master/slave attachment and retry; capacity overflow/recovery; distinct physical cameras with nonmatching USB-event and SDK-enumeration order, nontrivial SDK ids and duplicate model names. Verify correct detach identity, no orphan logical devices, and balanced handles, references, reservations and locks. |
| Initialization failures | Inject queue/registration, global-lock, open/init, USB claim/reset/parameter-read and required property-discovery failures where applicable. Verify rollback, public disconnected/ALERT state, no use of invalid outputs and successful subsequent connection. Unsupported optional capabilities must follow their documented hidden/fallback behavior. |
| Property contract | Enumerate before/after connection and after disconnect. Assert names (`X_` for custom properties), item names/counts, types, permissions, visibility, ranges, values/targets and capability-dependent changes. Exercise writable properties handled by the driver, valid limits and invalid requests, mode/format synchronization and reconnect rebuilding. Rejected busy changes must preserve accepted values and avoid SDK writes. |
| Controls | Gain, offset, egain, presets/conversion gain, sensor/readout/bin modes, advanced controls, fan/heater/LED and other exposed controls: verify SDK arguments, units, readback, partial/error paths and recovery. |
| Cooling and temperature | Supported/unsupported cooler profiles; cooler ON/OFF, temperature target conversion and separate measured value, power readback and units. Test periodic polling, settling/busy state, individual temperature/power/target/enable read and write failures, partial failures and recovery. Cross the first polling deadline during slow initialization; disconnect with polling pending and assert no SDK calls after close. Verify restored cooler settings where driver-owned. |
| Image contract | Every advertised SDK pixel format/bit depth; raw image handoff dimensions, payload length and deterministic pixel content, including channel order/Bayer metadata where applicable. Exercise full frame, ROI offsets/bounds/alignment, supported and invalid bins, frame types (light/bias/dark/flat/dark-flat), exposure-unit conversion and shortest bias exposure. Validate SDK geometry as well as delivered geometry. |
| Acquisition | Short and long exposure, BUSY followed by completion, overlap rejection, pending/not-ready reads and bounded retries, watchdog expiry, start/setup/readout failures and recovery. Abort before completion and at controlled setup/readout/final-frame boundaries; immediately reacquire. Assert exactly one successful frame or the expected failure, no duplicate publication/finalization and no stale error/frame affecting a new acquisition. |
| Streaming | Finite exact frame count, indefinite streaming, stop/abort/restart, SDK errors/timeouts and removal during streaming. Verify raw frame delivery and driver-owned completion/cleanup; use one output format without retesting framework encoders or upload destinations. |
| Guider | Apply the [Guider Driver Test Standard](#guider-driver-test-standard), including shared-camera acquisition and lifecycle interactions. |
| Queue and callback races | Prove SDK/USB operations run on the intended queue and serialize access to shared handles. Use barriers to force both final-frame/abort orders, disconnect during an SDK callback, stale notifications during stop/reconfiguration, removal with exposure/pulse/poll work pending, and shutdown with queued discovery. Verify rejected shutdown leaves required work alive; accepted shutdown drains/cancels it. Assert no calls after close, updates after detach, duplicate close or deadlock, then reconnect/reload and reacquire. |
| Configuration and strings | SAVE/change/LOAD roundtrip for persistent settings and every relevant logical interface; confirm restored SDK state, not only CONFIG OK. Test custom-property renames and driver-specific persistence. For suffix/name controls test empty, boundary and over-limit lengths, write failures, reconnect and bounded SDK strings; assert public state and accepted value. |
| Additional interfaces | Apply the [wheel](#wheel-driver-test-standard), [focuser](#focuser-driver-test-standard) or [rotator](#rotator-driver-test-standard) standard to each exposed logical device, including shared ownership and independent operation. |
| SX protocol/readout specifics | Model/PID recognition and USB-path identity; open/claim/reset and camera parameters; pending hardware timer versus completed image; long-exposure register-clear sequence; transfer failure/short read; progressive, interlaced and ICX453 image layout; equal 1x1/2x2/4x4 bins, subframes, shutter/light/dark behavior, model-gated flood LED/cooling (Star2K guiding follows the guider standard). Apply these only to drivers with the corresponding protocol or sensor behavior. |

#### Required real hardware acceptance

Run with the real driver and SDK/USB transport, using capability discovery to select applicable scenarios. Start with hardware-free validation, then execute the following on each available hardware profile; record remaining profiles explicitly.

| Area | Required workflow and evidence |
| --- | --- |
| Identity and shared connection | Verify actual model/serial and logical-device association. Connect CCD first and guider first, exercise either alone, disconnect either while the other remains operational, reconnect repeatedly, and test multiple cameras when available. A camera without ST4 must still pass its CCD workflows. |
| Images | Acquire repeated short and completed long exposures, full frame and subframes, supported bins and pixel formats. Check dimensions, bit depth, Bayer/channel layout, file integrity and actual usable image content. Exercise supported frame types and shutter behavior. On SX models cover interlaced/ICX453 readout and 1x1/2x2/4x4 bins with suitable hardware. |
| Abort and streaming | Abort a long exposure and reacquire; obtain an exact finite stream, run a sustained indefinite stream, stop/abort and restart. Verify frame counts and driver acquisition finalization; record duration, frames and observed abort/readout latency. |
| Controls and persistence | Exercise exposed gain/offset/presets, modes, advanced controls and configuration save/change/load with readback. On equipped hardware verify fan/heater, shutter and model-specific LEDs. Preserve and restore initial settings; for persistent suffix writes, record the original, verify naming after replug and restore it without repeated endurance-loop flash writes. |
| Cooling and temperature | On an available cooled camera change the target, observe measured temperature and power, toggle the cooler and restore initial settings. Verify polling and reconnect readback. An uncooled camera makes this hardware row not applicable; its fake cooled profile is still required where the driver supports cooling. Do not require cooling-rate or absolute thermometer calibration measurements. |
| Guiding | Apply the [Guider hardware acceptance](#guider-hardware-acceptance), including guiding during imaging. |
| Physical interruption | Actually unplug/replug USB during idle and active exposure/readout/streaming or guiding as applicable. Verify logical-device removal, re-enumeration, reconnection, a fresh image and guide pulse after recovery. With multiple cameras verify the survivor continues working. A synthetic removal or CONNECTION toggle does not establish physical hot-plug coverage. |
| Teardown and reload | Disconnect all logical devices, unload/reinitialize the driver in the supported host and reacquire. Distinguish driver INIT/SHUTDOWN from actual dynamic-library unloading if the harness does not unload the SDK. Verify no hung process, lingering callbacks or teardown assertion. |
| Additional interfaces | Apply the [wheel](#wheel-driver-test-standard), [focuser](#focuser-driver-test-standard) or [rotator](#rotator-driver-test-standard) standard to each exposed logical device, including shared ownership and independent operation. |

#### Harness quality and reporting

- Use independent named cases and fresh or explicitly reset fixtures. Observe property revisions/events after each request so stale OK states cannot satisfy a wait. Assert SDK/protocol call values/order and public states together; return codes alone are insufficient.
- Use deterministic gates for races, bounded waits and a process deadline for potential deadlocks. Release gates and stop/join fake callback threads on failure as well as success. Do not replace production queue ordering with synchronous stubs to make races disappear.
- Isolate configuration and output in temporary directories without changing HOME or user settings. Disconnect/detach, close handles, release USB references, and delete test artifacts on every exit path. Follow the test-clean requirement in `indigo_test/AGENTS.md`.
- Run the narrow integration suite, relevant platform-specific readout variants, and supported ASan/UBSan builds when changing lifetime, buffers or callbacks. State which objects were instrumented; an uninstrumented vendor/static library is not sanitizer-covered. Record builds separately from runtime evidence.
- Record automated coverage, gaps and physical results in the relevant driver's `REFACTOR.md` with date, source revision/working-tree state, OS/architecture, SDK/API, device model/firmware/capabilities, procedure and outcome. Mark each scenario passed, failed, unavailable or not applicable with evidence/reason. Keep defects in the applicable `REVIEW.md`; never mark the whole matrix passed from a smoke test or a single device.

## Wheel Drivers

Source: `../indigo_libs/indigo_wheel_driver.c`; scenarios from `../indigo_tests/filter_wheel_compliance.sh`.

Visible before connection:

- Common visible properties only.

Visible after connection:

- `WHEEL_SLOT`
- `WHEEL_SLOT_NAME`
- `WHEEL_SLOT_OFFSET`

Compliance scenarios:

- Verify wheel interface bit.
- Run the common connection battery.
- Assert `WHEEL_SLOT.SLOT`.
- Assert slot name items for all visible slots.
- Assert slot offset items for all visible slots.
- Read the slot count from `WHEEL_SLOT.SLOT` max.
- Exercise slot names/offsets only if the concrete driver implements device-side behavior for them; generic storage belongs in framework tests.
- Move to representative slots and wait for `BUSY -> OK` when movement is required.
- Verify driver command indices at slot boundaries; do not duplicate generic framework clamping tests.

### Wheel Driver Test Standard

Apply the shared scope above. Reference scenarios: `integration/test_wheel_asi_sdk.c`, `test_wheel_playerone_sdk.c`, `test_wheel_astroasis_sdk.c` and wheel serial simulator tests.

#### Fake SDK or protocol coverage

| Fake SDK/transport area | Required driver assertions |
| --- | --- |
| Discovery and slots | Model, slot count, initial position and capability-dependent properties; SDK zero-based versus public one-based indices. Cover unknown/moving initial position, failed initial reads and reconnect rebuilding with a different slot count. |
| Positioning | First/last and one intermediate valid slot, already-selected slot and device-specific restrictions. Verify command index, BUSY/poll/OK sequence and actual slot readback; overlapping requests follow documented rejection/replacement behavior. |
| Calibration/reset | When exposed, verify command, progress/completion, no-op behavior, failed start/poll, disconnect while calibrating and subsequent positioning. Do not require an abort command if the device has none. |
| Errors and settings | Move/read/stop errors, bounded readiness handling and recovery; driver-owned direction/speed/settings and SDK persistence. Slot names/offsets maintained solely by the framework do not need functional retesting. |

#### Wheel hardware acceptance

Discover actual slot count; move to representative slots and verify device position readback, completion and repeated selection. Exercise supported calibration once and restore settings. Interrupt an active operation, reconnect and select a slot successfully. This tests driver/device operation, not optical filter characteristics.

## Focuser Drivers

Source: `../indigo_libs/indigo_focuser_driver.c`; scenarios from `../indigo_tests/focuser_compliance.sh`.

Visible before connection:

- Common visible properties only.

Visible after connection in manual mode:

- `FOCUSER_SPEED`
- `FOCUSER_DIRECTION`
- `FOCUSER_STEPS`
- `FOCUSER_ABORT_MOTION`
- `FOCUSER_POSITION`

Visible after connection in all modes when not hidden:

- `FOCUSER_LIMITS`
- `FOCUSER_ON_POSITION_SET`
- `FOCUSER_TEMPERATURE`
- `FOCUSER_COMPENSATION`
- `FOCUSER_MODE`

Available but hidden by default or driver-dependent:

- `FOCUSER_REVERSE_MOTION`
- `FOCUSER_ON_POSITION_SET`
- `FOCUSER_BACKLASH`
- `FOCUSER_TEMPERATURE`
- `FOCUSER_COMPENSATION`
- `FOCUSER_MODE`
- `FOCUSER_LIMITS`

Compliance scenarios:

- Verify focuser interface bit.
- Run the common connection battery.
- Assert `FOCUSER_STEPS`, `FOCUSER_DIRECTION`, `FOCUSER_POSITION`, `FOCUSER_ABORT_MOTION`, and `FOCUSER_ON_POSITION_SET` when visible.
- Set `FOCUSER_ON_POSITION_SET.GOTO`, move to a valid absolute position, and verify final position.
- Set inward and outward direction items and verify the property reaches `OK`.
- Perform a relative step move.
- Start a long move, abort it, and verify motion-related properties are no longer `BUSY`; the aborted move ends `ALERT` at the stopped position.
- Set `FOCUSER_ON_POSITION_SET.SYNC`, sync to a test position, then restore the previous position.
- If visible, test reverse motion enabled/disabled and restore the original value.
- If visible, test backlash value changes and restore the original value.
- If visible, validate limits and compensation ranges.
- Assert the negative capability contract: optional properties the device lacks are not defined (a relative-only device has a read-only `FOCUSER_POSITION` and no `FOCUSER_ON_POSITION_SET` or `FOCUSER_LIMITS`; a device without a probe has no `FOCUSER_TEMPERATURE`, and no `FOCUSER_COMPENSATION` or `FOCUSER_MODE` where compensation depends on it); status-only properties are read-only. Driver-specific properties are checked under their `X_` names, are not defined before connect, are deleted on disconnect and after a refused connect, and legacy unprefixed names are asserted undefined.
- `INFO.DEVICE_MODEL` and `INFO.DEVICE_FW_REVISION` hold what the controller reported (the item count is raised so both are serialised); a failed optional identity query keeps the placeholder and connect completes; disconnect or a refused handshake resets them where the driver does.
- Start the simulated device in a non-default state (position, travel maximum, temperature, speed, reverse, backlash, limits, compensation, mode, driver options) and verify that connect publishes the device's state, not driver defaults. Reconnecting to a device with other capabilities (another model, a probe now present or absent) defines exactly that device's properties and ranges, with nothing kept from the previous session.
- A device that is not the expected product, or does not answer identification, or fails any connect-time command, is refused with `CONNECTION` ALERT, no focuser or `X_` properties left defined, the port or SDK handle released, and the next connect succeeds.
- `FOCUSER_ABORT_MOTION.ABORT_MOTION` and driver momentary switches (zero/sync, homing, calibration, reset, shutter) return to OFF once the action completes, also after a failure; a request with the item OFF is answered without a command.
- `INDIGO_DRIVER_SHUTDOWN` returns `INDIGO_BUSY` while a device or sibling is connected and the connection keeps working. For drivers with `ADDITIONAL_INSTANCES`, an extra instance on its own port publishes its own device's identity and state, and disconnecting one instance leaves the other working.

### Focuser Driver Test Standard

Apply the shared scope above. Reference scenarios: `integration/test_focuser_asi_sdk.c`, `test_focuser_dmfc_simulator.c`, `test_focuser_primaluce_simulator.c` and the other focuser protocol tests.

#### Fake SDK or protocol coverage

| Fake SDK/transport area | Required driver assertions |
| --- | --- |
| Capabilities/readback | Absolute versus relative-only profiles, step/position/speed ranges, optional temperature and settings; initial read failures and polling of externally changed position. Unsupported controls remain hidden/read-only as appropriate. Cover each model/firmware variant the driver distinguishes on the bus (properties, item counts, ranges) and on the wire: commands the detected model lacks are never sent, polling included; a failed identity or firmware query falls back to a conservative profile that still moves. Pin the connect command sequence against a reference trace where the protocol has one. `FOCUSER_POSITION` and `FOCUSER_STEPS` maxima follow the travel the device reports or `FOCUSER_LIMITS`. A failed, malformed, partial or overlong position poll publishes ALERT with the last valid value, polling continues and the next good poll restores OK and a move works. Temperature: a no-sensor sentinel publishes `FOCUSER_TEMPERATURE` IDLE once, a failed or implausible reading publishes ALERT with the last valid value and never as a value, the next valid reading restores OK and motion stays usable. Motion the driver did not command (running at connect, hand controller) is published BUSY then OK with the target equal to the measured value, no command is sent, and a later relative move starts from it. Replies split across reads are reassembled. |
| Motion | Inward/outward relative commands and absolute GOTO, zero/no-op, direction/reversal, units/sign and limit boundaries. Verify targets versus measured position, BUSY/progress/OK and overlapping request behavior. Test SYNC as a coordinate update without a move command where supported. Every move holds both `FOCUSER_POSITION` and `FOCUSER_STEPS` BUSY and ends both in the same state, including moves shorter than one poll period. A zero step or a GOTO to the current position ends OK promptly with the position unchanged and without waiting for a timeout; a GOTO right after connect is judged against the position read at connect. A relative move or compensation correction that would pass either end of travel or `FOCUSER_LIMITS` is sent as a move to that end, never wrapped, negative or refused; an absolute target beyond the limits is clamped on the wire or ends ALERT without a command. Changing `FOCUSER_LIMITS` updates the `FOCUSER_POSITION` and `FOCUSER_STEPS` ranges and is written and read back where the device stores the limit; a limit change that empties the interval (min above max), excludes the current position or arrives during motion ends ALERT without a command and keeps the old value. Where the driver implements reversal itself, both relative and absolute moves use the swapped device direction; where the controller owns it, the switch sends the device command and is read back. A move request (GOTO, steps, homing, zeroing) while a move or abort is BUSY sends no motion command and is answered with a BUSY message only: the motion properties keep the running move's state, value and target, and the running move ends at its original target with exactly one move command. SYNC reaches the controller also immediately after connect and for a value equal to the published one; a failed or unacknowledged SYNC ends `FOCUSER_POSITION` ALERT with the real position kept and the next SYNC is accepted; a model that cannot sync refuses with ALERT and no command. Homing, zeroing and calibration stay BUSY until the controller reports completion and then publish the resulting position. Protocol simulators move at finite speed and publish intermediate positions, so progress and mid-move abort actually run (`integration/test_focuser_motion.c`). |
| Stop and failure | Abort in motion and while idle, start/read/stop failures and recovery. After abort publish actual position, cancel pending completion and allow a fresh move. Include disconnect/removal during motion and polling. Make the abort land mid-move (a measured position strictly between origin and target) and prove the stop with the stop command and two equal fresh readbacks short of the target; an aborted move ends `FOCUSER_POSITION` and `FOCUSER_STEPS` ALERT, never OK and never at the requested target, with value equal to target equal to the stopped position, and a fresh move is accepted. On a focuser without position readback an aborted relative move ends `FOCUSER_STEPS` ALERT and no later completion stop is sent. An abort while idle ends OK, leaves the position unchanged, sends no stop where the protocol allows, and the next move works. An abort that overtakes a move still queued sends the stop once and never sends the queued move; an abort queued behind a start still ends the move not BUSY; `FOCUSER_ABORT_MOTION` also stops driver-specific motion operations (calibration, homing, batch), which end ALERT. A refused or unacknowledged stop ends `FOCUSER_ABORT_MOTION` ALERT, the move is not reported completed while the device keeps moving, and an immediate retry succeeds. Inject a rejection or bad reply at each command of a multi-command move (setup writes, target write, readback, execute): the move ends ALERT on both motion properties, no later command of the sequence is sent, the published position is unchanged, a controller refusal reason reaches the client message, and the next move works. A stalled move or a persistent failed/malformed position or status read during motion ends ALERT within bounded time (a single lost readback may be retried), sends the stop command, is never counted as arrival nor left BUSY, and a fresh move works; a failure is not turned OK by a later idle poll. Disconnecting or removing the device during motion sends the stop once before the transport closes; no command or poll follows; after reconnect `FOCUSER_POSITION` is OK at the real position, not BUSY, and a fresh move works. After transport loss (during or at the start of a move, or idle) every later request ends ALERT without stale BUSY, the driver does not silently re-run its handshake, and disconnect still completes. |
| Modes and controls | Manual/automatic mode permissions, speed, backlash, limits, beep/heater and other driver controls. Verify device commands and partial failures. Temperature compensation tests cover driver-owned thresholds/corrections and no unwanted movement on invalid temperature data; do not test autofocus algorithms. A successful setting write sends one command carrying that setting; a rejected, unacknowledged or mismatched write or readback ends ALERT showing the device's actual value (a switch keeps the previous item, a multi-item property changes only the applied items), and an immediate retry succeeds. Settings the controller stores are read back after reconnect, negative values (compensation coefficient) included; settings the driver owns send no device command. Backlash, limits, reverse, compensation, mode, sync and other motion-geometry controls requested during a move end ALERT without a command. Automatic mode sends the device command and the driver stops moving the focuser itself; manual mode sends its command, polling resumes and the next move is accepted; a `FOCUSER_POSITION` the driver keeps defined in automatic mode is read-only there. Compensation never moves the focuser in manual mode; below the threshold or on an invalid temperature nothing moves; above it one move of coefficient × ΔT from the reference taken when automatic mode started, clamped to the limits and published through `FOCUSER_POSITION` BUSY then OK; a failed compensation move ends `FOCUSER_POSITION` ALERT and is retried from the kept reference. Controller-computed compensation parameters are sent in device units. |
| Requests versus polls | A `FOCUSER_POSITION` or setting request pending in the device queue is not overwritten by a poll reply, a read-error path or a finalizer, and a poll already in flight when a move is accepted does not complete it. Use a queue gate or delayed reply so the poll comes due both before the handler and with its reply outstanding; assert the requested command is sent exactly once, the property stays BUSY until its handler runs, and the first result publication is OK with the requested value. Cover every polled writable setting (position, reverse, speed, mode, driver controls). |
| Shared controllers | Hub or multi-port controllers open and identify the shared transport once; each logical focuser passes this standard alone; a failed connect of one sibling leaves the others working; a controller-wide reset is refused while a sibling is connected. |

#### Focuser hardware acceptance

Use a small valid travel interval: move both directions, perform supported absolute/relative moves, abort and move again. Verify position readback and SYNC without motion. Exercise available speed/reversal/settings and temperature/mode readback, restoring originals. Do not require achieving optical focus, autofocus runs or travel to mechanical end stops.

## Guider Drivers

Source: `../indigo_libs/indigo_guider_driver.c`; scenarios from `../indigo_tests/guider_compliance.sh`.

Visible before connection:

- Common visible properties only.

Visible after connection:

- `GUIDER_GUIDE_DEC`
- `GUIDER_GUIDE_RA`

Available but hidden by default or driver-dependent:

- `GUIDER_RATE`

Compliance scenarios:

- Verify guider interface bit.
- Run the common connection battery.
- Assert `GUIDER_GUIDE_RA.EAST`, `GUIDER_GUIDE_RA.WEST`, `GUIDER_GUIDE_DEC.NORTH`, and `GUIDER_GUIDE_DEC.SOUTH`.
- Select pulse durations from item max values.
- Pulse east, west, north, and south; wait for `BUSY -> OK`; verify the pulsed item resets to `0`.
- If `GUIDER_RATE` is visible, set a representative RA rate and DEC rate when both items are exposed, then restore originals.
- For drivers with one shared `GUIDER_RATE.RATE` item, set and restore that item.

### Guider Driver Test Standard

Apply this standard to standalone guiders and guider logical devices embedded in cameras, mounts or other drivers. Sources include `integration/test_ccd_touptek_sdk.c`, `test_ccd_playerone_sdk.c` and mount simulator guider cases; SX/Star2K protocol requirements apply only to devices using those relay commands. These scenarios and timing requirements are moved here from the camera standard.

#### Fake SDK or protocol coverage

| Fake SDK/transport area | Required driver assertions |
| --- | --- |
| Directions and units | EAST/WEST/NORTH/SOUTH command mapping, duration conversion/rounding, representative limits and zero requests. Test opposed items according to the driver's documented selection/rejection behavior. A zero request must follow the intended stop/no-op semantics. |
| Completion | BUSY/start and zero/OK completion, command failures/ALERT and successful next pulse. For explicit relay protocols verify ON/OFF masks; for duration-based SDKs verify direction/duration at entry and the driver's completion behavior. |
| Replacement and axes | Same-axis replacement/reversal, queued-request coalescing, simultaneous RA/DEC and independent completion. A fake relay mask must show that completing one axis does not stop the other. Force stale-finalizer versus replacement order and ensure an old completion cannot clear a new pulse. |
| Shared work and lifetime | Guiding during camera acquisition or mount operation, each logical device connected alone and both connection orders. Disconnect/removal with a pulse pending must cancel driver work and send applicable stop commands without closing a sibling's live handle. Verify no calls after close, updates after detach or stuck BUSY; reconnect and pulse again. |
| Guide rate | Shared or independent RA/DEC rates where exposed: device units, accepted readback, command errors and restoration of driver-owned settings. |

#### Guider timing measurements

- Use a monotonic clock at fake SDK/USB entry. For explicit relay ON/OFF protocols, measure the interval between the same direction's ON and OFF edges and subtract the requested pulse length. Ignore redundant OFF writes and distinguish replacement/abort samples from normally completed pulses.
- For SDKs that accept a duration and terminate the pulse internally, assert direction/duration at SDK entry. Measure SDK-entry-to-public-completion latency separately and label it as software completion timing. Do not conflate the ToupTek completion measurement with the Player One ON/OFF measurement.
- Exercise all directions at representative durations spanning 20–500 ms, with repeats and discarded warm-up samples, both idle and during acquisition. Report requested/actual duration, signed error in milliseconds and percent, sample count, min/mean/median/p95/p99/max, standard deviation and maximum absolute error. State the exact measurement endpoints and workload.
- Keep functional assertions (correct direction, completion, cancellation, finite samples, bounded liveness) separate from host-dependent timing statistics. Do not introduce a tight universal precision threshold into normal integration tests. Standalone performance benchmarks follow `indigo_test/AGENTS.md` and stay outside the normal test target.

#### Guider hardware acceptance

Use the real driver and SDK/transport to send all four directions, overlapping axes, replacement and zero requests. Verify command results, BUSY/completion and cancellation/relay-off handling. Include imaging or mount activity for a shared device, disconnect during a pulse and a successful pulse after reconnect. No electrical measurement equipment or astronomical guiding-performance measurement is required.

## AO Drivers

Source: `../indigo_libs/indigo_ao_driver.c`; scenarios from `../indigo_tests/ao_compliance.sh`.

Visible before connection:

- Common visible properties only.

Visible after connection:

- `AO_GUIDE_DEC`
- `AO_GUIDE_RA`
- `AO_RESET`

Compliance scenarios:

- Verify AO interface bit.
- Run the common connection battery.
- Assert `AO_GUIDE_RA.EAST`, `AO_GUIDE_RA.WEST`, `AO_GUIDE_DEC.NORTH`, and `AO_GUIDE_DEC.SOUTH`.
- Select correction magnitudes from item ranges in the units used by the AO driver.
- Correct east, west, north, and south; wait for the expected final `OK` state; verify each handled item resets to `0` where specified by the driver.
- Assert `AO_RESET.CENTER`; if supported by the concrete driver, assert or exercise `AO_RESET.UNJAM`.
- Exercise center reset and verify `AO_RESET` reaches `OK`.

### AO Driver Test Standard

Apply the shared scope above. Reference scenarios: `integration/test_ao_sx_simulator.c` and the SX AO protocol simulator. The existing smoke test is a starting point, not evidence that all scenarios below are covered. An embedded ST4 guider follows the [guider standard](#guider-driver-test-standard) separately.

#### Fake SDK or protocol coverage

| Fake SDK/transport area | Required driver assertions |
| --- | --- |
| Identity and capabilities | Handshake/model/firmware parsing, supported axes and CENTER/UNJAM capabilities, initial status and initialization failure rollback. Do not require position readback if the device does not provide it. |
| Corrections | EAST/WEST/NORTH/SOUTH command mapping, step magnitude and protocol units, zero/no-op, representative boundaries and opposed-item handling. AO correction steps are not guider pulse durations. Verify command acknowledgement, final property state and item reset according to the driver's contract. |
| Limits and reset | Device limit/jam replies must produce the documented state without reporting an unsuccessful correction as complete. Test CENTER and supported UNJAM commands, completion, failed acknowledgement and a successful correction afterward. Cover reset during queued corrections according to driver ordering. |
| Queue and errors | Corrections on both axes, repeated/queued requests, short/malformed replies, write/read failures and timeout recovery. Verify serialization, axis independence and no stale completion after disconnect/removal. Apply shared-handle ownership rules to the AO and its guider. |

#### AO hardware acceptance

Connect and send small corrections in all four directions; verify device acknowledgements and public completion. Center the device, then make another correction. Exercise supported unjam only when applicable, without deliberately driving the mechanism into a jam or end stop. For an embedded guider verify independent operation and shared connection lifetime. Reconnect after interruption and center/correct again. Optical stabilization, correction accuracy and closed-loop guiding performance are outside driver acceptance.

## GPS Drivers

Source: `../indigo_libs/indigo_gps_driver.c`; scenarios from `../indigo_tests/gps_compliance.sh`.

Visible before connection:

- Common visible properties only.

Visible after connection by default:

- `GEOGRAPHIC_COORDINATES`
- `GPS_STATUS`

Available but hidden by default or driver-dependent:

- `UTC_TIME`
- `GPS_ADVANCED`
- `GPS_ADVANCED_STATUS`

Compliance scenarios:

- Verify GPS interface bit.
- Run the common connection battery.
- Assert `GEOGRAPHIC_COORDINATES.LATITUDE`, `LONGITUDE`, `ELEVATION`, and, when visible, `ACCURACY`.
- Assert `UTC_TIME.TIME` when `UTC_TIME` is visible.
- Assert `GPS_STATUS.NO_FIX`, `GPS_STATUS.2D_FIX`, and `GPS_STATUS.3D_FIX`.
- Assert `GPS_ADVANCED.ENABLED` and `GPS_ADVANCED.DISABLED` when visible.
- Validate latitude, longitude, elevation, and accuracy ranges.
- Verify at least one status light is active or that the simulator-specific readiness condition is met.
- Toggle advanced status on and off when `GPS_ADVANCED` is visible; verify `GPS_ADVANCED_STATUS` appears and disappears accordingly.

### GPS Driver Test Standard

Apply the shared scope above. Reference scenarios: `integration/test_gps_nmea_simulator.c`, `test_gps_simulator.c` and the NMEA protocol simulator. Apply sentence-specific requirements only to drivers that parse that protocol; these requirements do not imply existing exhaustive parser coverage.

#### Fake SDK or protocol coverage

| Fake SDK/transport area | Required driver assertions |
| --- | --- |
| Input and framing | Supported messages/talker IDs, split and consecutive messages, missing fields, malformed numbers, truncated/overlong input and checksum handling where defined by the protocol. Verify rejection/recovery at the driver parser boundary without retesting generic serial helpers. |
| Coordinates and time | Known fixtures for latitude/longitude conversion and hemisphere signs, elevation units, UTC date/time and day rollover where parsed by the driver. Invalid/no-fix data must not become a valid new position/time. Check the documented preservation or invalidation of previous values and property states. |
| Fix lifecycle | No fix, 2D and 3D transitions, loss and reacquisition, partial message sets and receiver silence. Check which fields are valid for each fix type and driver timeout/stale-data behavior where implemented; do not invent an unsupported freshness deadline. |
| Advanced data and selection | Satellite counts and DOP/accuracy fields actually supplied by the driver, optional advanced-property visibility, supported constellation/talker selection and mixed-source messages. Verify filtering/aggregation and receiver commands only where the driver implements them; changing a local filter need not send a device command. |
| Reader lifetime and errors | Connection failure, read failure/transport loss, disconnect while awaiting input, reader cancellation and reconnect with fresh data. Verify bounded teardown, no stale updates after detach and no reuse of a previous session's partial message or fix state. |

#### GPS hardware acceptance

Connect to the available receiver, verify incoming data is reflected in fix status, coordinates, UTC and supported advanced fields. Exercise available source selection, disconnect/reconnect and confirm fresh updates. Record no-fix conditions as such; do not wait indefinitely for satellite acquisition. Deterministic loss/reacquisition and malformed-input scenarios belong in the fake transport suite when they cannot be induced on available hardware. Survey-grade position accuracy, satellite acquisition speed, RF performance and precision clock measurements are outside driver acceptance; do not change the host clock.

## Rotator Drivers

Source: `../indigo_libs/indigo_rotator_driver.c`; scenarios from `../indigo_tests/rotator_compliance.sh`.

Visible before connection:

- Common visible properties only.

Visible after connection by default:

- `ROTATOR_ON_POSITION_SET`
- `ROTATOR_POSITION`
- `ROTATOR_ABORT_MOTION`

Available but hidden by default or driver-dependent:

- `ROTATOR_STEPS_PER_REVOLUTION`
- `ROTATOR_DIRECTION`
- `ROTATOR_RELATIVE_MOVE`
- `ROTATOR_BACKLASH`
- `ROTATOR_LIMITS`
- `ROTATOR_RAW_POSITION`
- `ROTATOR_POSITION_OFFSET`

Compliance scenarios:

- Verify rotator interface bit.
- Run the common connection battery.
- Assert `ROTATOR_POSITION.POSITION`.
- Assert `ROTATOR_ABORT_MOTION.ABORT_MOTION`.
- Assert `ROTATOR_ON_POSITION_SET.GOTO` and `ROTATOR_ON_POSITION_SET.SYNC`.
- Set GOTO mode, move to a valid position, and verify final position.
- Start a long move, abort it, and verify `ROTATOR_POSITION` is no longer `BUSY`.
- Set SYNC mode, sync to a position, and restore the previous position.
- If visible, test direction normal/reversed and restore original value.
- If visible, test backlash changes and restore original value.
- If visible, validate position limits and raw/offset position ranges.

### Rotator Driver Test Standard

Apply the shared scope above. Reference scenarios: `integration/test_rotator_asi_sdk.c`, `test_rotator_falcon_simulator.c`, `test_rotator_optec_simulator.c` and `test_rotator_wa_simulator.c`.

#### Fake SDK or protocol coverage

| Fake SDK/transport area | Required driver assertions |
| --- | --- |
| Position mapping | Device units/steps to degrees, sign/direction, raw position and offset where the driver translates them. Cover wrap boundary, negative/relative requests and device travel limits only where driver code implements that behavior; generic angle normalization belongs in framework tests. |
| Motion and sync | Absolute and supported relative moves, zero/no-op, busy overlap, position polling and final readback. Verify SYNC updates the device coordinate/offset without starting motion. Include movement initiated by a hand controller if the driver polls it. |
| Stop and failure | Abort active/idle, command/readback/stop failures, partial setting writes, stale completion, disconnect/removal and successful subsequent move. Verify no target is reported as measured position before device confirmation. |
| Controls | Direction/reversal, backlash, motor settings, limits and calibration/home where supported; command units, accepted-value preservation and driver-specific persistence. |

#### Rotator hardware acceptance

Perform a small absolute move and supported relative moves in both directions, verify angle readback, abort and move again. Exercise SYNC and available direction/settings, restoring originals. Keep within usable travel; field derotation, plate solving and image orientation accuracy are outside driver tests.

## Dome Drivers

Source: `../indigo_libs/indigo_dome_driver.c`.

Visible before connection:

- Common visible properties only.

Visible after connection by default:

- `DOME_SPEED`
- `DOME_DIRECTION`
- `DOME_STEPS`
- `DOME_HORIZONTAL_COORDINATES`
- `DOME_ABORT_MOTION`
- `DOME_SHUTTER`
- `DOME_PARK`
- `DOME_DIMENSION`
- `GEOGRAPHIC_COORDINATES`

Available but hidden by default or driver-dependent:

- `DOME_ON_COORDINATES_SET`
- `DOME_SLAVING_PARAMETERS` (optional; the dome simulator exposes it)
- `DOME_FLAP`
- `DOME_PARK_POSITION`
- `DOME_HOME`
- `UTC_TIME`
- `DOME_SET_HOST_TIME`

Compliance scenarios:

- Verify dome interface bit.
- Run the common connection battery.
- Assert movement properties: `DOME_SPEED`, `DOME_DIRECTION`, `DOME_STEPS`, `DOME_HORIZONTAL_COORDINATES`, and `DOME_ABORT_MOTION`.
- Assert state/control properties: `DOME_SHUTTER` and `DOME_PARK`.
- Validate azimuth, altitude, park position, speed, step, and dimension ranges where visible.
- Exercise a GOTO azimuth or relative step move on simulators/fake I/O and verify `BUSY -> OK`.
- Exercise abort while moving and verify movement properties are not left `BUSY`.
- Exercise shutter open/close and park/unpark only on simulators or fake I/O that can complete deterministically.
- Treat flap, home, UTC, set-host-time, and on-coordinate-set properties as optional.

## Mount Drivers

Source: `../indigo_libs/indigo_mount_driver.c`; current in-process scenarios from `integration/test_mount_simulator.c`.

Visible before connection:

- Common visible properties, plus any driver-specific connection options the concrete driver defines before connection (model/type selector, protocol or dialect selector, transport mode). Those stay defined with their documented default after disconnect, while the connection-time mount properties are deleted.

Visible after connection by default:

- `MOUNT_INFO`
- `GEOGRAPHIC_COORDINATES`
- `MOUNT_LST_TIME`
- `MOUNT_PARK`
- `MOUNT_SLEW_RATE`
- `MOUNT_MOTION_DEC`
- `MOUNT_MOTION_RA`
- `MOUNT_TRACK_RATE`
- `MOUNT_TRACKING`
- `MOUNT_GUIDE_RATE`
- `MOUNT_ON_COORDINATES_SET`
- `MOUNT_EQUATORIAL_COORDINATES`
- `MOUNT_HORIZONTAL_COORDINATES`
- `MOUNT_ABORT_MOTION`
- `MOUNT_EPOCH`

Available but hidden by default or driver-dependent:

- `UTC_TIME`
- `MOUNT_SET_HOST_TIME`
- `MOUNT_PARK_SET`
- `MOUNT_PARK_POSITION`
- `MOUNT_HOME`
- `MOUNT_HOME_SET`
- `MOUNT_HOME_POSITION`
- `MOUNT_CUSTOM_TRACKING_RATE`
- `MOUNT_ALIGNMENT_MODE`
- `MOUNT_RAW_COORDINATES`
- `MOUNT_ALIGNMENT_SELECT_POINTS`
- `MOUNT_ALIGNMENT_DELETE_POINTS`
- `MOUNT_ALIGNMENT_RESET`
- `MOUNT_SIDE_OF_PIER`
- `MOUNT_PEC`
- `MOUNT_PEC_TRAINING`
- `MOUNT_STATE`

Compliance scenarios:

- Verify mount interface bit.
- Run the common connection battery.
- Assert `MOUNT_INFO.MODEL`, `VENDOR`, and `FIRMWARE_VERSION`.
- Assert location/time properties: `GEOGRAPHIC_COORDINATES`, `MOUNT_LST_TIME`, and optional `UTC_TIME`.
- Assert park/home properties when visible: `MOUNT_PARK`, `MOUNT_PARK_SET`, `MOUNT_PARK_POSITION`, `MOUNT_HOME`, `MOUNT_HOME_SET`, and `MOUNT_HOME_POSITION`.
- Assert motion and tracking properties: `MOUNT_SLEW_RATE`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_TRACK_RATE`, `MOUNT_TRACKING`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_HORIZONTAL_COORDINATES`, and `MOUNT_ABORT_MOTION`. Assert `MOUNT_GUIDE_RATE` when the controller supports a guide-rate command; otherwise verify that the driver hides it.
- Validate latitude, longitude, park/home coordinates, guide rates, equatorial coordinates, and horizontal coordinates where initial values are stable.
- Unpark before testing tracking or motion on drivers that reject those requests while parked.
- Toggle tracking on/off and verify the property reaches `OK`.
- Change slew rate and verify the property reaches `OK`.
- If visible, set a custom tracking rate and verify the value.
- Set guide rate values and restore if practical when `MOUNT_GUIDE_RATE` is visible.
- Set `MOUNT_ON_COORDINATES_SET.SYNC`, sync to a valid RA/DEC, and verify final coordinates.
- Start RA/DEC motion, abort it, and verify `MOUNT_ABORT_MOTION` reaches `OK` and motion properties are not left `BUSY`.
- Treat alignment, side-of-pier, PEC, state lights, park/home, and set-host-time properties as optional unless the concrete driver exposes them.
- Assert the negative capability contract as well: optional properties the device lacks, or that are unsafe on the detected model, are not defined; `MOUNT_ON_COORDINATES_SET`, track-rate, park-set, home and guide-rate items match what the controller supports; trimmed ranges are in device units; `MOUNT_EPOCH` is read-only when the device works in one fixed epoch; `MOUNT_STATE` is read-only when present. Driver-specific properties are checked under their `X_` names and legacy unprefixed names are asserted undefined.
- Start the simulated device in a non-default state (parked, tracking off, non-sidereal rate, non-default guide/custom rates, meridian and device options) and verify that connect publishes the device's state, not driver defaults.
- When `MOUNT_STATE` is exposed, verify that its SLEW, PARK, HOME and TRACKING lights go BUSY while the operation runs and return to OK/IDLE when it ends, in the same update as the property that changed and only after follow-up work such as restarting tracking has finished.
- Momentary action switches (reference syncs, resets) return to all items OFF once the action completes.
- Across connect, disconnect and reconnect the driver publishes no update for a property it has not defined.

### Mount Driver Test Standard

Apply the shared scope above. Reference scenarios: `integration/test_mount_synscan_simulator.c`, `test_mount_ioptron_simulator.c`, `test_mount_lx200_simulator.c` and the other mount protocol tests.

#### Fake SDK or protocol coverage

| Fake SDK/transport area | Required driver assertions |
| --- | --- |
| Identity and coordinates | Model/firmware capability branches, valid and malformed replies, RA/DEC device units and signs, wrap boundaries and hemisphere/epoch handling where implemented by the driver. Check exact commands and parsed readback; do not repeat framework astrometry/alignment math tests. Check each capability branch on both sides of its model/firmware threshold, on the bus (properties defined or hidden, `MOUNT_INFO` fields) and on the wire (commands the dialect or older firmware lacks are never sent, polling included, and fallbacks such as a global stop are used instead). Encode the largest legal sub-unit value, a value just below a wrap point (rounding carries, never sends 60 or 24), a negative value with a zero whole-degree part, and clamp values the field cannot carry (DEC +90 at a pole park) to a legal string. Pin `MOUNT_EPOCH` to the device epoch before asserting exact coordinate strings. Assert coordinate readback on a publication made by polling after the request, not on the handler's echo of the request. Express targets as hour angles from `MOUNT_LST_TIME` at non-trivial angles on both pier sides. |
| Detection and connection | Dialect/model selection from the identity reply, fallback detection when the model query is unknown or fails, and explicit selection: a forced choice skips the autodetection probes, a wrong forced choice fails with `CONNECTION` ALERT and closes the port, and AUTO recovers in the same session. A device of another product of the same protocol family, or one that cannot be identified, is refused with `CONNECTION` ALERT and no mount properties left defined. A mount left in another protocol/interface mode is switched before identification. Baud/identity probing retries after a missing reply; a user-configured baud rate without a valid product reply fails instead of falling back. Pin the connect handshake order against a reference command trace. The driver does not toggle controller-wide modes other clients depend on (precision, alignment mode) and rewrites the controller clock only when it is wrong. Where the protocol simulator can record commands the selected dialect does not support or cannot parse, assert zero such commands outside the documented probes. |
| GOTO and SYNC | Coordinate slew, BUSY/progress/completion, already-at-target behavior and rejection/replacement while busy. SYNC sends the synchronization command without a slew. Verify requested post-slew tracking state and device readback rather than treating a command acknowledgement as arrival. Completion comes from the controller's slew status, not a position heuristic: frozen readback keeps the GOTO BUSY, a "done" short of the target ends ALERT, and a slew that never completes is abandoned within a bounded time with ALERT. A busy GOTO that ignores a second request shows exactly one slew command and ends at the original target; a poll already in flight when a GOTO is accepted must not complete it. Inject each refusal/status code (limits, below horizon, unaligned, generic error) and a rejection or dropped reply at each command of a multi-command sequence: the request ends ALERT, no later command is sent, coordinates keep the real position, the controller's reason reaches the client message, and the next GOTO is accepted. Warning replies follow the protocol's documented handling (retry or refuse) and SYNC does not retry. Acknowledgements with a documented prefix are accepted. Prerequisite commands (leave standby, refresh sidereal time) precede every GOTO/SYNC/park as one uninterrupted sequence. `MOUNT_TRACKING` keeps the client's setting throughout the BUSY phase, and tracking resumes at the selected rate. Check SYNC after a fresh poll with a target far from the raw position; a SYNC implemented as an alignment point persists across reconnect. Alignment controls that change how a running slew is translated are refused while it is BUSY and accepted once idle. |
| Manual motion and abort | RA/DEC directions, rate selection, simultaneous axes, stop/reversal and abort during slew/manual motion. Verify axis stop commands, final states, failed command/stop recovery and fresh subsequent movement. Check the device direction bit/sign for every direction on both pier sides and in both hemispheres, and that the readback moves the right way (north raises DEC); manual motion and guide pulses on the same axis use the same mapping. Slew-rate presets are sent in axis-rate units, and settings the controller loses on power cycle are re-sent before the next motion. Releasing RA motion, or aborting, on a tracking mount restores the tracking rate. Make an abort land mid-slew and prove the stop with two equal fresh readbacks short of the target; the abort is answered within bounded time, the stop is sent once, and an aborted GOTO ends `MOUNT_EQUATORIAL_COORDINATES` ALERT, never OK at the target. An abort while idle leaves position and tracking unchanged. Disconnecting during a GOTO, slew or park sends the stop before the transport closes; after reconnect the motion items read OFF and no stale BUSY or finalizer survives. Protocol simulators model axes that decelerate, so stop-and-wait paths actually run. |
| Manual motion ownership | Manual motion started by a client is released when that client detaches: fence the device queue after the request, detach the client and verify the `Aborting` log marker, the axis stop command and that no detach-abort entry is left. Cover a release, abort or takeover by another client, disconnect, and a detach while the motion handler is still running. Also cover: a finite GOTO has no owner and reaches its target; a refused motion request registers nothing and removes an existing entry; diagonal motion and a direction change release each axis exactly once; motion already stopped is not released again; a detach before the handler commits makes the handler stop the motion itself (`Releasing`) and a client at a reused address does not adopt it; the release passes a token lock; disconnect followed by SHUTDOWN leaves nothing to send. Reference: `integration/test_detach_abort.c`. |
| Tracking and rates | Tracking on/off, supported sidereal/solar/lunar/custom rates, guide-rate units and hemispheric direction where driver-owned. Verify settings, readback and failure propagation. Pulse guiding follows the [guider standard](#guider-driver-test-standard). Prove tracking by RA drift with tracking off and hold with it on where the simulator models the sky. A tracking command sets every axis rate the driver owns, zeroing axes that must not carry the drive. Initial rates come from the controller (set, reconnect, readback); a reply that carries no information keeps the current value. A setting the controller acknowledges but does not keep is published with the device value and ALERT. Exercise both ends of driver-scaled ranges on the wire. A guide rate shared by mount and guider is reflected on both devices. A failed tracking restore is retried; only persistent failure sets `MOUNT_TRACKING` OFF with ALERT, never ON while the axis is idle. Driver-owned mount geometry or tracking mode (EQ/Alt-Az/off) is checked with command, readback and its effect on tracking. Mount side of pulse guiding: an RA pulse on a tracking mount ends back at the tracking rate (without stopping the axis where the controller can change rate on the fly) and is checked with tracking on and off; displacement equals guide rate × duration, a replaced pulse moving only until replaced; alt-az geometry hides the guide rate and rejects pulses without a command; firmware selects the pulse command family. |
| Park and home | Supported park/unpark/home/set-position workflows, in-progress/failure states, capability gates, parked-motion rejection and tracking state after completion. A parked-request refusal must also leave the refused property holding the driver's own state, not the rejected values. Check encoder/home-index replies for drivers that implement homing; no invented support for missing commands. Park slews to the position `MOUNT_PARK_POSITION` currently holds, checked in signed device units including HA/DEC to alt-az conversion; set-current stores the mechanical position (test with a non-zero axis offset), and failed axis reads leave it unchanged. Park/home publish coordinates BUSY while moving and OK at completion. Unpark sends no motion command; park ends with the park position read back and tracking OFF, and no workflow sends unrequested state changes such as a pier flip. An abort of a running or queued park/home sends the stop, never sends the queued command, and leaves the mount not parked/homed and not BUSY; no later poll re-latches parked or home, and homing does not change the park state. A park the controller never starts or refuses ends ALERT, not BUSY forever, and shows the real unparked state. Park state is never inferred from an ambiguous status; an unpark stays unparked across polls, and a park interrupted by disconnect is not reported failed next session. Parked guards (motion, tracking, abort, guide pulses) restore the refused switch and do not latch after unpark. Home and home-search use distinct commands; a status-only item (AWAY) sends nothing and a later GOTO returns home to AWAY. Optional extras gated on controller feature bits are tested with a variant missing one bit. |
| Time/location and options | Driver translation of longitude, latitude, UTC/time-zone/DST and host-time requests; read/write failures. Cover pier-side, encoders, PEC/training and device alignment controls only when implemented; generic coordinate transformations and pointing-model quality are outside scope. Follow the protocol's required order (time-zone offset before clock, time and location at connect) and check the prime-meridian 0/360 boundary and east/west sign with write, reconnect and readback; controllers without site commands keep the site in the driver. A one-item change of a multi-item or combined setting resends the current values of the others, also after a failed readback. An unsupported write ends ALERT without a command. Meridian handling (auto flip, limit, tracking past the meridian) and device options (buzzer, max slew speed) are read at connect and written with readback, failure and retry. A request to a read-only device-reported property (pier side) sends nothing and changes no revision. Reference/zenith syncs send the exact sequence with conditional steps only when needed. Long-running device modes (PEC training) stop their timers on disconnect. Any refused request on a mount switch leaves it ALERT showing the device's real state, and an immediate retry succeeds. |
| Requests versus polls and finalizers | A client request pending in the device queue is not overwritten by a poll reply or by a background writer (GOTO/park/home completion, slew-end tracking, training end, auto-home, host-time write). Use a delayed reply or queue gate so the competing write comes due both before and after the request; assert the requested command is the last one the device receives, the opposite command is never sent, the property stays BUSY until its handler runs and exactly one result publication carries the requested value. Cover each polled or finalizer-written setting (tracking, track rate, UTC, encoders, PEC). |
| Polling and transport loss | Motion/coordinate/tracking read failures, malformed/partial messages, stale replies where relevant, bounded timeout and reconnect. Verify state recovery, shared guider ownership and cancellation of pending motion/pulse/poll work without accessing a closed transport. A failed coordinate poll publishes ALERT with the last valid values and the next good poll restores OK; a malformed initial readback marks only that property ALERT and connect completes; a lost reply during a GOTO/park poll does not leave it BUSY. Faults the controller reports (stall, motor fault, limit stop) publish ALERT and clear on the next clean status. Changes made outside the driver (hand controller, another client) are published once per change and not republished while unchanged; documented placeholder status values do not corrupt derived properties. When a reconnect resets the controller's counters, the last position is restored. A guider-only session does not poll the mount, and disconnecting the mount leaves the guider able to pulse. Transport-loss assertions accept both platform outcomes (devices disconnected, or later requests ALERT without stale BUSY) and capture ALERT from the client callback. |
| Transports and logical siblings | Each transport (serial, serial with DTR, TCP, UDP) connects with the same identity; a transport change is accepted while disconnected and refused while connected without dropping the link. Datagram transports survive a lost reply with one retry; discovery connects despite lost first replies within the driver's own budget and the test confirms the simulator answered. Network variants of the connect, GOTO/abort, shared-ownership and loss cases run as an opt-in set. Extra logical devices on the same connection (guider, GPS, focuser, aux shutter) pass their own class standard alone, refuse connection and define nothing when the controller lacks the feature, and keep working when the mount disconnects. A mount request during a sibling operation (GOTO during a guide pulse) is covered including refusal and recovery. SHUTDOWN is refused while the mount or a sibling is connected and the connection survives. |

#### Mount hardware acceptance

Verify model, coordinates and supported status readback; use a small reachable slew, supported SYNC, manual motion in both axes, tracking/rate changes and abort followed by a fresh command. Exercise available park/unpark/home with the actual setup and restore settings. Test guider interaction through its standard. Pointing accuracy, tracking accuracy, polar alignment, periodic-error measurement and long observing sessions are not driver acceptance requirements.

## Polar Aligner Drivers

Source: `../indigo_libs/indigo_polaralign_driver.c`.

Visible before connection:

- Common visible properties only.

Visible after connection:

- `POLARALIGN_OFFSET`
- `POLARALIGN_ABORT_MOTION`
- `POLARALIGN_STEPS_PER_DEGREE`
- `POLARALIGN_DIRECTION_ALT`
- `POLARALIGN_DIRECTION_AZ`
- `POLARALIGN_RESET_POSITION_ALT`
- `POLARALIGN_RESET_POSITION_AZ`
- `POLARALIGN_LIMITS`

Compliance scenarios:

- Verify polar-aligner interface bit.
- Run the common connection battery.
- Assert offset, abort, steps-per-degree, direction, reset, and limits properties.
- Validate offset, steps-per-degree, and limit ranges.
- Set altitude and azimuth direction normal/reversed and verify `OK`.
- Set offsets to representative in-range values and verify them.
- Exercise reset-position commands and verify switches reset to false.
- Exercise abort and verify `POLARALIGN_ABORT_MOTION` reaches `OK`.

## AUX Drivers

Source: `../indigo_libs/indigo_aux_driver.c`.

Visible before connection:

- Common visible properties only, as provided by `indigo_device_attach()`.

Visible after connection:

- No class-specific base AUX properties are defined by `indigo_aux_driver.c`.

Compliance scenarios:

- Verify the concrete AUX interface bit passed to `indigo_aux_attach()`.
- Run the common connection battery if the AUX device is connectable.
- Enumerate and assert concrete driver-specific properties.
- Exercise only properties documented by the concrete AUX driver.
- Keep helper-only AUX math coverage in unit tests, not driver compliance tests.

## Adding New Driver Compliance Tests

- Start with the common device rules.
- Assert the expected class interface bit.
- Use the relevant class section above for base property expectations.
- Add concrete driver properties separately.
- Keep optional hidden properties optional unless the concrete driver documents them as visible.
- Use simulators or fake I/O for movement, exposure, guiding, parking, and other stateful scenarios.
- Restore changed values where practical.
- Avoid real hardware, network sockets, and `indigo_server` in the normal `test-integration` target.
