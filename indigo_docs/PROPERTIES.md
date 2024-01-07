# INDIGO properties

## Introduction

INDIGO properties and items are abstraction of INDI properties and items. As far as INDIGO uses software bus instead of XML messages,
properties are first of all defined memory structures which are, if needed, mapped to XML or JSON textual representation.

## Common properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| CONNECTION | switch | no | yes | CONNECTED | yes | Item values are undefined if state is not Idle or Ok. |
|  |  |  |  | DISCONNECTED | yes |  |
| INFO | text | yes | yes | DEVICE_NAME | yes | "Device in INDIGO strictly represents device itself and not device driver. Valid DEVICE_INTERFACE values are defined in indigo_driver.h as indigo_device_interface enumeration." |
|  |  |  |  | DEVICE_VERSION | yes |  |
|  |  |  |  | DEVICE_INTERFACE | yes |  |
|  |  |  |  | FRAMEWORK_NAME | no |  |
|  |  |  |  | FRAMEWORK_VERSION | no |  |
|  |  |  |  | DEVICE_MODEL | no |  |
|  |  |  |  | DEVICE_FIRMWARE_REVISION | no |  |
|  |  |  |  | DEVICE_HARDWARE_REVISION | no |  |
|  |  |  |  | DEVICE_SERIAL_NUMBER | no |  |
| SIMULATION | switch | no | no | ENABLED | yes |  |
|  |  |  |  | DISABLED | yes |  |
| CONFIG | switch | no | yes | LOAD | yes |  |
|  |  |  |  | SAVE | yes |  |
|  |  |  |  | REMOVE | yes |  |
| PROFILE_NAME | text | no | yes | NAME_0,... | yes | Set profile name |
| PROFILE | switch | no | yes | PROFILE_0,... | yes | Select the profile number for subsequent CONFIG operation |
| DEVICE_PORT | text | no | no | PORT | no | Either device path like "/dev/tty0" or URL like "lx200://host:port". |
| DEVICE_BAUDRATE | text | no | no | BAUDRATE | no | Serial port configuration in a string like this: 9600-8N1 |
| DEVICE_PORTS | switch | no | no | valid serial port name |  | When selected, it is copied to DEVICE_PORT property. |
| GEOGRAPHIC_COORDINATES | number | no | yes | LATITUDE | yes |  |
|  |  |  |  | LONGITUDE | yes |  |
|  |  |  |  | ELEVATION | yes |  |
|  |  |  |  | ACCURACY | no| GPS driver only |
| UTC_TIME | number |  | no | TIME | yes | It depends on hardware if it is undefined, read-only or read-write. |
|  |  |  |  | OFFSET | yes |  |

Properties are implemented by driver base class in [indigo_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver.c).

