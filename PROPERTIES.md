# INDIGO properties

## Introduction

INDIGO properties and items are abstraction of INDI properties and items. As far as INDIGO uses software bus instead of XML messages,
properties are first of all defined memory structures which are, if needed, mapped to XML or JSON textual representation.

## Common properties

<table>
<tr><th colspan='4'>Property</th><th colspan='2'>Items</th><th>Comments</th></tr>
<tr><th>Name</th><th>Type</th><th>RO</th><th>Required</th><th>Name</th><th>Required</th><th></th></tr>
<tr><td>CONNECTION</td><td>switch</td><td>no</td><td>yes</td><td>CONNECTED</td><td>yes</td><td>Item values are undefined if state is not Idle or Ok.</td></tr>
<tr><td></td><td></td><td></td><td></td><td>DISCONNECTED</td><td>yes</td><td></td></tr>
<tr><td>INFO</td><td>text</td><td>yes</td><td>yes</td><td>DEVICE_NAME</td><td>yes</td><td>"Device in INDIGO strictly represents device itself and not device driver. Valid DEVICE_INTERFACE values are defined in indigo_driver.h as indigo_device_interface enumeration."</td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEVICE_VERSION</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEVICE_INTERFACE</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>FRAMEWORK_NAME</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>FRAMEWORK_VERSION</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEVICE_MODEL</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEVICE_FIRMWARE_REVISION</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEVICE_HARDWARE_REVISION</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEVICE_SERIAL_NUMBER</td><td>no</td><td></td></tr>
<tr><td>SIMULATION</td><td>switch</td><td>no</td><td>no</td><td>ENABLED</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DISABLED</td><td>yes</td><td></td></tr>
<tr><td>CONFIG</td><td>switch</td><td>no</td><td>yes</td><td>LOAD</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>SAVE</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>REMOVE</td><td>yes</td><td></td></tr>
<tr><td>PROFILE</td><td>switch</td><td>no</td><td>yes</td><td>PROFILE_0,...</td><td>yes</td><td>Select the profile number for subsequent CONFIG operation</td></tr>
<tr><td>DEVICE_PORT</td><td>text</td><td>no</td><td>no</td><td>PORT</td><td>no</td><td>Either device path like "/dev/tty0" or URL like "lx200://host:port".</td></tr>
<tr><td>DEVICE_PORTS</td><td>switch</td><td>no</td><td>no</td><td>valid serial port name</td><td></td><td>When selected, it is copied to DEVICE_PORT property.</td></tr>
</table>


Properties are implemented by driver base class in [indigo_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_driver.c).

## CCD specific properties

