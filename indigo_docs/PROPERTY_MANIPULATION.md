# INDIGO Property Manipulation in Examples
Revision: 05.03.2023 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumenastro@gmail.com*

## INDIGO Concepts

INDIGO is a platform for the communication between different software entities over a software **bus**.
These entities are typically either in a **device** or in a **client** role, but there are special
entities often referred as **agents** which act as both **devices** and **clients**.

INDIGO is asynchronous in its nature, so to be able to communicate over the **bus** a set of
properties are defined for each device. The communication between different entities attached to the
**bus** is done by INDIGO **messages** which contain **property** events. Properties on the other
hand contain one or more **items** of a specified type. One can think of the properties as both set
of variables and routines. Set of variables as they may store values like the *width* and *height*
of the CCD frame or routines like *start 1 second exposure*. Messages sent over the INDIGO **bus**
are abstractions of the INDI messages.

The messages sent from the **device** to a **client** may contain one of the following events:
- **definition** of a property - lets the client know that the property can be used, the message contains
the property definition: name, type, list of items, etc...
- **update** of property item values - lets the client know that certain property items have changed their values.
- **deletion** of a property - lets the client know that the property is removed and can not be used any more.

The messages sent from a **client** to the **device** can be:
- **request for enumeration** of available properties - client can request the definitions of one or all available
properties associated with the device.
- **request for change** of property item values - client can request change of the values of one or several property items.

There are three classes of properties: **mandatory**, **optional**, and **device specific**. Each device class has a set of **mandatory** properties, that must always be defined and a set of **optional** properties, that may or may not be defined, depending on if this device class feature is supported or not. The list of the well known properties is available
in [PROPERTIES.md](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/PROPERTIES.md).

Each device may have a set of **device specific** properties which exist only if there is a need to control some device specific features. In general clients should not care about the **device specific** properties.

Different instances of the INDIGO **bus** can be connected in a hierarchical structure, but from a **driver** or a **client**
point of view it is fully transparent.

## Properties

