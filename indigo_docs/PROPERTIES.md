# INDIGO properties

## Introduction

INDIGO properties and items are abstraction of INDI properties and items. As far as INDIGO uses software bus instead of XML messages,
properties are first of all defined memory structures which are, if needed, mapped to XML or JSON textual representation.

## Common properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| CONNECTION | switch | no | yes | CONNECTED | yes | Item values are undefined if state is not Idle or Ok. |
|  |  |  |  | DISCONNECTED | yes | Disconnected |
| INFO | text | yes | yes | DEVICE_NAME | yes | "Device in INDIGO strictly represents device itself and not device driver. Valid DEVICE_INTERFACE values are defined in indigo_driver.h as indigo_device_interface enumeration." |
|  |  |  |  | DEVICE_DRIVER | yes | Driver name |
|  |  |  |  | DEVICE_VERSION | yes | Driver version |
|  |  |  |  | DEVICE_INTERFACE | yes | Interface |
|  |  |  |  | DEVICE_MODEL | no | Model |
|  |  |  |  | DEVICE_FIRMWARE_REVISION | no | Firmware Rev. |
|  |  |  |  | DEVICE_HARDWARE_REVISION | no | Hardware Rev. |
|  |  |  |  | DEVICE_SERIAL_NUMBER | no | Serial No. |
| SIMULATION | switch | no | no | ENABLED | yes | Enabled |
|  |  |  |  | DISABLED | yes | Disabled |
| CONFIG | switch | no | yes | LOAD | yes | Load |
|  |  |  |  | SAVE | yes | Save |
|  |  |  |  | REMOVE | yes | Remove |
| PROFILE_NAME | text | no | yes | NAME_0,... | yes | Set profile name |
| PROFILE | switch | no | yes | PROFILE_0,... | yes | Select the profile number for subsequent CONFIG operation |
| DEVICE_PORT | text | no | no | PORT | no | Either device path like "/dev/tty0" or URL like "lx200://host:port". |
| DEVICE_BAUDRATE | text | no | no | BAUDRATE | no | Serial port configuration in a string like this: 9600-8N1 |
| DEVICE_PORTS | switch | no | no | valid serial port name |  | When selected, it is copied to DEVICE_PORT property. |
| GEOGRAPHIC_COORDINATES | number | no | yes | LATITUDE | yes | Defined in mount, GPS, and dome driver base classes. |
|  |  |  |  | LONGITUDE | yes | Longitude (0° to 360° +E) |
|  |  |  |  | ELEVATION | yes | Elevation (m) |
|  |  |  |  | ACCURACY | no | GPS driver only |
| UTC_TIME | number |  | no | TIME | yes | Defined in mount, GPS, and dome driver base classes. It depends on hardware if it is undefined, read-only or read-write. |
|  |  |  |  | OFFSET | yes | UTC Offset |
| AUTHENTICATION | text | no | no | PASSWORD | yes | Hidden by default. Write-only property used for device authorization. |
|  |  |  |  | USER | yes | User name |
| ADDITIONAL_INSTANCES | number | no | no | COUNT | yes | Hidden by default. Sets the number of additional device instances to create. |

Properties CONNECTION through ADDITIONAL_INSTANCES are implemented by the driver base class in [indigo_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver.c). GEOGRAPHIC_COORDINATES and UTC_TIME are implemented in the mount, GPS, and dome driver base classes.

## CCD specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| CCD_INFO | number | yes | yes | WIDTH | yes | Horizontal resolution |
|  |  |  |  | HEIGHT | yes | Vertical resolution |
|  |  |  |  | MAX_HORIZONTAL_BIN | yes | Max horizontal binning |
|  |  |  |  | MAX_VERTICAL_BIN | yes | Max vertical binning |
|  |  |  |  | PIXEL_SIZE | yes | in microns |
|  |  |  |  | PIXEL_WIDTH | yes | in microns |
|  |  |  |  | PIXEL_HEIGHT | yes | in microns |
|  |  |  |  | BITS_PER_PIXEL | yes | Bits/pixel |
| CCD_LENS | number | no | yes | APERTURE | yes | in centimeters |
|  |  |  |  | FOCAL_LENGTH | yes | in centimeters |
|  |  |  |  | PHYSICAL_LENGTH | yes | in centimeters |
| CCD_UPLOAD_MODE | switch | no | yes | CLIENT | yes | Upload to client |
|  |  |  |  | LOCAL | yes | Save on server |
|  |  |  |  | BOTH | yes | Upload and save |
|  |  |  |  | NONE | no | Hidden by default. |
| CCD_LOCAL_MODE | text | no | yes | DIR | yes | Directory |
|  |  |  |  | PREFIX | yes | XXX or XXXX is replaced by sequence or a template with %M (MD5), %E/%nE (exposure), %D/%xD (date), %N/%xN (session night date), %H/%xH (time), %C (filter name), %nS (sequence), %F (frame type), %T (chip temperature), %G (gain), %O (offset), %R (resolution), %B (binning), %P (focuser position) format specifier. |
|  |  |  |  | OBJECT | yes | Object name. |
| CCD_EXPOSURE | number | no | yes | EXPOSURE | yes | Start exposure |
| CCD_STREAMING | number | no | no | EXPOSURE | yes | The same as CCD_EXPOSURE, but will upload COUNT images. Use COUNT -1 for endless loop. |
|  |  |  |  | COUNT | yes | Frame count |
| CCD_STREAMING_SETTINGS | number | no | no | UPDATE_LIMIT | yes | Reduce traffic during high speed streaming. |
| CCD_FPS | number | yes | no | FPS | yes | Framerate |
| CCD_ABORT_EXPOSURE | switch | no | yes | ABORT_EXPOSURE | yes | Abort exposure |
| CCD_FRAME | number | no | no | LEFT | yes | If BITS_PER_PIXEL can't be changed, set min and max to the same value. |
|  |  |  |  | TOP | yes | Top |
|  |  |  |  | WIDTH | yes | Width |
|  |  |  |  | HEIGHT | yes | Height |
|  |  |  |  | BITS_PER_PIXEL | yes | Bits per pixel |
| CCD_BIN | number | yes | no | HORIZONTAL | yes | Read-only in the base class; individual drivers may make it writable. CCD_MODE is the preferred way to set binning. |
|  |  |  |  | VERTICAL | yes | Vertical binning |
| CCD_MODE | switch | no | yes | mode identifier | yes | CCD_MODE is the preferred way to set binning, resolution, color mode etc. |
| CCD_READ_MODE | switch | no | no | HIGH_SPEED | yes | High speed |
|  |  |  |  | LOW_NOISE | yes | Low noise |
| CCD_GAIN | number | no | no | GAIN | yes | Gain |
| CCD_EGAIN | number | yes | no | EGAIN | yes | Hidden by default. Electrons per A/D unit. |
| CCD_OFFSET | number | no | no | OFFSET | yes | Offset |
| CCD_GAMMA | number | no | no | GAMMA | yes | Gamma |
| CCD_FRAME_TYPE | switch | no | yes | LIGHT | yes | Light |
|  |  |  |  | BIAS | yes | Bias |
|  |  |  |  | DARK | yes | Dark |
|  |  |  |  | FLAT | yes | Flat |
|  |  |  |  | DARKFLAT | yes | Dark Flat |
| CCD_IMAGE_FORMAT | switch | no | yes | FITS | yes | FITS format |
|  |  |  |  | XISF | yes | XISF format |
|  |  |  |  | RAW | yes | Raw data |
|  |  |  |  | JPEG | yes | JPEG format |
|  |  |  |  | TIFF | yes | TIFF format |
|  |  |  |  | JPEG_AVI | no | JPEG for capture, AVI for streaming. Hidden by default. |
|  |  |  |  | RAW_SER | no | RAW for capture, SER for streaming. Hidden by default. |
| CCD_IMAGE_FILE | text | no | yes | FILE | yes | Filename |
| CCD_IMAGE | blob | no | yes | IMAGE | yes | Image data |
| CCD_TEMPERATURE | number |  | no | TEMPERATURE | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_COOLER | switch | no | no | ON | yes | On |
|  |  |  |  | OFF | yes | Off |
| CCD_COOLER_POWER | number | yes | no | POWER | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_FITS_HEADERS | text | yes | yes | FITS key name, ... | yes | String in form "value" or "'value'" |
| CCD_SET_FITS_HEADER | text | no | yes | KEYWORD | yes | FITS key name |
|  |  |  |  | VALUE | yes | FITS key value |
| CCD_REMOVE_FITS_HEADER | text | no | yes | KEYWORD | yes | FITS key name |
| CCD_PREVIEW | switch | no | yes | DISABLED | yes | Disabled |
|  |  |  |  | ENABLED | yes | Send JPEG preview to client. |
|  |  |  |  | ENABLED_WITH_HISTOGRAM | yes | Send JPEG preview with histogram to client. |
| CCD_PREVIEW_IMAGE | blob | no | no | IMAGE | yes | Hidden by default. |
| CCD_PREVIEW_HISTOGRAM | blob | no | no | IMAGE | yes | Hidden by default. |
| CCD_JPEG_SETTINGS | number | no | no | QUALITY | yes | JPEG conversion quality (10–100). |
|  |  |  |  | TARGET_BACKGROUND | yes | Target mean background level for auto-stretch. |
|  |  |  |  | CLIPPING_POINT | yes | Clipping point for auto-stretch. |
|  |  |  |  | REFERENCE_CHANNEL | yes | Reference channel for white balance (0=AWB, 1=R, 2=G, 3=B). |
| CCD_JPEG_STRETCH_PRESETS | switch | no | no | SLIGHT | yes | Slight |
|  |  |  |  | MODERATE | yes | Moderate |
|  |  |  |  | NORMAL | yes | Normal |
|  |  |  |  | HARD | yes | Hard |
| CCD_RBI_FLUSH_ENABLE | switch | no | no | ENABLED | yes | Hidden by default. Enable RBI (Residual Bulk Image) pre-flush. |
|  |  |  |  | DISABLED | yes | Disabled |
| CCD_RBI_FLUSH | number | no | no | EXPOSURE | yes | Hidden by default. RBI flush parameters. |
|  |  |  |  | COUNT | yes | Number of flushes |

Properties are implemented by CCD driver base class in [indigo_ccd_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ccd_driver.c).

### DSLR extensions (in addition to CCD specific properties)

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| DSLR_PROGRAM | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_APERTURE | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera/lens |
| DSLR_SHUTTER | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_COMPRESSION | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_WHITE_BALANCE | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_ISO | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_EXPOSURE_METERING | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_FOCUS_METERING | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_FOCUS_MODE | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_CAPTURE_MODE | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_CAPTURE_DESTINATION | switch |  | no | ... | yes | Capture destination, if supported by the camera |
| DSLR_FLASH_MODE | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_EXPOSURE_COMPENSATION | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_COMPENSATION_STEP | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_PICTURE_STYLE | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_COLOR_SPACE | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_ASPECT_RATIO | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_BATTERY_LEVEL | number | yes | no | VALUE | yes | Value |
| DSLR_FOCAL_LENGTH | number | yes | no | VALUE | yes | Value |
| DSLR_LOCK | switch | no | no | LOCK | yes | Lock camera UI |
|  |  |  |  | UNLOCK | yes | Off |
| DSLR_MIRROR_LOCKUP | switch | no | no | LOCK | yes | Lock camera mirror |
|  |  |  |  | UNLOCK | yes | Off |
| DSLR_AF | switch | no | no | AF | yes | Start autofocus |
| DSLR_AVOID_AF | switch | no | no | ON | yes | Avoid autofocus |
|  |  |  |  | OFF | yes | Off |
| DSLR_STREAMING_MODE | switch | no | no | LIVE_VIEW | yes | Operation used for streaming |
|  |  |  |  | BURST_MODE | yes | Burst mode |
| DSLR_ZOOM_PREVIEW | switch | no | no | ON | yes | LiveView zoom |
|  |  |  |  | OFF | yes | Off |
| DSLR_DELETE_IMAGE | switch | no | no | ON | yes | Delete image from camera memory/card |
|  |  |  |  | OFF | yes | Off |
| DSLR_SET_HOST_TIME | switch | no | no | SET | yes | Set host time |

DSLR properties are defined by the PTP CCD driver in `indigo_drivers/ccd_ptp`.

## Wheel specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| WHEEL_SLOT | number | no | yes | SLOT | yes | Slot number |
| WHEEL_SLOT_NAME | text | no | yes | SLOT_NAME_1, ... | yes |  |
| WHEEL_SLOT_OFFSET | number | no | yes | SLOT_OFFSET_1, ... | yes | Value is number of focuser steps |

Properties are implemented by wheel driver base class in [indigo_wheel_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_wheel_driver.c).