<table>
<tr><th colspan='4'>Property</th><th colspan='2'>Items</th><th>Comments</th></tr>
<tr><th>Name</th><th>Type</th><th>RO</th><th>Required</th><th>Name</th><th>Required</th><th></th></tr>
<tr><td>CCD_INFO</td><td>number</td><td>yes</td><td>yes</td><td>WIDTH</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>HEIGHT</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>MAX_HORIZONTAL_BIN</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>MAX_VERTICAL_BIN</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PIXEL_SIZE</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PIXEL_WIDTH</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PIXEL_HEIGHT</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>BITS_PER_PIXEL</td><td>yes</td><td></td></tr>
<tr><td>CCD_UPLOAD_MODE</td><td>switch</td><td>no</td><td>yes</td><td>CLIENT</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>LOCAL</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>BOTH</td><td>yes</td><td></td></tr>
<tr><td>CCD_LOCAL_MODE</td><td>text</td><td>no</td><td>yes</td><td>DIR</td><td>yes</td><td>XXX is replaced by sequence.</td></tr>
<tr><td></td><td></td><td></td><td></td><td>PREFIX</td><td>yes</td><td></td></tr>
<tr><td>CCD_EXPOSURE</td><td>number</td><td>no</td><td>yes</td><td>EXPOSURE</td><td>yes</td><td></td></tr>
<tr><td>CCD_STREAMING</td><td>number</td><td>no</td><td>no</td><td>EXPOSURE</td><td>yes</td><td>The same as CCD_EXPOSURE, but will upload COUNT images. Use COUNT -1 for endless loop.</td></tr>
<tr><td></td><td></td><td></td><td></td><td>COUNT</td><td>yes</td><td></td></tr>
<tr><td>CCD_ABORT_EXPOSURE</td><td>switch</td><td>no</td><td>yes</td><td>ABORT_EXPOSURE</td><td>yes</td><td></td></tr>
<tr><td>CCD_FRAME</td><td>number</td><td>no</td><td>no</td><td>LEFT</td><td>yes</td><td>If BITS_PER_PIXEL can't be changed, set min and max to the same value.</td></tr>
<tr><td></td><td></td><td></td><td></td><td>TOP</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>WIDTH</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>HEIGHT</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>BITS_PER_PIXEL</td><td>yes</td><td></td></tr>
<tr><td>CCD_BIN</td><td>number</td><td>no</td><td>no</td><td>HORIZONTAL</td><td>yes</td><td>CCD_MODE is prefered way how to set binning.</td></tr>
<tr><td></td><td></td><td></td><td></td><td>VERTICAL</td><td>yes</td><td></td></tr>
<tr><td>CCD_MODE</td><td>switch</td><td>no</td><td>yes</td><td>mode identifier</td><td>yes</td><td>CCD_MODE is a prefered way how to set binning, resolution, color mode etc.</td></tr>
<tr><td>CCD_READ_MODE</td><td>switch</td><td>no</td><td>no</td><td>HIGH_SPEED</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>LOW_NOISE</td><td>yes</td><td></td></tr>
<tr><td>CCD_GAIN</td><td>number</td><td>no</td><td>no</td><td>GAIN</td><td>yes</td><td></td></tr>
<tr><td>CCD_OFFSET</td><td>number</td><td>no</td><td>no</td><td>OFFSET</td><td>yes</td><td></td></tr>
<tr><td>CCD_GAMMA</td><td>number</td><td>no</td><td>no</td><td>GAMMA</td><td>yes</td><td></td></tr>
<tr><td>CCD_FRAME_TYPE</td><td>switch</td><td>no</td><td>yes</td><td>LIGHT</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>BIAS</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DARK</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>FLAT</td><td>yes</td><td></td></tr>
<tr><td>CCD_IMAGE_FORMAT</td><td>switch</td><td>no</td><td>yes</td><td>RAW</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>FITS</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>XISF</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>JPEG</td><td>yes</td><td></td></tr>
<tr><td>CCD_IMAGE_FILE</td><td>text</td><td>no</td><td>yes</td><td>FILE</td><td>yes</td><td></td></tr>
<tr><td>CCD_TEMPERATURE</td><td>number</td><td></td><td>no</td><td>TEMPERATURE</td><td>yes</td><td>It depends on hardware if it is undefined, read-only or read-write.</td></tr>
<tr><td>CCD_COOLER</td><td>switch</td><td>no</td><td>no</td><td>ON</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OFF</td><td>yes</td><td></td></tr>
<tr><td>CCD_COOLER_POWER</td><td>number</td><td>yes</td><td>no</td><td>POWER</td><td>yes</td><td>It depends on hardware if it is undefined, read-only or read-write.</td></tr>
<tr><td>CCD_FITS_HEADERS</td><td>text</td><td>no</td><td>yes</td><td>HEADER_1, ...</td><td>yes</td><td>String in form "name = value", "name = 'value'" or "comment text"</td></tr>
</table>


Properties are implemented by CCD driver base class in [indigo_ccd_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_ccd_driver.c).

## Wheel specific properties

