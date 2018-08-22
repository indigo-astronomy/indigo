# INDIGO protocols

## Introduction

INDIGO properties and items are abstraction of INDI properties and items. As far as INDIGO uses software bus instead of XML messages,
properties are first of all defined memory structures wich are, if needed, mapped to XML or JSON textual representation.

## Common properties

| Property |  |  |  |  | Items |  |  | Comments |
| --- | --- | --- | --- | --- |  ---  | --- |  ---  |  ---  |
| Name | Legacy mapping | Type | RO | Required | Name | Legacy mapping | Required |  |
| --- | --- | --- | --- | --- |  ---  | --- |  ---  |  ---  |
| CONNECTION | CONNECTION | switch | no | yes | CONNECTED | CONNECT | yes | Item values are undefined if state is not Idle or Ok. |
|  |  |  |  |  | DISCONNECTED | DISCONNECT | yes |  |
| INFO | DRIVER_INFO | text | yes | yes | DEVICE_NAME | DRIVER_NAME | yes | "Device in INDIGO strictly represents device itself and not device driver. |
|  |
| Valid DEVICE_INTERFACE values are defined in indigo_driver.h as indigo_device_interface enumeration." |
|  |  |  |  |  | DEVICE_VERSION | DRIVER_VERSION | yes |  |
|  |  |  |  |  | DEVICE_INTERFACE | DRIVER_INTERFACE | yes |  |
|  |  |  |  |  | FRAMEWORK_NAME |  | no |  |
|  |  |  |  |  | FRAMEWORK_VERSION |  | no |  |
|  |  |  |  |  | DEVICE_MODEL |  | no |  |
|  |  |  |  |  | DEVICE_FIRMWARE_REVISION |  | no |  |
|  |  |  |  |  | DEVICE_HARDWARE_REVISION |  | no |  |
|  |  |  |  |  | DEVICE_SERIAL_NUMBER |  | no |  |
| SIMULATION | SIMULATION | switch | no | no | ENABLED | ENABLE | yes |  |
|  |  |  |  |  | DISABLED | DISABLE | yes |  |
| CONFIG | CONFIG_PROCESS | switch | no | yes | LOAD | CONFIG_LOAD | yes | DEFAULT is not implemented by INDIGO yet. |
|  |  |  |  |  | SAVE | CONFIG_SAVE | yes |  |
|  |  |  |  |  | DEFAULT | CONFIG_DEFAULT | yes |  |
| PROFILE |  | switch | no | yes | PROFILE_0,... |  | yes | Select the profile number for subsequent CONFIG operation |
| DEVICE_PORT | DEVICE_PORT | text | no | no | PORT | PORT | no | Either device path like "/dev/tty0" or url like "lx200://host:port". |
| DEVICE_PORTS |  | switch | no | no | valid serial port name |  |  | When selected, it is copied to DEVICE_PORT property. |

Properties are implemented by driver base class in [indigo_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver.c).

## CCD specific properties