## Focuser specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| FOCUSER_SPEED | number | no | no | SPEED | yes | Speed |
| FOCUSER_REVERSE_MOTION | switch | no | no | DISABLED | yes | Disabled |
|  |  |  |  | ENABLED | yes | Enabled |
| FOCUSER_DIRECTION | switch | no | yes | MOVE_INWARD | yes | Move inward |
|  |  |  |  | MOVE_OUTWARD | yes | Move outward |
| FOCUSER_STEPS | number | no | yes | STEPS | yes | Relative move (steps) |
| FOCUSER_ON_POSITION_SET | switch | no | no | GOTO | yes | Goto to position |
|  |  |  |  | SYNC | yes | Sync to position |
| FOCUSER_POSITION | number |  | no | POSITION | yes | It depends on hardware if it is undefined, read-only or read-write. |
| FOCUSER_ABORT_MOTION | switch | no | yes | ABORT_MOTION | yes | Abort motion |
| FOCUSER_TEMPERATURE | number | yes | no | TEMPERATURE | yes | Temperature (°C) |
| FOCUSER_BACKLASH | number | no | no | BACKLASH | yes | Mechanical backlash compensation |
| FOCUSER_COMPENSATION | number | no | no | COMPENSATION | yes | Temperature compensation (if FOCUSER_MODE.AUTOMATIC is set). |
|  |  |  |  | THRESHOLD | no | Compensation threshold |
|  |  |  |  | PERIOD | no | Compensation period |
| FOCUSER_MODE | switch | no | no | MANUAL | yes | Manual mode |
|  |  |  |  | AUTOMATIC | yes | Temperature compensated mode |
| FOCUSER_LIMITS | number | no | no | MIN_POSITION | yes | Minimum (steps) |
|  |  |  |  | MAX_POSITION | yes | Maximum (steps) |

Properties are implemented by focuser driver base class in [indigo_focuser_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_focuser_driver.c).

## Mount specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| MOUNT_INFO | text | yes | yes | VENDOR | yes | Vendor |
|  |  |  |  | MODEL | yes | Model |
|  |  |  |  | FIRMWARE_VERSION | yes | Firmware |
| MOUNT_LST_TIME | number | yes | yes | TIME | yes | LST Time |
| MOUNT_SET_HOST_TIME | switch | no | no | SET | yes | Hidden by default. |
| MOUNT_PARK | switch | no | no | PARKED | yes | Mount parked |
|  |  |  |  | UNPARKED | yes | Mount unparked |
| MOUNT_PARK_SET | switch | no | no | CURRENT | yes | Hidden by default. |
|  |  |  |  | DEFAULT | yes | Set default position |
| MOUNT_PARK_POSITION | number | no | no | HA | yes | Hidden by default. |
|  |  |  |  | DEC | yes | Declination (-90 to 90°) |
| MOUNT_HOME | switch | no | no | HOME | yes | Hidden by default. |
|  |  |  |  | AWAY | no | Hidden by default. |
|  |  |  |  | SEARCH | no | Hidden by default. |
| MOUNT_HOME_SET | switch | no | no | CURRENT | yes | Hidden by default. |
|  |  |  |  | DEFAULT | yes | Set default position |
| MOUNT_HOME_POSITION | number | no | no | HA | yes | Hidden by default. |
|  |  |  |  | DEC | yes | Declination (-90 to 90°) |
| MOUNT_ON_COORDINATES_SET | switch | no | yes | TRACK | yes | Slew to target and track |
|  |  |  |  | SYNC | yes | Sync to target |
|  |  |  |  | SLEW | no | Slew to target and stop |
| MOUNT_SLEW_RATE | switch | no | no | GUIDE | no | Guide rate |
|  |  |  |  | CENTERING | no | Centering rate |
|  |  |  |  | FIND | no | Find rate |
|  |  |  |  | MAX | no | Max rate |
| MOUNT_MOTION_DEC | switch | no | yes | NORTH | yes | North |
|  |  |  |  | SOUTH | yes | South |
| MOUNT_MOTION_RA | switch | no | yes | WEST | yes | West |
|  |  |  |  | EAST | yes | East |
| MOUNT_TRACK_RATE | switch | no | no | SIDEREAL | no | Sidereal rate |
|  |  |  |  | SOLAR | no | Solar rate |
|  |  |  |  | LUNAR | no | Lunar rate |
|  |  |  |  | KING | no | Hidden by default. |
|  |  |  |  | CUSTOM | no | Hidden by default. |
| MOUNT_CUSTOM_TRACKING_RATE | number | no | no | RATE | yes | Hidden by default. |
| MOUNT_TRACKING | switch | no | no | ON | yes | Tracking |
|  |  |  |  | OFF | yes | Stopped |
| MOUNT_GUIDE_RATE | number | no | no | RA | yes | Guiding rate (% of sidereal) |
|  |  |  |  | DEC | yes | DEC Guiding rate (% of sidereal) |
| MOUNT_EQUATORIAL_COORDINATES | number | no | yes | RA | yes | Right ascension (0 to 24 hrs) |
|  |  |  |  | DEC | yes | Declination (-90 to 90°) |
| MOUNT_HORIZONTAL_COORDINATES | number | yes | no | AZ | yes | Azimuth (0 to 360°) |
|  |  |  |  | ALT | yes | Altitude (0 to 90°) |
| MOUNT_ABORT_MOTION | switch | no | yes | ABORT_MOTION | yes | Abort motion |
| MOUNT_ALIGNMENT_MODE | switch | no | no | SINGLE_POINT | yes | Hidden by default. |
|  |  |  |  | NEAREST_POINT | yes | Nearest point |
|  |  |  |  | MULTI_POINT | yes | Multi point |
|  |  |  |  | CONTROLLER | yes | Mount controller |
| MOUNT_RAW_COORDINATES | number | yes | no | RA | yes | Hidden by default. |
|  |  |  |  | DEC | yes | Raw declination (-90 to 90°) |
| MOUNT_ALIGNMENT_SELECT_POINTS | switch | no | no | point id | yes | Hidden by default. |
| MOUNT_ALIGNMENT_DELETE_POINTS | switch | no | no | point id | yes | Hidden by default. |
| MOUNT_ALIGNMENT_RESET | switch | no | no | RESET | yes | Hidden by default. |
| MOUNT_EPOCH | number | no | yes | EPOCH | yes | Valid values are 0, 1900, 1950, 2000 and 2050 |
| MOUNT_SIDE_OF_PIER | switch | yes | no | EAST | yes | Hidden by default. |
|  |  |  |  | WEST | yes | West |
| MOUNT_PEC | switch | no | no | ENABLED | yes | Hidden by default. |
|  |  |  |  | DISABLED | yes | Disabled |
| MOUNT_PEC_TRAINING | switch | no | no | STARTED | yes | Hidden by default. |
|  |  |  |  | STOPPED | yes | Stopped |
| MOUNT_STATE | light | yes | no | SLEW | yes | Hidden by default. |
|  |  |  |  | PARK | yes | Park |
|  |  |  |  | HOME | yes | Home |
|  |  |  |  | TRACK | yes | Tracking |

Properties are implemented by mount driver base class in [indigo_mount_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_mount_driver.c).

## Guider specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| GUIDER_GUIDE_DEC | number | no | yes | NORTH | yes | Guide north |
|  |  |  |  | SOUTH | yes | Guide south |
| GUIDER_GUIDE_RA | number | no | yes | EAST | yes | Guide east |
|  |  |  |  | WEST | yes | Guide west |
| GUIDER_RATE | number | no | no | RATE | yes | Hidden by default. % of sidereal rate (RA or both) |
|  |  |  |  | DEC_RATE | no | Hidden by default. % of sidereal rate (DEC) |

Properties are implemented by guider driver base class in [indigo_guider_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_guider_driver.c).

## AO specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| AO_GUIDE_DEC | number | no | yes | NORTH | yes | Guide north |
|  |  |  |  | SOUTH | yes | Guide south |
| AO_GUIDE_RA | number | no | yes | EAST | yes | Guide east |
|  |  |  |  | WEST | yes | Guide west |
| AO_RESET | switch | no | yes | CENTER | yes | Center |
|  |  |  |  | UNJAM | yes | Unjam |

Properties are implemented by AO driver base class in [indigo_ao_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ao_driver.c).

## GPS specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| GEOGRAPHIC_COORDINATES | number | yes | yes | LATITUDE | yes | Latitude (-S / +N) |
|  |  |  |  | LONGITUDE | yes | Longitude (-W / +E) |
|  |  |  |  | ELEVATION | yes | Elevation (m) |
|  |  |  |  | ACCURACY | yes | GPS-specific item (position accuracy in metres). |
| UTC_TIME | text | yes | no | TIME | yes | Hidden by default. |
|  |  |  |  | OFFSET | yes | UTC Offset |
| GPS_STATUS | light | yes | yes | NO_FIX | yes | GPS fix status |
|  |  |  |  | 2D_FIX | yes | 2D Fix |
|  |  |  |  | 3D_FIX | yes | 3D Fix |
| GPS_ADVANCED | switch | no | no | ENABLED | yes | Hidden by default. Enable advanced status report. |
|  |  |  |  | DISABLED | yes | Disable |
| GPS_ADVANCED_STATUS | number | yes | no | SVS_IN_USE | yes | Hidden by default. Advanced status report. |
|  |  |  |  | SVS_IN_VIEW | yes | SVs in view |
|  |  |  |  | PDOP | yes | Position DOP |
|  |  |  |  | HDOP | yes | Horizontal DOP |
|  |  |  |  | VDOP | yes | Vertical DOP |

Properties are implemented by GPS driver base class in [indigo_gps_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_gps_driver.c).

## Dome specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| DOME_SPEED | number | no | no | SPEED | yes | Speed |
| DOME_DIRECTION | switch | no | no | MOVE_CLOCKWISE | yes | Move clockwise |
|  |  |  |  | MOVE_COUNTERCLOCKWISE | yes | Move counterclockwise |
| DOME_ON_COORDINATES_SET | switch | no | no | GOTO | yes | Go to position |
|  |  |  |  | SYNC | no | Hidden by default. |
| DOME_STEPS | number | no | no | STEPS | yes | Relative move (steps/ms) |
| DOME_HORIZONTAL_COORDINATES | number | no | no | AZ | yes | Azimuth (0 to 360°) |
|  |  |  |  | ALT | no | Hidden by default. |
| DOME_SLAVING_PARAMETERS | number | no | no | MOVE_THRESHOLD | yes | Hidden by default. |
| DOME_ABORT_MOTION | switch | no | no | ABORT_MOTION | yes | Abort motion |
| DOME_SHUTTER | switch | no | no | CLOSED | yes | Shutter closed |
|  |  |  |  | OPENED | yes | Shutter opened |
| DOME_FLAP | switch | no | no | CLOSED | yes | Hidden by default. |
|  |  |  |  | OPENED | yes | Flap opened |
| DOME_PARK | switch | no | no | PARKED | yes | Dome parked |
|  |  |  |  | UNPARKED | yes | Dome unparked |
| DOME_PARK_POSITION | number | no | no | AZ | yes | Hidden by default. |
|  |  |  |  | ALT | no | Hidden by default. |
| DOME_HOME | switch | no | no | HOME | yes | Hidden by default. |
| DOME_DIMENSION | number | no | no | RADIUS | yes | Dome radius (m) |
|  |  |  |  | SHUTTER_WIDTH | yes | Dome shutter width (m) |
|  |  |  |  | MOUNT_PIVOT_OFFSET_NS | yes | Mount Pivot Offset N/S (m, +N/-S) |
|  |  |  |  | MOUNT_PIVOT_OFFSET_EW | yes | Mount Pivot Offset E/W (m, +E/-W) |
|  |  |  |  | MOUNT_PIVOT_VERTICAL_OFFSET | yes | Mount Pivot Vertical Offset (m) |
|  |  |  |  | MOUNT_PIVOT_OTA_OFFSET | yes | Optical axis offset from the RA axis (m) |
| GEOGRAPHIC_COORDINATES | number | no | no | LATITUDE | yes | Latitude (-90 to +90° +N) |
|  |  |  |  | LONGITUDE | yes | Longitude (0 to 360° +E) |
|  |  |  |  | ELEVATION | yes | Elevation (m) |
| UTC_TIME | text | no | no | TIME | yes | Hidden by default. |
|  |  |  |  | OFFSET | yes | UTC Offset |
| DOME_SET_HOST_TIME | switch | no | no | SET | yes | Hidden by default. |
| DOME_STATE | light | yes | no | SLEW | yes | Hidden by default. |
|  |  |  |  | PARK | yes | Park |
|  |  |  |  | OPEN | yes | Open |

Properties are implemented by dome driver base class in [indigo_dome_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_dome_driver.c)

