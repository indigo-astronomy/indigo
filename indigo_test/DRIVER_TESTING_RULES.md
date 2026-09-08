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
- `CCD_REMOVE_FITS_HEADERS`
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

## Camera Driver Test Standard: Fake SDK/USB and Hardware

Apply this standard when implementing, migrating or extending CCD driver tests. It is the union of the scenarios in `indigo_test/integration/test_ccd_touptek_sdk.c`, `indigo_test/integration/test_ccd_playerone_sdk.c`, their corresponding `indigo_test/hardware/test_ccd_*_hw.c` programs, and the SX-specific validation requirements in `indigo_drivers/ccd_sx/REFACTOR.md`. There is currently no dedicated CCD SX automated test in the repository; `test_ao_sx_simulator.c` tests a different driver. SX requirements below are required coverage, not evidence of completed tests.

### Scope and evidence

- Test only behavior implemented by the driver: SDK/USB arguments, raw image handoff, driver-owned property behavior, acquisition, guiding and lifecycle. Framework codecs, output containers, upload destinations and generic property/configuration mechanics belong in framework tests. Do not multiply driver acquisitions across JPEG/TIFF/FITS/XISF/SER/AVI encodings. Test configuration only where it restores driver-specific SDK settings.

- Maintain two separate layers: hardware-free integration tests prove driver logic with a fake SDK or USB transport; opt-in hardware tests prove behavior with the real transport, vendor library and camera. Neither substitutes for the other.
- Apply each scenario to capabilities the driver exposes. Test both supported and unsupported capability profiles with the fake; on hardware, record unavailable capabilities/models as deferred with a reason. Do not require a guider, cooler, streaming, wheel or focuser on a camera that does not provide it.
- Inventory every published property and writable item, including inherited base properties and every logical device. Map each applicable scenario below to a named test and its assertions. Record missing coverage in `indigo_test/CHANGES.md`; a reference driver's passing suite does not establish compliance for another driver.
- Keep the normal integration target free of physical USB access, vendor SDK discovery, servers and network sockets. Place real-camera programs under `indigo_test/hardware/`, require explicit opt-in, and exclude them from normal `test`/`test-integration` targets. Add new source files to relevant project groups, including Xcode.

### Required fake SDK or fake USB coverage

Compile the production driver separately and exercise its public entry point and bus properties with the real framework, handler queues and image processing. Replace the SDK/USB boundary; for direct-libusb drivers such as SX, emulate the actual protocol commands and transfers rather than inventing an SDK. Keep injection hooks local to test builds.

Reuse the reference image data from the CCD simulator: `indigo_drivers/ccd_simulator/indigo_ccd_simulator_data.c`, declared in `indigo_ccd_simulator.h` as `indigo_ccd_simulator_raw_image` and `indigo_ccd_simulator_rgb_image`. Link the data object into the test and use these fixtures for fake image delivery and independent expected-pixel assertions, including the appropriate 8/16-bit or RGB conversion and ROI/bin mapping. Do not duplicate the large image arrays in test sources.

