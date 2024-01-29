# INDI conversion

* INDI Standard Properties: <https://indilib.org/develop/developer-manual/101-standard-properties.html>
* INDIGO Properties: <https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/PROPERTIES.md>

## Common properties

| INDI Property name | INDI Item name | INDIGO Property name | INDIGO Item name | Note |
| ---- | ---- | ---- | ---- | ---- |
| CONNECTION | | CONNECTION | | - |
| | CONNECT | | **CONNECTED** | CONNECT **ED** |
| | DISCONNECT | | **DISCONNECTED** | DISCONNECT **ED** |
| DEVICE_PORT | | DEVICE_PORT | | |
| | PORT | | PORT | |
| TIME_LST | | | | |
| | LST | | | |
| TIME_UTC | | **UTC_TIME** | | **rename!** |
| | UTC | | **TIME** | **rename!** |
| | OFFSET | | OFFSET | |
| GEOGRAPHIC_COORD | | **GEOGRAPHIC_COORDINATES** | | GEOGRAPHIC_COORD **INATES** |
| | LAT | | LATITUDE | LAT **ITUDE** |
| | LONG | | LONGITUDE | LONG **ITUDE** |
| | ELEV | | ELEVATION | ELEV **ATION** |
| | | | ACCURACY | **new!** |
| ATMOSPHERE | | | | |
| | TEMPERATURE | | | |
| | PRESSURE | | | |
| | HUMIDITY | | | |
| UPLOAD_MODE | | | | |
| | UPLOAD_CLIENT | | | |
| | UPLOAD_LOCAL | | | |
| | UPLOAD_BOTH | | | |
| UPLOAD_SETTINGS | | | | |
| | UPLOAD_DIR | | | |
| | UPLOAD_PREFIX | | | |
| ACTIVE_DEVICES | | | | |
| | ACTIVE_TELESCOPE | | | |
| | ACTIVE_CCD | | | |
| | ACTIVE_FILTER | | | |
| | ACTIVE_FOCUSER | | | |
| | ACTIVE_DOME | | | |
| | ACTIVE_GPS | | | |
| SIMULATION | | SIMULATION | | (INDI: not documented) |
| | ENABLE | | **ENABLED** | ENABLE **D** |
| | DISABLE | | **DISABLED** | DISABLE **D** |
| | | **INFO** | | **new!** |
| | | | DEVICE_NAME | |
| | | | DEVICE_VERSION | |
| | | | DEVICE_INTERFACE | |
| | | | FRAMEWORK_NAME | |
| | | | FRAMEWORK_VERSION | |
| | | | DEVICE_MODEL | |
| | | | DEVICE_FIRMWARE_REVISION | |
| | | | DEVICE_HARDWARE_REVISION | |
| | | | DEVICE_SERIAL_NUMBER | |
| | | **PROFILE** | | **new!** |
| | | | PROFILE_0,... | |
| | | **DEVICE_BAUDRATE** | | **new!** |
| | | | BAUDRATE | |
| | | **DEVICE_PORTS** | | **new!** |
| | | | valid serial port name | |

## Telescopes