## CCD specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| CCD_INFO | number | yes | yes | WIDTH | yes |  |
|  |  |  |  | HEIGHT | yes |  |
|  |  |  |  | MAX_HORIZONTAL_BIN | yes |  |
|  |  |  |  | MAX_VERTICAL_BIN | yes |  |
|  |  |  |  | PIXEL_SIZE | yes | in microns |
|  |  |  |  | PIXEL_WIDTH | yes | in microns |
|  |  |  |  | PIXEL_HEIGHT | yes | in microns |
|  |  |  |  | BITS_PER_PIXEL | yes |  |
| CCD_LENS | number | no | yes | APERTURE | yes | in centimeters |
|  |  |  | FOCAL_LENGTH | yes | in centimeters |
| CCD_UPLOAD_MODE | switch | no | yes | CLIENT | yes |  |
|  |  |  |  | LOCAL | yes |  |
|  |  |  |  | BOTH | yes |  |
| CCD_LOCAL_MODE | text | no | yes | DIR | yes |  |
|  |  |  |  | PREFIX | yes | XXX or XXXX is replaced by sequence or a template with %M (MD5), %E/%nE (exposure), %D/%xD (date), %H/%xH (time), %C (filter name), %nS (sequence) format specifier. |
| CCD_EXPOSURE | number | no | yes | EXPOSURE | yes |  |
| CCD_STREAMING | number | no | no | EXPOSURE | yes | The same as CCD_EXPOSURE, but will upload COUNT images. Use COUNT -1 for endless loop. |
|  |  |  |  | COUNT | yes |  |
| CCD_ABORT_EXPOSURE | switch | no | yes | ABORT_EXPOSURE | yes |  |
| CCD_FRAME | number | no | no | LEFT | yes | If BITS_PER_PIXEL can't be changed, set min and max to the same value. |
|  |  |  |  | TOP | yes |  |
|  |  |  |  | WIDTH | yes |  |
|  |  |  |  | HEIGHT | yes |  |
|  |  |  |  | BITS_PER_PIXEL | yes |  |
| CCD_BIN | number | no | no | HORIZONTAL | yes | CCD_MODE is prefered way how to set binning. |
|  |  |  |  | VERTICAL | yes |  |
| CCD_MODE | switch | no | yes | mode identifier | yes | CCD_MODE is a prefered way how to set binning, resolution, color mode etc. |
| CCD_READ_MODE | switch | no | no | HIGH_SPEED | yes |  |
|  |  |  |  | LOW_NOISE | yes |  |
| CCD_GAIN | number | no | no | GAIN | yes |  |
| CCD_OFFSET | number | no | no | OFFSET | yes |  |
| CCD_GAMMA | number | no | no | GAMMA | yes |  |
| CCD_FRAME_TYPE | switch | no | yes | LIGHT | yes |  |
|  |  |  |  | BIAS | yes |  |
|  |  |  |  | DARK | yes |  |
|  |  |  |  | FLAT | yes |  |
|  |  |  |  | DARKFLAT | yes |  |
| CCD_IMAGE_FORMAT | switch | no | yes | RAW | yes |  |
|  |  |  |  | FITS | yes |  |
|  |  |  |  | XISF | yes |  |
|  |  |  |  | JPEG | yes |  |
|  |  |  |  | JPEG_AVI | yes | JPEG for capture, AVI for streaming |
|  |  |  |  | RAW_SER | yes | RAW for capture, SER for streaming |
| CCD_IMAGE_FILE | text | no | yes | FILE | yes |  |
| CCD_IMAGE | blob | no | yes | IMAGE | yes |  |
| CCD_TEMPERATURE | number |  | no | TEMPERATURE | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_COOLER | switch | no | no | ON | yes |  |
|  |  |  |  | OFF | yes |  |
| CCD_COOLER_POWER | number | yes | no | POWER | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_FITS_HEADERS | text | yes | yes | FITS key name, ... | yes | String in form "value" or "'value'" |
| CCD_SET_FITS_HEADER | text | no | yes | KEYWORD | yes | FITS key name |
|  |  |  |  | VALUE | yes | FITS key value |
| CCD_REMOVE_FITS_HEADER | text | no | yes | KEYWORD | yes | FITS key name |
| CCD_PREVIEW | switch | no | yes | ENABLED | yes | Send JPEG preview to client |
|  |  |  |  | DISABLED | yes | |
| CCD_PREVIEW_IMAGE | blob | no | yes | IMAGE | yes |  |

Properties are implemented by CCD driver base class in [indigo_ccd_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ccd_driver.c).

### DSLR extensions (in addition to CCD specific properties)

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| DSLR_PROGRAM | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
| DSLR_APERTURE | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera/lens |
| DSLR_SHUTTER | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
| DSLR_COMPRESSION | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
| DSLR_WHITE_BALANCE | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
| DSLR_ISO | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
|  DSLR_EXPOSURE_METERING | switch | ... | no |  | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FOCUS_METERING | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FOCUS_MODE | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FOCUS_MODE | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
|  DSLR_CAPTURE_MODE | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FLASH_MODE | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
|  DSLR_EXPOSURE_COMPENSATION | switch |  | no | ... | yes | RO/RW status and items set depends on particular camera |
|  DSLR_BATTERY_LEVEL | switch | yes | no | VALUE | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FOCAL_LENGTH | switch | yes | no | VALUE | yes | RO/RW status and items set depends on particular camera |
|  DSLR_LOCK | switch | no | no | LOCK | yes | Lock camera UI |
|  |  |  |  | UNLOCK | yes |  |
|  DSLR_MIRROR_LOCKUP | switch | no | no | LOCK | yes | Lock camera mirror |
|  |  |  |  | UNLOCK | yes |  |
|  DSLR_AF | switch | no | no | AF | yes | Start autofocus |
|  DSLR_AVOID_AF | switch | no | no | ON | yes | Avoid autofocus |
|  |  |  |  | OF | yes |  |
|  DSLR_STREAMING_MODE | switch | no | no | LIVE_VIEW | yes | Operation used for streaming |
|  |  |  |  | BURST_MODE | yes |  |
|  DSLR_ZOOM_PREVIEW | switch | no | no | ON | yes | LiveView zoom |
|  |  |  |  | OFF | yes |  |
|  DSLR_DELETE_IMAGE | switch | no | no | ON | yes | Delete image from camera memory/card |
|  |  |  |  | OFF | yes |  |

