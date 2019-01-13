[![Build Status](https://travis-ci.org/indigo-astronomy/indigo.svg?branch=master)](https://travis-ci.org/indigo-astronomy/indigo)
[![Tag Version](https://img.shields.io/github/tag/indigo-astronomy/indigo.svg)](https://github.com/indigo-astronomy/indigo/tags)
[![License](http://img.shields.io/badge/INDIGO-license-red.svg)](https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md)

# INDIGO is the next generation of INDI, based on layered architecture and software bus.

This is the list of requirements taken into the consideration:

1. No GPL dependency to allow commercial use under application stores licenses.
2. ANSI C for portability and to allow simple wrapping into .Net, Java, GOlang, Objective-C or Swift in future
3. Layered architecture to allow direct linking of the drivers into applications and or to replace wire protocol.
4. Atomic approach to device drivers. E.g. if camera has imaging and guiding chip, driver should expose two independent simple devices instead of one complex. It is much easier and transparent for client.
5. Drivers should support hot-plug at least for USB devices. If device is connected/disconnected while driver is running, its properties should appear/disappear on the bus.
6. FITS, XISF, JPEG and RAW format supported directly by the framework.

## This is already done

#### Framework

1. INDIGO Bus framework
2. XML & JSON protocol adapter for client and server side
3. indigo_server - server with loadable (indigo .so/.dylib and indi executables) drivers
4. indigo_server_standalone - server with built-in drivers
5. INDIGO Server for macOS wrapper (provided just as an example)
6. Integrated HTTP server for BLOB download & server control (= web based INDI control panel)
7. indigo_prop_tool - command line tool to list and set properties

#### Drivers

1. CCD (with wheel, guider, AO and focuser) simulator
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
14. FLI filter wheel driver
15. FLI CCD driver
16. FLI focuser driver (testers needed)
17. USB_Focus v3 driver
18. SBIG CCD driver (with guider, guider CCD and external guider head)
19. SBIG Filter wheel driver (part of CCD driver)
20. ASCOM driver for INDIGO Camera
21. ASCOM driver for INDIGO Filter Wheel
22. QHY CCD driver (NOTE: Maybe unstable due to inherited instability in the QHY SDK)
23. ZWO USB-ST4 port
24. Meade DSI Camera driver
25. Takahashi Temma mount driver
26. ICA (ImageCapture API) driver for DSLRs (Mac only)
27. GPS Simulator
28. NMEA 0183 GPS driver
29. Andor CCD driver (32/64-bit Intel Linux only)
30. WeMacro Rail focuser driver (platform independent USB & Mac only bluetooth)
31. EQMac guider driver (Mac only)
32. Apogee CCD driver
33. Moravian Intruments CCD and filter wheel driver
34. HID Joystick AUX driver
35. CGUSBST4 guider driver
36. Brightstar Quantum filter wheel driver (untested)
37. Trutek filter wheel driver (untested)
38. Xagyl filter wheel driver (untested)
39. Optec filter wheel driver (untested)
40. Pegasus DMFC focuser driver
41. RigelSys nSTEP focuser driver
42. RigelSys nFOCUS focuser driver (untested)
43. iOptron mount driver
44. MoonLite focuser driver (untested)
45. MJKZZ rail focuser driver (untested, platform independent USB & Mac only bluetooth)
46. GPhoto2 CCD driver
47. Optec focuser driver (untested)
48. ToupTek CCD driver
49. AltairAstro CCD driver
50. RTS-on-COM aux (shutter) driver (untested)
51. DSUSB aux (shutter) driver (untested)
52. GPUSB guider driver (untested)
53. LakesideAstro focuser (untested)

## This is under development
1. ASCOM driver for INDIGO mount
2. ASCOM driver for INDIGO focuser
3. Apogee filter wheel driver
4. SynScan (EQMod) mount driver
5. ASCOL driver

------------------------------------------------------------------------------------------------
## How to build INDIGO

### Prerequisits
#### Ubuntu / Debian / Raspbian

`sudo apt-get install build-essential autoconf autotools-dev libtool cmake libudev-dev libavahi-compat-libdnssd-dev libusb-1.0-0-dev fxload libcurl4-gnutls-dev libgphoto2-dev git curl`

It is advised to remove libraw1394-dev

`sudo apt-get remove libraw1394-dev`

#### Fedora

`dnf install automake autoconf cmake libtool gcc gcc-c++ libusb-devel avahi-compat-libdns_sd-devel libudev-devel libgphoto2-devel git curl`

It is advised to remove libraw1394-devel

`dnf remove libraw1394-devel`

#### macOS

Install XCode and download and build autoconf, automake and libtool (use tools/cltools.sh script).

### Get code and build it

`git clone https://github.com/indigo-astronomy/indigo.git`

`cd indigo`

`make all`

`build/bin/indigo_server -v -s`

and connect from any INDIGO/INDI client or web browser to localhost on port 7624...