| Area | Required scenarios and assertions |
| --- | --- |
| Identity and lifecycle | Driver metadata and interface bits; INIT, enumeration, connect/disconnect and SHUTDOWN; repeated INIT/SHUTDOWN; rejection of shutdown while connected without losing active work. Exercise CCD-only, guider-only, both connection and disconnection orders, and each additional logical interface. Verify one shared open/init, no premature close while a sibling remains connected, and one final close/unlock. |
| Discovery and ownership | Mono/color and optional-interface profiles; combined camera/guider/wheel/focuser discovery where supported; duplicate and burst arrivals/removals; failed master/slave attachment and retry; capacity overflow/recovery; distinct physical cameras with nonmatching USB-event and SDK-enumeration order, nontrivial SDK ids and duplicate model names. Verify correct detach identity, no orphan logical devices, and balanced handles, references, reservations and locks. |
| Initialization failures | Inject queue/registration, global-lock, open/init, USB claim/reset/parameter-read and required property-discovery failures where applicable. Verify rollback, public disconnected/ALERT state, no use of invalid outputs and successful subsequent connection. Unsupported optional capabilities must follow their documented hidden/fallback behavior. |
| Property contract | Enumerate before/after connection and after disconnect. Assert names (`X_` for custom properties), item names/counts, types, permissions, visibility, ranges, values/targets and capability-dependent changes. Exercise writable properties handled by the driver, valid limits and invalid requests, mode/format synchronization and reconnect rebuilding. Rejected busy changes must preserve accepted values and avoid SDK writes. |
| Controls | Gain, offset, egain, presets/conversion gain, sensor/readout/bin modes, advanced controls, fan/heater/LED and other exposed controls: verify SDK arguments, units, readback, partial/error paths and recovery. Test temperature/cooler target conversion, periodic polling, read/write errors, slow initialization crossing the first polling deadline, and no SDK access from recurring tasks after disconnect. |
| Image contract | Every advertised SDK pixel format/bit depth; raw image handoff dimensions, payload length and deterministic pixel content, including channel order/Bayer metadata where applicable. Exercise full frame, ROI offsets/bounds/alignment, supported and invalid bins, frame types (light/bias/dark/flat/dark-flat), exposure-unit conversion and shortest bias exposure. Validate SDK geometry as well as delivered geometry. |
| Acquisition | Short and long exposure, BUSY followed by completion, overlap rejection, pending/not-ready reads and bounded retries, watchdog expiry, start/setup/readout failures and recovery. Abort before completion and at controlled setup/readout/final-frame boundaries; immediately reacquire. Assert exactly one successful frame or the expected failure, no duplicate publication/finalization and no stale error/frame affecting a new acquisition. |
| Streaming and destinations | Finite exact frame count, indefinite streaming, stop/abort/restart, SDK errors/timeouts and removal during streaming. Verify raw frame delivery and driver-owned completion/cleanup; use one output format without retesting framework encoders or upload destinations. |
| Guider | All four directions and duration arguments, zero requests, replacement/reversal on each axis, simultaneous RA/DEC, and the documented coalescing of queued requests. Verify BUSY/start and zero/OK completion (ALERT on failure), relay-off commands, axis independence, guiding during acquisition and cancellation on disconnect/removal. A fake relay mask must show that completing one axis does not stop the other. |
| Queue and callback races | Prove SDK/USB operations run on the intended queue and serialize access to shared handles. Use barriers to force both final-frame/abort orders, disconnect during an SDK callback, stale notifications during stop/reconfiguration, removal with exposure/pulse/poll work pending, and shutdown with queued discovery. Verify rejected shutdown leaves required work alive; accepted shutdown drains/cancels it. Assert no calls after close, updates after detach, duplicate close or deadlock, then reconnect/reload and reacquire. |
| Configuration and strings | SAVE/change/LOAD roundtrip for persistent settings and every relevant logical interface; confirm restored SDK state, not only CONFIG OK. Test custom-property renames and driver-specific persistence. For suffix/name controls test empty, boundary and over-limit lengths, write failures, reconnect and bounded SDK strings; assert public state and accepted value. |
| Additional interfaces | Where exposed by the same driver, test wheel model/slot count, slot indexing and limits, positioning, calibration, polling and SDK failures. For focusers test direction/reversal, beep/backlash/limits, sync versus goto, relative/absolute motion, abort, manual/automatic mode, temperature compensation, permissions and error recovery. Verify simultaneous discovery and independent operation of all logical interfaces. |
| SX protocol/readout specifics | Model/PID recognition and USB-path identity; open/claim/reset and camera parameters; pending hardware timer versus completed image; long-exposure register-clear sequence; transfer failure/short read; progressive, interlaced and ICX453 image layout; equal 1x1/2x2/4x4 bins, subframes, shutter/light/dark behavior, Star2K relay masks and model-gated flood LED/cooling. Apply these only to drivers with the corresponding protocol or sensor behavior. |

### Guider timing measurements

- Use a monotonic clock at fake SDK/USB entry. For explicit relay ON/OFF protocols, measure the interval between the same direction's ON and OFF edges and subtract the requested pulse length. Ignore redundant OFF writes and distinguish replacement/abort samples from normally completed pulses.
- For SDKs that accept a duration and terminate the pulse internally, assert direction/duration at SDK entry. Measure SDK-entry-to-public-completion latency separately and label it as software completion timing. Do not conflate the ToupTek completion measurement with the Player One ON/OFF measurement.
- Exercise all directions at representative durations spanning 20–500 ms, with repeats and discarded warm-up samples, both idle and during acquisition. Report requested/actual duration, signed error in milliseconds and percent, sample count, min/mean/median/p95/p99/max, standard deviation and maximum absolute error. State the exact measurement endpoints and workload.
- Keep functional assertions (correct direction, completion, cancellation, finite samples, bounded liveness) separate from host-dependent timing statistics. Do not introduce a tight universal precision threshold into normal integration tests. Standalone performance benchmarks follow `indigo_test/AGENTS.md` and stay outside the normal test target.

### Required real hardware acceptance

Run with the real driver and SDK/USB transport, using capability discovery to select applicable scenarios. Start with hardware-free validation, then execute the following on each available hardware profile; record remaining profiles explicitly.