| INDI Property name | INDI Item name | INDIGO Property name | INDIGO Item name | Note |
| ---- | ---- | ---- | ---- | ---- |
| EQUATORIAL_COORD | | **MOUNT_EQUATORIAL_COORDINATES** | | **rename!** |
| | RA | | RA | |
| | DEC | | DEC | |
| EQUATORIAL_EOD_COORD | | | | use `MOUNT_EQUATORIAL_COORDINATES` |
| | RA | | | |
| | DEC | | | |
| TARGET_EOD_COORD | | | | ? |
| | RA | | | |
| | DEC | | | |
| HORIZONTAL_COORD | | **MOUNT_HORIZONTAL_COORDINATES** | | **rename!** |
| | ALT | | ALT | |
| | AZ | | AZ | |
| ON_COORD_SET | | **MOUNT_ON_COORDINATES_SET** | | **rename!** |
| | SLEW | | SLEW | |
| | TRACK | | TRACK | |
| | SYNC | | SYNC | |
| TELESCOPE_MOTION_NS | | **MOUNT_MOTION_DEC** | | **rename!** |
| | MOTION_NORTH | | **NORTH** | remove `MOTION_` |
| | MOTION_SOUTH | | **SOUTH** | remove `MOTION_` |
| TELESCOPE_MOTION_WE | | **MOUNT_MOTION_RA** | | **rename!** |
| | MOTION_WEST | | **WEST** | remove `MOTION_` |
| | MOTION_EAST | | **EAST** | remove `MOTION_` |
| TELESCOPE_TIMED_GUIDE_NS | | **GUIDER_GUIDE_DEC** | | **rename!** |
| | TIMED_GUIDE_N | | **NORTH** | **rename!** |
| | TIMED_GUIDE_S | | **SOUTH** | **rename!** |
| TELESCOPE_TIMED_GUIDE_WE | | **GUIDER_GUID_RA** | | **rename!** |
| | TIMED_GUIDE_W | | **WEST** | **rename!** |
| | TIMED_GUIDE_E | | **EAST** | **rename!** |
| TELESCOPE_SLEW_RATE | | **MOUNT_SLEW_RATE** | | **rename!** |
| | SLEW_GUIDE | | **GUIDE** | remove `SLEW_` |
| | SLEW_CENTERING | | **CENTERING** | remove `SLEW_` |
| | SLEW_FIND | | **FIND** | remove `SLEW_` |
| | SLEW_MAX | | **MAX** | remove `SLEW_` |
| TELESCOPE_PARK | | **MOUNT_PARK** | | **rename!** |
| | PARK | | **PARKED** | PARK **ED** |
| | UNPARK | | **UNPARKED** | UNPARK **ED** |
| TELESCOPE_PARK_POSITION | | **MOUNT_PARK_POSITION** | | **rename!** |
| | PARK_RA/AZ | | **RA** | remove `PARK_` |
| | PARK_DEC/ALT | | **DEC** | remove `PARK_` |
| TELESCOPE_PARK_OPTION | | **MOUNT_PARK_SET** | | **rename!** |
| | PARK_CURRENT | | **CURRENT** | **rename!** |
| | PARK_DEFAULT | | **DEFAULT** | **rename!** |
| | PARK_WRITE_DATA | | | |
| TELESCOPE_ABORT_MOTION | | **MOUNT_ABORT_MOTION** | | **rename!** |
| | ABORT_MOTION | | ABORT_MOTION | |
| TELESCOPE_TRACK_RATE | | **MOUNT_TRACK_RATE** | | **rename!** |
| | TRACK_SIDEREAL | | **SIDEREAL** | remove `TRACK_` |
| | TRACK_SOLAR | | **SOLAR** | remove `TRACK_` |
| | TRACK_LUNAR | | **LUNAR** | remove `TRACK_` |
| | TRACK_CUSTOM | | | |
| TELESCOPE_INFO | | **MOUNT_INFO** | | **rename!** |
| | TELESCOPE_APERTURE | | | |
| | TELESCOPE_FOCAL_LENGTH | | | |
| | GUIDER_APERTURE | | | |
| | GUIDER_FOCAL_LENGTH | | | |
| | | | MODEL | **new!** |
| | | | VENDOR | **new!** |
| | | | FIRMWARE_VERSION | **new!** |
| TELESCOPE_PIER_SIDE | | **MOUNT_SIDE_OF_PIER | | **rename!** |
| | PIER_EAST | | **EAST** | remove `PIER_` |
| | PIER_WEST | | **WEST** | remove `PIER_` |
| | | **MOUNT_LST_TIME** | | **new!** |
| | | | TIME | |
| | | **MOUNT_SET_HOST_TIME** | | **new!** |
| | | | SET | |
| | | **MOUNT_TRACKING** | | **new!** |
| | | | ON | |
| | | | OFF | |
| | | **MOUNT_GUIDE_RATE** | | **new!** |
| | | | RA | |
| | | | DEC | |
| | | **MOUNT_RAW_COORDINATES** | | **new!** |
| | | | RA | |
| | | | DEC | |
| | | | | |
| | | **MOUNT_ALIGNMENT_MODE** | | **new!** |
| | | | CONTROLLER | |
| | | | SINGLE_POINT | |
| | | | NEAREST_POINT | |
| | | | MULTI_POINT | |
| | | **MOUNT_ALIGNMENT_SELECT_POINTS** | | **new!** |
| | | | point id | |
| | | **MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY** | | **new!** |
| | | | point id | |
| | | **MOUNT_EPOCH** | | **new!** |
| | | | EPOCH | |
| | | **MOUNT_PEC** | | **new!** |
| | | | ENABLED | |
| | | | DISABLED | |
| | | **MOUNT_PEC_TRAINING** | | **new!** |
| | | | STARTED | |
| | | | STOPPED | |
| | | **GUIDER_RATE** | | **new!** |
| | | | RATE | |
| | | | DEC_RATE | |

