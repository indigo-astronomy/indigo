<<<<<<< HEAD
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
3. indigo_server_standalone - server with built-in drivers
4. indigo_server - server with loadable (.so/.dylib) drivers
5. INDIGO Server for macOS wrapper
6. Integrated HTTP server for BLOB download & server control (= web based INDI control panel)

#### Drivers

1. CCD (with wheel, guider and focuser) simulator
2. Mount simulator
3. Atik CCD (Titan, 3xx/4xx/One with built in wheel, VS/Infinity, 11000/4000) driver
4. Atik EFW2 filterwheel driver
5. SX CCD driver
6. SX wheel driver
7. Shoestring FCUSB focuser driver
8. SSAG/QHY5 CCD driver 
9. ASI wheel driver
10. IIDC CCD driver
11. ASI CCD driver
12. NexStar mount driver (supports Celestron NexStar and Sky-Watcher mounts)
13. LX200 mount driver (supports Meade mounts and EQMac)

## This is under development

1. QHY (5II-L) CCD driver

------------------------------------------------------------------------------------------------

##To build PoC

### Prerequisits
#### Ubuntu / Debian / Raspbian

`sudo apt-get install build-essential autotools-dev libtool libudev-dev libavahi-compat-libdnssd-dev`

It is advised to remove libraw1394-dev

`sudo apt-get remove libraw1394-dev`

#### Fedora

TBD - see ubuntu and install the corresponding packages.

#### macOS

Install XCode and download and build autoconf, automake and libtool (use tools/cltools.sh script). 

### Get code and build it

`git clone https://github.com/indigo-astronomy/indigo.git`

`cd indigo`

`make all`

`bin/indigo_server_standalone`

or

`bin/indigo_server indigo_ccd_asi [other drivers]`

and connect from any INDI client to port 7624...
=======
# libindi
[![Build Status](https://travis-ci.org/indilib/indi.svg?branch=master)](https://travis-ci.org/indilib/indi)

INDI is the defacto standard for open astronomical device control. INDI Library is an Open Source POSIX implementation of the [Instrument-Neutral-Device-Interface protocol](http://www.clearskyinstitute.com/INDI/INDI.pdf). The library is composed of server, tools, and device drivers for astronomical instrumentation and auxiliary devices. Core device drivers are shipped with INDI library by default. 3rd party drivers are also available in the repository and maintained by their respective owners.

## [Features](http://indilib.org/about/features.html)
## [Discover INDI](http://indilib.org/about/discover-indi.html)
## [Supported Devices](http://indilib.org/devices/)
## [Clients](http://indilib.org/about/clients.html)

# Building

## Install Pre-requisites

On Debian/Ubuntu:

```
sudo apt-get install libnova-dev libcfitsio3-dev libusb-1.0-0-dev zlib1g-dev libgsl0-dev build-essential cmake git libjpeg-dev libcurl4-gnutls-dev
```
## Get the code
```
git clone https://github.com/indilib/indi.git
cd indi
```
## Build libindi

```
mkdir -p build/libindi
cd build/libindi
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../../libindi
make
sudo make install
```

## Build 3rd party drivers

You can build the all the 3rd party drivers at once (not recommended) or build the required 3rd party driver as required (recommended). Each 3rd party driver may have its own pre-requisites and requirements. For example, to build INDI EQMod driver:

```
cd build
mkdir indi-eqmod
cd indi-eqmod
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../../3rdparty/indi-eqmod
make
sudo make install
```

The complete list of system dependancies for all drivers on Debian / Ubuntu

```
sudo apt-get install libftdi-dev libgps-dev dcraw libgphoto2-dev libboost-dev libboost-regex-dev
```

To build **all** 3rd party drivers, you need to run cmake and make install **twice**. First time is to install any dependencies of the 3rd party drivers (for example indi-qsi depends on libqsi), and second time to install the actual drivers themselves.

```
cd build
mkdir 3rdparty
cd 3rdparty
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../../3rdparty
make
sudo make install
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../../3rdparty
make
sudo make install
```

# Architecture


Typical INDI Client / Server / Driver / Device connectivity:


    INDI Client 1 ----|                  |---- INDI Driver A  ---- Dev X
                      |                  |
    INDI Client 2 ----|                  |---- INDI Driver B  ---- Dev Y
                      |                  |                     |
     ...              |--- indiserver ---|                     |-- Dev Z
                      |                  |
                      |                  |
    INDI Client n ----|                  |---- INDI Driver C  ---- Dev T
    
    
     Client       INET       Server       UNIX     Driver          Hardware
     processes    sockets    process      pipes    processes       devices



INDI server is the public network access point where one or more INDI Clients may contact one or more INDI Drivers. indiserver launches each driver process and arranges for it to receive the INDI protocol from clients on its stdin and expects to find commands destined for clients on the driver's stdout. Anything arriving from a driver process' stderr is copied to indiserver's stderr. INDI server only provides convenient port, fork and data steering services. If desired, a client may run and connect to INDI Drivers directly.

# Support

## [FAQ](http://indilib.org/support/faq.html)
## [Forum](http://indilib.org/forum.html)
## [Tutorials](http://indilib.org/support/tutorials.html)

# Development

## [INDI API](http://www.indilib.org/api/index.html)
## [INDI Developer Manual](http://indilib.org/develop/developer-manual.html)
## [Tutorials](http://indilib.org/develop/tutorials.html)
## [![Join the chat at https://gitter.im/knro/indi](https://badges.gitter.im/knro/indi.svg)](https://gitter.im/knro/indi?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

# Unit tests

In order to run the unit test suite you must first install the [Google Test Framework](https://github.com/google/googletest). You will need to build and install this from source code as Google does not recommend package managers for distributing distros.(This is because each build system is often unique and a one size fits all aproach does not work well).

Once you have the Google Test Framework installed follow this alternative build sequence:-

```
mkdir -p build/libindi
cd build/libindi
cmake -DINDI_BUILD_UNITTESTS=ON -DCMAKE_BUILD_TYPE=Debug ../../libindi
make
cd test
ctest -V
```

For more details refer to teh scripts in the tavis-ci directory.

 
>>>>>>> latest/master
