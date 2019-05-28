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
| PROFILE | switch | no | yes | PROFILE_0,... | yes | Select the profile number for subsequent CONFIG operation |
| DEVICE_PORT | text | no | no | PORT | no | Either device path like "/dev/tty0" or URL like "lx200://host:port". |
| DEVICE_BAUDRATE | text | no | no | BAUDRATE | no | Serial port configuration in a string like this: 9600-8N1 |
| DEVICE_PORTS | switch | no | no | valid serial port name |  | When selected, it is copied to DEVICE_PORT property. |
| GEOGRAPHIC_COORDINATES | number | no | yes | LATITUDE | yes |  |
|  |  |  |  | LONGITUDE | yes |  |
|  |  |  |  | ELEVATION | yes |  |
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
|  |  |  |  | PIXEL_SIZE | yes |  |
|  |  |  |  | PIXEL_WIDTH | yes |  |
|  |  |  |  | PIXEL_HEIGHT | yes |  |
|  |  |  |  | BITS_PER_PIXEL | yes |  |
| CCD_UPLOAD_MODE | switch | no | yes | CLIENT | yes |  |
|  |  |  |  | LOCAL | yes |  |
|  |  |  |  | BOTH | yes |  |
|  |  |  |  | PREVIEW | yes | Send JPEG preview to client |
|  |  |  |  | PREVIEW_LOCAL | yes | Send JPEG preview to client and save original format locally |
| CCD_LOCAL_MODE | text | no | yes | DIR | yes | XXX is replaced by sequence. |
|  |  |  |  | PREFIX | yes |  |
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
| CCD_IMAGE_FORMAT | switch | no | yes | RAW | yes |  |
|  |  |  |  | FITS | yes |  |
|  |  |  |  | XISF | yes |  |
|  |  |  |  | JPEG | yes |  |
| CCD_IMAGE_FILE | text | no | yes | FILE | yes |  |
| CCD_IMAGE | blob | no | yes | IMAGE | yes |  |
| CCD_TEMPERATURE | number |  | no | TEMPERATURE | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_COOLER | switch | no | no | ON | yes |  |
|  |  |  |  | OFF | yes |  |
| CCD_COOLER_POWER | number | yes | no | POWER | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_FITS_HEADERS | text | no | yes | HEADER_1, ... | yes | String in form "name = value", "name = 'value'" or "comment text" |

Properties are implemented by CCD driver base class in [indigo_ccd_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ccd_driver.c).

### DSLR extensions (in addition to CCD specific properties)

| Property name | Type | RO | Required | Item name | Required | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| DSLR_PROGRAM | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
| DSLR_APERTURE | switch |  | no |  | yes | RO/RW status and items set depends on particular camera/lens |
| DSLR_SHUTTER | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
| DSLR_COMPRESSION | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
| DSLR_WHITE_BALANCE | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
| DSLR_ISO | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
|  DSLR_EXPOSURE_METERING | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FOCUS_METERING | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FOCUS_MODE | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FOCUS_MODE | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
|  DSLR_CAPTURE_MODE | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FLASH_MODE | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
|  DSLR_EXPOSURE_COMPENSATION | switch |  | no |  | yes | RO/RW status and items set depends on particular camera |
|  DSLR_BATTERY_LEVEL | switch | yes | no | VALUE | yes | RO/RW status and items set depends on particular camera |
|  DSLR_FOCAL_LENGTH | switch | yes | no | VALUE | yes | RO/RW status and items set depends on particular camera |
|  DSLR_LOCK | switch | no | no | LOCK | yes | Lock camera UI |
|   |  |  |  | UNLOCK | yes |  |
|  DSLR_MIRROR_LOCKUP | switch | no | no | LOCK | yes | Lock camera mirror |
|   |  |  |  | UNLOCK | yes |  |
|  DSLR_AF | switch | no | no | AF | yes | Start autofocus |
|  DSLR_AVOID_AF | switch | no | no | ON | yes | Avoid autofocus |
|   |  |  |  | OF | yes |  |
|  DSLR_STREAMING_MODE | switch | no | no | LIVE_VIEW | yes | Operation used for streaming |
|   |  |  |  | BURST_MODE | yes |  |
|  DSLR_ZOOM_PREVIEW | switch | no | no | ON | yes | LiveView zoom |
|   |  |  |  | OFF | yes |  |
|  DSLR_DELETE_IMAGE | switch | no | no | ON | yes | Delete image from camera memory/card |
|   |  |  |  | OFF | yes |  |

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
| MOUNT_PARK_POSITION | number | no | no | RA | yes |  |
|  |  |  |  | DEC | yes |  |
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
| MOUNT_ALIGNMENT_MODE | switch | no | yes | CONTROLLER | yes |  |
|  |  |  |  | SINGLE_POINT | yes |  |
|  |  |  |  | NEAREST_POINT | yes |  |
|  |  |  |  | MULTI_POINT | yes |  |
| MOUNT_ALIGNMENT_SELECT_POINTS | switch | no | yes | point id | yes |  |
| MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY | switch | no | yes | point id | yes |  |
| MOUNT_EPOCH | number | no | yes | EPOCH | yes |  |
| MOUNT_SIDE_OF_PIER | switch | no | yes | EAST | yes |  |
|  |  |  |  | WEST | yes |  |

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