<table>
<tr><th colspan='4'>Property</th><th colspan='2'>Items</th><th>Comments</th></tr>
<tr><th>Name</th><th>Type</th><th>RO</th><th>Required</th><th>Name</th><th>Required</th><th></th></tr>
<tr><td>WHEEL_SLOT</td><td>number</td><td>no</td><td>yes</td><td>SLOT</td><td>yes</td><td></td></tr>
<tr><td>WHEEL_SLOT_NAME</td><td>switch</td><td>no</td><td>yes</td><td>SLOT_NAME_1, ...</td><td>yes</td><td></td></tr>
<tr><td>WHEEL_SLOT_OFFSET</td><td>switch</td><td>no</td><td>yes</td><td>SLOT_OFFSET_1, ...</td><td>yes</td><td>Value is number of focuser steps</td></tr>
</table>


Properties are implemented by wheel driver base class in [indigo_wheel_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_wheel_driver.c).

## Focuser specific properties

<table>
<tr><th colspan='4'>Property</th><th colspan='2'>Items</th><th>Comments</th></tr>
<tr><th>Name</th><th>Type</th><th>RO</th><th>Required</th><th>Name</th><th>Required</th><th></th></tr>
<tr><td>FOCUSER_SPEED</td><td>number</td><td>no</td><td>no</td><td>SPEED</td><td>yes</td><td></td></tr>
<tr><td>FOCUSER_DIRECTION</td><td>switch</td><td>no</td><td>yes</td><td>MOVE_INWARD</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>MOVE_OUTWARD</td><td>yes</td><td></td></tr>
<tr><td>FOCUSER_STEPS</td><td>number</td><td>no</td><td>yes</td><td>STEPS</td><td>yes</td><td></td></tr>
<tr><td>FOCUSER_ON_POSITION_SET</td><td>switch</td><td>no</td><td>no</td><td>GOTO</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>SYNC</td><td>yes</td><td></td></tr>
<tr><td>FOCUSER_POSITION</td><td>number</td><td></td><td>no</td><td>POSITION</td><td>yes</td><td>It depends on hardware if it is undefined, read-only or read-write.</td></tr>
<tr><td>FOCUSER_ABORT_MOTION</td><td>switch</td><td>no</td><td>yes</td><td>ABORT_MOTION</td><td>yes</td><td></td></tr>
<tr><td>FOCUSER_TEMPERATURE</td><td>number</td><td>no</td><td>no</td><td>TEMPERATURE</td><td>yes</td><td></td></tr>
<tr><td>FOCUSER_BACKLASH</td><td>number</td><td>no</td><td>no</td><td>BACKLASH</td><td>yes</td><td>Mechanical backlash compensation</td></tr>
<tr><td>FOCUSER_COMPENSATION</td><td>number</td><td>no</td><td>no</td><td>COMPENSATION</td><td>yes</td><td>Temperature compensation (if FOCUSER_MODE.AUTOMATIC is set</td></tr>
<tr><td>FOCUSER_MODE</td><td>switch</td><td>no</td><td>no</td><td>MANUAL</td><td>yes</td><td>Manual mode</td></tr>
<tr><td></td><td></td><td></td><td></td><td>AUTOMATIC</td><td>yes</td><td>Temperature compensated mode</td></tr>
</table>


Properties are implemented by focuser driver base class in [indigo_focuser_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_focuser_driver.c).

## Mount specific properties

<table>
<tr><th colspan='4'>Property</th><th colspan='2'>Items</th><th>Comments</th></tr>
<tr><th>Name</th><th>Type</th><th>RO</th><th>Required</th><th>Name</th><th>Required</th><th></th></tr>
<tr><td>MOUNT_INFO</td><td>text</td><td>no</td><td>no</td><td>MODEL</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>VENDOR</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>FIRMWARE_VERSION</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_GEOGRAPHIC_COORDINATES</td><td>number</td><td>no</td><td>yes</td><td>LATITUDE</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>LONGITUDE</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>ELEVATION</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_LST_TIME</td><td>number</td><td></td><td>no</td><td>TIME</td><td>yes</td><td>It depends on hardware if it is undefined, read-only or read-write.</td></tr>
<tr><td>MOUNT_UTC_TIME</td><td>number</td><td></td><td>no</td><td>TIME</td><td>yes</td><td>It depends on hardware if it is undefined, read-only or read-write.</td></tr>
<tr><td></td><td></td><td></td><td></td><td>OFFSET</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_SET_HOST_TIME</td><td>switch</td><td>no</td><td>no</td><td>SET</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_PARK</td><td>switch</td><td>no</td><td>no</td><td>PARKED</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>UNPARKED</td><td>yes</td><td></td></tr>
<tr><td>MOUT_PARK_SET</td><td>switch</td><td>no</td><td>no</td><td>DEFAULT</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>CURRENT</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_PARK_POSITION</td><td>number</td><td>no</td><td>no</td><td>RA</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEC</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_ON_COORDINATES_SET</td><td>switch</td><td>no</td><td>yes</td><td>TRACK</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>SYNC</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>SLEW</td><td>no</td><td></td></tr>
<tr><td>MOUNT_SLEW_RATE</td><td>switch</td><td>no</td><td>no</td><td>GUIDE</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>CENTERING</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>FIND</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>MAX</td><td>no</td><td></td></tr>
<tr><td>MOUNT_MOTION_DEC</td><td>switch</td><td>no</td><td>yes</td><td>NORTH</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>SOUTH</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_MOTION_RA</td><td>switch</td><td>no</td><td>yes</td><td>WEST</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>EAST</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_TRACK_RATE</td><td>switch</td><td>no</td><td>no</td><td>SIDEREAL</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>SOLAR</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>LUNAR</td><td>no</td><td></td></tr>
<tr><td>MOUNT_TRACKING</td><td>switch</td><td>no</td><td>no</td><td>ON</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OFF</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_GUIDE_RATE</td><td>nuber</td><td>no</td><td>no</td><td>RA</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEC</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_EQUATORIAL_COORDINATES</td><td>number</td><td>no</td><td>yes</td><td>RA</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEC</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_HORIZONTAL_COORDINATES</td><td>number</td><td>no</td><td>no</td><td>ALT</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>AZ</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_ABORT_MOTION</td><td>switch</td><td>no</td><td>yes</td><td>ABORT_MOTION</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_RAW_COORDINATES</td><td>number</td><td>yes</td><td>yes</td><td>RA</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEC</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_ALIGNMENT_MODE</td><td>switch</td><td>no</td><td>yes</td><td>CONTROLLER</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>SINGLE_POINT</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>MULTI_POINT</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_ALIGNMENT_SELECT_POINTS</td><td>switch</td><td>no</td><td>yes</td><td>point id</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY</td><td>switch</td><td>no</td><td>yes</td><td>point id</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_EPOCH</td><td>number</td><td>no</td><td>yes</td><td>EPOCH</td><td>yes</td><td></td></tr>
<tr><td>MOUNT_SIDE_OF_PIER</td><td>switch</td><td>no</td><td>yes</td><td>EAST</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>WEST</td><td>yes</td><td></td></tr>
</table>


Properties are implemented by mount driver base class in [indigo_mount_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_mount_driver.c).

## Guider specific properties

<table>
<tr><th colspan='4'>Property</th><th colspan='2'>Items</th><th>Comments</th></tr>
<tr><th>Name</th><th>Type</th><th>RO</th><th>Required</th><th>Name</th><th>Required</th><th></th></tr>
<tr><td>GUIDER_GUIDE_DEC</td><td>number</td><td>no</td><td>yes</td><td>NORTH</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>SOUTH</td><td>yes</td><td></td></tr>
<tr><td>GUIDER_GUIDE_RA</td><td>number</td><td>no</td><td>yes</td><td>EAST</td><td>yes</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>WEST</td><td>yes</td><td></td></tr>
<tr><td>GUIDER_RATE</td><td>number</td><td>no</td><td>no</td><td>RATE</td><td>yes</td><td>% of sidereal rate</td></tr>
</table>


Properties are implemented by guider driver base class in [indigo_guider_driver.c](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo_guider_driver.c).

## Auxiliary properties

To be used by auxiliary devices like powerboxes, weather stations, etc.

<table>
<tr><th colspan='4'>Property</th><th colspan='2'>Items</th><th>Comments</th></tr>
<tr><th>Name</th><th>Type</th><th>RO</th><th>Required</th><th>Name</th><th>Required</th><th></th></tr>
<tr><td>AUX_POWER_OUTLET</td><td>switch</td><td>no</td><td>no</td><td>OUTLET_1</td><td>yes</td><td>Enable power outlets</td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_2</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_3</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_4</td><td>no</td><td></td></tr>

<tr><td>AUX_POWER_OUTLET_STATE</td><td>light</td><td>yes</td><td>no</td><td>OUTLET_1</td><td>yes</td><td>Power outlets state (IDLE = unused, OK = used, ALERT = over-current etc.)</td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_2</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_3</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_4</td><td>no</td><td></td></tr>

<tr><td>AUX_POWER_OUTLET_CURRENT</td><td>number</td><td>yes</td><td>no</td><td>OUTLET_1</td><td>yes</td><td>Power outlets current</td></tr>
<tr><td></td><td></td><td></td><td>no</td><td>OUTLET_2</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td>no</td><td>OUTLET_3</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td>no</td><td>OUTLET_4</td><td>no</td><td></td></tr>

<tr><td>AUX_HEATER_OUTLET</td><td>number</td><td>no</td><td>no</td><td>OUTLET_1</td><td>yes</td><td>Set heater outlets power</td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_2</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_3</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_4</td><td>no</td><td></td></tr>

<tr><td>AUX_HEATER_OUTLET_STATE</td><td>light</td><td>yes</td><td>no</td><td>OUTLET_1</td><td>yes</td><td>Heater outlets state (IDLE = unused, OK = used, ALERT = over-current etc.)</td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_2</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_3</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_4</td><td>no</td><td></td></tr>

<tr><td>AUX_HEATER_OUTLET_CURRENT</td><td>number</td><td>yes</td><td>no</td><td>OUTLET_1</td><td>yes</td><td>Heater outlets current</td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_2</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_3</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>OUTLET_4</td><td>no</td><td></td></tr>

<tr><td>AUX_USB_PORT</td><td>switch</td><td>no</td><td>no</td><td>PORT_1</td><td>yes</td><td>Enable USB ports on smart hub</td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_2</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_3</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_4</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_5</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_6</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_7</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_8</td><td>no</td><td></td></tr>

<tr><td>AUX_USB_PORT_STATE</td><td>light</td><td>yes</td><td>no</td><td>PORT_1</td><td>yes</td><td>USB port state (IDLE = unused or disabled, OK = used, BUSY = transient state, ALERT = over-current etc.)</td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_2</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_3</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_4</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_5</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_6</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_7</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>PORT_8</td><td>no</td><td></td></tr>

<tr><td>AUX_DEW_CONTROL</td><td>switch</td><td>no</td><td>no</td><td>MANUAL</td><td>yes</td><td>Use AUX_HEATER_OUTLET values</td></tr>
<tr><td></td><td></td><td></td><td></td><td>AUTOMATIC</td><td>yes</td><td>Set power automatically</td></tr>

<tr><td>AUX_WEATHER</td><td>number</td><td>yes</td><td>no</td><td>TEMPERATURE</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>HUMIDITY</td><td>no</td><td></td></tr>
<tr><td></td><td></td><td></td><td></td><td>DEVPOINT</td><td>no</td><td></td></tr>

<tr><td>AUX_INFO</td><td>number</td><td>yes</td><td>no</td><td>...</td><td>no</td><td>Any number of any number items</td></tr>
<tr><td>AUX_CONTROL</td><td>switch</td><td>no</td><td>no</td><td>...</td><td>no</td><td>Any number of any switch items</td></tr>
</table>
