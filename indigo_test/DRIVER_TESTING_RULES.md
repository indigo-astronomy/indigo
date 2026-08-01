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
- `DOME_EQUATORIAL_COORDINATES`
- `DOME_HORIZONTAL_COORDINATES`
- `DOME_SLAVING`
- `DOME_ABORT_MOTION`
- `DOME_SHUTTER`
- `DOME_PARK`
- `DOME_DIMENSION`
- `GEOGRAPHIC_COORDINATES`
- `SNOOP_DEVICES`

Available but hidden by default or driver-dependent:

- `DOME_ON_HORIZONTAL_COORDINATES_SET`
- `DOME_SLAVING_PARAMETERS`
- `DOME_FLAP`
- `DOME_PARK_POSITION`
- `DOME_HOME`
- `UTC_TIME`
- `DOME_SET_HOST_TIME`

Compliance scenarios:

- Verify dome interface bit.
- Run the common connection battery.
- Assert movement properties: `DOME_SPEED`, `DOME_DIRECTION`, `DOME_STEPS`, `DOME_HORIZONTAL_COORDINATES`, and `DOME_ABORT_MOTION`.
- Assert state/control properties: `DOME_SHUTTER`, `DOME_PARK`, and `DOME_SLAVING`.
- Validate azimuth, altitude, park position, speed, step, and dimension ranges where visible.
- Exercise a GOTO azimuth or relative step move on simulators/fake I/O and verify `BUSY -> OK`.
- Exercise abort while moving and verify movement properties are not left `BUSY`.
- Toggle slaving when safe; if `DOME_SLAVING_PARAMETERS` is visible, validate threshold range.
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
- `SNOOP_DEVICES`

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