## Rotator specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| ROTATOR_STEPS_PER_REVOLUTION | number | no | no | STEPS_PER_REVOLUTION | yes | Hidden by default. |
| ROTATOR_DIRECTION | switch | no | no | NORMAL | yes | Hidden by default. |
|  |  |  |  | REVERSED | yes | Reversed |
| ROTATOR_ON_POSITION_SET | switch | no | no | GOTO | yes | Goto to position |
|  |  |  |  | SYNC | yes | Sync to position |
| ROTATOR_POSITION | number | no | yes | POSITION | yes | Absolute position [°] |
| ROTATOR_RELATIVE_MOVE | number | no | no | RELATIVE_MOVE | yes | Hidden by default. |
| ROTATOR_ABORT_MOTION | switch | no | yes | ABORT_MOTION | yes | Abort motion |
| ROTATOR_BACKLASH | number | no | no | BACKLASH | yes | Hidden by default. |
| ROTATOR_LIMITS | number | no | no | MIN_POSITION | yes | Hidden by default. |
|  |  |  |  | MAX_POSITION | yes | Maximum position [°] |
| ROTATOR_RAW_POSITION | number | yes | no | RAW_POSITION | yes | Hidden by default. |
| ROTATOR_POSITION_OFFSET | number | no | no | POSITION_OFFSET | yes | Hidden by default. |

Properties are implemented by rotator driver base class in [indigo_rotator_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_rotator_driver.c)

## Auxiliary properties

To be used by auxiliary devices like powerboxes, weather stations, etc.

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| AUX_POWER_OUTLET | switch | no | no | OUTLET_1 | yes | Enable power outlets |
|  |  |  |  | OUTLET_2 | no | Outlet #2 |
|  |  |  |  | OUTLET_3 | no | Outlet #3 |
|  |  |  |  | OUTLET_4 | no | Outlet #4 |
| AUX_POWER_OUTLET_STATE | light | yes | no | OUTLET_1 | yes | Power outlets state (IDLE = unused, OK = used, ALERT = over-current etc.) |
|  |  |  |  | OUTLET_2 | no | Outlet #2 state |
|  |  |  |  | OUTLET_3 | no | Outlet #3 state |
|  |  |  |  | OUTLET_4 | no | Outlet #4 state |
| AUX_POWER_OUTLET_CURRENT | number | yes | no | OUTLET_1 | yes | Power outlets current |
|  |  |  |  | OUTLET_2 | no | Outlet #2 current [A] |
|  |  |  |  | OUTLET_3 | no | Outlet #3 current [A] |
|  |  |  |  | OUTLET_4 | no | Outlet #4 current [A] |
| AUX_POWER_OUTLET_VOLTAGE | number | yes | no | OUTLET_1 | yes | Power outlets voltage |
|  |  |  |  | OUTLET_2 | no | Outlet #2 voltage [V] |
|  |  |  |  | OUTLET_3 | no | Outlet #3 voltage [V] |
|  |  |  |  | OUTLET_4 | no | Outlet #4 voltage [V] |
| AUX_HEATER_OUTLET | number | no | no | OUTLET_1 | yes | Set heater outlets power |
|  |  |  |  | OUTLET_2 | no | Heater #2 [%] |
|  |  |  |  | OUTLET_3 | no | Heater #3 [%] |
|  |  |  |  | OUTLET_4 | no | Heater #4 [%] |
| AUX_HEATER_OUTLET_STATE | light | yes | no | OUTLET_1 | yes | Heater outlets state (IDLE = unused, OK = used, ALERT = over-current etc.) |
|  |  |  |  | OUTLET_2 | no | Heater #2 state |
|  |  |  |  | OUTLET_3 | no | Heater #3 state |
|  |  |  |  | OUTLET_4 | no | Heater #4 state |
| AUX_HEATER_OUTLET_CURRENT | number | yes | no | OUTLET_1 | yes | Heater outlets current |
|  |  |  |  | OUTLET_2 | no | Heater #2 current [A] |
|  |  |  |  | OUTLET_3 | no | Heater #3 current [A] |
|  |  |  |  | OUTLET_4 | no | Heater #4 current [A] |
| AUX_USB_PORT | switch | no | no | PORT_1 | yes | Enable USB ports on smart hub |
|  |  |  |  | PORT_2 | no | Port #2 |
|  |  |  |  | PORT_3 | no | Port #3 |
|  |  |  |  | PORT_4 | no | Port #4 |
|  |  |  |  | PORT_5 | no | Port #5 |
|  |  |  |  | PORT_6 | no | Port #6 |
|  |  |  |  | PORT_7 | no | Port #7 |
|  |  |  |  | PORT_8 | no | Port #8 |
| AUX_USB_PORT_STATE | light | yes | no | PORT_1 | yes | USB port state (IDLE = unused or disabled, OK = used, BUSY = transient state, ALERT = over-current etc.) |
|  |  |  |  | PORT_2 | no | Port #2 state |
|  |  |  |  | PORT_3 | no | Port #3 state |
|  |  |  |  | PORT_4 | no | Port #4 state |
|  |  |  |  | PORT_5 | no | Port #5 state |
|  |  |  |  | PORT_6 | no | Port #6 state |
|  |  |  |  | PORT_7 | no | Port #7 state |
|  |  |  |  | PORT_8 | no | Port #8 state |
| AUX_DEW_CONTROL | switch | no | no | MANUAL | yes | Use AUX_HEATER_OUTLET values |
|  |  |  |  | AUTOMATIC | yes | Set power automatically |
| AUX_WEATHER | number | yes | no | TEMPERATURE | no | Temperature [C] |
|  |  |  |  | HUMIDITY | no | Humidity [%] |
|  |  |  |  | DEWPOINT | no | Dewpoint [C] |
|  |  |  |  | WIND_SPEED | no | Wind speed [raw] |
|  |  |  |  | WIND_DIRECTION | no | Wind direction |
|  |  |  |  | ATMOSPHERIC_PRESSURE | no | Pressure [hPa] |
|  |  |  |  | RAIN | no | Dampness [raw] |
|  |  |  |  | SKY_BRIGHTNESS | no | Sky brightness [m/arcsec²] |
|  |  |  |  | SKY_TEMPERATURE | no | Sky temperature [°C] |
|  |  |  |  | SKY_BORTLE_CLASS | no | Sky Bortle class |
| AUX_INFO | number | yes | no | ... | no | Any number of any number items |
| AUX_CONTROL | switch | no | no | ... | no | Any number of any switch items |
| AUX_LIGHT_SWITCH | switch | no | no | ON | yes | Flatbox light on |
|  |  |  |  | OFF | yes | Turn light off |
| AUX_LIGHT_INTENSITY | number | no | no | LIGHT_INTENSITY | yes | Flatbox light intensity |

Auxiliary property names are defined in `indigo_libs/indigo/indigo_names.h`; auxiliary devices use the common attach/change/enumerate hooks from `indigo_libs/indigo_aux_driver.c`.

## Agent filter properties

All agents inherit a set of device-selector and relation properties from the agent filter base class. Each agent enables only the device-list properties relevant to its function; the rest remain hidden.

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| FILTER_CCD_LIST | switch | no | no | device name | yes | Hidden by default. Select camera. |
| FILTER_WHEEL_LIST | switch | no | no | device name | yes | Hidden by default. Select filter wheel. |
| FILTER_FOCUSER_LIST | switch | no | no | device name | yes | Hidden by default. Select focuser. |
| FILTER_ROTATOR_LIST | switch | no | no | device name | yes | Hidden by default. Select rotator. |
| FILTER_MOUNT_LIST | switch | no | no | device name | yes | Hidden by default. Select mount. |
| FILTER_GUIDER_LIST | switch | no | no | device name | yes | Hidden by default. Select guider. |
| FILTER_DOME_LIST | switch | no | no | device name | yes | Hidden by default. Select dome. |
| FILTER_GPS_LIST | switch | no | no | device name | yes | Hidden by default. Select GPS. |
| FILTER_JOYSTICK_LIST | switch | no | no | device name | yes | Hidden by default. Select joystick. |
| FILTER_AUX_1_LIST | switch | no | no | device name | yes | Hidden by default. Select AUX #1 device. |
| FILTER_AUX_2_LIST | switch | no | no | device name | yes | Hidden by default. Select AUX #2 device. |
| FILTER_AUX_3_LIST | switch | no | no | device name | yes | Hidden by default. Select AUX #3 device. |
| FILTER_AUX_4_LIST | switch | no | no | device name | yes | Hidden by default. Select AUX #4 device. |
| FILTER_RELATED_CCD_LIST | switch | no | no | device name | yes | Hidden by default. Select related camera. |
| FILTER_RELATED_WHEEL_LIST | switch | no | no | device name | yes | Hidden by default. Select related filter wheel. |
| FILTER_RELATED_FOCUSER_LIST | switch | no | no | device name | yes | Hidden by default. Select related focuser. |
| FILTER_RELATED_ROTATOR_LIST | switch | no | no | device name | yes | Hidden by default. Select related rotator. |
| FILTER_RELATED_MOUNT_LIST | switch | no | no | device name | yes | Hidden by default. Select related mount. |
| FILTER_RELATED_GUIDER_LIST | switch | no | no | device name | yes | Hidden by default. Select related guider. |
| FILTER_RELATED_DOME_LIST | switch | no | no | device name | yes | Hidden by default. Select related dome. |
| FILTER_RELATED_GPS_LIST | switch | no | no | device name | yes | Hidden by default. Select related GPS. |
| FILTER_RELATED_JOYSTICK_LIST | switch | no | no | device name | yes | Hidden by default. Select related joystick. |
| FILTER_RELATED_AUX_1_LIST | switch | no | no | device name | yes | Hidden by default. Select related AUX #1 device. |
| FILTER_RELATED_AUX_2_LIST | switch | no | no | device name | yes | Hidden by default. Select related AUX #2 device. |
| FILTER_RELATED_AUX_3_LIST | switch | no | no | device name | yes | Hidden by default. Select related AUX #3 device. |
| FILTER_RELATED_AUX_4_LIST | switch | no | no | device name | yes | Hidden by default. Select related AUX #4 device. |
| FILTER_RELATED_AGENT_LIST | switch | no | no | agent name | yes | Hidden by default. Select related agents. Uses ANY_OF_MANY rule. |
| FILTER_FORCE_SYMMETRIC_RELATIONS | switch | no | no | ENABLED | yes | Hidden by default. Force symmetric device relations. |
|  |  |  |  | DISABLED | yes | Disable |
| CCD_LENS_FOV | number | yes | no | FOV_WIDTH | yes | Hidden by default. FOV and pixel scale, computed from connected camera and lens parameters. |
|  |  |  |  | FOV_HEIGHT | yes | FOV height (°) |
|  |  |  |  | PIXEL_SCALE_WIDTH | yes | Pixel scale width (°/px) |
|  |  |  |  | PIXEL_SCALE_HEIGHT | yes | Pixel scale height (°/px) |

Common agent properties are defined in `indigo_libs/indigo_filter.c`.

## Agent specific properties

Agent drivers define these properties on top of the common agent filter properties listed above. The table also lists common filter properties whose visibility or meaning is changed by a particular agent.

### Alpaca agent

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| AGENT_ALPACA_DISCOVERY | number | no | PORT | Alpaca discovery server port. |
| AGENT_ALPACA_DEVICES | text | no | dynamic device numbers | Device mapping table, initially empty and resized as devices are discovered. |
| AGENT_ALPACA_CAMERA_BAYERPAT | text | no | dynamic camera numbers | Per-camera Bayer pattern mapping. |

Source: `indigo_drivers/agent_alpaca/indigo_agent_alpaca.c`.

### ASTAP agent

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| AGENT_ASTAP_INDEX | switch | no | dynamic ASTAP index names | Installed ASTAP index management. Also updates `AGENT_PLATESOLVER_USE_INDEX`. |

Source: `indigo_drivers/agent_astap/indigo_agent_astap.c`.

### Astrometry agent

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| AGENT_ASTROMETRY_INDEX_41XX | switch | no | dynamic Tycho-2 index names | Installed 41xx index management. Also updates `AGENT_PLATESOLVER_USE_INDEX`. |
| AGENT_ASTROMETRY_INDEX_42XX | switch | no | dynamic 2MASS index names | Installed 42xx index management. Also updates `AGENT_PLATESOLVER_USE_INDEX`. |

Source: `indigo_drivers/agent_astrometry/indigo_agent_astrometry.c`.

### Auxiliary agent

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| FILTER_AUX_1_LIST | switch | no | device name | Exposes the common AUX #1 selector. |
| FILTER_AUX_2_LIST | switch | no | device name | Exposes the common AUX #2 selector. |
| FILTER_AUX_3_LIST | switch | no | device name | Exposes the common AUX #3 selector. |
| FILTER_AUX_4_LIST | switch | no | device name | Exposes the common AUX #4 selector. |
| FILTER_RELATED_AGENT_LIST | switch | no | agent name | Exposes the common related-agent selector. |

Source: `indigo_drivers/agent_auxiliary/indigo_agent_auxiliary.c`.

### Config agent

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| AGENT_CONFIG_SETUP | switch | no | AUTOSAVE_DEVICE_CONFIGS, UNLOAD_UNUSED_DRIVERS | Agent configuration options. |
| AGENT_CONFIG_SAVE | text | no | NAME | Save the current setup as a named configuration. |
| AGENT_CONFIG_REMOVE | text | no | NAME | Remove a named configuration. |
| AGENT_CONFIG_LAST_CONFIG | text | yes | NAME | Last configuration used. |
| AGENT_CONFIG_LOAD | switch | no | dynamic configuration names | Load one available configuration. |
| AGENT_CONFIG_DRIVERS | switch | yes | dynamic driver names | Drivers referenced by configurations. |
| AGENT_CONFIG_PROFILES | text | yes | dynamic profile names | Profiles referenced by configurations. |