## CCDs

| INDI Property name | INDI Item name | INDIGO Property name | INDIGO Item name | Note |
| ---- | ---- | ---- | ---- | ---- |
| CCD_EXPOSURE | | CCD_EXPOSURE | | |
| | CCD_EXPOSURE_VALUE | | **EXPOSURE** | **rename!** |
| CCD_ABORT_EXPOSURE | | CCD_ABORT_EXPOSURE | | |
| | ABORT | | **ABORT_EXPOSURE** | **rename!** |
| CCD_FRAME | | CCD_FRAME | | |
| | X | | **LEFT** | **rename!** |
| | Y | | **TOP** | **rename!** |
| | WIDTH | | WIDTH | |
| | HEIGHT | | HEIGHT | |
| | | | **BITS_PER_PIXEL** | **new!** |
| CCD_TEMPERATURE | | CCD_TEMPERATURE | | |
| | CCD_TEMPERATURE_VALUE | | **TEMPERATURE** | **rename!** |
| CCD_COOLER | | CCD_COOLER | | |
| | COOLER_ON | | **ON** | remove `COOLER_` |
| | COOLER_OFF | | **OFF** | remove `COOLER_` |
| CCD_COOLER_POWER | | CCD_COOLER_POWER | | |
| | CCD_COOLER_VALUE | | **POWER** | **rename!** |
| CCD_FRAME_TYPE | | CCD_FRAME_TYPE_PROPERTY | | |
| | FRAME_LIGHT | | **LIGHT** | remove `FRAME_` |
| | FRAME_BIAS | | **BIAS** | remove `FRAME_` |
| | FRAME_DARK | | **DARK** | remove `FRAME_` |
| | FRAME_FLAT | | **FLAT** | remove `FRAME_` |
| | | | **DARKFLAT** | **new!** |
| CCD_BINNING | | **CCD_BIN** | | **rename!** |
| | HOR_BIN | | **HORIZONTAL** | **rename!** |
| | VER_BIN | | **VERTICAL** | **rename!** |
| CCD_COMPRESSION | | **CCD_IMAGE_FORMAT** | | **rename!** |
| | CCD_COMPRESS | | | FITS? |
| | CCD_RAW | | **RAW** | remove `CCD_` |
| | | | **FITS** | CCD_COMPRESS -&gt; FITS? |
| | | | **XISF** | **new!** |
| | | | **JPEG** | **new!** |
| CCD_FRAME_RESET | | | | |
| | RESET | | | |
| CCD_INFO | | CCD_INFO | | |
| | CCD_MAX_X | | **WIDTH** | **rename!** |
| | CCD_MAX_Y | | **HEIGHT** | **rename!** |
| | CCD_PIXEL_SIZE | | **PIXEL_SIZE** | remove `CCD_` |
| | CCD_PIXEL_SIZE_X | | **PIXEL_WIDTH** | **rename!** |
| | CCD_PIXEL_SIZE_Y | | **PIXEL_HEIGHT** | **rename!** |
| | CCD_BITSPERPIXEL | | **BITS_PER_PIXEL** | **rename!** |
| | | | **MAX_HORIZONTAL_BIN** | **new!** |
| | | | **MAX_VERTICAL_BIN** | **new!** |
| CCD_CFA | | | | |
| | CFA_OFFSET_X | | | |
| | CFA_OFFSET_Y | | | |
| | CFA_TYPE | | | |
| CCD1 | | **CCD_IMAGE** | | **rename!** |
| | CCD1 | | **IMAGE** | **rename!** |
| UPLOAD_MODE | | **CCD_UPLOAD_MODE** | | INDI Common property -&gt; INDIGO CCD property |
| | UPLOAD_CLIENT | | **CLIENT** | remove `UPLOAD_` |
| | UPLOAD_LOCAL | | **LOCAL** | remove `UPLOAD_` |
| | UPLOAD_BOTH | | **BOTH** | remove `UPLOAD_` |
| | | **CCD_LOCAL_MODE** | | **new!** |
| | | | DIR | |
| | | | PREFIX | |
| STREAMING_EXPOSURE | | **CCD_STREAMING** | | INDI Streaming property -&gt; INDIGO CCD Property |
| | STREAMING_EXPOSURE_VALUE | | **EXPOSURE** | **rename!** |
| | STREAMING_DIVISOR_VALUE | | | ? |
| | | | **COUNT** | **new!** |
| | | **CCD_MODE** | | **new!** |
| | | | mode identifier | |
| | | **CCD_READ_MODE** | | **new!** |
| | | | HIGH_SPEED | |
| | | | LOW_NOISE | |
| | | **CCD_GAIN** | | |
| | | | GAIN | |
| | | **CCD_OFFSET** | | |
| | | | OFFSET | |
| | | **CCD_GAMMA** | | |
| | | | GAMMA | |
| | | **CCD_IMAGE_FILE** | | **new!** |
| | | | FILE | |
| | | **CCD_FITS_HEADERS** | | **new!** |
| | | | KEY,... | |
| | | **CCD_SET_FITS_HEADER** | | **new!** |
| | | | NAME | |
| | | | VALUE | |
| | | **CCD_REMOVE_FITS_HEADER** | | **new!** |
| | | | NAME | |
| | | **CCD_PREVIEW** | | **new!** |
| | | | ENABLED | |
| | | | DISABLED | |
| | | **CCD_PREVIEW_IMAGE** | | **new!** |
| | | | IMAGE | |