| Property |  |  |  |  | Items |  |  | Comments |
| --- | --- | --- | --- | --- |  ---  | --- |  ---  |  ---  |
| Name | Legacy mapping | Type | RO | Required | Name | Legacy mapping | Required |  |
| --- | --- | --- | --- | --- |  ---  | --- |  ---  |  ---  |
| CCD_INFO | CCD_INFO | number | yes | yes | WIDTH | CCD_MAX_X | yes |  |
|  |  |  |  |  | HEIGHT | CCD_MAX_Y | yes |  |
|  |  |  |  |  | MAX_HORIZONTAL_BIN | CCD_MAX_BIN_X | yes |  |
|  |  |  |  |  | MAX_VERTICAL_BIN | CCD_MAX_BIN_Y | yes |  |
|  |  |  |  |  | PIXEL_SIZE | CCD_PIXEL_SIZE | yes |  |
|  |  |  |  |  | PIXEL_WIDTH | CCD_PIXEL_SIZE_X | yes |  |
|  |  |  |  |  | PIXEL_HEIGHT | CCD_PIXEL_SIZE_Y | yes |  |
|  |  |  |  |  | BITS_PER_PIXEL | CCD_BITSPERPIXEL | yes |  |
| CCD_UPLOAD_MODE | UPLOAD_MODE | switch | no | yes | CLIENT | UPLOAD_CLIENT | yes |  |
|  |  |  |  |  | LOCAL | UPLOAD_LOCAL | yes |  |
|  |  |  |  |  | BOTH | UPLOAD_BOTH | yes |  |
| CCD_LOCAL_MODE | UPLOAD_DIR | text | no | yes | DIR | UPLOAD_DIR | yes | XXX is replaced by sequence. |
|  |  |  |  |  | PREFIX | UPLOAD_PREFIX | yes |  |
| CCD_EXPOSURE | CCD_EXPOSURE | number | no | yes | EXPOSURE | CCD_EXPOSURE_VALUE | yes |  |
| CCD_STREAMING |  | number | no | no | EXPOSURE |  | yes | The same as CCD_EXPOSURE, but will upload COUNT images. Use COUNT -1 for endless loop. |
|  |  |  |  |  | COUNT |  | yes |  |
| CCD_ABORT_EXPOSURE | CCD_ABORT_EXPOSURE | switch | no | yes | ABORT_EXPOSURE | ABORT | yes |  |
| CCD_FRAME | CCD_FRAME | number | no | no | LEFT | X | yes | If BITS_PER_PIXEL can't be changed, set min and max to the same value. |
|  |  |  |  |  | TOP | Y | yes |  |
|  |  |  |  |  | WIDTH | WIDTH | yes |  |
|  |  |  |  |  | HEIGHT | HEIGHT | yes |  |
|  |  |  |  |  | BITS_PER_PIXEL |  | yes |  |
| CCD_BIN | CCD_BINNING | number | no | no | HORIZONTAL | VER_BIN | yes | CCD_MODE is prefered way how to set binning. |
|  |  |  |  |  | VERTICAL | HOR_BIN | yes |  |
| CCD_MODE |  | switch | no | yes | mode identifier |  | yes | CCD_MODE is a prefered way how to set binning, resolution, color mode etc. |
| CCD_READ_MODE |  | switch | no | no | HIGH_SPEED |  | yes |  |
|  |  |  |  |  | LOW_NOISE |  | yes |  |
| CCD_GAIN |  | number | no | no | GAIN |  | yes |  |
| CCD_OFFSET |  | number | no | no | OFFSET |  | yes |  |
| CCD_GAMMA |  | number | no | no | GAMMA |  | yes |  |
| CCD_FRAME_TYPE | CCD_FRAME_TYPE | switch | no | yes | LIGHT | FRAME_LIGHT | yes |  |
|  |  |  |  |  | BIAS | FRAME_BIAS | yes |  |
|  |  |  |  |  | DARK | FRAME_DARK | yes |  |
|  |  |  |  |  | FLAT | FRAME_FLAT | yes |  |
| CCD_IMAGE_FORMAT |  | switch | no | yes | RAW |  | yes |  |
|  |  |  |  |  | FITS |  | yes |  |
|  |  |  |  |  | JPEG |  | yes |  |
| CCD_IMAGE_FILE | CCD_FILE_PATH | text | no | yes | FILE | FILE_PATH | yes |  |
| CCD_TEMPERATURE | CCD_TEMPERATURE | number |  | no | TEMPERATURE | CCD_TEMPERATURE_VALUE | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_COOLER | CCD_COOLER | switch | no | no | ON | COOLER_ON | yes |  |
|  |  |  |  |  | OFF | COOLER_OFF | yes |  |
| CCD_COOLER_POWER | CCD_COOLER_POWER | number | yes | no | POWER | CCD_COOLER_VALUE | yes | It depends on hardware if it is undefined, read-only or read-write. |
| CCD_FITS_HEADERS |  | text | no | yes | HEADER_1, ... |  | yes | String in form "name = value", "name = 'value'" or "comment text"

Properties are implemented by CCD driver base class in [indigo_ccd_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ccd_driver.c).

