# INDIGO is a proof-of-concept of future INDI based on layered architecture and software bus.

This is the list of requirements taken into the consideration:

1. No GPL dependency to allow commercial use under application stores licenses.
2. ANSI C for portability and to allow simple wrapping into .Net, Java, GOlang, Objective-C or Swift in future
3. Layered architecture to allow direct linking of the drivers into applications and or to replace wire protocol.
4. Atomic approach to device drivers. E.g. if camera has imaging and guiding chip, driver should expose two independent simple devices instead of one complex. It is much easier and transparent for client.
5. Drivers should support hot-plug at least for USB devices. If device is connected/disconnected while driver is running, its properties should appear/disappear on the bus.

## This is already done

#### Framework

1. INDIGO Bus framework
2. XML adapter for client and server side
3. indigo_server with built-in drivers
4. INDIGO Server for macOS wrapper
5. Integrated HTTP server for BLOB download & server control (= web based INDI control panel)

#### Drivers

1. CCD (with wheel, guider and focuser) simulator
2. Mount simulator
3. Atik CCD (Titan, 3xx/4xx/One with built in wheel, VS/Infinity, 11000/4000) driver
4. SX CCD driver
5. SX wheel driver
6. Shoestring FCUSB focuser driver
7. SSAG/QHY5 CCD driver 
8. ASI wheel driver
9. IIDC CCD driver
10. ASI CCD driver

## This is under development

1. QHY (5II-L) CCD driver

------------------------------------------------------------------------------------------------

##To build PoC

### Prerequisits
#### Ubuntu / Debian

`sudo apt-get install build-essential autotools-dev libtool libudev-dev libavahi-compat-libdnssd-dev`

It is advised to remove libraw1394-dev

`sudo apt-get remove libraw1394-dev`

#### Fedora

TBD - see ubuntu and install the corresponding packages.

### Get code and build it

`git clone https://github.com/indigo-astronomy/indigo.git`

`cd indigo`

`make all`

`bin/indigo_server_standalone`

or

`bin/indigo_server indigo_ccd_asi [other drivers]`

and connect from any INDI client to port 7624...