Source: `indigo_drivers/agent_config/indigo_agent_config.c`.

### Guider agent

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| FILTER_CCD_LIST | switch | no | device name | Exposes the common CCD selector for guider frames. |
| FILTER_GUIDER_LIST | switch | no | device name | Exposes the common guider output selector. |
| FILTER_RELATED_AGENT_LIST | switch | no | agent name | Exposes the common related-agent selector. |
| AGENT_GUIDER_CORRECTION_MODE_RA | switch | no | PI_CONTROLLER, HYSTERESIS, LINEAR_TREND, PPEC | RA drift correction mode. |
| AGENT_GUIDER_CORRECTION_MODE_DEC | switch | no | PI_CONTROLLER, HYSTERESIS, LINEAR_TREND, RESIST_SWITCH | Dec drift correction mode. |
| AGENT_GUIDER_DETECTION_MODE | switch | no | SELECTION, WEIGHTED_SELECTION, DONUTS, CENTROID | Drift detection mode. |
| AGENT_GUIDER_DEC_MODE | switch | no | BOTH, NORTH, SOUTH, NONE | Dec guiding mode. |
| AGENT_GUIDER_APPLY_DEC_BACKLASH | switch | no | DISABLED, ENABLED | Dec backlash compensation switch. |
| AGENT_START_PROCESS | switch | no | PREVIEW_1, PREVIEW, CALIBRATION, CALIBRATION_AND_GUIDING, GUIDING, CLEAR_SELECTION, RESET | Guider start/reset commands. |
| AGENT_ABORT_PROCESS | switch | no | ABORT | Abort the current guider process. |
| AGENT_PROCESS_FEATURES | switch | no | ENABLE_LOGGING, FAIL_ON_CALIBRATION_ERROR, RESET_ON_CALIBRATION_ERROR, FAIL_ON_GUIDING_ERROR, CONTINUE_ON_GUIDING_ERROR, RESET_ON_GUIDING_ERROR, RESET_ON_GUIDING_ERROR_WAIT_ALL_STARS, USE_INCLUDE_FOR_DONUTS | Guider process behavior options. |
| AGENT_GUIDER_MOUNT_COORDINATES | number | no | RA, DEC, SIDE_OF_PIER | Telescope coordinates used by the guider. |
| AGENT_GUIDER_SETTINGS | number | no | EXPOSURE, DELAY, STEP0, MAX_BL_STEPS, MIN_BL_DRIFT, MAX_CALIBRATION_STEPS, MIN_CALIBRATION_DRIFT, ANGLE, SIDE_OF_PIER, BACKLASH, SPEED_RA, SPEED_DEC, MIN_ERROR, MIN_PULSE, MAX_PULSE, AGGRESSIVITY_RA, AGGRESSIVITY_DEC, I_GAIN_RA, I_GAIN_DEC, STACK, HYSTERESIS_AGGRESSIVENESS_RA, HYSTERESIS_AGGRESSIVENESS_DEC, HYSTERESIS_HYSTERESIS_RA, HYSTERESIS_HYSTERESIS_DEC, LINEAR_TREND_AGGRESSIVENESS_RA, LINEAR_TREND_AGGRESSIVENESS_DEC, RESIST_SWITCH_AGGRESSIVENESS_DEC, RESIST_SWITCH_FAST_THRESHOLD_DEC, DITHERING_MAX_AMOUNT, DITHERING_SETTLE_TIME_LIMIT, DITHERING_LIMIT, PPEC_REACTIVE_GAIN_RA, PPEC_PREDICTION_GAIN_RA, PPEC_PERIOD_RA, PPEC_PERIOD_FIXED, PPEC_RETAIN_MODEL_RA | Guider calibration, guiding, dithering, and PPEC settings. |
| AGENT_GUIDER_FLIP_REVERSES_DEC | switch | no | ENABLED, DISABLED | Reverse Dec speed after meridian flip. |
| AGENT_GUIDER_STARS | switch | no | REFRESH, dynamic star names | Detected guider stars. |
| AGENT_GUIDER_SELECTION | number | no | RADIUS, SUBFRAME, EDGE_CLIPPING, INCLUDE_LEFT, INCLUDE_TOP, INCLUDE_WIDTH, INCLUDE_HEIGHT, EXCLUDE_LEFT, EXCLUDE_TOP, EXCLUDE_WIDTH, EXCLUDE_HEIGHT, COUNT, X, Y, dynamic X_n/Y_n | Guider star and region selection. |
| AGENT_GUIDER_STATS | number | yes | PHASE, FRAME, REFERENCE_X, REFERENCE_Y, DRIFT_X, DRIFT_Y, DRIFT_RA, DRIFT_DEC, DRIFT_RA_S, DRIFT_DEC_S, CORR_RA, CORR_DEC, RMSE_RA, RMSE_DEC, RMSE_RA_S, RMSE_DEC_S, RMSE_RA_ST, RMSE_DEC_ST, RMSE_RA_S_ST, RMSE_DEC_S_ST, SNR, DELAY, DITHERING, PPEC_LEARNING, PPEC_PERIOD, CORR_RESPONSE_RA, CORR_RESPONSE_DEC | Guider process statistics. |
| AGENT_GUIDER_LOG | text | no | DIR, TEMPLATE | Guider log output location and filename template. |
| AGENT_GUIDER_DITHERING_OFFSETS | number | no | X, Y | Manual dithering offsets. |
| AGENT_GUIDER_DITHERING_STRATEGY | switch | no | RANDOMIZED_SPIRAL, RANDOM, SPIRAL | Dithering pattern selection. |
| AGENT_GUIDER_DITHER | switch | no | TRIGGER, RESET | Trigger or reset dithering. |
| AGENT_GUIDER_RESET_PPEC | switch | no | RESET | Reset the predictive PEC model. |

Source: `indigo_drivers/agent_guider/indigo_agent_guider.c`.

### Imager agent

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| FILTER_CCD_LIST | switch | no | device name | Exposes the common CCD selector. |
| FILTER_WHEEL_LIST | switch | no | device name | Exposes the common filter-wheel selector. |
| FILTER_FOCUSER_LIST | switch | no | device name | Exposes the common focuser selector. |
| FILTER_AUX_1_LIST | switch | no | device name | Exposes the common AUX #1 selector as an external shutter. |
| FILTER_RELATED_AGENT_LIST | switch | no | agent name | Exposes the common related-agent selector. |
| AGENT_IMAGER_BATCH | number | no | COUNT, EXPOSURE, DELAY, FRAMES_TO_SKIP_BEFORE_DITHER, PAUSE_AFTER_TRANSIT | Imaging batch settings. |
| AGENT_IMAGER_FOCUS | number | no | INITIAL, FINAL, ITERATIVE_INITIAL, ITERATIVE_FINAL, U_CURVE_SAMPLES, U_CURVE_STEP, BAHTINOV_SIGMA, BRACKETING_STEP, BACKLASH, BACKLASH_OVERSHOOT_FACTOR, STACK, REPEAT, DELAY | Autofocus settings. |
| AGENT_IMAGER_FOCUS_FAILURE | switch | no | STOP, RESTORE | Action on autofocus failure. |
| AGENT_IMAGER_FOCUS_ESTIMATOR | switch | no | U_CURVE, HFD_PEAK, RMS_CONTRAST, BAHTINOV | Autofocus estimator selection. |
| AGENT_IMAGER_CAPTURE | number | no | CAPTURE | Capture trigger/control value. |
| AGENT_START_PROCESS | switch | no | PREVIEW_1, PREVIEW, EXPOSURE, STREAMING, FOCUSING, CLEAR_SELECTION, RESET | Imager start/reset commands. |
| AGENT_PAUSE_PROCESS | switch | no | PAUSE, PAUSE_WAIT, PAUSE_AFTER_TRANSIT | Pause modes for the active process. |
| AGENT_ABORT_PROCESS | switch | no | ABORT | Abort the current imager process. |
| AGENT_PROCESS_FEATURES | switch | no | ENABLE_DITHERING, DITHER_AFTER_LAST_FRAME, PAUSE_AFTER_TRANSIT, APPLY_FILTER_OFFSETS, MACRO_MODE | Imager process behavior options. |
| AGENT_IMAGER_DOWNLOAD_FILE | text | no | FILE | Select file to download. |
| AGENT_IMAGER_DOWNLOAD_FILES | switch | no | REFRESH, dynamic file names | Downloadable image-cache files. |
| AGENT_IMAGER_DOWNLOAD_IMAGE | blob | yes | IMAGE | Downloaded image data. |
| AGENT_IMAGER_DELETE_FILE | text | no | FILE | Delete image-cache file. |
| AGENT_IMAGER_DISK_USAGE | number | yes | TOTAL, USED, FREE | Image-cache disk usage. |
| AGENT_WHEEL_FILTER | switch | no | dynamic filter slots | Agent-side filter selection. |
| AGENT_FOCUSER_CONTROL | switch | no | FOCUS_IN, FOCUS_OUT | Agent-side focuser manual control. |
| AGENT_IMAGER_STARS | switch | no | REFRESH, dynamic star names | Detected imager stars. |
| AGENT_IMAGER_SELECTION | number | no | RADIUS, SUBFRAME, INCLUDE_LEFT, INCLUDE_TOP, INCLUDE_WIDTH, INCLUDE_HEIGHT, EXCLUDE_LEFT, EXCLUDE_TOP, EXCLUDE_WIDTH, EXCLUDE_HEIGHT, COUNT, X, Y, dynamic X_n/Y_n | Imager star and region selection. |
| AGENT_IMAGER_SPIKES | number | yes | RHO_1, THETA_1, RHO_2, THETA_2, RHO_3, THETA_3 | Bahtinov spike fit data. |
| AGENT_IMAGER_STATS | number | yes | EXPOSURE, DELAY, FRAME, FRAMES, BATCH_INDEX, BATCH, BATCHES, PHASE, DRIFT_X, DRIFT_Y, DITHERING, FOCUS_OFFSET, FOCUS_POSITION, RMS_CONTRAST, BEST_FOCUS_DEVIATION, FRAMES_TO_DITHERING, BAHTINOV_ERROR, MAX_STARS_TO_USE, PEAK, FWHM, HFD, dynamic HFD_n | Imager process statistics. |
| AGENT_IMAGER_BREAKPOINT | switch | no | PRE_BATCH, PRE_CAPTURE, POST_CAPTURE, PRE_DELAY, POST_DELAY, POST_BATCH | Breakpoints for scripted imaging flows. |
| AGENT_IMAGER_RESUME_CONDITION | switch | no | TRIGGER, BARRIER | Resume condition after a breakpoint. |
| AGENT_IMAGER_BARRIER_STATE | light | yes | dynamic breakpoint names | Barrier state for paused imaging flows. |

Source: `indigo_drivers/agent_imager/indigo_agent_imager.c`.

### Mount agent

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| FILTER_MOUNT_LIST | switch | no | device name | Exposes the common mount selector. |
| FILTER_DOME_LIST | switch | no | device name | Exposes the common dome selector. |
| FILTER_ROTATOR_LIST | switch | no | device name | Exposes the common rotator selector. |
| FILTER_GPS_LIST | switch | no | device name | Exposes the common GPS selector. |
| FILTER_JOYSTICK_LIST | switch | no | device name | Exposes the common joystick selector. |
| FILTER_RELATED_AGENT_LIST | switch | no | agent name | Exposes the common related-agent selector. |
| GEOGRAPHIC_COORDINATES | number | no | LATITUDE, LONGITUDE, ELEVATION | Agent-owned geographic coordinates. |
| AGENT_SITE_DATA_SOURCE | switch | no | HOST, MOUNT, DOME, GPS | Source for site coordinates. |
| AGENT_SET_HOST_TIME | switch | no | MOUNT, DOME | Use host time for selected devices. |
| ABORT_RELATED_PROCESS | switch | no | IMAGER, GUIDER | Abort related imager/guider processes. |
| AGENT_LX200_SERVER | switch | no | STARTED, STOPPED | LX200 server state. |
| AGENT_LX200_CONFIGURATION | number | no | PORT, EPOCH | LX200 server configuration. |
| AGENT_LIMITS | number | no | HA_TRACKING, LOCAL_TIME, COORDINATES_PROPAGATE_THRESHOLD | Mount-agent limits and propagation threshold. |
| AGENT_MOUNT_FOV | number | no | ANGLE, WIDTH, HEIGHT | Field-of-view values used by mount clients. |
| AGENT_MOUNT_EQUATORIAL_COORDINATES | number | no | RA, DEC | Mount-agent target coordinates. |
| AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY | number | yes | RA_JNOW, DEC_JNOW, ALT, AZ, AIRMASS, HA, RISE, TRANSIT, SET, TIME_TO_TRANSIT, FLIP_REQUIRED, PARALLACTIC_ANGLE, DEROTATION_RATE | Calculated display coordinates and derotation data. |
| AGENT_START_PROCESS | switch | no | SLEW, SYNC, PARK, UNPARK, HOME, TRACK_ON, TRACK_OFF, DOME_PARK, DOME_UNPARK, DOME_OPEN, DOME_CLOSE, RESET | Mount and dome start/reset commands. |
| AGENT_ABORT_PROCESS | switch | no | ABORT | Abort active mount-agent process. |
| AGENT_PROCESS_FEATURES | switch | no | ENABLE_HA_LIMIT, ENABLE_TIME_LIMIT, ENABLE_DOME_SLAVING, MAKE_DOME_SLAVING_PERSISTENT, ENABLE_FIELD_DEROTATION, MAKE_FIELD_DEROTATION_PERSISTENT, ENABLE_JOYSTICK_CONTROL | Mount-agent process behavior options. |
| AGENT_MOUNT_STATE | light | yes | SLEW, PARK, HOME, TRACK, DOME_SLAVING, FIELD_DEROTATION | Mount-agent state lights. |
| AGENT_DOME_STATE | light | yes | SLEW, PARK, OPEN | Dome state lights. |
| AGENT_MOUNT_FEATURE | switch | yes | SLEW, SYNC, PARK, HOME, TRACK | Capabilities detected from the selected mount. |
| AGENT_DOME_FEATURE | switch | yes | SLEW, SYNC, PARK, OPEN | Capabilities detected from the selected dome. |

