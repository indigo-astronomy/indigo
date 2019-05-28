# INDIGO properties

## Introduction

INDIGO properties and items are abstraction of INDI properties and items. As far as INDIGO uses software bus instead of XML messages,
properties are first of all defined memory structures which are, if needed, mapped to XML or JSON textual representation.

## Common properties

<table>
<tr><th  align="left" colspan='4'>Property&nbsp;</th><th  align="left" colspan='2'>Items&nbsp;</th><th  align="left">Comments&nbsp;</th></tr>
<tr><th  align="left">Name&nbsp;</th><th  align="left">Type&nbsp;</th><th  align="left">RO&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">Name&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">&nbsp;</th></tr>
<tr><td>CONNECTION&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>CONNECTED&nbsp;</td><td>yes&nbsp;</td><td>Item values are undefined if state is not Idle or Ok.&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DISCONNECTED&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>INFO&nbsp;</td><td>text&nbsp;</td><td>yes&nbsp;</td><td>yes&nbsp;</td><td>DEVICE_NAME&nbsp;</td><td>yes&nbsp;</td><td>"Device in INDIGO strictly represents device itself and not device driver. Valid DEVICE_INTERFACE values are defined in indigo_driver.h as indigo_device_interface enumeration."&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEVICE_VERSION&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEVICE_INTERFACE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>FRAMEWORK_NAME&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>FRAMEWORK_VERSION&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEVICE_MODEL&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEVICE_FIRMWARE_REVISION&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEVICE_HARDWARE_REVISION&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEVICE_SERIAL_NUMBER&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>SIMULATION&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>ENABLED&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DISABLED&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CONFIG&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>LOAD&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>SAVE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>REMOVE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>PROFILE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>PROFILE_0,...&nbsp;</td><td>yes&nbsp;</td><td>Select the profile number for subsequent CONFIG operation&nbsp;</td></tr>
<tr><td>DEVICE_PORT&nbsp;</td><td>text&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>PORT&nbsp;</td><td>no&nbsp;</td><td>Either device path like "/dev/tty0" or URL like "lx200://host:port".&nbsp;</td></tr>
<tr><td>DEVICE_BAUDRATE&nbsp;</td><td>text&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>BAUDRATE&nbsp;</td><td>no&nbsp;</td><td>Serial port configuration in a string like this: 9600-8N1&nbsp;</td></tr>
<tr><td>DEVICE_PORTS&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>valid serial port name&nbsp;</td><td>&nbsp;</td><td>When selected, it is copied to DEVICE_PORT property.&nbsp;</td></tr>
<tr><td>GEOGRAPHIC_COORDINATES&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>LATITUDE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>LONGITUDE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>ELEVATION&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>UTC_TIME&nbsp;</td><td>number&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>TIME&nbsp;</td><td>yes&nbsp;</td><td>It depends on hardware if it is undefined, read-only or read-write.&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OFFSET&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
</table>


Properties are implemented by driver base class in [indigo_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver.c).

## CCD specific properties

