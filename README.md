[![Build Status](https://travis-ci.org/indigo-astronomy/indigo.svg?branch=master)](https://travis-ci.org/indigo-astronomy/indigo)
[![GitHub tag (latest by date)](http://img.shields.io/github/v/tag/indigo-astronomy/indigo)](https://github.com/indigo-astronomy/indigo/blob/master/CHANGELOG.md)
[![License](http://img.shields.io/badge/license-INDIGO-blueviolet.svg)](https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md)
[![Platform](http://img.shields.io/badge/platform-linux%20%7C%20macos%20%7C%20windows-success.svg)](#)

# INDIGO is the next generation of INDI, based on layered architecture and software bus.

This is the list of requirements taken into the consideration:

1. No GPL dependency to allow commercial use under application stores licenses.
2. ANSI C for portability and to allow simple wrapping into .Net, Java, GOlang, Objective-C or Swift in future
3. Layered architecture to allow direct linking of the drivers into applications and or to replace wire protocol.
4. Atomic approach to device drivers. E.g. if camera has imaging and guiding chip, driver should expose two independent simple devices instead of one complex. It is much easier and transparent for client.
5. Drivers should support hot-plug at least for USB devices. If device is connected/disconnected while driver is running, its properties should appear/disappear on the bus.
6. FITS, XISF, JPEG, TIFF, RAW, AVI and SER format supported directly by the framework.

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
13. LX200 mount driver (supports Meade, Avalon, Losmandy, 10microns, AstroPhysics, ZWO and PegasusAstro mounts and EQMac)
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
26. ICA (ImageCapture API) driver for DSLRs (Mac only, deprecated)
27. GPS Simulator
28. NMEA 0183 GPS driver
29. Andor CCD driver (32/64-bit Intel Linux only)
30. WeMacro Rail focuser driver (platform independent USB & Mac only bluetooth)
31. EQMac guider driver (Mac only, deprecated)
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
44. MoonLite focuser driver
45. MJKZZ rail focuser driver (untested, platform independent USB & Mac only bluetooth)
46. GPhoto2 CCD driver (deprecated, excluded from build)
47. Optec focuser driver (untested)
48. ToupTek CCD driver
49. AltairAstro CCD driver
50. RTS-on-COM aux (shutter) driver (untested)
51. DSUSB aux (shutter) driver
52. GPUSB guider driver
53. LakesideAstro focuser (untested)
54. SX AO driver
55. SBIG AO driver (part of CCD driver)
56. SynScan (EQMod) mount driver
57. ASCOL driver
58. ASI Focuser driver
59. DeepSky Dad AF1 and AF2 focuser driver
60. Baader Planetarium SteelDrive II focuser driver
61. Unihedron SQM driver
62. Artesky Flat Box USB driver
63. NexDome dome driver (untested, based on G.Rozema's firmware)
64. USB_Dewpoint V1 and V2 AUX driver
65. Pegasus Astro FlatMaster driver
66. AstroGadget FocusDreamPro / ASCOM Jolo focuser driver added
67. Lacerta Flat Box Controller AUX driver
68. UVC (USB Video Class) CCD driver
69. NexDome v3 dome driver (untested, requires firmware v.3.0.0 or newer)
70. Optec Flip-Flap driver
71. GPS Service Daemon (GPSD) driver
72. Lunatico Astronomy Limpet/Armadillo/Platypus Focuser/Rotator/Powerbox/GPIO Drivers
73. Lunatico Astronomy Limpet/Armadillo/Platypus Rotator/Focuser/Powerbox/GPIO driver
74. Lunatico Astronomy Dragonfly Dome / Relay Controller GPIO driver
75. Rotator Simulator Driver
76. Lunatico AAG CloudWacher driver
77. Baader Planetarium Classic (Rotating) dome driver
78. MGBox driver
79. Manual wheel driver
80. PMC8 mount controller driver
81. RoboFocus focuser driver
82. QHY CFW filter wheel driver
83. RainbowAstro Mount driver
84. NexStar AUX protocol mount driver
85. Lunatico Astronomy AAG CloudWatcher driver
86. MyFocuserPro 2 focuser driver
87. Interactive Astronomy SkyRoof driver
88. ASTROMECHANICS focuser driver
89. Nexdome Beaver dome driver
90. ASTROMECHANICS Light Pollution Meter PRO driver
91. Optec Pyxis rotator driver
92. SvBony camera driver
93. Geoptik flat field generator driver
94. Talon 6 dome driver
95. Interactive Astronomy SkyAlert driver
96. Vixen StarBook mount driver
97. PlayerOne camera driver
98. PegasusAstro Prodigy Microfocuser driver
99. PegasusAstro Indigo wheel driver
100. PegasusAstro Falcon rotator driver
101. OmegonPro CCD driver
102. MallinCam CCD driver
103. RisingCam CCD driver
104. Orion StarShotG CCD driver
105. OGMA CCD driver
106. PrimaLuceLab focuser driver
107. ZWO ASIAIR Power Ports
108. Wanderer Astro WandererBox Plus V3
109. Wanderer Astro WandererBox Pro V3
110. Wanderer Astro WandererCover V4-EC
111. iOptron iEAF focuser driver
112. Optec FocusLynx focuser driver
113. PegasusAstro FocusCube3 driver
114. Lacerta LACERTA Motorfocus driver
115. Baccam (Touptek OEM) camera driver
116. Meade (Touptek OEM) camera driver

## This is under development
1. a-Box Adaptive optics driver

------------------------------------------------------------------------------------------------
## How to build INDIGO

### Prerequisites
#### Ubuntu / Debian / Raspbian

`sudo apt-get install build-essential autoconf autotools-dev libtool cmake libudev-dev libavahi-compat-libdnssd-dev libusb-1.0-0-dev libcurl4-gnutls-dev libz-dev git curl bsdmainutils bsdextrautils patchelf`

It is advised to remove libraw1394-dev

`sudo apt-get remove libraw1394-dev`

#### Fedora

`dnf install automake autoconf cmake libtool gcc gcc-c++ avahi-compat-libdns_sd-devel libudev-devel git curl curl-devel zlib-devel libusb-compat-0.1-devel systemd-devel avahi-compat-libdns_sd-devel libcurl-devel patchelf`

It is advised to remove libraw1394-devel

`dnf remove libraw1394-devel`

#### macOS

Install XCode and download and build autoconf, automake and libtool (use tools/cltools.sh script).

### Get code and build it

`git clone https://github.com/indigo-astronomy/indigo.git`

`cd indigo`

`make all`

`build/bin/indigo_server -v indigo_ccd_simulator [other drivers]`

and connect from any INDIGO/INDI client or web browser to localhost on port 7624...

### No pthread_yield()
New linux distributions come with the latest glibc that does not provide pthread_yield() However libqhy.a depends on it. We do not have control over this library and it is not officially supprted by QHY any more. So if you get complaints by the linker for missing *pthread_yield* call please execute the folowing command in *indigo_drivers/ccd_qhy/bin_externals/pthread_yield_compat/*:

`make patchlib`

and rerun the build