Source: `indigo_drivers/agent_mount/indigo_agent_mount.c`.

### Plate solver agents

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| FILTER_RELATED_AGENT_LIST | switch | no | agent name | Exposes the common related-agent selector. |
| AGENT_PLATESOLVER_USE_INDEX | switch | no | dynamic index names | Index selection shared by plate solver implementations. |
| AGENT_PLATESOLVER_HINTS | number | no | RADIUS, RA, DEC, EPOCH, SCALE, PARITY, DOWNSAMPLE, DEPTH, CPULIMIT | Solver hints. |
| AGENT_PLATESOLVER_WCS | number | yes | STATE, RA, DEC, EPOCH, ANGLE, WIDTH, HEIGHT, SCALE, PARITY, INDEX | WCS result. |
| AGENT_PLATESOLVER_SYNC | switch | no | DISABLED, SYNC, CENTER, CALCULATE_PA_ERROR, RECALCULATE_PA_ERROR | Obsolete sync/center mode property retained for compatibility. |
| AGENT_START_PROCESS | switch | no | SOLVE, SYNC, CENTER, PRECISE_GOTO, CALCULATE_PA_ERROR, RECALCULATE_PA_ERROR, RESET | Plate solver start/reset commands. |
| AGENT_ABORT_PROCESS | switch | no | ABORT | Abort active plate solving process. |
| AGENT_PLATESOLVER_SOLVE_IMAGES | switch | no | ENABLED, DISABLED | Enable or disable solving incoming images. |
| AGENT_PLATESOLVER_EXPOSURE | number | no | EXPOSURE | Exposure used by plate solver capture flows. |
| AGENT_PLATESOLVER_PA_SETTINGS | number | no | EXPOSURE, HA_MOVE, COMPENSATE_REFRACTION | Polar-alignment settings. |
| AGENT_PLATESOLVER_PA_STATE | number | yes | STATE, DEC_DRIFT_2, DEC_DRIFT_3, TARGET_RA, TARGET_DEC, CURRENT_RA, CURRENT_DEC, ALT_POLAR_ERROR, AZ_POLAR_ERROR, ALT_CORRECTION_UP, AZ_CORRECTION_CW, POLAR_ERROR, ACCURACY_WARNING | Polar-alignment state and correction values. |
| AGENT_PLATESOLVER_GOTO_SETTINGS | number | no | RA, DEC | Target coordinates for solver-assisted goto. |
| AGENT_PLATESOLVER_MOUNT_SETTLE_TIME | number | no | SETTLE_TIME | Settle time after mount motion. |
| AGENT_PLATESOLVER_ABORT | switch | no | ABORT | Obsolete abort property retained for compatibility. |
| AGENT_PLATESOLVER_IMAGE | blob | no | IMAGE | Input image for solving. |
| AGENT_PLATESOLVER_IMAGE_OUTPUT | blob | yes | IMAGE | Solver output image. |
| CCD_PREVIEW, CCD_PREVIEW_IMAGE, CCD_JPEG_SETTINGS, CCD_JPEG_STRETCH_PRESETS | mixed | mixed | see CCD property sections | Reuses CCD preview and JPEG properties for plate-solver image preview. |

Source: `indigo_libs/indigo_platesolver.c`; used by `indigo_drivers/agent_solver/indigo_agent_solver.c`, `indigo_drivers/agent_astap/indigo_agent_astap.c`, and `indigo_drivers/agent_astrometry/indigo_agent_astrometry.c`.

### Scripting agent

| Property name | Type | RO | Items | Comments |
| ----- | ----- | ----- | ----- | ----- |
| AGENT_SCRIPTING_RUN_SCRIPT | text | no | SCRIPT | Run an ad-hoc script. |
| AGENT_SCRIPTING_ADD_SCRIPT | text | no | NAME, SCRIPT | Add a named script. |
| AGENT_SCRIPTING_EXECUTE_SCRIPT | switch | no | dynamic script names | Execute one saved script. |
| AGENT_SCRIPTING_DELETE_SCRIPT | text | no | NAME | Delete a saved script. |
| AGENT_SCRIPTING_ON_LOAD_SCRIPT | switch | no | AGENT_SCRIPTING_ADD_SCRIPT, dynamic script names | Scripts executed when the agent loads. |
| AGENT_SCRIPTING_ON_UNLOAD_SCRIPT | switch | no | AGENT_SCRIPTING_ADD_SCRIPT, dynamic script names | Scripts executed when the agent unloads. |
| AGENT_SCRIPTING_SCRIPT_%d | text | no | NAME, SCRIPT | Dynamic property created for each saved script. |
| dynamic cached script properties | text, number, switch, light | mixed | dynamic item names | Script-created or script-cached INDIGO properties. |

Source: `indigo_drivers/agent_scripting/indigo_agent_scripting.c`.

## Driver specific properties

This section lists driver-level additions and driver-specific use of the common properties above. A driver is listed when it defines its own property names, exposes common properties for a secondary logical device, or changes the visibility, item count, or semantics of base properties.

### ao_sx

Driver-specific use of existing properties: `AO_GUIDE_DEC`, `AO_GUIDE_RA`, `AO_RESET`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`.

Source: `indigo_drivers/ao_sx/indigo_ao_sx.c`.

### aux_arteskyflat

Driver-specific use of existing properties: `AUX_LIGHT_INTENSITY`, `AUX_LIGHT_SWITCH`.

Source: `indigo_drivers/aux_arteskyflat/indigo_aux_arteskyflat.c`.

### aux_astromechanics

Driver-specific use of existing properties: `AUX_WEATHER`.

Source: `indigo_drivers/aux_astromechanics/indigo_aux_astromechanics.c`.

### aux_cloudwatcher

Custom properties: `AUX_CLOUD`, `AUX_CLOUD_THRESHOLDS`, `AUX_DEW_THRESHOLD`, `AUX_DEW_WARNING`, `AUX_GPIO_OUTLETS`, `AUX_HUMIDITY`, `AUX_HUMIDITY_THRESHOLDS`, `AUX_OUTLET_NAMES`, `AUX_RAIN`, `AUX_RAIN_THRESHOLD`, `AUX_RAIN_THRESHOLDS`, `AUX_RAIN_WARNING`, `AUX_SKY`, `AUX_SKY_THRESHOLDS`, `AUX_WIND`, `AUX_WIND_THRESHOLD`, `AUX_WIND_THRESHOLDS`, `AUX_WIND_WARNING`, `X_AAG_CONSTANTS`, `X_ANEMOMETER_TYPE`, `X_HEATER_CONTROL_STATE`, `X_RAIN_SENSOR_HEATER_SETUP`, `X_SKY_CORRECTION`.

Driver-specific use of existing properties: `AUX_INFO`, `AUX_WEATHER`.

Source: `indigo_drivers/aux_cloudwatcher/indigo_aux_cloudwatcher.c`.

### aux_dragonfly

Custom properties: `AUX_GPIO_OUTLETS`, `AUX_GPIO_SENSORS`, `AUX_OUTLET_NAMES`, `AUX_OUTLET_PULSE_LENGTHS`, `AUX_SENSOR_NAMES`.

Source: `indigo_drivers/aux_dragonfly/indigo_aux_dragonfly.c`.

### aux_dsusb

Custom properties: `X_CONFIG`.

Driver-specific use of existing properties: `CCD_ABORT_EXPOSURE`, `CCD_EXPOSURE`.

Source: `indigo_drivers/aux_dsusb/indigo_aux_dsusb.c`.

### aux_fbc

Custom properties: `AUX_LIGHT_IMPULSE`.

Driver-specific use of existing properties: `AUX_LIGHT_INTENSITY`, `CCD_EXPOSURE`.

Source: `indigo_drivers/aux_fbc/indigo_aux_fbc.c`.

### aux_flatmaster

Driver-specific use of existing properties: `AUX_LIGHT_INTENSITY`, `AUX_LIGHT_SWITCH`.

Source: `indigo_drivers/aux_flatmaster/indigo_aux_flatmaster.c`.

### aux_flipflat

Custom properties: `AUX_COVER`.

Driver-specific use of existing properties: `AUX_LIGHT_INTENSITY`, `AUX_LIGHT_SWITCH`.

Source: `indigo_drivers/aux_flipflat/indigo_aux_flipflat.c`.

### aux_geoptikflat

Driver-specific use of existing properties: `AUX_LIGHT_INTENSITY`, `AUX_LIGHT_SWITCH`.

Source: `indigo_drivers/aux_geoptikflat/indigo_aux_geoptikflat.c`.

### aux_joystick

Custom properties: `JOYSTICK_AXES`, `JOYSTICK_BUTTONS`, `JOYSTICK_MAPPING`, `JOYSTICK_OPTIONS`.

Driver-specific use of existing properties: `MOUNT_ABORT_MOTION`, `MOUNT_HOME`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_PARK`, `MOUNT_SLEW_RATE`, `MOUNT_TRACKING`.

Source: `indigo_drivers/aux_joystick/indigo_aux_joystick.c`.

### aux_mgbox

Custom properties: `AUX_DEW_THRESHOLD`, `AUX_DEW_WARNING`, `AUX_GPIO_OUTLETS`, `AUX_OUTLET_NAMES`, `AUX_OUTLET_PULSE_LENGTHS`, `X_REBOOT_DEVICE`, `X_REBOOT_GPS`, `X_SEND_GPS_DATA_TO_MOUNT`, `X_SEND_WEATHER_DATA_TO_MOUNT`, `X_WEATHER_CALIBRATION`.

Driver-specific use of existing properties: `AUX_WEATHER`, `GEOGRAPHIC_COORDINATES`, `GPS_ADVANCED`, `UTC_TIME`.

Source: `indigo_drivers/aux_mgbox/indigo_aux_mgbox.c`.

### aux_ppb

Custom properties: `AUX_OUTLET_NAMES`, `AUX_SAVE_OUTLET_STATES_AS_DEFAULT`, `X_AUX_REBOOT`, `X_DSLR_POWER`.

Driver-specific use of existing properties: `AUX_DEW_CONTROL`, `AUX_HEATER_OUTLET`, `AUX_INFO`, `AUX_POWER_OUTLET`, `AUX_POWER_OUTLET_STATE`, `AUX_WEATHER`.

Source: `indigo_drivers/aux_ppb/indigo_aux_ppb.c`.

### aux_rts

Driver-specific use of existing properties: `CCD_ABORT_EXPOSURE`, `CCD_EXPOSURE`.

Source: `indigo_drivers/aux_rts/indigo_aux_rts.c`.

### aux_skyalert

Driver-specific use of existing properties: `AUX_INFO`, `AUX_WEATHER`.

Source: `indigo_drivers/aux_skyalert/indigo_aux_skyalert.c`.

### aux_sqm

Driver-specific use of existing properties: `AUX_INFO`, `AUX_WEATHER`.

Source: `indigo_drivers/aux_sqm/indigo_aux_sqm.c`.

### aux_svbpowerbox

Custom properties: `AUX_DEW_WARNING`, `AUX_OUTLET_NAMES`, `AUX_TEMPERATURE_SENSORS`.

Driver-specific use of existing properties: `AUX_DEW_CONTROL`, `AUX_HEATER_OUTLET`, `AUX_INFO`, `AUX_POWER_OUTLET`, `AUX_POWER_OUTLET_CURRENT`, `AUX_POWER_OUTLET_VOLTAGE`, `AUX_USB_PORT`, `AUX_WEATHER`.

Source: `indigo_drivers/aux_svbpowerbox/indigo_aux_svbpowerbox.c`.

### aux_uch

Custom properties: `AUX_OUTLET_NAMES`, `AUX_SAVE_OUTLET_STATES_AS_DEFAULT`, `X_AUX_REBOOT`.