A reference implementation is ICA driver [indigo_ccd_ica.m](https://github.com/indigo-astronomy/indigo/blob/master/indigo_mac_drivers/ccd_ica/indigo_ccd_ica.m).

## Wheel specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| WHEEL_SLOT | number | no | yes | SLOT | yes |  |
| WHEEL_SLOT_NAME | switch | no | yes | SLOT_NAME_1, ... | yes |  |
| WHEEL_SLOT_OFFSET | switch | no | yes | SLOT_OFFSET_1, ... | yes | Value is number of focuser steps |

Properties are implemented by wheel driver base class in [indigo_wheel_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_wheel_driver.c).

## Focuser specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| FOCUSER_SPEED | number | no | no | SPEED | yes |  |
| FOCUSER_REVERSE_MOTION | switch | no | no | ENABLED | yes |  |
|  |  |  |  | DIABLED | yes |  |
| FOCUSER_DIRECTION | switch | no | yes | MOVE_INWARD | yes |  |
|  |  |  |  | MOVE_OUTWARD | yes |  |
| FOCUSER_STEPS | number | no | yes | STEPS | yes |  |
| FOCUSER_ON_POSITION_SET | switch | no | no | GOTO | yes |  |
|  |  |  |  | SYNC | yes |  |
| FOCUSER_POSITION | number |  | no | POSITION | yes | It depends on hardware if it is undefined, read-only or read-write. |
| FOCUSER_ABORT_MOTION | switch | no | yes | ABORT_MOTION | yes |  |
| FOCUSER_TEMPERATURE | number | no | no | TEMPERATURE | yes |  |
| FOCUSER_BACKLASH | number | no | no | BACKLASH | yes | Mechanical backlash compensation |
| FOCUSER_COMPENSATION | number | no | no | COMPENSATION | yes | Temperature compensation (if FOCUSER_MODE.AUTOMATIC is set |
|  |  |  |  | PERIOD | no | Compensation period |
|  |  |  |  | THRESHOLD | no | Compensation threshold |
| FOCUSER_MODE | switch | no | no | MANUAL | yes | Manual mode |
|  |  |  |  | AUTOMATIC | yes | Temperature compensated mode |
| FOCUSER_LIMITS | number | no | no | MIN_POSITION | yes |  |
|  |  |  |  | MAX_POSITION | yes |  |

Properties are implemented by focuser driver base class in [indigo_focuser_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_focuser_driver.c).

## Mount specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| MOUNT_INFO | text | no | no | MODEL | yes |  |
|  |  |  |  | VENDOR | yes |  |
|  |  |  |  | FIRMWARE_VERSION | yes |  |
| MOUNT_LST_TIME | number |  | yes | TIME | yes | It depends on hardware if it is undefined, read-only or read-write. |
| MOUNT_SET_HOST_TIME | switch | no | no | SET | yes |  |
| MOUNT_PARK | switch | no | no | PARKED | yes |  |
|  |  |  |  | UNPARKED | yes |  |
| MOUT_PARK_SET | switch | no | no | DEFAULT | yes |  |
|  |  |  |  | CURRENT | yes |  |
| MOUNT_PARK_POSITION | number | no | no | HA | yes |  |
|  |  |  |  | DEC | yes |  |
| MOUNT_HOME  | switch | no | no | HOME  | yes |  |
| MOUNT_ON_COORDINATES_SET | switch | no | yes | TRACK | yes |  |
|  |  |  |  | SYNC | yes |  |
|  |  |  |  | SLEW | no |  |
| MOUNT_SLEW_RATE | switch | no | no | GUIDE | no |  |
|  |  |  |  | CENTERING | no |  |
|  |  |  |  | FIND | no |  |
|  |  |  |  | MAX | no |  |
| MOUNT_MOTION_DEC | switch | no | yes | NORTH | yes |  |
|  |  |  |  | SOUTH | yes |  |
| MOUNT_MOTION_RA | switch | no | yes | WEST | yes |  |
|  |  |  |  | EAST | yes |  |
| MOUNT_TRACK_RATE | switch | no | no | SIDEREAL | no |  |
|  |  |  |  | SOLAR | no |  |
|  |  |  |  | LUNAR | no |  |
| MOUNT_TRACKING | switch | no | no | ON | yes |  |
|  |  |  |  | OFF | yes |  |
| MOUNT_GUIDE_RATE | nuber | no | no | RA | yes |  |
|  |  |  |  | DEC | yes |  |
| MOUNT_EQUATORIAL_COORDINATES | number | no | yes | RA | yes |  |
|  |  |  |  | DEC | yes |  |
| MOUNT_HORIZONTAL_COORDINATES | number | no | no | ALT | yes |  |
|  |  |  |  | AZ | yes |  |
| MOUNT_ABORT_MOTION | switch | no | yes | ABORT_MOTION | yes |  |
| MOUNT_RAW_COORDINATES | number | yes | yes | RA | yes |  |
|  |  |  |  | DEC | yes |  |
| MOUNT_ALIGNMENT_MODE | switch | no | no | CONTROLLER | yes |  |
|  |  |  |  | SINGLE_POINT | yes |  |
|  |  |  |  | NEAREST_POINT | yes |  |
|  |  |  |  | MULTI_POINT | yes |  |
| MOUNT_ALIGNMENT_SELECT_POINTS | switch | no | no | point id | yes |  |
| MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY | switch | no | no | point id | yes |  |
| MOUNT_ALIGNMENT_RESET | switch | no | no | RESET | yes |  |
| MOUNT_EPOCH | number | no | yes | EPOCH | yes |  |
| MOUNT_SIDE_OF_PIER | switch | no | no | EAST | yes |  |
|  |  |  |  | WEST | yes |  |
| MOUNT_PEC | switch | no | no | ENABLED | yes |  |
|  |  |  |  | DISABLED | yes |  |
| MOUNT_PEC_TRAINING | switch | no | no | STARTED | yes |  |
|  |  |  |  | STOPPED | yes |  |

Properties are implemented by mount driver base class in [indigo_mount_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_mount_driver.c).

## Guider specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| GUIDER_GUIDE_DEC | number | no | yes | NORTH | yes |  |
|  |  |  |  | SOUTH | yes |  |
| GUIDER_GUIDE_RA | number | no | yes | EAST | yes |  |
|  |  |  |  | WEST | yes |  |
| GUIDER_RATE | number | no | no | RATE | yes | % of sidereal rate (RA or both) |
|  | number | no | no | DEC_RATE | no | % of sidereal rate (DEC) |

Properties are implemented by guider driver base class in [indigo_guider_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_guider_driver.c).

## AO specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| AO_GUIDE_DEC | number | no | yes | NORTH | yes | |
|  |  |  |  | SOUTH | yes | |
| AO_GUIDE_RA | number | no | yes | EAST | yes | |
|  |  |  |  | WEST | yes | |
| AO_RESET | switch | no | yes | CENTER | yes | |
|  |  |  |  | UNJAM | no | |


Properties are implemented by AO driver base class in [indigo_ao_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ao_driver.c).

## GPS specific properties

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| GPS_STATUS | light | yes | no | NO_FIX | yes | GPS fix status |
|  |  |  |  | 2D_FIX | yes | |
|  |  |  |  | 3D_FIX | yes | |
| GPS_ADVANCED | switch | no | no | ENABLED | yes |  Enable advanced status report |
|  |  |  |  | DISABLED | yes | |
| GPS_ADVANCED_STATUS | number | yes | no | SVS_IN_USE | yes | Advanced status report |
|  |  |  |  | SVS_IN_VIEW | yes | |
|  |  |  |  | PDOP | yes | |
|  |  |  |  | HDOP | yes | |
|  |  |  |  | VDOP | yes | |

Properties are implemented by GPS driver base class in [indigo_gps_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_gps_driver.c).

## Dome specific properties

| Property name  | Type  | RO | Required | Item name  | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| DOME_SPEED  | number | no | no  | SPEED  | yes  |  |
| DOME_DIRECTION | switch | no | no  | CLOCKWISE  | yes  |  |
|  |  |  |  | COUNTERCLOCKWISE | yes  |  |
| DOME_ON_HORIZONTAL_COORDINATES_SET  | switch | no | no  | GOTO  | yes  |  |
|  |  |  |  | SYNC | yes  |  |
| DOME_STEPS | number | no | no  | STEPS  | yes  |  |
| DOME_EQUATORIAL_COORDINATES | number | no | no  | RA  | yes  |  |
|  |  |  |  | DEC | yes  |  |
| DOME_HORIZONTAL_COORDINATES | number | no | no  | AZ  | yes  |  |
|  |  |  |  | ALT | yes  |  |
| DOME_SLAVING | switch | no | no  | ENABLED  | yes  |  |
|  |  |  |  | DISABLED | yes  |  |
| DOME_SLAVING_PARAMETERS | number | no | no  | MOVE_THRESHOLD  | yes  |  |
| DOME_ABORT_MOTION  | switch | no | no  | ABORT_MOTION  | yes  |  |
| DOME_SHUTTER  | switch | no | no  | OPENED  | yes  |  |
|  |  |  |  | CLOSED | yes  |  |
| DOME_FLAP  | switch | no | no  | OPENED  | yes  |  |
|  |  |  |  | CLOSED | yes  |  |
| DOME_PARK  | switch | no | no  | PARKED  | yes  |  |
|  |  |  |  | UNPARKED | yes  |  |
| DOME_PARK_POSITION  | number | no | no  | AZ  | yes |  |
|  |  |  |  | ALT | no  |  |
| DOME_HOME  | switch | no | no | HOME  | yes |  |
| DOME_DIMENSION | number | no | no  | RADIUS  | yes  |  |
|  |  |  |  | SHUTTER_WIDTH | yes  |  |
|  |  |  |  | MOUNT_PIVOT_OFFSET_NS | yes  |  |
|  |  |  |  | MOUNT_PIVOT_OFFSET_EW | yes  |  |
|  |  |  |  | MOUNT_PIVOT_VERTICAL_OFFSET | yes  |  |
|  |  |  |  | MOUNT_PIVOT_OTA_OFFSET | yes  |  |
|  |  |  |  | MOUNT_PIVOT_VERTICAL_OFFSET | yes  |  |
|  |  |  |  | MOUNT_PIVOT_VERTICAL_OFFSET | yes  |  |
| DOME_SET_HOST_TIME  | switch | no | no  | SET  | yes  |  |

Properties are implemented by dome driver base class in [indigo_dome_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_dome_driver.c)

## Rotator specific properties

| Property name  | Type  | RO | Required | Item name  | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| ROTATOR_ON_POSITION_SET | switch | no  | no  | GOTO  | yes  |  |
|  |  |  |  | SYNC  | yes  |  |
| ROTATOR_POSITION  | number | no  | yes  | POSITION  | yes  |  |
| ROTATOR_DIRECTION  | switch | no  | no  | NORMAL  | yes  |  |
|  |  |  |  | REVERSED  | yes  |  |
| ROTATOR_ABORT_MOTION  | switch | no  | yes  | ABORT_MOTION | yes  |  |
| ROTATOR_BACKLASH  | number | no  | no  | BACKLASH  | yes  |  |
| ROTATOR_LIMITS  | number | no  | no  | MIN_POSITION | yes  |  |
|  |  |  |  | MAX_POSITION | yes  |  |
| ROTATOR_STEPS_PER_REVOLUTION | number | no | no | STEPS_PER_REVOLUTION| yes |  |

Properties are implemented by rotator driver base class in [indigo_rotator_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_rotator_driver.c)

## Auxiliary properties

To be used by auxiliary devices like powerboxes, weather stations, etc.

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| AUX_POWER_OUTLET | switch | no | no | OUTLET_1 | yes | Enable power outlets |
|  |  |  |  | OUTLET_2 | no |  |
|  |  |  |  | OUTLET_3 | no |  |
|  |  |  |  | OUTLET_4 | no |  |
| AUX_POWER_OUTLET_STATE | light | yes | no | OUTLET_1 | yes | Power outlets state (IDLE = unused, OK = used, ALERT = over-current etc.) |
|  |  |  |  | OUTLET_2 | no |  |
|  |  |  |  | OUTLET_3 | no |  |
|  |  |  |  | OUTLET_4 | no |  |
| AUX_POWER_OUTLET_CURRENT | number | yes | no | OUTLET_1 | yes | Power outlets current |
|  |  |  | no | OUTLET_2 | no |  |
|  |  |  | no | OUTLET_3 | no |  |
|  |  |  | no | OUTLET_4 | no |  |
| AUX_HEATER_OUTLET | number | no | no | OUTLET_1 | yes | Set heater outlets power |
|  |  |  |  | OUTLET_2 | no |  |
|  |  |  |  | OUTLET_3 | no |  |
|  |  |  |  | OUTLET_4 | no |  |
| AUX_HEATER_OUTLET_STATE | light | yes | no | OUTLET_1 | yes | Heater outlets state (IDLE = unused, OK = used, ALERT = over-current etc.) |
|  |  |  |  | OUTLET_2 | no |  |
|  |  |  |  | OUTLET_3 | no |  |
|  |  |  |  | OUTLET_4 | no |  |
| AUX_HEATER_OUTLET_CURRENT | number | yes | no | OUTLET_1 | yes | Heater outlets current |
|  |  |  |  | OUTLET_2 | no |  |
|  |  |  |  | OUTLET_3 | no |  |
|  |  |  |  | OUTLET_4 | no |  |
| AUX_USB_PORT | switch | no | no | PORT_1 | yes | Enable USB ports on smart hub |
|  |  |  |  | PORT_2 | no |  |
|  |  |  |  | PORT_3 | no |  |
|  |  |  |  | PORT_4 | no |  |
|  |  |  |  | PORT_5 | no |  |
|  |  |  |  | PORT_6 | no |  |
|  |  |  |  | PORT_7 | no |  |
|  |  |  |  | PORT_8 | no |  |
| AUX_USB_PORT_STATE | light | yes | no | PORT_1 | yes | USB port state (IDLE = unused or disabled, OK = used, BUSY = transient state, ALERT = over-current etc.) |
|  |  |  |  | PORT_2 | no |  |
|  |  |  |  | PORT_3 | no |  |
|  |  |  |  | PORT_4 | no |  |
|  |  |  |  | PORT_5 | no |  |
|  |  |  |  | PORT_6 | no |  |
|  |  |  |  | PORT_7 | no |  |
|  |  |  |  | PORT_8 | no |  |
| AUX_DEW_CONTROL | switch | no | no | MANUAL | yes | Use AUX_HEATER_OUTLET values |
|  |  |  |  | AUTOMATIC | yes | Set power automatically |
| AUX_WEATHER | number | yes | no | TEMPERATURE | no |  |
|  |  |  |  | HUMIDITY | no |  |
|  |  |  |  | DEWPOINT | no |  |
| AUX_INFO | number | yes | no | ... | no | Any number of any number items |
| AUX_CONTROL | switch | no | no | ... | no | Any number of any switch items |
| AUX_LIGHT_SWITCH | switch | no | no | ON | yes | Flatbox light on |
|  |  |  |  | OFF | yes | Turn light off |
| AUX_LIGHT_INTENSITY | number | no | no | LIGHT_INTENSITY | yes | Flatbox light intensity |

## Agent specific properties

### Snoop agent (obsoleted by various filter agents)

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| SNOOP_ADD_RULE | text | no | yes | SOURCE_DEVICE | yes | Add new rule |
|  |  |  |  | SOURCE_PROPERTY | yes | |
|  |  |  |  | TARGET_DEVICE | yes | |
|  |  |  |  | TARGET_PROPERTY | yes | |
| SNOOP_REMOVE_RULE | text | no | yes | SOURCE_DEVICE | yes | Remove existing rule |
|  |  |  |  | SOURCE_PROPERTY | yes | |
|  |  |  |  | TARGET_DEVICE | yes | |
|  |  |  |  | TARGET_PROPERTY | yes | |
| SNOOP_RULES | light | yes | yes | ... | yes | Lists all rules |

### LX200 server agent (obsoleted by Mount Agent)

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| LX200_DEVICES | text | no | yes | MOUNT | yes | Select snooped mount |
|  |  |  |  | GUIDER | yes | Select snooped guider (not used yet) |
| LX200_CONFIGURATION | number | no | yes | PORT | yes | Server port number |
| LX200_SERVER | switch | no | yes | STARTED | yes | Select server state |
|  |  |  |  | STOPPED | yes | |

### Imager agent

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| FILTER_CCD_LIST | switch | no | yes | ... | yes | Select CCD |
| FILTER_WHEEL_LIST | switch | no | yes | ... | yes | Select wheel |
| FILTER_FOCUSER_LIST | switch | no | yes | ... | yes | Select focuser |
| FILTER_ROTATOR_LIST | switch | no | yes | ... | yes | Select rotator |
| FILTER_RELATED_AGENT_LIST | switch | no | yes | ... | yes | Select related agents |
| FILTER_FORCE_SYMMETRIC_RELATIONS | switch | no | yes | ENABLED | yes | Set symmetric relation |
|  |  |  |  | DISABLED | yes | |
| AGENT_START_PROCESS | switch | no | yes | EXPOSURE | yes | Start exposure |
|  |  |  |  | STREAMING | yes | Start streaming |
| AGENT_PAUSE_PROCESS | switch | no | yes | PAUSE | yes | Pause batch immediately (abort running capture) or resume |
|  |  |  |  | PAUSE_WAIT | yes | Pause batch after running capture or resume |
|  |  |  |  | PAUSE_BEFORE_TRANSIT | yes | Resume batch paused before configured transit time (e.g. after meridian flip) |
| AGENT_ABORT_PROCESS | switch | no | yes | ABORT | yes | Abort running process |
| AGENT_IMAGER_BATCH | number | no | yes | COUNT | yes | Frame count |
|  |  |  |  | EXPOSURE | yes | Exposure duration (in seconds) |
|  |  |  |  | DELAY | yes | Delay between exposures duration (in seconds) |
|  |  |  |  | PAUSE_BEFORE_TRANSIT | yes | Pause batch when transit time reached; e.g. before meridian flip; use 24:00:00 to turn it off  |
| AGENT_IMAGER_DOWNLOADFILE | text | no | yes | FILE | yes | Files to load into AGENT_IMAGER_DOWNLOAD_IMAGE property and remove on the host |
| AGENT_IMAGER_DOWNLOADFILES | switch | no | yes | REFRESH | yes | Refresh the list of available files |
|  |  |  |  | file name | yes | Set the file to AGENT_IMAGER_DOWNLOADFILE |
| AGENT_IMAGER_DOWNLOAD_IMAGE | blob | no | yes | IMAGE | yes |  |
| AGENT_IMAGER_SEQUENCE | text | no | yes | SEQUENCE | yes | Sequence control string |
|  |  |  |  | 01...16 | yes | Batch control string |
| AGENT_IMAGER_SEQUENCE_STATE | light | no | yes | SEQUENCE | yes | Sequence state |
|  |  |  |  | 01...16 | yes | Batch state |

### Guider agent

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| FILTER_CCD_LIST | switch | no | yes | ... | yes | Select CCD |
| FILTER_GUIDER_LIST | switch | no | yes | ... | yes | Select guider |
| FILTER_RELATED_AGENT_LIST | switch | no | yes | ... | yes | Select related agents |
| FILTER_FORCE_SYMMETRIC_RELATIONS | switch | no | yes | ENABLED | yes | Set symmetric relation |
|  |  |  |  | DISABLED | yes | |
| AGENT_IMAGER_BATCH | switch | no | yes | PREVIEW | yes | Start preview |
|  |  |  |  | CALIBRATION | yes | Start calibration |
|  |  |  |  | GUIDING | yes | Start guiding |
| AGENT_ABORT_PROCESS | switch | no | yes | ABORT | yes | Abort running process |
| AGENT_GUIDER_DETECTION_MODE | switch | no | yes | DONUTS | yes | Use DONUTS algorithm |
|  |  |  |  | CENTROID | yes | Use full frame centroid algorithm |
|  |  |  |  | SELECTION | yes | Use selected star centroid algorithm |
| AGENT_GUIDER_DEC_MODE | switch | no | yes | BOTH | yes | Guide both north and south |
|  |  |  |  | NORTH | yes | Guide north only |
|  |  |  |  | SOUTH | yes | Guide south only |
|  |  |  |  | NONE | yes | Don't guide in declination axis |
| AGENT_GUIDER_SELECTION | switch | no | yes | X | yes | Selected star coordinates (pixels) |
|  |  |  |  | Y | yes | Guide north only |
| AGENT_GUIDER_SETTINGS | number | no | yes | EXPOSURE | yes | Exposure duration (in seconds) |
|  |  |  |  | STEP0 | yes | Initial step size (in pixels) |
|  |  |  |  | ANGLE | yes | Measured angle (in degrees) |
|  |  |  |  | BACKLASH | yes | Measured backlash (in pixels) |
|  |  |  |  | SPEED_RA | yes | Measured RA speed (in pixels/second) |
|  |  |  |  | SPEED_DEC | yes | Measured dec speed (in pixels/seconds) |
|  |  |  |  | MAX_BL_STEPS | yes | Max backlash clearing steps |
|  |  |  |  | MIN_BL_DRIFT | yes | Min required backlash drift (in pixels) |
|  |  |  |  | MAX_CALIBRATION_STEPS | yes | Max calibration steps |
|  |  |  |  | AGGRESSIVITY_RA | yes | RA aggressivity (in %) |
|  |  |  |  | AGGRESSIVITY_DEC | yes | Dec aggressivity (in %) |
|  |  |  |  | MIN_ERROR | yes | Min error to coorect (in pixels) |
|  |  |  |  | MIN_PULSE | yes | Min pulse length to emit (in seconds) |
|  |  |  |  | MAX_PULSE | yes | Max pulse length to emit (in seconds) |
|  |  |  |  | DITHERING_X | yes | Dithering offset (in pixels) |
|  |  |  |  | DITHERING_Y | yes |  |
| AGENT_GUIDER_STATS | number | yes | yes | PHASE | yes | Process phase |
|  |  |  |  | FRAME | yes | Frame number |
|  |  |  |  | DRIFT_X | yes | Measured drift (X/Y) |
|  |  |  |  | DRIFT_Y | yes |  |
|  |  |  |  | DRIFT_RA | yes | Measured drift (RA/dec) |
|  |  |  |  | DRIFT_DEC | yes |  |
|  |  |  |  | CORR_RA | yes | Correction (RA/dec) |
|  |  |  |  | CORR_DEC | yes | |
|  |  |  |  | RMSE_RA | yes | Root Mean Square Error (RA/dec) |
|  |  |  |  | RMSE_DEC | yes | |

### Mount agent

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| FILTER_DOME_LIST | switch | no | yes | ... | yes | Select dome |
| FILTER_MOUNT_LIST | switch | no | yes | ... | yes | Select mount |
| FILTER_GPS_LIST | switch | no | yes | ... | yes | Select GPS |
| FILTER_RELATED_AGENT_LIST | switch | no | yes | ... | yes | Select related agents |
| FILTER_FORCE_SYMMETRIC_RELATIONS | switch | no | yes | ENABLED | yes | Set symmetric relation |
|  |  |  |  | DISABLED | yes | |
| GEOGRAPHIC_COORDINATES | number | no | yes | LATITUDE | yes | Observatory coordinates |
|  |  |  |  | LONGITUDE | yes | |
|  |  |  |  | ELEVATION | yes | |
| AGENT_SITE_DATA_SOURCE | switch | no | yes | HOST | yes | Use host coordinates |
|  |  |  |  | MOUNT | yes | Use mount controller coordinates |
|  |  |  |  | DOME | yes | Use dome controller coordinates |
|  |  |  |  | GPS | yes | Use GPS coordinates |
| AGENT_SET_HOST_TIME | switch | no | yes | MOUNT | yes | Set host time to device |
|  |  |  |  | DOME | yes | |
| AGENT_LIMITS | number | no | yes | HA_TRACKING | yes | HA limit for tracking; park when reached; use 24:00:00 to turn it off |
|  |  |  |  | LOCAL_TIME | yes | Time limit for tracking; park when reached; use 12:00:00 to turn it off  |
|  |  |  |  | COORDINATES_PROPAGATE_THESHOLD | yes | The minimal difference in geographic coordinates that trigger change propagation  |