| Area | Required workflow and evidence |
| --- | --- |
| Identity and shared connection | Verify actual model/serial and logical-device association. Connect CCD first and guider first, exercise either alone, disconnect either while the other remains operational, reconnect repeatedly, and test multiple cameras when available. A camera without ST4 must still pass its CCD workflows. |
| Images | Acquire repeated short and completed long exposures, full frame and subframes, supported bins and pixel formats. Check dimensions, bit depth, Bayer/channel layout, file integrity and actual usable image content. Exercise supported frame types and shutter behavior. On SX models cover interlaced/ICX453 readout and 1x1/2x2/4x4 bins with suitable hardware. |
| Abort and streaming | Abort a long exposure and reacquire; obtain an exact finite stream, run a sustained indefinite stream, stop/abort and restart. Verify frame counts and driver acquisition finalization; record duration, frames and observed abort/readout latency. |
| Controls and persistence | Exercise exposed gain/offset/presets, modes, advanced controls and configuration save/change/load with readback. On equipped hardware verify temperature/target/power, cooler on/off, fan/heater, shutter and model-specific LEDs. Preserve and restore initial settings; for persistent suffix writes, record the original, verify naming after replug and restore it without repeated endurance-loop flash writes. |
| Guiding | Send all four directions, overlapping axes and replacement/zero requests, also during imaging and disconnect during a pulse. Verify SDK command results, property completion and driver cancellation/relay-off handling. |
| Physical interruption | Actually unplug/replug USB during idle and active exposure/readout/streaming or guiding as applicable. Verify logical-device removal, re-enumeration, reconnection, a fresh image and guide pulse after recovery. With multiple cameras verify the survivor continues working. A synthetic removal or CONNECTION toggle does not establish physical hot-plug coverage. |
| Teardown and reload | Disconnect all logical devices, unload/reinitialize the driver in the supported host and reacquire. Distinguish driver INIT/SHUTDOWN from actual dynamic-library unloading if the harness does not unload the SDK. Verify no hung process, lingering callbacks or teardown assertion. |
| Additional interfaces | If the driver exposes a wheel or focuser, exercise real positioning, limits, calibration or sync, abort, relevant settings and temperature compensation where supported. Record unavailable accessories separately; camera success does not cover these interfaces. |

### Harness quality and reporting

- Use independent named cases and fresh or explicitly reset fixtures. Observe property revisions/events after each request so stale OK states cannot satisfy a wait. Assert SDK/protocol call values/order and public states together; return codes alone are insufficient.
- Use deterministic gates for races, bounded waits and a process deadline for potential deadlocks. Release gates and stop/join fake callback threads on failure as well as success. Do not replace production queue ordering with synchronous stubs to make races disappear.
- Isolate configuration and output in temporary directories without changing HOME or user settings. Disconnect/detach, close handles, release USB references, and delete test artifacts on every exit path. Follow the test-clean requirement in `indigo_test/AGENTS.md`.
- Run the narrow integration suite, relevant platform-specific readout variants, and supported ASan/UBSan builds when changing lifetime, buffers or callbacks. State which objects were instrumented; an uninstrumented vendor/static library is not sanitizer-covered. Record builds separately from runtime evidence.
- Record automated coverage/gaps in `indigo_test/CHANGES.md`; record physical results in `TESTING.md` with date, source revision/working-tree state, OS/architecture, SDK/API, camera model/firmware/capabilities, procedure and outcome. Mark each scenario passed, failed, unavailable or not applicable with evidence/reason. Keep migration status in `REFACTOR.md` and defects in the applicable `REVIEW.md`; never mark the whole matrix passed from a smoke test or a single camera.

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
- Set each slot name to a bounded test value and verify it.
- Set each slot offset to positive, negative, and zero values and verify each update.
- Move to representative slots and wait for `BUSY -> OK` when movement is required.
- Request below-minimum and above-maximum slots and verify clamping to slot `1` and the max slot.

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
- Start a long move, abort it, and verify motion-related properties are no longer `BUSY`.
- Set `FOCUSER_ON_POSITION_SET.SYNC`, sync to a test position, then restore the previous position.
- If visible, test reverse motion enabled/disabled and restore the original value.
- If visible, test backlash value changes and restore the original value.
- If visible, validate limits and compensation ranges.

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
- Select pulse values from item max values.
- Pulse east, west, north, and south; wait for the expected final `OK` state; verify each pulsed item resets to `0`.
- Assert `AO_RESET.CENTER`; if supported by the concrete driver, assert or exercise `AO_RESET.UNJAM`.
- Exercise center reset and verify `AO_RESET` reaches `OK`.

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

- Common visible properties only.

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
- Assert motion and tracking properties: `MOUNT_SLEW_RATE`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_TRACK_RATE`, `MOUNT_TRACKING`, `MOUNT_GUIDE_RATE`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_HORIZONTAL_COORDINATES`, and `MOUNT_ABORT_MOTION`.
- Validate latitude, longitude, park/home coordinates, guide rates, equatorial coordinates, and horizontal coordinates where initial values are stable.
- Unpark before testing tracking or motion on drivers that reject those requests while parked.
- Toggle tracking on/off and verify the property reaches `OK`.
- Change slew rate and verify the property reaches `OK`.
- If visible, set a custom tracking rate and verify the value.
- Set guide rate values and restore if practical.
- Set `MOUNT_ON_COORDINATES_SET.SYNC`, sync to a valid RA/DEC, and verify final coordinates.
- Start RA/DEC motion, abort it, and verify `MOUNT_ABORT_MOTION` reaches `OK` and motion properties are not left `BUSY`.
- Treat alignment, side-of-pier, PEC, state lights, park/home, and set-host-time properties as optional unless the concrete driver exposes them.

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