Driver-specific use of existing properties: `AUX_INFO`, `AUX_USB_PORT`.

Source: `indigo_drivers/aux_uch/indigo_aux_uch.c`.

### aux_upb

Custom properties: `AUX_OUTLET_NAMES`, `AUX_SAVE_OUTLET_STATES_AS_DEFAULT`, `X_AUX_HUB`, `X_AUX_REBOOT`, `X_AUX_VARIABLE_POWER_OUTLET`.

Driver-specific use of existing properties: `AUX_DEW_CONTROL`, `AUX_HEATER_OUTLET`, `AUX_HEATER_OUTLET_CURRENT`, `AUX_HEATER_OUTLET_STATE`, `AUX_INFO`, `AUX_POWER_OUTLET`, `AUX_POWER_OUTLET_CURRENT`, `AUX_POWER_OUTLET_STATE`, `AUX_USB_PORT`, `AUX_USB_PORT_STATE`, `AUX_WEATHER`, `FOCUSER_ABORT_MOTION`, `FOCUSER_BACKLASH`, `FOCUSER_LIMITS`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_STEPS`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/aux_upb/indigo_aux_upb.c`.

### aux_upb3

Custom properties: `AUX_OUTLET_NAMES`, `AUX_REBOOT`, `AUX_SAVE_OUTLET_STATES_AS_DEFAULT`, `AUX_VARIABLE_POWER_OUTLET`.

Driver-specific use of existing properties: `AUX_DEW_CONTROL`, `AUX_HEATER_OUTLET`, `AUX_INFO`, `AUX_POWER_OUTLET`, `AUX_POWER_OUTLET_CURRENT`, `AUX_POWER_OUTLET_STATE`, `AUX_USB_PORT`, `AUX_WEATHER`, `FOCUSER_ABORT_MOTION`, `FOCUSER_BACKLASH`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_STEPS`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/aux_upb3/indigo_aux_upb3.c`.

### aux_usbdp

Custom properties: `AUX_DEW_THRESHOLD`, `AUX_DEW_WARNING`, `AUX_HEATER_AGGRESSIVITY`, `AUX_LINK_CHANNELS_2AND3`, `AUX_OUTLET_NAMES`, `AUX_TEMPERATURE_CALLIBRATION`, `AUX_TEMPERATURE_SENSORS`.

Driver-specific use of existing properties: `AUX_DEW_CONTROL`, `AUX_HEATER_OUTLET`, `AUX_HEATER_OUTLET_STATE`, `AUX_WEATHER`.

Source: `indigo_drivers/aux_usbdp/indigo_aux_usbdp.c`.

### aux_wbplusv3

Custom properties: `AUX_DEW_WARNING`, `AUX_OUTLET_NAMES`, `AUX_TEMPERATURE_SENSORS`, `X_AUX_CALIBRATE`.

Driver-specific use of existing properties: `AUX_DEW_CONTROL`, `AUX_HEATER_OUTLET`, `AUX_INFO`, `AUX_POWER_OUTLET`, `AUX_POWER_OUTLET_VOLTAGE`, `AUX_USB_PORT`, `AUX_WEATHER`.

Source: `indigo_drivers/aux_wbplusv3/indigo_aux_wbplusv3.c`.

### aux_wbprov3

Custom properties: `AUX_DEW_WARNING`, `AUX_OUTLET_NAMES`, `AUX_TEMPERATURE_SENSORS`, `X_AUX_CALIBRATE`.

Driver-specific use of existing properties: `AUX_DEW_CONTROL`, `AUX_HEATER_OUTLET`, `AUX_INFO`, `AUX_POWER_OUTLET`, `AUX_POWER_OUTLET_CURRENT`, `AUX_POWER_OUTLET_VOLTAGE`, `AUX_USB_PORT`, `AUX_WEATHER`.

Source: `indigo_drivers/aux_wbprov3/indigo_aux_wbprov3.c`.

### aux_wcv4ec

Custom properties: `AUX_COVER`, `X_COVER_DETECT_OPEN_CLOSE`, `X_COVER_SET_OPEN_CLOSE`, `X_HEATER`.

Driver-specific use of existing properties: `AUX_LIGHT_INTENSITY`, `AUX_LIGHT_SWITCH`.

Source: `indigo_drivers/aux_wcv4ec/indigo_aux_wcv4ec.c`.

### ccd_asi

Custom properties: `ASI_ADVANCED`, `ASI_CUSTOM_SUFFIX`, `ASI_PRESETS`, `PIXEL_FORMAT`.

Driver-specific use of existing properties: `CCD_COOLER`, `CCD_COOLER_POWER`, `CCD_EGAIN`, `CCD_EXPOSURE`, `CCD_GAIN`, `CCD_GAMMA`, `CCD_IMAGE_FORMAT`, `CCD_MODE`, `CCD_OFFSET`, `CCD_STREAMING`, `CCD_STREAMING_SETTINGS`, `CCD_TEMPERATURE`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`.

Source: `indigo_drivers/ccd_asi/indigo_ccd_asi.c`.

### ccd_atik

Custom properties: `ATIK_PRESETS`, `ATIK_WINDOW_HEATER`.

Driver-specific use of existing properties: `CCD_COOLER`, `CCD_COOLER_POWER`, `CCD_GAIN`, `CCD_MODE`, `CCD_OFFSET`, `CCD_READ_MODE`, `CCD_TEMPERATURE`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/ccd_atik/indigo_ccd_atik.c`.

### ccd_dsi

Driver-specific use of existing properties: `CCD_ABORT_EXPOSURE`, `CCD_BIN`, `CCD_EXPOSURE`, `CCD_GAIN`, `CCD_MODE`, `CCD_OFFSET`, `CCD_TEMPERATURE`.

Source: `indigo_drivers/ccd_dsi/indigo_ccd_dsi.c`.

### ccd_fli

Custom properties: `FLI_CAMERA_MODE`, `FLI_NFLUSHES`.

Driver-specific use of existing properties: `CCD_COOLER`, `CCD_COOLER_POWER`, `CCD_RBI_FLUSH`, `CCD_RBI_FLUSH_ENABLE`, `CCD_TEMPERATURE`.

Source: `indigo_drivers/ccd_fli/indigo_ccd_fli.c`.

### ccd_iidc

Driver-specific use of existing properties: `CCD_GAIN`, `CCD_GAMMA`, `CCD_IMAGE_FORMAT`, `CCD_MODE`, `CCD_STREAMING`, `CCD_STREAMING_SETTINGS`, `CCD_TEMPERATURE`.

Source: `indigo_drivers/ccd_iidc/indigo_ccd_iidc.c`.

### ccd_mi

Driver-specific use of existing properties: `CCD_COOLER`, `CCD_COOLER_POWER`, `CCD_EGAIN`, `CCD_GAIN`, `CCD_MODE`, `CCD_READ_MODE`, `CCD_TEMPERATURE`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/ccd_mi/indigo_ccd_mi.c`.

### ccd_pentax

Driver-specific use of existing properties: `DSLR_APERTURE`, `DSLR_ISO`, `DSLR_PROGRAM`, `DSLR_SHUTTER`.

Source: `indigo_drivers/ccd_pentax/indigo_ccd_pentax.c`.

### ccd_playerone

Custom properties: `PIXEL_FORMAT`, `POA_ADVANCED`, `POA_CUSTOM_SUFFIX`, `POA_PRESETS`, `POA_SENSOR_MODE`.

Driver-specific use of existing properties: `CCD_COOLER`, `CCD_COOLER_POWER`, `CCD_EGAIN`, `CCD_EXPOSURE`, `CCD_GAIN`, `CCD_IMAGE_FORMAT`, `CCD_MODE`, `CCD_OFFSET`, `CCD_STREAMING`, `CCD_STREAMING_SETTINGS`, `CCD_TEMPERATURE`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`.

Source: `indigo_drivers/ccd_playerone/indigo_ccd_playerone.c`.

### ccd_ptp

Driver-specific use of existing properties: `CCD_JPEG_SETTINGS`, `CCD_MODE`, `CCD_PREVIEW_IMAGE`, `CCD_STREAMING`, `CCD_UPLOAD_MODE`, `DSLR_AF`, `DSLR_DELETE_IMAGE`, `DSLR_LOCK`, `DSLR_MIRROR_LOCKUP`, `DSLR_SET_HOST_TIME`, `DSLR_ZOOM_PREVIEW`, `FOCUSER_POSITION`, `FOCUSER_SPEED`.

Source: `indigo_drivers/ccd_ptp/indigo_ccd_ptp.c`.

### ccd_sbig

Custom properties: `SBIG_ABG_STATE`, `SBIG_ADD_AO`, `SBIG_ADD_WHEEL`, `SBIG_FREEZE_TEC`.

Driver-specific use of existing properties: `CCD_COOLER`, `CCD_COOLER_POWER`, `CCD_INFO`, `CCD_MODE`, `CCD_TEMPERATURE`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`, `SBIG_ABG`.

Source: `indigo_drivers/ccd_sbig/indigo_ccd_sbig.c`.

### ccd_ssag

Driver-specific use of existing properties: `CCD_GAIN`, `CCD_IMAGE_FORMAT`, `CCD_STREAMING`, `CCD_STREAMING_SETTINGS`.

Source: `indigo_drivers/ccd_ssag/indigo_ccd_ssag.c`.

### ccd_svb

Custom properties: `PIXEL_FORMAT`, `SVB_ADVANCED`.

Driver-specific use of existing properties: `CCD_COOLER`, `CCD_COOLER_POWER`, `CCD_EXPOSURE`, `CCD_GAIN`, `CCD_GAMMA`, `CCD_IMAGE_FORMAT`, `CCD_MODE`, `CCD_OFFSET`, `CCD_STREAMING`, `CCD_STREAMING_SETTINGS`, `CCD_TEMPERATURE`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`.

Source: `indigo_drivers/ccd_svb/indigo_ccd_svb.c`.

### ccd_sx

Custom properties: `X_CCD_FLOOD_LED`.

