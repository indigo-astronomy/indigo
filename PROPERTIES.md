# INDIGO protocols

## Introduction

INDIGO properties and items are abstraction of INDI properties and items. As far as INDIGO uses software bus instead of XML messages,
properties are first of all defined memory structures wich are, if needed, mapped to XML or JSON textual representation.

## Common properties

| Property |  |  |  | Items |  | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| Name | Type | RO | Required | Name | Required |  |
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
| CONFIG | switch | no | yes | LOAD | yes | DEFAULT is not implemented by INDIGO yet. |
|  |  |  |  | SAVE | yes |  |
|  |  |  |  | DEFAULT | yes |  |
| PROFILE | switch | no | yes | PROFILE_0,... | yes | Select the profile number for subsequent CONFIG operation |
| DEVICE_PORT | text | no | no | PORT | no | Either device path like "/dev/tty0" or url like "lx200://host:port". |
| DEVICE_PORTS | switch | no | no | valid serial port name |  | When selected, it is copied to DEVICE_PORT property. |

Properties are implemented by driver base class in [indigo_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver.c).

## CCD specific properties

| Property |  |  |  | Items |  | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| Name | Type | RO | Required | Name | Required |  |
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
|  |  |  |  | JPEG | yes |  |
| CCD_IMAGE_FILE | text | no | yes | FILE | yes |  |
| CCD_TEMPERATURE | number |  | no | TEMPERATURE | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_COOLER | switch | no | no | ON | yes |  |
|  |  |  |  | OFF | yes |  |
| CCD_COOLER_POWER | number | yes | no | POWER | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_FITS_HEADERS | text | no | yes | HEADER_1, ... | yes | String in form "name = value", "name = 'value'" or "comment text" |

Properties are implemented by CCD driver base class in [indigo_ccd_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ccd_driver.c).

## Wheel specific properties

| Property |  |  |  | Items |  | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| Name | Type | RO | Required | Name | Required |  |
| WHEEL_SLOT | number | no | yes | SLOT | yes |  |
| WHEEL_SLOT_NAME | switch | no | yes | SLOT_NAME_1, ... | yes |  |

Properties are implemented by wheel driver base class in [indigo_wheel_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_wheel_driver.c).

## Focuser specific properties

| Property |  |  |  | Items |  | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| Name | Type | RO | Required | Name | Required |  |
| FOCUSER_SPEED | number | no | no | SPEED | yes |  |
| FOCUSER_DIRECTION | switch | no | yes | MOVE_INWARD | yes |  |
|  |  |  |  | MOVE_OUTWARD | yes |  |
| FOCUSER_STEPS | number | no | yes | STEPS | yes |  |
| FOCUSER_POSITION | number |  | no | POSITION | yes | It depends on hardware if it is undefined, read-only or read-write. |
| FOCUSER_ABORT_MOTION | switch | no | yes | ABORT_MOTION | yes |  |
| FOCUSER_TEMPERATURE | number | no | no | TEMPERATURE | yes |  |
| FOCUSER_COMPENSATION | number | no | no | COMPENSATION | yes |  |

Properties are implemented by focuser driver base class in [indigo_focuser_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_focuser_driver.c).

## Mount specific properties

| Property |  |  |  | Items |  | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| Name | Type | RO | Required | Name | Required |  |
| MOUNT_INFO | text | no | no | MODEL | yes |  |
|  |  |  |  | VENDOR | yes |  |
|  |  |  |  | FIRMWARE_VERSION | yes |  |
| MOUNT_GEOGRAPHIC_COORDINATES | number | no | yes | LATITUDE | yes |  |
|  |  |  |  | LONGITUDE | yes |  |
|  |  |  |  | ELEVATION | yes |  |
| MOUNT_LST_TIME | number |  | no | TIME | yes | It depends on hardware if it is undefined, read-only or read-write. |
| MOUNT_UTC_TIME | number |  | no | TIME | yes | It depends on hardware if it is undefined, read-only or read-write. |
|  |  |  |  | OFFSET | yes |  |
| MOUNT_SET_HOST_TIME | switch | no | no | SET | yes |  |
| MOUNT_PARK | switch | no | no | PARKED | yes |  |
|  |  |  |  | UNPARKED | yes |  |
| MOUT_PARK_SET | switch | no | no | DEFAULT | yes |  |
|  |  |  |  | CURRENT | yes |  |
| MOUNT_PARK_POSITION | nuber | no | no | RA | yes |  |
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
|  |  |  |  | MULTI_POINT | yes |  |
| MOUNT_ALIGNMENT_SELECT_POINTS | switch | no | yes | point id | yes |  |
| MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY | switch | no | yes | point id | yes |  |

Properties are implemented by mount driver base class in [indigo_mount_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_mount_driver.c).

## Guider specific properties

| Property |  |  |  | Items |  | Comments |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| Name | Type | RO | Required | Name | Required |  |
| GUIDER_GUIDE_DEC | number | no | yes | NORTH | yes |  |
|  |  |  |  | SOUTH | yes |  |
| GUIDER_GUIDE_RA | number | no | yes | EAST | yes |  |
|  |  |  |  | WEST | yes |  |
| GUIDER_RATE | number | no | no | RATE | yes | % of sidereal rate |

Properties are implemented by guider driver base class in [indigo_guider_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_guider_driver.c).
