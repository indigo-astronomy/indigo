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
| CCD_UPLOAD_MODE | switch | no | yes | CLIENT | yes | Upload to client |
|  |  |  |  | LOCAL | yes | Save on server |
|  |  |  |  | BOTH | yes | Upload and save |
|  |  |  |  | NONE | no | Hidden by default. |
| CCD_LOCAL_MODE | text | no | yes | DIR | yes | Directory |
|  |  |  |  | PREFIX | yes | XXX or XXXX is replaced by sequence or a template with %M (MD5), %E/%nE (exposure), %D/%xD (date), %H/%xH (time), %C (filter name), %nS (sequence), %F (frame type), %T (chip temperature), %G (gain), %O (offset), %R (resolution), %B (binning), %P (focuser position) format specifier. |
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
| DSLR_FLASH_MODE | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
| DSLR_EXPOSURE_COMPENSATION | switch |  | no | ... | yes | RO/RW status and items depend on the particular camera |
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

A reference implementation is ICA driver [indigo_ccd_ica.m](https://github.com/indigo-astronomy/indigo/blob/master/indigo_mac_drivers/ccd_ica/indigo_ccd_ica.m).

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
| DOME_EQUATORIAL_COORDINATES | number | no | no | RA | yes | Right ascension (0 to 24 hrs) |
|  |  |  |  | DEC | yes | Declination (-90 to 90°) |
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

## Agent specific properties

### Common agent properties

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

### Imager agent

The imager agent exposes the following common properties: FILTER_CCD_LIST, FILTER_WHEEL_LIST, FILTER_FOCUSER_LIST, FILTER_ROTATOR_LIST, FILTER_AUX_1_LIST, FILTER_RELATED_AGENT_LIST, FILTER_FORCE_SYMMETRIC_RELATIONS, CCD_LENS_FOV.

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| AGENT_START_PROCESS | switch | no | yes | PREVIEW_1 | yes | Start single-frame preview |
|  |  |  |  | PREVIEW | yes | Start preview |
|  |  |  |  | EXPOSURE | yes | Start exposure batch |
|  |  |  |  | STREAMING | yes | Start streaming batch |
|  |  |  |  | FOCUSING | yes | Start autofocus |
|  |  |  |  | CLEAR_SELECTION | yes | Clear star selection |
|  |  |  |  | RESET | yes | Reset to defaults |
| AGENT_PAUSE_PROCESS | switch | no | yes | PAUSE | yes | Pause batch immediately (abort running capture) or resume |
|  |  |  |  | PAUSE_WAIT | yes | Pause batch after running capture or resume |
|  |  |  |  | PAUSE_AFTER_TRANSIT | yes | Resume batch paused at configured transit time (e.g. after meridian flip) |
| AGENT_ABORT_PROCESS | switch | no | yes | ABORT | yes | Abort running process |
| AGENT_PROCESS_FEATURES | switch | no | yes | ENABLE_DITHERING | yes | Enable dithering |
|  |  |  |  | DITHER_AFTER_LAST_FRAME | yes | Dither after last frame in batch |
|  |  |  |  | PAUSE_AFTER_TRANSIT | yes | Pause at configured transit time |
|  |  |  |  | APPLY_FILTER_OFFSETS | yes | Apply focus offsets for filters |
|  |  |  |  | MACRO_MODE | yes | Use macro mode |
| AGENT_IMAGER_BATCH | number | no | yes | COUNT | yes | Frame count |
|  |  |  |  | EXPOSURE | yes | Exposure duration (in seconds) |
|  |  |  |  | DELAY | yes | Delay between exposures (in seconds) |
|  |  |  |  | FRAMES_TO_SKIP_BEFORE_DITHER | yes | Frames to skip before each dither |
|  |  |  |  | PAUSE_AFTER_TRANSIT | yes | Pause batch at transit time (e.g. before meridian flip); use 24:00:00 to turn off |
| AGENT_IMAGER_DOWNLOAD_FILE | text | no | yes | FILE | yes | File to load into AGENT_IMAGER_DOWNLOAD_IMAGE and remove from host |
| AGENT_IMAGER_DOWNLOAD_FILES | switch | no | yes | REFRESH | yes | Refresh the list of available files |
|  |  |  |  | file name | yes | Set file to AGENT_IMAGER_DOWNLOAD_FILE |
| AGENT_IMAGER_DOWNLOAD_IMAGE | blob | no | yes | IMAGE | yes | Downloaded image data |

Imager agent properties are defined in `indigo_drivers/agent_imager/indigo_agent_imager.c`.

### Guider agent

The guider agent exposes the following common properties: FILTER_CCD_LIST, FILTER_GUIDER_LIST, FILTER_RELATED_AGENT_LIST, FILTER_FORCE_SYMMETRIC_RELATIONS.

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| AGENT_START_PROCESS | switch | no | yes | PREVIEW | yes | Start preview |
|  |  |  |  | CALIBRATION | yes | Start calibration |
|  |  |  |  | GUIDING | yes | Start guiding |
| AGENT_ABORT_PROCESS | switch | no | yes | ABORT | yes | Abort running process |
| AGENT_PROCESS_FEATURES | switch | no | yes | ENABLE_LOGGING | yes | Make guiding log |
| AGENT_GUIDER_LOG | text | no | yes | DIR | yes | Guiding log folder |
|  |  |  |  | TEMPLATE | yes | File name template, strftime() format specifiers accepted |
| AGENT_GUIDER_DETECTION_MODE | switch | no | yes | DONUTS | yes | Use DONUTS algorithm |
|  |  |  |  | CENTROID | yes | Use full frame centroid algorithm |
|  |  |  |  | SELECTION | yes | Use selected star centroid algorithm |
| AGENT_GUIDER_DEC_MODE | switch | no | yes | BOTH | yes | Guide both north and south |
|  |  |  |  | NORTH | yes | Guide north only |
|  |  |  |  | SOUTH | yes | Guide south only |
|  |  |  |  | NONE | yes | Don't guide in declination axis |
| AGENT_GUIDER_SELECTION | number | no | yes | X | yes | Selected star X coordinate (pixels) |
|  |  |  |  | Y | yes | Selected star Y coordinate (pixels) |
| AGENT_GUIDER_SETTINGS | number | no | yes | EXPOSURE | yes | Exposure duration (in seconds) |
|  |  |  |  | STEP0 | yes | Initial step size (in pixels) |
|  |  |  |  | ANGLE | yes | Measured angle (in degrees) |
|  |  |  |  | BACKLASH | yes | Measured backlash (in pixels) |
|  |  |  |  | SPEED_RA | yes | Measured RA speed (in pixels/second) |
|  |  |  |  | SPEED_DEC | yes | Measured dec speed (in pixels/second) |
|  |  |  |  | MAX_BL_STEPS | yes | Max backlash clearing steps |
|  |  |  |  | MIN_BL_DRIFT | yes | Min required backlash drift (in pixels) |
|  |  |  |  | MAX_CALIBRATION_STEPS | yes | Max calibration steps |
|  |  |  |  | AGGRESSIVITY_RA | yes | RA aggressivity (in %) |
|  |  |  |  | AGGRESSIVITY_DEC | yes | Dec aggressivity (in %) |
|  |  |  |  | MIN_ERROR | yes | Min error to correct (in pixels) |
|  |  |  |  | MIN_PULSE | yes | Min pulse length to emit (in seconds) |
|  |  |  |  | MAX_PULSE | yes | Max pulse length to emit (in seconds) |
|  |  |  |  | DITHERING_X | yes | Dithering offset (in pixels) |
|  |  |  |  | DITHERING_Y | yes | Dithering offset Y (px) |
| AGENT_GUIDER_STATS | number | yes | yes | PHASE | yes | Process phase |
|  |  |  |  | FRAME | yes | Frame number |
|  |  |  |  | DRIFT_X | yes | Measured drift (X/Y) |
|  |  |  |  | DRIFT_Y | yes | Drift Y (px) |
|  |  |  |  | DRIFT_RA | yes | Measured drift (RA/dec) |
|  |  |  |  | DRIFT_DEC | yes | Drift Dec (px) |
|  |  |  |  | CORR_RA | yes | Correction (RA/dec) |
|  |  |  |  | CORR_DEC | yes | Correction Dec (s) |
|  |  |  |  | RMSE_RA | yes | Root Mean Square Error (RA/dec) |
|  |  |  |  | RMSE_DEC | yes | RMSE Dec (px) |

Guider agent properties are defined in `indigo_drivers/agent_guider/indigo_agent_guider.c`.

### Mount agent

The mount agent exposes the following common properties: FILTER_MOUNT_LIST, FILTER_DOME_LIST, FILTER_GPS_LIST, FILTER_JOYSTICK_LIST, FILTER_RELATED_AGENT_LIST, FILTER_FORCE_SYMMETRIC_RELATIONS.

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| GEOGRAPHIC_COORDINATES | number | no | yes | LATITUDE | yes | Observatory coordinates |
|  |  |  |  | LONGITUDE | yes | Longitude (0 to 360° +E) |
|  |  |  |  | ELEVATION | yes | Elevation (m) |
| AGENT_SITE_DATA_SOURCE | switch | no | yes | HOST | yes | Use agent coordinates |
|  |  |  |  | MOUNT | yes | Use mount controller coordinates |
|  |  |  |  | DOME | yes | Use dome controller coordinates |
|  |  |  |  | GPS | yes | Use GPS coordinates |
| AGENT_SET_HOST_TIME | switch | no | yes | MOUNT | yes | Set host time to mount |
|  |  |  |  | DOME | yes | Set host time to dome |
| ABORT_RELATED_PROCESS | switch | no | yes | IMAGER | yes | Allow aborting imager agent process |
|  |  |  |  | GUIDER | yes | Allow aborting guider agent process |
| AGENT_LX200_SERVER | switch | no | yes | STARTED | yes | LX200 server running |
|  |  |  |  | STOPPED | yes | LX200 server stopped |
| AGENT_LX200_CONFIGURATION | number | no | yes | PORT | yes | LX200 server port |
|  |  |  |  | EPOCH | yes | Epoch (0 = JNow, 2000 = J2000) |
| AGENT_LIMITS | number | no | yes | HA_TRACKING | yes | HA limit for tracking; park when reached; use 24:00:00 to turn off |
|  |  |  |  | LOCAL_TIME | yes | Time limit for tracking; park when reached; use 12:00:00 to turn off |
|  |  |  |  | COORDINATES_PROPAGATE_THRESHOLD | yes | Min geographic coordinate difference that triggers propagation |
| AGENT_MOUNT_FOV | number | no | yes | ANGLE | yes | FOV rotation angle (°) |
|  |  |  |  | WIDTH | yes | FOV width (°) |
|  |  |  |  | HEIGHT | yes | FOV height (°) |
| AGENT_MOUNT_EQUATORIAL_COORDINATES | number | no | yes | RA | yes | Target right ascension (0 to 24 hrs) |
|  |  |  |  | DEC | yes | Target declination (−90° to +90°) |
| AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY | number | yes | yes | RA_JNOW | yes | Right ascension JNow |
|  |  |  |  | DEC_JNOW | yes | Declination JNow |
|  |  |  |  | ALT | yes | Altitude (°) |
|  |  |  |  | AZ | yes | Azimuth (°) |
|  |  |  |  | AIRMASS | yes | Airmass |
|  |  |  |  | HA | yes | Hour angle |
|  |  |  |  | RISE | yes | Rise time |
|  |  |  |  | TRANSIT | yes | Transit time |
|  |  |  |  | SET | yes | Set time |
|  |  |  |  | TIME_TO_TRANSIT | yes | Time to transit |
|  |  |  |  | FLIP_REQUIRED | yes | Flip required (0 or 1) |
|  |  |  |  | PARALLACTIC_ANGLE | yes | Parallactic angle (°) |
|  |  |  |  | DEROTATION_RATE | yes | Derotation rate ("/s) |
| AGENT_START_PROCESS | switch | no | yes | SLEW | yes | Slew mount to target |
|  |  |  |  | SYNC | yes | Sync mount to target |
|  |  |  |  | PARK | yes | Park mount |
|  |  |  |  | UNPARK | yes | Unpark mount |
|  |  |  |  | HOME | yes | Go to home position |
|  |  |  |  | TRACK_ON | yes | Start tracking |
|  |  |  |  | TRACK_OFF | yes | Stop tracking |
|  |  |  |  | DOME_SLEW | yes | Slew dome |
|  |  |  |  | DOME_SYNC | yes | Sync dome |
|  |  |  |  | DOME_PARK | yes | Park dome |
|  |  |  |  | DOME_UNPARK | yes | Unpark dome |
|  |  |  |  | DOME_OPEN | yes | Open dome shutter |
|  |  |  |  | DOME_CLOSE | yes | Close dome shutter |
|  |  |  |  | RESET | yes | Reset to defaults |
| AGENT_ABORT_PROCESS | switch | no | yes | ABORT | yes | Abort running process |
| AGENT_PROCESS_FEATURES | switch | no | yes | ENABLE_HA_LIMIT | yes | Enable HA limit |
|  |  |  |  | ENABLE_TIME_LIMIT | yes | Enable time limit |
|  |  |  |  | ENABLE_DOME_SLAVING | yes | Enable dome slaving |
|  |  |  |  | MAKE_DOME_SLAVING_PERSISTENT | yes | Make ENABLE_DOME_SLAVING persistent |
|  |  |  |  | ENABLE_FIELD_DEROTATION | yes | Enable field derotation |
|  |  |  |  | MAKE_FIELD_DEROTATION_PERSISTENT | yes | Make ENABLE_FIELD_DEROTATION persistent |
|  |  |  |  | ENABLE_JOYSTICK_CONTROL | yes | Enable joystick control |
| AGENT_MOUNT_STATE | light | yes | yes | SLEW | yes | Mount slew state |
|  |  |  |  | PARK | yes | Mount park state |
|  |  |  |  | HOME | yes | Mount home state |
|  |  |  |  | TRACK | yes | Mount tracking state |
| AGENT_DOME_STATE | light | yes | yes | SLEW | yes | Dome slew state |
|  |  |  |  | PARK | yes | Dome park state |
|  |  |  |  | OPEN | yes | Dome open state |
| AGENT_MOUNT_FEATURE | switch | yes | yes | SLEW | yes | Mount supports slewing |
|  |  |  |  | SYNC | yes | Mount supports sync |
|  |  |  |  | PARK | yes | Mount supports park |
|  |  |  |  | HOME | yes | Mount supports home |
|  |  |  |  | TRACK | yes | Mount supports tracking |
| AGENT_DOME_FEATURE | switch | yes | yes | SLEW | yes | Dome supports slewing |
|  |  |  |  | SYNC | yes | Dome supports sync |
|  |  |  |  | PARK | yes | Dome supports park |
|  |  |  |  | OPEN | yes | Dome supports open/close |

Mount agent properties are defined in `indigo_drivers/agent_mount/indigo_agent_mount.c`.