Driver-specific use of existing properties: `CCD_ABORT_EXPOSURE`, `CCD_BIN`, `CCD_EXPOSURE`, `CCD_FRAME`, `CCD_MODE`, `CCD_TEMPERATURE`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`, `30`.

Source: `indigo_drivers/ccd_sx/indigo_ccd_sx.c`.

### ccd_touptek

Custom properties: `X_AAF_BEEP`, `X_CALIBRATE`, `X_CCD_ADVANCED`, `X_CCD_BIN_MODE`, `X_CCD_CONVERSION_GAIN`, `X_CCD_FAN`, `X_CCD_HEATER`, `X_CCD_LED`, `X_WHEEL_MODEL`.

Driver-specific use of existing properties: `CCD_COOLER`, `CCD_COOLER_POWER`, `CCD_GAIN`, `CCD_IMAGE_FORMAT`, `CCD_MODE`, `CCD_OFFSET`, `CCD_STREAMING`, `CCD_TEMPERATURE`, `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/ccd_touptek/indigo_ccd_touptek.c`.

### ccd_uvc

Driver-specific use of existing properties: `CCD_BIN`, `CCD_GAIN`, `CCD_GAMMA`, `CCD_IMAGE_FORMAT`, `CCD_INFO`, `CCD_MODE`, `CCD_STREAMING`, `CCD_STREAMING_SETTINGS`.

Source: `indigo_drivers/ccd_uvc/indigo_ccd_uvc.c`.

### dome_baader

Custom properties: `X_EMERGENCY_CLOSE`.

Driver-specific use of existing properties: `DOME_FLAP`, `DOME_ON_COORDINATES_SET`, `DOME_SLAVING_PARAMETERS`, `DOME_SPEED`.

Source: `indigo_drivers/dome_baader/indigo_dome_baader.c`.

### dome_beaver

Custom properties: `X_CLEAR_FAILURES`, `X_CONDITIONS_SAFETY`, `X_FAILURE_MESSAGES`, `X_ROTATOR_CALIBRATE`, `X_SHUTTER_CALIBRATE`.

Driver-specific use of existing properties: `DOME_HOME`, `DOME_ON_COORDINATES_SET`, `DOME_PARK_POSITION`, `DOME_SHUTTER`, `DOME_SLAVING_PARAMETERS`, `DOME_SPEED`.

Source: `indigo_drivers/dome_beaver/indigo_dome_beaver.c`.

### dome_dragonfly

Custom properties: `AUX_GPIO_OUTLETS`, `AUX_GPIO_SENSORS`, `AUX_OUTLET_NAMES`, `AUX_OUTLET_PULSE_LENGTHS`, `AUX_SENSOR_NAMES`, `LA_DOME_BUTTON_FUNCTION`, `LA_DOME_SETTINGS`.

Driver-specific use of existing properties: `DOME_DIMENSION`, `DOME_DIRECTION`, `DOME_HORIZONTAL_COORDINATES`, `DOME_PARK`, `DOME_SLAVING_PARAMETERS`, `DOME_SPEED`, `DOME_STEPS`.

Source: `indigo_drivers/dome_dragonfly/indigo_dome_dragonfly.c`.

### dome_nexdome

Custom properties: `NEXDOME_CALLIBRATE`, `NEXDOME_FIND_HOME`, `NEXDOME_POWER`, `NEXDOME_RESET_SHUTTER_COMM`, `NEXDOME_REVERSED`.

Driver-specific use of existing properties: `DOME_ON_COORDINATES_SET`, `DOME_SLAVING_PARAMETERS`, `DOME_SPEED`.

Source: `indigo_drivers/dome_nexdome/indigo_dome_nexdome.c`.

### dome_nexdome3

Custom properties: `NEXDOME_ACCELERATION_TIME`, `NEXDOME_BATTERY_POWER`, `NEXDOME_COMMAND`, `NEXDOME_FIND_HOME`, `NEXDOME_HOME_POSITION`, `NEXDOME_MOVE_THRESHOLD`, `NEXDOME_RAIN_SENSOR`, `NEXDOME_RANGE`, `NEXDOME_SETTINGS`, `NEXDOME_VELOCITY`, `NEXDOME_XB_STATE`.

Driver-specific use of existing properties: `DOME_ON_COORDINATES_SET`, `DOME_SLAVING_PARAMETERS`, `DOME_SPEED`.

Source: `indigo_drivers/dome_nexdome3/indigo_dome_nexdome3.c`.

### dome_skyroof

Custom properties: `HEATER_CONTROL`.

Driver-specific use of existing properties: `DOME_ABORT_MOTION`, `DOME_DIMENSION`, `DOME_DIRECTION`, `DOME_HORIZONTAL_COORDINATES`, `DOME_PARK`, `DOME_SHUTTER`, `DOME_SLAVING_PARAMETERS`, `DOME_SPEED`, `DOME_STEPS`.

Source: `indigo_drivers/dome_skyroof/indigo_dome_skyroof.c`.

### dome_talon6ror

Custom properties: `X_CLOSE_COND`, `X_DELAY_CONF`, `X_MOTOR_CONF`, `X_POSITION`, `X_SENSORS`, `X_STATUS`, `X_TIMER_COND`.

Driver-specific use of existing properties: `DOME_DIMENSION`, `DOME_DIRECTION`, `DOME_HORIZONTAL_COORDINATES`, `DOME_PARK`, `DOME_SLAVING_PARAMETERS`, `DOME_SPEED`, `DOME_STEPS`.

Source: `indigo_drivers/dome_talon6ror/indigo_dome_talon6ror.c`.

### focuser_asi

Custom properties: `EAF_BATTERY_INFO`, `EAF_BEEP_ON_MOVE`, `EAF_CUSTOM_SUFFIX`, `EAF_SCAN_BLUETOOTH`.

Driver-specific use of existing properties: `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_asi/indigo_focuser_asi.c`.

### focuser_askar

Custom properties: `X_FOCUSER_MOTOR_MODE`.

Driver-specific use of existing properties: `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_askar/indigo_focuser_askar.c`.

### focuser_astroasis

Custom properties: `BACKLASH_DIRECTION_PROPERTY`, `BEEP_ON_MOVE_PROPERTY`, `BEEP_ON_POWER_UP_PROPERTY`, `BLUETOOTH_PROPERTY`, `BLUETOOTH_NAME_PROPERTY`, `BOARD_TEMPERATURE_PROPERTY`, `CUSTOM_SUFFIX`, `FACTORY_RESET_PROPERTY`.

Driver-specific use of existing properties: `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_astroasis/indigo_focuser_astroasis.c`.

### focuser_astromechanics

Custom properties: `X_FOCUSER_APERTURE`.

Driver-specific use of existing properties: `FOCUSER_ABORT_MOTION`, `FOCUSER_POSITION`, `FOCUSER_SPEED`, `FOCUSER_STEPS`.

Source: `indigo_drivers/focuser_astromechanics/indigo_focuser_astromechanics.c`.

### focuser_dmfc

Custom properties: `X_FOCUSER_ENCODER`, `X_FOCUSER_LED`, `X_FOCUSER_MOTOR_TYPE`.

Driver-specific use of existing properties: `FOCUSER_ABORT_MOTION`, `FOCUSER_BACKLASH`, `FOCUSER_LIMITS`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_STEPS`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_dmfc/indigo_focuser_dmfc.c`.

### focuser_dsd

Custom properties: `DSD_COILS_MODE`, `DSD_CURRENT_CONTROL`, `DSD_MODEL_HINT`, `DSD_STEP_MODE`, `DSD_TIMINGS`.

Driver-specific use of existing properties: `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_dsd/indigo_focuser_dsd.c`.

### focuser_efa

Custom properties: `X_FOCUSER_CALIBRATION`, `X_FOCUSER_FANS`.

Driver-specific use of existing properties: `FOCUSER_LIMITS`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_efa/indigo_focuser_efa.c`.

### focuser_fc3

Driver-specific use of existing properties: `FOCUSER_ABORT_MOTION`, `FOCUSER_BACKLASH`, `FOCUSER_DIRECTION`, `FOCUSER_LIMITS`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_STEPS`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_fc3/indigo_focuser_fc3.c`.

### focuser_fcusb

Custom properties: `X_FOCUSER_FREQUENCY`.

Driver-specific use of existing properties: `FOCUSER_ABORT_MOTION`, `FOCUSER_POSITION`, `FOCUSER_STEPS`.

Source: `indigo_drivers/focuser_fcusb/indigo_focuser_fcusb.c`.

### focuser_fli

Driver-specific use of existing properties: `FOCUSER_SPEED`.

Source: `indigo_drivers/focuser_fli/indigo_focuser_fli.c`.

### focuser_focusdreampro

Custom properties: `X_FOCUSER_DUTY_CYCLE`.

Driver-specific use of existing properties: `FOCUSER_LIMITS`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_focusdreampro/indigo_focuser_focusdreampro.c`.

### focuser_ioptron

Custom properties: `ZERO_SYNC`.

Driver-specific use of existing properties: `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_ioptron/indigo_focuser_ioptron.c`.

### focuser_lacerta

Driver-specific use of existing properties: `FOCUSER_BACKLASH`, `FOCUSER_LIMITS`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_lacerta/indigo_focuser_lacerta.c`.

### focuser_lakeside

Custom properties: `X_FOCUSER_ACTIVE_SLOPE`.

Driver-specific use of existing properties: `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_MODE`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_lakeside/indigo_focuser_lakeside.c`.

### focuser_lunatico / rotator_lunatico shared

Custom properties: `AUX_GPIO_SENSORS`, `AUX_OUTLET_NAMES`, `AUX_SENSOR_NAMES`, `LA_MOTOR_TYPE`, `LA_MOTOR_WIRING`, `LA_POWER_CONTROL`, `LA_STEP_MODE`, `LA_TEMPERATURE_SENSOR`, `LUNATICO_MODEL`, `LUNATICO_PORT_EXP_CONFIG`, `LUNATICO_PORT_THIRD_CONFIG`.

Driver-specific use of existing properties: `AUX_POWER_OUTLET`, `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`, `ROTATOR_BACKLASH`, `ROTATOR_DIRECTION`, `ROTATOR_LIMITS`, `ROTATOR_STEPS_PER_REVOLUTION`.

Source: `indigo_drivers/focuser_lunatico/shared/lunatico_shared.c`.

### focuser_mjkzz

Driver-specific use of existing properties: `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_mjkzz/indigo_focuser_mjkzz.c`.

### focuser_moonlite

Custom properties: `X_FOCUSER_STEPPING_MODE`.

Driver-specific use of existing properties: `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_moonlite/indigo_focuser_moonlite.c`.

### focuser_mypro2

Custom properties: `X_COILS_MODE`, `X_SETTLE_TIME`, `X_STEP_MODE`.

Driver-specific use of existing properties: `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_mypro2/indigo_focuser_mypro2.c`.

### focuser_nfocus

Driver-specific use of existing properties: `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_nfocus/indigo_focuser_nfocus.c`.

### focuser_nstep

Custom properties: `X_FOCUSER_PHASE_WIRING`, `X_FOCUSER_STEPPING_MODE`.

Driver-specific use of existing properties: `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_MODE`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_nstep/indigo_focuser_nstep.c`.

### focuser_optec

Driver-specific use of existing properties: `FOCUSER_ABORT_MOTION`, `FOCUSER_COMPENSATION`, `FOCUSER_MODE`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_optec/indigo_focuser_optec.c`.

### focuser_optecfl

Custom properties: `X_FOCUSER_TYPE`.

Driver-specific use of existing properties: `FOCUSER_COMPENSATION`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_optecfl/indigo_focuser_optecfl.c`.

### focuser_primaluce

Custom properties: `X_CALIBRATE`, `X_CALIBRATE_A`, `X_CONFIG`, `X_HOLD_CURR`, `X_LEDS`, `X_RUNPRESET`, `X_RUNPRESET_1`, `X_RUNPRESET_2`, `X_RUNPRESET_3`, `X_RUNPRESET_L`, `X_RUNPRESET_M`, `X_RUNPRESET_S`, `X_STATE`, `X_WIFI`, `X_WIFI_AP`, `X_WIFI_STA`.

Driver-specific use of existing properties: `FOCUSER_ABORT_MOTION`, `FOCUSER_BACKLASH`, `FOCUSER_POSITION`, `FOCUSER_SPEED`, `FOCUSER_STEPS`, `FOCUSER_TEMPERATURE`, `ROTATOR_ABORT_MOTION`, `ROTATOR_ON_POSITION_SET`, `ROTATOR_POSITION`.

Source: `indigo_drivers/focuser_primaluce/indigo_focuser_primaluce.c`.

### focuser_prodigy

Custom properties: `AUX_OUTLET_NAMES`, `X_AUX_REBOOT`, `X_FOCUSER_PARK`.

Driver-specific use of existing properties: `AUX_POWER_OUTLET`, `AUX_USB_PORT`, `FOCUSER_BACKLASH`, `FOCUSER_LIMITS`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_prodigy/indigo_focuser_prodigy.c`.

### focuser_qhy

Driver-specific use of existing properties: `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_qhy/indigo_focuser_qhy.c`.

### focuser_robofocus

Custom properties: `X_FOCUSER_CONFIG`, `X_FOCUSER_POWER_CHANNELS`.

Driver-specific use of existing properties: `FOCUSER_LIMITS`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_robofocus/indigo_focuser_robofocus.c`.

### focuser_steeldrive2

Custom properties: `X_NAME`, `X_PID_SETTINGS`, `X_RESET`, `X_SAVED_VALUES`, `X_SELECT_AMB_SENSOR`, `X_SELECT_PID_SENSOR`, `X_SELECT_TC_SENSOR`, `X_START_ZEROING`, `X_STATUS`, `X_USE_AUTO_DEW`, `X_USE_ENDSTOP`, `X_USE_PID`.

Driver-specific use of existing properties: `AUX_HEATER_OUTLET`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_steeldrive2/indigo_focuser_steeldrive2.c`.

### focuser_usbv3

Custom properties: `X_FOCUSER_STEP_SIZE`.

Driver-specific use of existing properties: `FOCUSER_ABORT_MOTION`, `FOCUSER_COMPENSATION`, `FOCUSER_LIMITS`, `FOCUSER_MODE`, `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`, `FOCUSER_SPEED`, `FOCUSER_STEPS`, `FOCUSER_TEMPERATURE`.

Source: `indigo_drivers/focuser_usbv3/indigo_focuser_usbv3.c`.

### focuser_wemacro

Custom properties: `X_RAIL_CONFIG`, `X_RAIL_EXECUTE`, `X_RAIL_SHUTTER`.

Driver-specific use of existing properties: `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`.

Source: `indigo_drivers/focuser_wemacro/indigo_focuser_wemacro.c`.

### gps_gpsd

Driver-specific use of existing properties: `GEOGRAPHIC_COORDINATES`, `GPS_ADVANCED`, `UTC_TIME`.

Source: `indigo_drivers/gps_gpsd/indigo_gps_gpsd.c`.

### gps_nmea

Custom properties: `X_GPS_SELECTED_SYSTEM`.

Driver-specific use of existing properties: `GEOGRAPHIC_COORDINATES`, `GPS_ADVANCED`, `UTC_TIME`.

Source: `indigo_drivers/gps_nmea/indigo_gps_nmea.c`.

### guider_asi

Driver-specific use of existing properties: `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`.

Source: `indigo_drivers/guider_asi/indigo_guider_asi.c`.

### guider_cgusbst4

Driver-specific use of existing properties: `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`.

Source: `indigo_drivers/guider_cgusbst4/indigo_guider_cgusbst4.c`.

### guider_gpusb

Driver-specific use of existing properties: `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`.

Source: `indigo_drivers/guider_gpusb/indigo_guider_gpusb.c`.

### mount_asi

Custom properties: `X_BUZZER`, `X_MAX_SLEW_SPEED`, `X_MERIDIAN`, `X_MERIDIAN_LIMIT`, `X_MOUNT_MODE`.

Driver-specific use of existing properties: `MOUNT_ALIGNMENT_RESET`, `MOUNT_GUIDE_RATE`, `MOUNT_HOME`, `MOUNT_INFO`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_SET_HOST_TIME`, `MOUNT_SIDE_OF_PIER`, `MOUNT_SLEW_RATE`, `MOUNT_TRACKING`, `MOUNT_TRACK_RATE`, `UTC_TIME`.