## Streamings

| INDI Property name | INDI Item name | INDIGO Property name | INDIGO Item name | Note |
| ---- | ---- | ---- | ---- | ---- |
| CCD_VIDEO_STREAM | | | | |
| | STREAM_ON | | | |
| | STREAM_OFF | | | |
| STREAMING_EXPOSURE | | **CCD_STREAMING** | | INDI Streaming property -&gt; INDIGO CCD Property |
| | STREAMING_EXPOSURE_VALUE | | **EXPOSURE** | **rename!** |
| | STREAMING_DIVISOR_VALUE | | | ? |
| | | | **COUNT** | **new!** |
| FPS | | | | |
| | EST_FPS | | | |
| | AVG_FPS | | | |
| RECORD_FILE | | | | |
| | RECORD_FILE_DIR | | | |
| | RECORD_FILE_NAME | | | |
| RECORD_OPTIONS | | | | |
| | RECORD_DURATION | | | |
| | RECORD_FRAME_TOTAL | | | |
| RECORD_STREAM | | | | |
| | RECORD_ON | | | |
| | RECORD_DURATION_ON | | | |
| | RECORD_FRAME_ON | | | |
| | RECORD_OFF | | | |

## Filter wheels

| INDI Property name | INDI Item name | INDIGO Property name | INDIGO Item name | Note |
| ---- | ---- | ---- | ---- | ---- |
| FILTER_SLOT | | **WHEEL_SLOT** | | **rename!** |
| | FILTER_SLOT_VALUE | | **SLOT** | **rename!** |
| FILTER_NAME | | | | FILTER_NAME (Text) -&gt; FILTER_SLOT_NAME (Switch) |
| | FILTER_NAME_VALUE | | | |
| | | **WHEEL_SLOT_NAME** | | FILTER_NAME (Text) -&gt; FILTER_SLOT_NAME (Switch) |
| | | | SLOT_NAME_1,... | |
| | | **WHEEL_SLOT_OFFSET** | | **new!** |
| | | | SLOT_OFFSET_1,... | |

## Focusers