<table>
<tr><th  align="left" colspan='4'>Property&nbsp;</th><th  align="left" colspan='2'>Items&nbsp;</th><th  align="left">Comments&nbsp;</th></tr>
<tr><th  align="left">Name&nbsp;</th><th  align="left">Type&nbsp;</th><th  align="left">RO&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">Name&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">&nbsp;</th></tr>
<tr><td>CCD_INFO&nbsp;</td><td>number&nbsp;</td><td>yes&nbsp;</td><td>yes&nbsp;</td><td>WIDTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>HEIGHT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>MAX_HORIZONTAL_BIN&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>MAX_VERTICAL_BIN&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PIXEL_SIZE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PIXEL_WIDTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PIXEL_HEIGHT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>BITS_PER_PIXEL&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_UPLOAD_MODE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>CLIENT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>LOCAL&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>BOTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PREVIEW&nbsp;</td><td>yes&nbsp;</td><td>Send JPEG preview to client&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PREVIEW_LOCAL&nbsp;</td><td>yes&nbsp;</td><td>Send JPEG preview to client and save original format locally&nbsp;</td></tr>
<tr><td>CCD_LOCAL_MODE&nbsp;</td><td>text&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>DIR&nbsp;</td><td>yes&nbsp;</td><td>XXX is replaced by sequence.&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PREFIX&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_EXPOSURE&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>EXPOSURE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_STREAMING&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>EXPOSURE&nbsp;</td><td>yes&nbsp;</td><td>The same as CCD_EXPOSURE, but will upload COUNT images. Use COUNT -1 for endless loop.&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>COUNT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_ABORT_EXPOSURE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>ABORT_EXPOSURE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_FRAME&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>LEFT&nbsp;</td><td>yes&nbsp;</td><td>If BITS_PER_PIXEL can't be changed, set min and max to the same value.&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>TOP&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>WIDTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>HEIGHT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>BITS_PER_PIXEL&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_BIN&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>HORIZONTAL&nbsp;</td><td>yes&nbsp;</td><td>CCD_MODE is prefered way how to set binning.&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>VERTICAL&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_MODE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>mode identifier&nbsp;</td><td>yes&nbsp;</td><td>CCD_MODE is a prefered way how to set binning, resolution, color mode etc.&nbsp;</td></tr>
<tr><td>CCD_READ_MODE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>HIGH_SPEED&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>LOW_NOISE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_GAIN&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>GAIN&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_OFFSET&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>OFFSET&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_GAMMA&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>GAMMA&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_FRAME_TYPE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>LIGHT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>BIAS&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DARK&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>FLAT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_IMAGE_FORMAT&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>RAW&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>FITS&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>XISF&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>JPEG&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_IMAGE_FILE&nbsp;</td><td>text&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>FILE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_IMAGE&nbsp;</td><td>blob&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>IMAGE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_TEMPERATURE&nbsp;</td><td>number&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>TEMPERATURE&nbsp;</td><td>yes&nbsp;</td><td>It depends on hardware if it is undefined, read-only or read-write.&nbsp;</td></tr>
<tr><td>CCD_COOLER&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>ON&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OFF&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>CCD_COOLER_POWER&nbsp;</td><td>number&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>POWER&nbsp;</td><td>yes&nbsp;</td><td>It depends on hardware if it is undefined, read-only or read-write.&nbsp;</td></tr>
<tr><td>CCD_FITS_HEADERS&nbsp;</td><td>text&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>HEADER_1, ...&nbsp;</td><td>yes&nbsp;</td><td>String in form "name = value", "name = 'value'" or "comment text"&nbsp;</td></tr>
</table>


Properties are implemented by CCD driver base class in [indigo_ccd_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ccd_driver.c).

### DSLR extensions (in addition to CCD specific properties)

<table>
<tr><th  align="left" colspan='4'>Property&nbsp;</th><th  align="left" colspan='2'>Items&nbsp;</th><th  align="left">Comments&nbsp;</th></tr>
<tr><th  align="left">Name&nbsp;</th><th  align="left">Type&nbsp;</th><th  align="left">RO&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">Name&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">&nbsp;</th></tr>
<tr><td>DSLR_PROGRAM&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td>DSLR_APERTURE&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera/lens&nbsp;</td></tr>
<tr><td>DSLR_SHUTTER&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td>DSLR_COMPRESSION&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td>DSLR_WHITE_BALANCE&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td>DSLR_ISO&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_EXPOSURE_METERING&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_FOCUS_METERING&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_FOCUS_MODE&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_FOCUS_MODE&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_CAPTURE_MODE&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_FLASH_MODE&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_EXPOSURE_COMPENSATION&nbsp;</td><td>switch&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_BATTERY_LEVEL&nbsp;</td><td>switch&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>VALUE&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_FOCAL_LENGTH&nbsp;</td><td>switch&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>VALUE&nbsp;</td><td>yes&nbsp;</td><td>RO/RW status and items set depends on particular camera&nbsp;</td></tr>
<tr><td> DSLR_LOCK&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>LOCK&nbsp;</td><td>yes&nbsp;</td><td>Lock camera UI&nbsp;</td></tr>
<tr><td> &nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>UNLOCK&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td> DSLR_MIRROR_LOCKUP&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>LOCK&nbsp;</td><td>yes&nbsp;</td><td>Lock camera mirror&nbsp;</td></tr>
<tr><td> &nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>UNLOCK&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td> DSLR_AF&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>AF&nbsp;</td><td>yes&nbsp;</td><td>Start autofocus&nbsp;</td></tr>
<tr><td> DSLR_AVOID_AF&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>ON&nbsp;</td><td>yes&nbsp;</td><td>Avoid autofocus&nbsp;</td></tr>
<tr><td> &nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OF&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td> DSLR_STREAMING_MODE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>LIVE_VIEW&nbsp;</td><td>yes&nbsp;</td><td>Operation used for streaming&nbsp;</td></tr>
<tr><td> &nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>BURST_MODE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td> DSLR_ZOOM_PREVIEW&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>ON&nbsp;</td><td>yes&nbsp;</td><td>LiveView zoom&nbsp;</td></tr>
<tr><td> &nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OFF&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td> DSLR_DELETE_IMAGE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>ON&nbsp;</td><td>yes&nbsp;</td><td>Delete image from camera memory/card&nbsp;</td></tr>
<tr><td> &nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OFF&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
</table>