Source: `indigo_drivers/mount_asi/indigo_mount_asi.c`.

### mount_ioptron

Custom properties: `MOUNT_MERIDIAN_HANDLING`, `MOUNT_MERIDIAN_LIMIT`, `PROTOCOL_VERSION`.

Driver-specific use of existing properties: `GEOGRAPHIC_COORDINATES`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`, `GUIDER_RATE`, `MOUNT_ABORT_MOTION`, `MOUNT_CUSTOM_TRACKING_RATE`, `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_GUIDE_RATE`, `MOUNT_HOME`, `MOUNT_INFO`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_PARK_SET`, `MOUNT_PEC`, `MOUNT_PEC_TRAINING`, `MOUNT_SET_HOST_TIME`, `MOUNT_SIDE_OF_PIER`, `MOUNT_SLEW_RATE`, `MOUNT_STATE`, `MOUNT_TRACKING`, `MOUNT_TRACK_RATE`, `UTC_TIME`.

Source: `indigo_drivers/mount_ioptron/indigo_mount_ioptron.c`.

### mount_lx200

Custom properties: `X_ALTITUDE_LIMITS`, `X_MOUNT_MODE`, `X_MOUNT_TYPE`, `X_NYX_LEVELER`, `X_NYX_WIFI_AP`, `X_NYX_WIFI_CL`, `X_NYX_WIFI_RESET`, `X_ONSTEP_AUTOMATIC_MERIDIAN_FLIP`, `X_ONSTEP_MERIDIAN_LIMITS`, `X_ONSTEP_PREFERRED_PIER_SIDE`, `X_ZWO_BUZZER`.

Driver-specific use of existing properties: `AUX_HEATER_OUTLET`, `AUX_INFO`, `AUX_POWER_OUTLET`, `AUX_WEATHER`, `FOCUSER_POSITION`, `FOCUSER_REVERSE_MOTION`, `MOUNT_GUIDE_RATE`, `MOUNT_HOME`, `MOUNT_HOME_SET`, `MOUNT_INFO`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_PARK_SET`, `MOUNT_PEC`, `MOUNT_SET_HOST_TIME`, `MOUNT_SIDE_OF_PIER`, `MOUNT_SLEW_RATE`, `MOUNT_STATE`, `MOUNT_TRACKING`, `MOUNT_TRACK_RATE`, `UTC_TIME`.

Source: `indigo_drivers/mount_lx200/indigo_mount_lx200.c`.

### mount_mxhd

Driver-specific use of existing properties: `GUIDER_RATE`, `MOUNT_GUIDE_RATE`, `MOUNT_HOME`, `MOUNT_INFO`, `MOUNT_PARK`, `MOUNT_SET_HOST_TIME`, `MOUNT_SIDE_OF_PIER`, `MOUNT_STATE`, `MOUNT_TRACK_RATE`, `UTC_TIME`.

Source: `indigo_drivers/mount_mxhd/indigo_mount_mxhd.c`.

### mount_nexstar

Custom properties: `COMMAND_GUIDE_RATE`, `TRACKING_MODE`.

Driver-specific use of existing properties: `GEOGRAPHIC_COORDINATES`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`, `MOUNT_GUIDE_RATE`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK_POSITION`, `MOUNT_SET_HOST_TIME`, `MOUNT_SIDE_OF_PIER`, `MOUNT_SLEW_RATE`, `MOUNT_TRACK_RATE`, `UTC_TIME`.

Source: `indigo_drivers/mount_nexstar/indigo_mount_nexstar.c`.

### mount_nexstaraux

Driver-specific use of existing properties: `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`, `GUIDER_RATE`, `MOUNT_ABORT_MOTION`, `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_GUIDE_RATE`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_STATE`, `MOUNT_TRACKING`, `MOUNT_TRACK_RATE`.

Source: `indigo_drivers/mount_nexstaraux/indigo_mount_nexstaraux.c`.

### mount_pmc8

Custom properties: `CONNECTION_MODE`, `MOUNT_TYPE` (`AUTO`, `G11`, `TITAN`, `EXOS-2`, `iEXOS-100`).

Driver-specific use of existing properties: `GUIDER_RATE`, `MOUNT_GUIDE_RATE`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_SIDE_OF_PIER`.

Source: `indigo_drivers/mount_pmc8/indigo_mount_pmc8.driver`; generated output in `indigo_drivers/mount_pmc8/indigo_mount_pmc8.c`.

### mount_rainbow

Driver-specific use of existing properties: `MOUNT_GUIDE_RATE`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_SET_HOST_TIME`, `UTC_TIME`.

Source: `indigo_drivers/mount_rainbow/indigo_mount_rainbow.c`.

### mount_starbook

Custom properties: `STARBOOK_RESET`, `STARBOOK_TIMEZONE`.

Driver-specific use of existing properties: `MOUNT_GUIDE_RATE`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_PARK_POSITION`, `MOUNT_PARK_SET`, `MOUNT_SET_HOST_TIME`, `MOUNT_SIDE_OF_PIER`, `MOUNT_TRACKING`, `MOUNT_TRACK_RATE`, `UTC_TIME`.

Source: `indigo_drivers/mount_starbook/indigo_mount_starbook.c`.

### mount_synscan

Custom properties: `MOUNT_AUTOHOME`, `MOUNT_AUTOHOME_SETTINGS`, `MOUNT_OPERATING_MODE`, `MOUNT_USE_ENCODERS`, `POLARSCOPE`.

Driver-specific use of existing properties: `CCD_ABORT_EXPOSURE`, `CCD_EXPOSURE`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`, `GUIDER_RATE`, `MOUNT_ABORT_MOTION`, `MOUNT_ALIGNMENT_DELETE_POINTS`, `MOUNT_ALIGNMENT_MODE`, `MOUNT_ALIGNMENT_SELECT_POINTS`, `MOUNT_EPOCH`, `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_GUIDE_RATE`, `MOUNT_HOME`, `MOUNT_HOME_POSITION`, `MOUNT_HOME_SET`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_PARK`, `MOUNT_PARK_POSITION`, `MOUNT_PARK_SET`, `MOUNT_PEC`, `MOUNT_PEC_TRAINING`, `MOUNT_RAW_COORDINATES`, `MOUNT_SIDE_OF_PIER`, `MOUNT_STATE`, `MOUNT_TRACKING`, `MOUNT_TRACK_RATE`.

Source: `indigo_drivers/mount_synscan/indigo_mount_synscan.c`.

### mount_temma

Custom properties: `TEMMA_CORRECTION_SPEED`, `TEMMA_HIGH_SPEED`, `TEMMA_ZENITH`.

Driver-specific use of existing properties: `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_PARK_POSITION`, `MOUNT_PARK_SET`, `MOUNT_SET_HOST_TIME`, `MOUNT_SIDE_OF_PIER`, `UTC_TIME`.

Source: `indigo_drivers/mount_temma/indigo_mount_temma.c`.

### rotator_asi

Custom properties: `CAA_BEEP_ON_MOVE`, `CAA_CUSTOM_SUFFIX`.

Driver-specific use of existing properties: `ROTATOR_BACKLASH`, `ROTATOR_DIRECTION`, `ROTATOR_LIMITS`, `ROTATOR_ON_POSITION_SET`, `ROTATOR_RELATIVE_MOVE`.

Source: `indigo_drivers/rotator_asi/indigo_rotator_asi.c`.

### rotator_falcon

Driver-specific use of existing properties: `ROTATOR_ABORT_MOTION`, `ROTATOR_DIRECTION`, `ROTATOR_POSITION`, `ROTATOR_RELATIVE_MOVE`.

Source: `indigo_drivers/rotator_falcon/indigo_rotator_falcon.c`.

### rotator_optec

Custom properties: `X_HOME`, `X_RATE`, `X_ROTATE`.

Driver-specific use of existing properties: `ROTATOR_ABORT_MOTION`, `ROTATOR_DIRECTION`, `ROTATOR_ON_POSITION_SET`.

Source: `indigo_drivers/rotator_optec/indigo_rotator_optec.c`.

### rotator_wa

Custom properties: `X_SET_ZERO_POSITION`.

Driver-specific use of existing properties: `ROTATOR_ABORT_MOTION`, `ROTATOR_BACKLASH`, `ROTATOR_DIRECTION`, `ROTATOR_ON_POSITION_SET`, `ROTATOR_POSITION_OFFSET`, `ROTATOR_RAW_POSITION`, `ROTATOR_RELATIVE_MOVE`.

Source: `indigo_drivers/rotator_wa/indigo_rotator_wa.c`.

### system_ascol

Custom properties: `ASCOL_ABERRATION`, `ASCOL_ABERRATION_NUTATION`, `ASCOL_ALARMS`, `ASCOL_AXIS_CALIBRATED`, `ASCOL_CORRECTION_MODEL`, `ASCOL_COUDE_TUBE`, `ASCOL_DEC_CALIBRATION`, `ASCOL_DOME_POWER`, `ASCOL_DOME_SHUTTER_STATE`, `ASCOL_DOME_STATE`, `ASCOL_ERROR_CORRECTION`, `ASCOL_FLAP_STATE`, `ASCOL_FLAP_TUBE`, `ASCOL_FOCUSER_STATE`, `ASCOL_GLME`, `ASCOL_GUIDE_CORRECTION`, `ASCOL_GUIDE_MODE`, `ASCOL_HADEC_COORDINATES`, `ASCOL_HADEC_RELATIVE_MOVE`, `ASCOL_MOUNT_STATE`, `ASCOL_OIL_POWER`, `ASCOL_OIL_STATE`, `ASCOL_OIMV`, `ASCOL_RADEC_RELATIVE_MOVE`, `ASCOL_RA_CALIBRATION`, `ASCOL_REFRACTION`, `ASCOL_T1_SPEED`, `ASCOL_T2_SPEED`, `ASCOL_T3_SPEED`, `ASCOL_TELESCOPE_POWER`, `ASCOL_USER_SPEED`, `DOME_SLAVING`.

Driver-specific use of existing properties: `DOME_DIMENSION`, `DOME_PARK`, `DOME_SPEED`, `FOCUSER_BACKLASH`, `FOCUSER_COMPENSATION`, `FOCUSER_MODE`, `FOCUSER_SPEED`, `FOCUSER_TEMPERATURE`, `GEOGRAPHIC_COORDINATES`, `GUIDER_GUIDE_DEC`, `GUIDER_GUIDE_RA`, `GUIDER_RATE`, `MOUNT_GUIDE_RATE`, `MOUNT_INFO`, `MOUNT_MOTION_DEC`, `MOUNT_MOTION_RA`, `MOUNT_ON_COORDINATES_SET`, `MOUNT_PARK`, `MOUNT_SET_HOST_TIME`, `MOUNT_SIDE_OF_PIER`, `MOUNT_SLEW_RATE`, `MOUNT_TRACK_RATE`, `UTC_TIME`.

Source: `indigo_drivers/system_ascol/indigo_system_ascol.c`.

### wheel_asi

Custom properties: `X_CALIBRATE`, `X_CUSTOM_SUFFIX`.

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_asi/indigo_wheel_asi.c`.

### wheel_astroasis

Custom properties: `X_BLUETOOTH_PROPERTY`, `X_BLUETOOTH_NAME_PROPERTY`, `X_CALIBRATE`, `X_CUSTOM_SUFFIX`, `X_FACTORY_RESET`.

Driver-specific use of existing properties: `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_astroasis/indigo_wheel_astroasis.c`.

### wheel_atik

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_atik/indigo_wheel_atik.c`.

### wheel_fli

Driver-specific use of existing properties: `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_fli/indigo_wheel_fli.c`.

### wheel_indigo

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_indigo/indigo_wheel_indigo.c`.

### wheel_manual

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_manual/indigo_wheel_manual.c`.

### wheel_mi

Custom properties: `MI_SFW_COMMANDS`.

Driver-specific use of existing properties: `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_mi/indigo_wheel_mi.c`.

### wheel_optec

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_optec/indigo_wheel_optec.c`.

### wheel_playerone

Custom properties: `POA_CUSTOM_SUFFIX`, `POA_RESET`.

Driver-specific use of existing properties: `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_playerone/indigo_wheel_playerone.c`.

### wheel_qhy

Custom properties: `X_MODEL`.

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_qhy/indigo_wheel_qhy.c`.

### wheel_quantum

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_quantum/indigo_wheel_quantum.c`.

### wheel_sx

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_sx/indigo_wheel_sx.c`.

### wheel_trutek

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_trutek/indigo_wheel_trutek.c`.

### wheel_xagyl

Driver-specific use of existing properties: `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`.

Source: `indigo_drivers/wheel_xagyl/indigo_wheel_xagyl.c`.