| INDI Property name | INDI Item name | INDIGO Property name | INDIGO Item name | Note |
| ---- | ---- | ---- | ---- | ---- |
| FOCUS_SPEED | | **FOCUSER_SPEED** | | **rename!** |
| | FOCUS_SPEED_VALUE | | **SPEED** | **rename!** |
| FOCUS_MOTION | | **FOCUSER_DIRECTION** | | **rename!** |
| | FOCUS_INWARD | | **MOVE_INWARD** | **rename!** |
| | FOCUS_OUTWARD | | **MOVE_OUTWARD** | **rename!** |
| FOCUS_TIMER | | | | |
| | FOCUS_TIMER_VALUE | | | |
| REL_FOCUS_POSITION | | | | use FOCUSER_POSITION |
| | FOCUS_RELATIVE_POSITION | | | |
| ABS_FOCUS_POSITION | | | | use FOCUSER_POSITION |
| | FOCUS_ABSOLUTE_POSITION | | | |
| FOCUS_MAX | | | | |
| | FOCUS_MAX_VALUE | | | |
| FOCUS_REVERSE_MOTION | | **FOCUSER_REVERSE_MOTION** | | **rename!** |
| | ENABLED | | ENABLED | |
| | DISABLED | | DISABLED | |
| FOCUS_ABORT_MOTION | | **FOCUSER_ABORT_MOTION** | | **rename!** |
| | ABORT | | **ABORT_MOTION** | **rename!** |
| FOCUS_SYNC | | | | |
| | FOCUS_SYNC_VALUE | | | |
| | | **FOCUSER_STEPS** | | **new!** |
| | | | STEPS | |
| | | **FOCUSER_ON_POSITION_SET** | | |
| | | | GOTO | |
| | | | SYNC | |
| | | **FOCUSER_POSITION** | | **new!** |
| | | | POSITION | |
| | | **FOCUSER_TEMPERATURE** | | **new!** |
| | | | TEMPERATURE | |
| | | | | |
| | | **FOCUSER_BACKLASH** | | **new!** |
| | | | BACKLASH | |
| | | **FOCUSER_COMPENSATION** | | **new!** |
| | | | COMPENSATION | |
| | | | PERIOD | |
| | | | THRESHOLD | |
| | | **FOCUSER_MODE** | | **new!** |
| | | | MANUAL | |
| | | | AUTOMATIC | |
| | | **FOCUSER_LIMITS** | | **new!** |
| | | | MIN_POSITION | |
| | | | MAX_POSITION | |

## Domes

| INDI Property name | INDI Item name | INDIGO Property name | INDIGO Item name | Note |
| ---- | ---- | ---- | ---- | ---- |
| DOME_SPEED | | | | |
| | DOME_SPEED_VALUE | | | |
| DOME_MOTION | | | | |
| | DOME_CW | | | |
| | DOME_CCW | | | |
| DOME_TIMER | | | | |
| | DOME_TIMER_VALUE | | | |
| REL_DOME_POSITION | | | | |
| | DOME_RELATIVE_POSITION | | | |
| ABS_DOME_POSITION | | | | |
| | DOME_ABSOLUTE_POSITION | | | |
| DOME_ABORT_MOTION | | | | |
| | ABORT | | | |
| DOME_SHUTTER | | | | |
| | SHUTTER_OPEN | | | |
| | SHUTTER_CLOSE | | | |
| DOME_GOTO | | | | |
| | DOME_HOME | | | |
| | DOME_PARK | | | |
| DOME_PARAMS | | | | |
| | HOME_POSITION | | | |
| | PARK_POSITION | | | |
| | AUTOSYNC_THRESHOLD | | | |
| DOME_AUTOSYNC | | | | |
| | DOME_AUTOSYNC_ENABLE | | | |
| | DOME_AUTOSYNC_DISABLE | | | |
| DOME_MEASUREMENTS | | | | |
| | DM_DOME_RADIUS | | | |
| | DOME_SHUTTER_WIDTH | | | |
| | DM_NORTH_DISPLACEMENT | | | |
| | DM_EAST_DISPLACEMENT | | | |
| | DM_UP_DISPLACEMENT | | | |
| | DM_OTA_OFFSET | | | |