## Wheel specific properties

<table>
<tr><th  align="left" colspan='4'>Property&nbsp;</th><th  align="left" colspan='2'>Items&nbsp;</th><th  align="left">Comments&nbsp;</th></tr>
<tr><th  align="left">Name&nbsp;</th><th  align="left">Type&nbsp;</th><th  align="left">RO&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">Name&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">&nbsp;</th></tr>
<tr><td>WHEEL_SLOT&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>SLOT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>WHEEL_SLOT_NAME&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>SLOT_NAME_1, ...&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>WHEEL_SLOT_OFFSET&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>SLOT_OFFSET_1, ...&nbsp;</td><td>yes&nbsp;</td><td>Value is number of focuser steps&nbsp;</td></tr>
</table>


Properties are implemented by wheel driver base class in [indigo_wheel_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_wheel_driver.c).

## Focuser specific properties

<table>
<tr><th  align="left" colspan='4'>Property&nbsp;</th><th  align="left" colspan='2'>Items&nbsp;</th><th  align="left">Comments&nbsp;</th></tr>
<tr><th  align="left">Name&nbsp;</th><th  align="left">Type&nbsp;</th><th  align="left">RO&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">Name&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">&nbsp;</th></tr>
<tr><td>FOCUSER_SPEED&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>SPEED&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>FOCUSER_REVERSE_MOTION&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>ENABLED&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DIABLED&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>FOCUSER_DIRECTION&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>MOVE_INWARD&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>MOVE_OUTWARD&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>FOCUSER_STEPS&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>STEPS&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>FOCUSER_ON_POSITION_SET&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>GOTO&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>SYNC&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>FOCUSER_POSITION&nbsp;</td><td>number&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>POSITION&nbsp;</td><td>yes&nbsp;</td><td>It depends on hardware if it is undefined, read-only or read-write.&nbsp;</td></tr>
<tr><td>FOCUSER_ABORT_MOTION&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>ABORT_MOTION&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>FOCUSER_TEMPERATURE&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>TEMPERATURE&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>FOCUSER_BACKLASH&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>BACKLASH&nbsp;</td><td>yes&nbsp;</td><td>Mechanical backlash compensation&nbsp;</td></tr>
<tr><td>FOCUSER_COMPENSATION&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>COMPENSATION&nbsp;</td><td>yes&nbsp;</td><td>Temperature compensation (if FOCUSER_MODE.AUTOMATIC is set&nbsp;</td></tr>
<tr><td>FOCUSER_MODE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>MANUAL&nbsp;</td><td>yes&nbsp;</td><td>Manual mode&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>AUTOMATIC&nbsp;</td><td>yes&nbsp;</td><td>Temperature compensated mode&nbsp;</td></tr>
<tr><td>FOCUSER_LIMITS&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>MIN_POSITION&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>MAX_POSITION&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
</table>


Properties are implemented by focuser driver base class in [indigo_focuser_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_focuser_driver.c).

## Mount specific properties

<table>
<tr><th  align="left" colspan='4'>Property&nbsp;</th><th  align="left" colspan='2'>Items&nbsp;</th><th  align="left">Comments&nbsp;</th></tr>
<tr><th  align="left">Name&nbsp;</th><th  align="left">Type&nbsp;</th><th  align="left">RO&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">Name&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">&nbsp;</th></tr>
<tr><td>MOUNT_INFO&nbsp;</td><td>text&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>MODEL&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>VENDOR&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>FIRMWARE_VERSION&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_LST_TIME&nbsp;</td><td>number&nbsp;</td><td>&nbsp;</td><td>yes&nbsp;</td><td>TIME&nbsp;</td><td>yes&nbsp;</td><td>It depends on hardware if it is undefined, read-only or read-write.&nbsp;</td></tr>
<tr><td>MOUNT_SET_HOST_TIME&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>SET&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_PARK&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>PARKED&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>UNPARKED&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUT_PARK_SET&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>DEFAULT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>CURRENT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_PARK_POSITION&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>RA&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEC&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_ON_COORDINATES_SET&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>TRACK&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>SYNC&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>SLEW&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_SLEW_RATE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>GUIDE&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>CENTERING&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>FIND&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>MAX&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_MOTION_DEC&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>NORTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>SOUTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_MOTION_RA&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>WEST&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>EAST&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_TRACK_RATE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>SIDEREAL&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>SOLAR&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>LUNAR&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_TRACKING&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>ON&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OFF&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_GUIDE_RATE&nbsp;</td><td>nuber&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>RA&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEC&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_EQUATORIAL_COORDINATES&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>RA&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEC&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_HORIZONTAL_COORDINATES&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>ALT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>AZ&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_ABORT_MOTION&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>ABORT_MOTION&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_RAW_COORDINATES&nbsp;</td><td>number&nbsp;</td><td>yes&nbsp;</td><td>yes&nbsp;</td><td>RA&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEC&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_ALIGNMENT_MODE&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>CONTROLLER&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>SINGLE_POINT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>NEAREST_POINT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>MULTI_POINT&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_ALIGNMENT_SELECT_POINTS&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>point id&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>point id&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_EPOCH&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>EPOCH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>MOUNT_SIDE_OF_PIER&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>EAST&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>WEST&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
</table>


Properties are implemented by mount driver base class in [indigo_mount_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_mount_driver.c).

## Guider specific properties

<table>
<tr><th  align="left" colspan='4'>Property&nbsp;</th><th  align="left" colspan='2'>Items&nbsp;</th><th  align="left">Comments&nbsp;</th></tr>
<tr><th  align="left">Name&nbsp;</th><th  align="left">Type&nbsp;</th><th  align="left">RO&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">Name&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">&nbsp;</th></tr>
<tr><td>GUIDER_GUIDE_DEC&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>NORTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>SOUTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>GUIDER_GUIDE_RA&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>EAST&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>WEST&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>GUIDER_RATE&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>RATE&nbsp;</td><td>yes&nbsp;</td><td>% of sidereal rate (RA or both)&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>DEC_RATE&nbsp;</td><td>no&nbsp;</td><td>% of sidereal rate (DEC)&nbsp;</td></tr>
</table>


Properties are implemented by guider driver base class in [indigo_guider_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_guider_driver.c).

## AO specific properties

<table>
<tr><th  align="left" colspan='4'>Property&nbsp;</th><th  align="left" colspan='2'>Items&nbsp;</th><th  align="left">Comments&nbsp;</th></tr>
<tr><th  align="left">Name&nbsp;</th><th  align="left">Type&nbsp;</th><th  align="left">RO&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">Name&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">&nbsp;</th></tr>
<tr><td>AO_GUIDE_DEC&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>NORTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>SOUTH&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>AO_GUIDE_RA&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>EAST&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>WEST&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>AO_RESET&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>yes&nbsp;</td><td>CENTER&nbsp;</td><td>yes&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>UNJAM&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
</table>


Properties are implemented by AO driver base class in [indigo_ao_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ao_driver.c).

## Auxiliary properties

To be used by auxiliary devices like powerboxes, weather stations, etc.

<table>
<tr><th  align="left" colspan='4'>Property&nbsp;</th><th  align="left" colspan='2'>Items&nbsp;</th><th  align="left">Comments&nbsp;</th></tr>
<tr><th  align="left">Name&nbsp;</th><th  align="left">Type&nbsp;</th><th  align="left">RO&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">Name&nbsp;</th><th  align="left">Required&nbsp;</th><th  align="left">&nbsp;</th></tr>
<tr><td>AUX_POWER_OUTLET&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>OUTLET_1&nbsp;</td><td>yes&nbsp;</td><td>Enable power outlets&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_2&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_3&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_4&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>

<tr><td>AUX_POWER_OUTLET_STATE&nbsp;</td><td>light&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>OUTLET_1&nbsp;</td><td>yes&nbsp;</td><td>Power outlets state (IDLE = unused, OK = used, ALERT = over-current etc.)&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_2&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_3&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_4&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>

<tr><td>AUX_POWER_OUTLET_CURRENT&nbsp;</td><td>number&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>OUTLET_1&nbsp;</td><td>yes&nbsp;</td><td>Power outlets current&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>OUTLET_2&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>OUTLET_3&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>no&nbsp;</td><td>OUTLET_4&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>

<tr><td>AUX_HEATER_OUTLET&nbsp;</td><td>number&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>OUTLET_1&nbsp;</td><td>yes&nbsp;</td><td>Set heater outlets power&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_2&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_3&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_4&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>

<tr><td>AUX_HEATER_OUTLET_STATE&nbsp;</td><td>light&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>OUTLET_1&nbsp;</td><td>yes&nbsp;</td><td>Heater outlets state (IDLE = unused, OK = used, ALERT = over-current etc.)&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_2&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_3&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_4&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>

<tr><td>AUX_HEATER_OUTLET_CURRENT&nbsp;</td><td>number&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>OUTLET_1&nbsp;</td><td>yes&nbsp;</td><td>Heater outlets current&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_2&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_3&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>OUTLET_4&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>

<tr><td>AUX_USB_PORT&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>PORT_1&nbsp;</td><td>yes&nbsp;</td><td>Enable USB ports on smart hub&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_2&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_3&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_4&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_5&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_6&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_7&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_8&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>

<tr><td>AUX_USB_PORT_STATE&nbsp;</td><td>light&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>PORT_1&nbsp;</td><td>yes&nbsp;</td><td>USB port state (IDLE = unused or disabled, OK = used, BUSY = transient state, ALERT = over-current etc.)&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_2&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_3&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_4&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_5&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_6&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_7&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>PORT_8&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>

<tr><td>AUX_DEW_CONTROL&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>MANUAL&nbsp;</td><td>yes&nbsp;</td><td>Use AUX_HEATER_OUTLET values&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>AUTOMATIC&nbsp;</td><td>yes&nbsp;</td><td>Set power automatically&nbsp;</td></tr>

<tr><td>AUX_WEATHER&nbsp;</td><td>number&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>TEMPERATURE&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>HUMIDITY&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>
<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>DEWPOINT&nbsp;</td><td>no&nbsp;</td><td>&nbsp;</td></tr>

<tr><td>AUX_INFO&nbsp;</td><td>number&nbsp;</td><td>yes&nbsp;</td><td>no&nbsp;</td><td>...&nbsp;</td><td>no&nbsp;</td><td>Any number of any number items&nbsp;</td></tr>
<tr><td>AUX_CONTROL&nbsp;</td><td>switch&nbsp;</td><td>no&nbsp;</td><td>no&nbsp;</td><td>...&nbsp;</td><td>no&nbsp;</td><td>Any number of any switch items&nbsp;</td></tr>
</table>