Standard property names are defined in
[indigo_names.h](https://github.com/indigo-astronomy/indigo/blob/master/indigo_libs/indigo/indigo_names.h)

### Property States, Types and Permissions

Properties can be in one of the four states:
- **IDLE** - the values may not be initialized
- **OK** - the property item values are valid and it is safe to read or set them
- **BUSY** - the values are not reliable, some operation is in progress (like exposure is in progress)
- **ALERT** - the values are not reliable, some operation has failed or the values set are not valid

Each property has a predefined type which is one of the following:
- **TEXT_VECTOR** - strings of limited width
- **NUMBER_VECTOR** - floating point numbers with defined min and max values and increment
- **SWITCH_VECTOR** - logical values representing “on” and “off” state, there are several behavior rules for this type: *ONE_OF_MANY*
(only one switch can be "on" at a time), *AT_MOST_ONE* (none or one switch can be "on" at a time) and *ANY_OF_MANY*
(independent checkbox group)
- **LIGHT_VECTOR** - status values with four possible values **IDLE**, **OK**, **BUSY** and **ALERT**
- **BLOB_VECTOR** - binary data of any type and any length usually image data

Properties have permissions assigned to them:
- **RO** - Read only permission, which means that the client can not modify their item values
- **RW** - Read and write permission, client can read the value and can change it
- **WO** - Write only permission, client can change it but can not read its value (can be used for passwords)

## Command Line Property Manipulation Tool: **indigo_prop_tool**

**indigo_prop_tool** is a console based tool that allows users to configure the indigo_server and to set/get item values and get property states and even taking exposures and saving the images.
Here is the indigo_prop_tool help:
```
indigo@indigosky:~ $ indigo_prop_tool -h
INDIGO property manipulation tool v.2.0-226 built on Mar  5 2023 23:30:32.
usage: indigo_prop_tool [options] device.property.item=value[;item=value;..]
       indigo_prop_tool set [options] device.property.item=value[;item=value;..]
       indigo_prop_tool set_script [options] agent.property.SCRIPT=filename[;NAME=filename]
       indigo_prop_tool get [options] device.property.item[;item;..]
       indigo_prop_tool get_state [options] device.property
       indigo_prop_tool list [options] [device[.property]]
       indigo_prop_tool list_state [options] [device[.property]]
       indigo_prop_tool discover [options]
set write-only BLOBs:
       indigo_prop_tool set [options] device.property.item=filename[;NAME=filename]
options:
       -h  | --help
       -b  | --save-blobs
       -l  | --use_legacy_blobs
       -e  | --extended-info
       -v  | --enable-log
       -vv | --enable-debug
       -vvv| --enable-trace
       -r  | --remote-server host[:port]   (default: localhost)
       -p  | --port port                   (default: 7624)
       -T  | --token token
       -t  | --time-to-wait seconds        (default: 2)
```

The commands are self-explanatory:
- **set** or no command - set listed property items
- **get** - get listed item values
- **get_state** - get the property state
- **list_state** - list the state of the properties
- **discover** - auto discover and list INDIGO services visible on the network

### Listing and Getting Item Values and Property States

The main difference between **get** and **list** commands is that **list** will show more verbose
output and can show several values or states while **get** is more script friendly and
will show only the explicitly requested values:
```
indigo@indigosky:~ $ indigo_prop_tool list "CCD Imager Simulator.INFO"
CCD Imager Simulator.INFO.DEVICE_NAME = "CCD Imager Simulator"
CCD Imager Simulator.INFO.DEVICE_VERSION = "2.0.0.8"
CCD Imager Simulator.INFO.DEVICE_INTERFACE = "2"

indigo@indigosky:~ $ indigo_prop_tool get "CCD Imager Simulator.INFO.DEVICE_VERSION"
2.0.0.8

indigo@indigosky:~ $ indigo_prop_tool get "CCD Imager Simulator.INFO.DEVICE_VERSION;DEVICE_NAME"
2.0.0.8
CCD Imager Simulator
```

The same applies for **get_state** and **list_state**. They will return the requested property state:
```
indigo@indigosky:~ $ indigo_prop_tool get_state "CCD Imager Simulator.INFO"
OK

indigo@indigosky:~ $ indigo_prop_tool list_state "CCD Imager Simulator.INFO"
CCD Imager Simulator.INFO = OK
```

List without parameters will show all devices with all properties available on the INDIGO bus:
```
indigo@indigosky:~ $ indigo_prop_tool list
Server.INFO.VERSION = "2.0-123"
Server.INFO.SERVICE = "indigosky"
Server.DRIVERS.indigo_aux_cloudwatcher = ON
Server.DRIVERS.indigo_focuser_focusdreampro = OFF
Server.DRIVERS.indigo_agent_alignment = OFF
Server.DRIVERS.indigo_ccd_altair = OFF
...
```

### Getting Extended Property Information

More information about properties, like types, permissions, etc., can be obtained
using **-e** switch with **list** or **list_state** command:
```
ndigo@indigosky:~ $ indigo_prop_tool list -e "CCD Imager Simulator.CCD_EXPOSURE"
Protocol version = 2.0

Name : CCD Imager Simulator.CCD_EXPOSURE (RW, NUMBER_VECTOR)
State: OK
Group: Camera
Label: Start exposure
Items:
CCD Imager Simulator.CCD_EXPOSURE.EXPOSURE = 0.000000

indigo@indigosky:~ $ indigo_prop_tool list_state -e "CCD Imager Simulator.CCD_EXPOSURE"
Protocol version = 2.0

Name : CCD Imager Simulator.CCD_EXPOSURE (RW, NUMBER_VECTOR)
Group: Camera
Label: Start exposure
CCD Imager Simulator.CCD_EXPOSURE = OK
```

### Setting Item Values

The values of the items can be set by the client only on **RW** and **WO** properties, which are not a **LIGHT_VECTOR** or a **BLOB_VECTOR**. This is achieved by **set** command.

- Setting **SWITCH_VECTOR**:
```
indigo@indigosky:~ $ indigo_prop_tool set "CCD Imager Simulator.CONNECTION.CONNECTED=ON"
CCD Imager Simulator.CONNECTION.CONNECTED = ON
CCD Imager Simulator.CONNECTION.DISCONNECTED = OFF
```

- Setting **TEXT_VECTOR** (please note quotes have to be escaped with '\' as shown):
```
indigo@indigosky:~ $ indigo_prop_tool set "CCD Imager Simulator.CCD_SET_FITS_HEADER.NAME=\"MYKEY\";VALUE=\"5\""
CCD Imager Simulator.CCD_SET_FITS_HEADER.NAME = "MYKEY"
CCD Imager Simulator.CCD_SET_FITS_HEADER.VALUE = "5"
```

- Setting **NUMBER_VECTOR**:
```
indigo@indigosky:~ $ indigo_prop_tool set "CCD Imager Simulator.CCD_FRAME.WIDTH=1000;HEIGHT=800"
CCD Imager Simulator.CCD_FRAME.LEFT = 0.000000
CCD Imager Simulator.CCD_FRAME.TOP = 0.000000
CCD Imager Simulator.CCD_FRAME.WIDTH = 1000.000000
CCD Imager Simulator.CCD_FRAME.HEIGHT = 800.000000
CCD Imager Simulator.CCD_FRAME.BITS_PER_PIXEL = 16.000000
```

If you specify more than one item, like in the example above, you should separate them with ';'.

### Working with remote servers

By default **indigo_prop_tool** works with localhost and INDIGO's default port 7624. In case you need to set properties on remote servers or on servers running on a different port, **-r** and **-p** switches should be used. A list of INDIGO services available on the network can be obtained using the **discover** command:
```
indigo@indigosky:~ $ indigo_prop_tool discover
sirius -> sirius.local:7624
vega -> vega.local:7624
indigosky -> indigosky.local:7624
Rumens-Mini-2 -> Rumens-Mac-mini-2.local:7624
```

Service 'sirius' can be accessed at host 'sirius.local' and port 7624 as shown:
```
indigo@indigosky:~ $ indigo_prop_tool list -r sirius.local:7624 "Server"
Server.INFO.VERSION = "2.0-123"
Server.INFO.SERVICE = "sirius"
...
```
If **-e** switch is specified to the **discover** command each service will be listed with the network interface through which the service can be reached.

## A Working Example 1 - Loading and Unloading a Driver in indigo_server

There are two ways to achieve this goal. The easiest way is to set the driver's switch to **ON** or **OFF** in the **Server.DRIVERS** property, and the second way is to provide driver's name to **Server.LOAD** or **Server.UNLOAD** respectively.

Let us assume we need **indigo_ccd_simulator** loaded and **indigo_wheel_asi** unloaded.

First we need to list the drivers that the server sees:
```
indigo@indigosky:~ $ indigo_prop_tool list Server.DRIVERS
Server.DRIVERS.indigo_aux_cloudwatcher = OFF
Server.DRIVERS.indigo_focuser_focusdreampro = OFF
Server.DRIVERS.indigo_agent_alignment = OFF
Server.DRIVERS.indigo_ccd_altair = OFF
...
Server.DRIVERS.indigo_ccd_simulator = OFF
...
Server.DRIVERS.indigo_mount_simulator = OFF
...
Server.DRIVERS.indigo_wheel_asi = ON
Server.DRIVERS.indigo_focuser_asi = OFF
Server.DRIVERS.indigo_guider_asi = OFF
indigo@indigosky:~ $
```

We see that **indigo_ccd_simulator** is not loaded and **indigo_wheel_asi** is loaded.
This means we need to load **indigo_ccd_simulator** and unload **indigo_wheel_asi**:
```
indigo@indigosky:~ $ indigo_prop_tool set "Server.DRIVERS.indigo_ccd_simulator=ON;indigo_wheel_asi=OFF"
...
Server.DRIVERS.indigo_ccd_simulator = ON
...
Server.DRIVERS.indigo_wheel_asi = OFF
...
indigo@indigosky:~ $
```
Now we see that the **indigo_ccd_simulator** driver is loaded and **indigo_wheel_asi** is not.

Maybe it is a good idea to have the mount simulator also loaded. Let us do this the other way, by using **Server.LOAD** property.
If for some reason the driver is not listed by the server in **Server.DRIVERS**, e. g. because it is not in the standard path, this is the only way to load it:
```
indigo@indigosky:~ $ indigo_prop_tool set "Server.LOAD.DRIVER=indigo_mount_simulator"
...
Server.DRIVERS.indigo_ccd_simulator = ON
...
Server.DRIVERS.indigo_mount_simulator = ON
...
Server.LOAD.DRIVER = "indigo_mount_simulator"
```
Now we have **indigo_mount_simulator** loaded.

## A Working Example 2 - Taking Exposure and Saving The Image.

Let us take exposure with the **Imager Simulator** device provided by the already loaded **indigo_ccd_simulator** driver but let us make it native for PixInsight and use *XISF* image format.

First We need to connect that device if not already connected:
```
indigo@indigosky:~ $ indigo_prop_tool list "CCD Imager Simulator.CONNECTION"
CCD Imager Simulator.CONNECTION.CONNECTED = OFF
CCD Imager Simulator.CONNECTION.DISCONNECTED = ON
```

It is not connected, we must connect it:
```
indigo@indigosky:~ $ indigo_prop_tool set "CCD Imager Simulator.CONNECTION.CONNECTED=ON"
CCD Imager Simulator.CONNECTION.CONNECTED = ON
CCD Imager Simulator.CONNECTION.DISCONNECTED = OFF
CCD Imager Simulator.CONNECTION.CONNECTED = ON
CCD Imager Simulator.CONNECTION.DISCONNECTED = OFF
```

Next, make sure that we are using *XISF* format:
```
indigo@indigosky:~ $ indigo_prop_tool list "CCD Imager Simulator.CCD_IMAGE_FORMAT"
CCD Imager Simulator.CCD_IMAGE_FORMAT.FITS = ON
CCD Imager Simulator.CCD_IMAGE_FORMAT.XISF = OFF
CCD Imager Simulator.CCD_IMAGE_FORMAT.RAW = OFF
CCD Imager Simulator.CCD_IMAGE_FORMAT.JPEG = OFF
```

As we can see *FITS* is selected, we need to select *XISF*:
```
indigo@indigosky:~ $ indigo_prop_tool set "CCD Imager Simulator.CCD_IMAGE_FORMAT.FITS=OFF;XISF=ON"
CCD Imager Simulator.CCD_IMAGE_FORMAT.FITS = OFF
CCD Imager Simulator.CCD_IMAGE_FORMAT.XISF = ON
CCD Imager Simulator.CCD_IMAGE_FORMAT.RAW = OFF
CCD Imager Simulator.CCD_IMAGE_FORMAT.JPEG = OFF
indigo@indigosky:~ $
```

Now we are ready to take an exposure. This is the tricky part. By default the image related events (BLOB updates) are not delivered to the clients, so we need to register our client explicitly in order to receive them. In **indigo_prop_tool** this is done with the **-b** switch, which also saves the image to a file. Another tricky part is that we have to wait for the exposure to complete, in order to receive the image. To do so, we need to specify a timeout, which is longer than the exposure time, using **-t** switch. Exposure itself is taken by setting **CCD_EXPOSURE.EXPOSURE** to the desired duration. The value is in seconds. When the exposure is ready, the **CCD_EXPOSURE.EXPOSURE** value will become 0 again, **CCD_EXPOSURE** state will be set to **OK** and the image data will be available in **CCD_IMAGE.IMAGE**.

Let us get the frame:
- request BLOB updates with **-b**
- set the timeout to 5 seconds with **-t 5**
- start the 4 second exposure by setting **CCD_EXPOSURE.EXPOSURE=4**
```
indigo@indigosky:~ $ indigo_prop_tool set -b -t 5 "CCD Imager Simulator.CCD_EXPOSURE.EXPOSURE=4"
CCD Imager Simulator.CCD_EXPOSURE.EXPOSURE = 4.000000
CCD Imager Simulator.CCD_EXPOSURE.EXPOSURE = 3.000000
CCD Imager Simulator.CCD_EXPOSURE.EXPOSURE = 2.000000
CCD Imager Simulator.CCD_EXPOSURE.EXPOSURE = 1.000000
CCD Imager Simulator.CCD_EXPOSURE.EXPOSURE = 0.000000
CCD Imager Simulator.CCD_IMAGE.IMAGE = <http://localhost:7624/blob/0xaf68b730.xisf => CCD Imager Simulator.CCD_IMAGE.IMAGE.xisf>
CCD Imager Simulator.CCD_EXPOSURE.EXPOSURE = 0.000000
```
Et voila, we have the image in *CCD Imager Simulator.CCD_IMAGE.IMAGE.xisf* file let us check it:
```
indigo@indigosky:~ $ ls -l *.xisf
-rw-r--r-- 1 indigo indigo 3842880 Jul 23 12:41 'CCD Imager Simulator.CCD_IMAGE.IMAGE.xisf'
indigo@indigosky:~ $
```
Now we can open the image in PixInsight.
