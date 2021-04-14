# Changelog

All notable changes to INDIGO framework will be documented in this file.

## [2.0-146] - Wed Apr 14 2021
### Overall:
- compression added to HTTP BLOB transfer - image transfer can be about 2x faster
- introduce DOME_HOME and DOME_PARK_POSITION properties
- prepare the build for arm64 version of INDIGOSky
- platesolver agents: dec limit fixed
- fix indigo_dtos() showing negative degrees < 1 as positive
- HTTP server: dynamic content handling support added
- indigo_driver: save port and baud rate at property change
- additional serial ports removed from macOS list
- select() timeout fixed
- fixed memory leaks in mount base class
- DSLR JPEG->RAW conversion issue fixed
- debug log level setting fixed
- add indigo_compensate_backlash() call to be used in rotators and focusers

### New Drivers:
- indigo_agent_alpaca: Agent that exposes INDIGO devices to ASCOM/Alpaca clients.
- dome_talon6ror: driver for Talon 6 dome controller

### Driver Fixes:
- indigo_agent_imager:
	- focuser backlash and agent backlash properties synchronized

- indigo_agent_mount:
	- geographic coordinates propagation is limited by this threshold
	- synchronization fixed

- indigo_agent_astrometry:
	- index names made consistent
	- index management fixed
	- remote mount agent snooping fixed
	- solved message added

- indigo_guider_agent:
	- rename Integral stacking -> Integral stack size; docs updted

- indigo_mount_ioptron:
	- firmware 210105 detection added
	- driver refactoring
	- synchronization issues fixed
	- MSH used instead of MH for protocol 3.0
	- search for mechanical zero item added for 2.5 and 3.0 protocols

- indigo_mount_nexstar:
	- fix the busy state of the coordinate properties
	- fix equatorial tracking start / stop
	- fix tracking mode being stopped on connect

- insigo_ccd_altair:
	- SDK updated

- indigo_focuser_efa:
	- stack overrun fixed

- indigo_focuser_asi:
	- STEPS and POSITION states synchronized
	- SDK updated

- indigo_ccd_asi:
	- SDK updated

- indigo_wheel_asi:
	- SDK updated

- indigo_ccd_qhy:
	- firmware updated
	- READ_MODE change resets CCD_INFO, CCD_FRAME and CCD_MODE now
	- resolution fixed

- indigo_ccd_qhy2:
	- SDK and firmware updated
	- READ_MODE change resets CCD_INFO, CCD_FRAME and CCD_MODE now
	- resolution fixed

- indigo_wheel_qhy:
	- fix move Busy state

- indigo_focuser_dsd:
	- STEPS and POSITION states synchronized
	- software backlash compensation implemented

- indigo_focuser_fli:
	- STEPS and POSITION states synchronized

- indigo_focuser_lunatico:
	- STEPS and POSITION states synchronized

- indigo_focuser_mypro2:
	- STEPS and POSITION states synchronized
	- software backlash compensation implemented
	- load settings fixed

- indigo_dome_baader:
	- synchronized steps and horizontal coordinates states
	- fixed stale busy state in some situations

- indigo_dome_nexdome:
	- abort handled gracefully
	- fixed stale busy state in some situations

- indigo_dome_nexdome3:
	- abort handled gracefully
	- fixed property states

- indigo_aux_rpio:
	- replace GPIO 19 with GPIO 21
	- replace GPIO 04 with GPIO 19

- indigo_aux_upb:
	- outlet name items order fixed

- indigo_aux_ppb:
	- outlet names property name fixed

- indigo_ccd_simulator:
	- focuser absolute position setting enabled

- indigo_dome_simulator:
	- made shutter open and close timely operation

- indigo_wheel_quantum:
	- refactoring

- indigo_wheel_xagyl:
	- filter count fixed

- indigo_dome_skyroof:
	- fix abort state

- indigo_ccd_sbig:
	- driver now available for arm64 linux

## [2.0-144] - Sun Feb 28 2021
### Overall:
- add conditional deb dependency to indigo-astrometry or astrometry.net
- indigo_client: fix race condition
- weather & sky conditions properties standardized
- indigo_docs: GUIDING_PI_CONTROLLER_TUNING added

### New Drivers:
- indigo_dome_skyroof: Interactive Astronomy SkyRoof driver added
- insigo_aux_skyalert: Interactive Astronomy SkyAlert driver added

### Driver Fixes:
- indigo_agent_imager:
	- DSLR raw formats added to download code

- indigo_agent_guider:
	- multi-star selection mode added
	- donuts will repeat 3x in case of poor SNR
	- fix buffer size in indigo_find_stars_precise()
	- make drift detection more precise
	- new Proportional-Integral controller implementation, Proportional Weight is removed in favor of Integral gain

- indigo_agent_mount:
	- disconnect bug fixed
	- lx200 server cleanup

- indigo_mount_ioptron:
	- GEM45EC support added
	- MOUNT_CUSTOM_TRACKING_RATE support implemented
	- lunar tracking rate fixed
	- hc8407 support added
	- GEM45 support added

- indigo_mount_rainbow:
	- SYNC fixed
	- detach fixed

- indigo_ccd_asi:
	- BPP can be set from CCD_FRAME

- indigo_ccd_qsi:
	- fix CCD_TEMPERATURE handling

- indigo_ccd_touptek:
	- fix CCD_TEMPERATURE handling

- indigo_ccd_altair:
	- fix CCD_TEMPERATURE handling

- indigo_mount_lx200:
	- simulator fixed
	- aGotino support added

- indigo_ccd_simulator:
	- cooler control is consistent with real ccd drivers now

- indigo_dome_simulator:
	- state fixes

## [2.0-142] - Sat Jan 23 2021
### Overall:
- indigo_server: BLOB proxy added for remote servers
- WebGUI: fixed to work with proxied BLOBs
- various documentation updates
- agents can not change selected device while they are in use
- agent INTERFACE item can now e used to identify what devices are controlled
- agents crashes on device unload fixed
- devices opened by agents are closed on agent detach
- better star detection algorithm
- more precise version of find stars added
- XML client/driver cleanup
- JSON BLOB definition carries URL now
- long TEXT property support fixes
- property cache access synchronization in XML client added
- large buffers moved from stack to heap
- JPEG to RAW conversion added to indigo_process_dslr_image()
- several races and crashes fixed in indigo client
- fix several small memory leaks
- fix agents crash when some properties are deleted
- device detach code fixes: no need to explicitly call disconnect at detach

### New Drivers:
- indigo_agent_astrometry: Agent for plate solving using astrometry.net (indigo-astrometry should be installed).

### Driver Fixes:
- indigo_agent_imager:
	- sub-framing support added to preview process
	- sub-frame selection fixes
	- BLOB content race fixed

- indigo_agent_guider:
	- subframe selection fixes
	- make Donuts guider resilient to hot columns and rows
	- make Selection guide resilient to hot columns and rows
	- Donuts guider optimized
	- Edge Clipping added to Donuts guider
	- BLOB content race fixed
	- some parts are refactored

- indigo_agent_mount: AGENT_SET_HOST_TIME added

- indigo_ccd_uvc:
	- libuvc updated
	- mono formats supported
	- exposure countdown fixed
	- bug fixes

- indigo_ccd_ptp:
	- automatic use of LiveView during focusing
	- DSLR_EXPOSURE_METERING_PROPERTY_NAME on Canon fixed
	- high precision bulb time measurement
	- RAW format support added
	- Nikon focusing fixed

- indigo_mount_rainbow:
	- communication with the mount while it is not connected is fixed
	- lat/long format fixed

- indigo_ccd_simulator:
	- fix mixed width and height
	- DSLR simulator RAW format support added

- indigo_ccd_asi: update SDK v.1.16.3
- indigo_agent_scripting: enhancements
- indigo_aux_rpio: change pins which overlap with the default i2c


## [2.0-140] - Sun Dec 27 2020
### New Drivers:
- indigo_mount_rainbow: RainbowAstro mount driver
- indigo_agent_scripting: ECMAscript support for INDIGO


## [2.0-138] - Sun Dec 27 2020
### License Change
- INDIGO Astronomy open-source license version 2.0: Changed the license to more permissive and more compatible one.

### Overall:
- Added ECMAscript support for INDIGO through indigo_agent_scripting.
- Text property can have unlimited length
- Web GUI has support for scripting
- indigo_prop_tool has support for scripting
- Offset and Gamma added to XISF metadata
- Some JSON escaping fixed
- Memory leaks fixed in XML and JSON parsers and  indigo_ccd_driver
- JPEG preview stretching fixed for 16 bit images
- CCD_JPEG_SETTINGS_PROPERTY doesn't show and hide
- Data can be passed to timers using indigo_set_timer_with_data()
- indigo_resize_property() call fixed

### Driver Fixes:
- indigo_agent_guider:
	- fix guiding issue when with Declination guiding off
	- calculate correctly RMSE RA and Dec
	- dithering RMSE is cleared
	- preview and guiding state transition fixed
	- fix dithering issue

- indigo_agent_alignment: interface code fixed
- indigo_agent_imager: do not fail batch if the dithering failed to settle
- indigo_focuser_asi: workaround for SDK issue related to device hotplug applied
- indigo_mount_ioptron: park bug fixed
- indigo_aux_dsusb: dual arch macOS libraries added
- indigo_focuser_fcusb: dual arch macOS libraries added
- indigo_guider_gpusb: dual arch macOS libraries added
- indigo_wheel_atik: dual arch macOS libraries added

## [2.0-136] - Sat Dec 5 2020
### Overall:
- Optional automatic subframing added to autofocus and guiding processes in imager and guider agent
- More robust automatic stretching for JPEG preview
- CCD File Simulator device added to CCD Simulator driver (provides an image from specified RAW file)
- RGBA and ABGR raw format support added to guider utility functions
- Large files support added to RPi builds
- Histogram added to webGUI of imager agent
- Optional double buffering support added in INDIGO server for BLOB requests
- DEVICE_DRIVER added to INFO property (issue #132)
- DARKFLAT added to CCD_IMAGE_FORMAT property (issue #387)
- indigo_property_match_w() added
- fixed read only property handling in core library

### Driver Fixes:
- indigo_mount_nexstar:
	- experimental guide pulse support added
	- several bugfixes

- indigo_ccd_qhy2:
	- SDK updated

- indigo_ccd_altair:
	- SDK updated

- indigo_ccd_ptp:
	- Support for Canon EOS R5 and R6 cameras

- indigo_focuser_lunatico:
	- temperature compensation fixed

- indigo_focuser_dsd:
	- temperature compensation fixed

- indigo_focuser_asi:
	- temperature compensation fixed

### New Drivers:
- indigo_focuser_mypro2

## [2.0-134] - Mon Nov 16 2020
### Overall:
- Fix IndigoSky regression


## [2.0-132] - Sat Nov 14 2020
### Overall:
- More robust socket handling
- More robust FWHM calculation
- Internet sharing option added to IndigoSky build
- BLOB caching synchronization fixes
- AVI support added to DSLR drivers

### Driver Fixes:
- indigo_aux_cloudwatcher:
	- support for Pocket CW
	- support for older AAG units
	- bug fixes

- indigo_agent_guider: add steps to AGENT_GUIDER_SETTINGS

- indigo_ccd_ptp:
 	- Support for Fujifilm cameras
	- Sony camera fixes

- indigo_mount_pmc8: connection issues fixed
- indigo_mount_nexstar: StarSense related fixes
- indigo_ccd_qhy / indigo_ccd_qhy2: many driver and SDK fixes

### New Drivers:
- indigo_wheel_qhy: QHY standalone filter wheels driver
- indigo_mount_nexstaraux: Celestron Wifi mount drivers

### Obsoleted drivers
- indigo_ccd_gphoto2: removed from the build - indigo_ccd_ptp replaces it

## [2.0-130] - Sun Nov 01 2020
### Overall:
- TIFF image format added to CCD drivers
- AVI and SER format support added to streaming CCD drivers
- indigo_guider_utils: more robust HFD/FWHM calculation
- indigo_server_tcp: empty resource list bug fixed
- VERIFY_NOT_CONNECTED() macro fixed
- scientific number notation support added to JSON
- CLIENT_DEVELOPMENT_BASICS.md and INDIGO_AGENTS.md documents updated

### Driver Fixes:
- indigo_agent_imager:
  - report failure if focusing did not converge
  - more robust focusing
  - fix rare crash due to unitialized frame digest


- indigo_mount_nexstar:
  - set correct timezone and DST
  - GPS detection changed
  - deadlock fixed


- indigo_ccd_simulator:
  - focuser: more realistic out of focus images
  - bug fixes


- indigo_ccd_touptek: fix failing exposures problem
- indigo_ccd_altair: fix failing exposures problem
- indigo_mount_ioptron: CEM70 identification added
- indigo_focuser_usbv3: bug fixes


### New Drivers:
- wheel_qhy: QHY standalone filter wheel driver

## [2.0-128] - Mon Oct 05 2020
### Overall:
- Agents can select only devices that are not currently being used.
- Add star auto detection used for focusing and guiding via indigo_find_stars()
- Better HFD calculation with unlimited radius
- webGUI: auto focus, guiding mode and dithering added
- XISF metadata fixed and extended

### Driver Fixes:
- indigo_agent_guider:  many enhancements the main ones are:
  - Guiding now uses real Proportional-Integral (PI) controller. Explained in the driver README.md.
  - Dithering process auto detects when the guiding settled.
  - Selection guide: more robust, sensitive and stable implementation,
    a suitable star is auto selected if no selection is made,
    stars can be manually selected from a list too.
  - Donuts guide: more robust, sensitive and stable implementation
  - Centroid guide: more robust, sensitive and stable implementation
  - Code optimizations
  - Bug fixes


- indigo_agent_imager:
  - Auto select star for focusing process.
  - dithering with remote guider agent fixed
  - Bug fixes


- indigo_ccd_simulator:
  - simulate hotpixels and hot rows/columns
  - RA/DEC_OFFSET items added to GUIDER_SETTINGS to simulate random tracking errors
  - Bug fixes


- ccd_qhy/ccd_qhy2:
  - SDK & firmare updated
  - Remove hot-plug support - SDK does not really support this
  - increase buffer size for 60+ MPx cameras


- ccd_altair:
  - binning issues fixed
  - SDK updated to 48.17729.2020.0922


- ccd_touptek:
  - binning issues fixed
  - SDK updated to 46.17309.20200616


- indigo_ccd_asi: SDK updated to 1.15.0915
- indigo_ccd_ptp: bugfixes

## [2.0-126] - Wed Sep 01 2020
### Overal:
- alignment routines improved
- security token support fixed
- DLL build added to Windows makefile
- PSF calculation fixed

### Driver Fixes:
- indigo_ccd_atik: macOS Horizon issues fixed
- indigo_ccd_qsi: improved, fixed and tested with physical hardware
- indigo_mount_pmc8: protocol switching fixed
- indigo_ccd_ptp: Nikon Z5 support added
- indigo_ccd_simulator: streaming and BULB support added
- indigo_agent_mount: stops capture on related ccd agent on park etc.

## [2.0-124] - Sat Aug 01 2020
### Overal:
- Double connection issue fixed
- PSF computation fixed
- several executable drivers were not built
- indigo_server: better process handling
- added indigo_examples folder and created new examples
- Documentation update
- added user documentation about indigo_prop_tool and indigo_server
- fixed blob transfer for executable drivers
- blob transfer improvements - name resolution is done only once
- support ROWORDER keyword in fits.

### Driver Fixes:
- indigo_agent_guider: PSF radius added, fixes
- indigo_agent_imager: PSF radius added, fixes
- indigo_aux_ppb: PPB Advanced support added
- indigo_aux_sqm: crash fixed
- indigo_ccd_qsi: initial slot position fixed, added rules
- indigo_ccd_mi: SDK updated to 0.5.6
- indigo_ccd_asi: SDK updated to 1.15.0617
- indigo_wheel_asi: SDK updated to 1.5.0615
- indigo_focuser_asi: SDK updated to 0.2.0605
- indigo_mount_lx200: meridian flip control added to AvalonGO dialect
- indigo_ccd_atik: SDK updated to 2020_06_23
- indigo_ccd_qhy2: SDK updated to V2020.06.05 for macOS and  V2020.05.22 for linux

### New Drivers:
- indigo_mount_pmc8: PMC8 mount controller driver
- indigo_aux_rpio: Raspberry Pi GPIO driver

## [2.0-122] - Fri May 05 2020
### Overal:
- Device access control finished
- New more robust timer handling
- INDIGO_AGENT class added
- Dynamic libraries ARE NOT binary compatible due to changes in indigo_device structure
- Drivers can not be unloaded if any of the handled devices is connected

### Driver Fixes:
- ALL DRIVERS are updated due to the new timer handling
- indigo_efa_focuser: calibration added for celestron devices
- indigo_lx200_mount: unpark added for OneStep dialect, tracking status fixed, ...
- indigo_ioptron_mount: iEQ25 parking fixed, protocol 2.5+ guiding rate setting fixed, ...
- indigo_focuser_optec: relative move orientation fixed, reverse orientation property added
- indigo_focuser_dmfc: relative move response reading fixed
- indigo_aux_sqm: crash on lost connection fixed
- indigo_aux_rts: disconnect sequence fixed
- indigo_aux_ppb: dev heater initial target values fixed, P3/P4 fixed, ...
- indigo_aux_flipflat: light intensity reading fixed
- indigo_focuser_moonlite: timeout fixed
- indigo_ccd_ptp: D3000/3100 capture fixed
- indigo_ccd_atik: AtikOne wheel control fixed, new SDK used
- indigo_ccd_mi: support for new CMOS cameras added

### New Drivers:
- indigo_dome_baader
- indigo_aux_mgbox
- indigo_wheel_manual
- indigo_aux_cloudwatcher

## [2.0-120] - Sat Apr 04 2020
### Overall:
- New experimental feature: Add device access control
- indigo_server:
    - Fix deadlock on indigosky
    - Fix password trucation bug

### Driver Fixes:
- indigo_ccd_ptp: Sony camera related fixes
- indigo_mount_temma: Side of pier fixed

## [2.0-118] - Sun Mar 22 2020

### Overall:
- indigo framework: fix "&" character escape in XML

### Driver Fixes:
- indigo_ccd_ptp: Sony and Canon camera related fixes
- indigo_mount_temma: Add pier side update notify

### New Drivers:
- indigo_dome_dragonfly: Lunatico Astronomia Dragonfly Dome / Relay controller driver
- indigo_aux_dragonfly: Lunatico Astronomia Dragonfly Relay controller driver


## [2.0-116] - Mon Mar 9 2020

### Overall:
- indigosky fixes

### Driver fixes:
- lunatico drivers: show proper description

## [2.0-114] - Sat Mar 7 2020

### Overall:
- indigo_server: several fixes
- indigo_prop_tool: fix quotes management in set value
- indigo_framework:
    - added Rotator device support
    - added AUX GPIO (General puspose I/O device, to control relays, sensor
      arrays, switches etc.) device support
    - added several new utility functions
    - drivers can mutually exclude each other
    - usbfs_memory_mb fixed - no more failed frames with several camera connected
    - use updated version of fxload
- Developer Documentation updated

### Driver Fixes:
- indigo_mount_nexstar: fixes and worarounds for StarSense HC
- indigo_focuser_dsd: add support for DSD AF3
- indigo_agent_guider: abort fixed, remote driver fixed
- indigo_agent_imager: remote driver fixed, several enhancements
- indigo_ccd_ptp: Added support for several Sony, Canon and Nikon cameras, Sony
  and Nikon camera related fixes
- indigo_focuser_moonlite: timeout increased
- indigo_ccd_asi: firmware updated to v.1.14.1227
- indigo_ccd_qhy: camera firmware updated, but the QHY provided SDK is still badly unstable

### New Drivers:
- indigo_focuser_lunatico: Lunatico Astronomy Limpet/Armadillo/Platypus Focuser/Rotator/Powerbox/GPIO driver
- indigo_rotator_lunatico: Lunatico Astronomy Limpet/Armadillo/Platypus Rotator/Focuser/Powerbox/GPIO driver
- indigo_rotator_simulator: Rotator simulator deiver
- indigo_ccd_qhy2: The same as main qhy driver but uses different less stable sdk :( but hopefully it will support more cameras.

## [2.0-112] - Sun Jan 26 2020

### Overall:
- Developer documentation added

### Driver fixes:
- indigo_mount_temma: bug fixes
- infigo_ccd_atik: SDK updated
- indigo_focuser_dmfc, indigo_aux_upb: focuser speed limits fixed
- indigo_mount_lx200: AvalonGO support improved
- indigo_ccd_ptp: LiveView buffer race fixed


## [2.0-110] - Tue Jan 14 2020

### Overall:
- Add missing FITS keywords to xisf format
- fix FITS keyword DATE-OBS to indicate exposure start

### New Drivers:
- indigo_gps_gpsd: GPSD client driver

### Driver fixes:
- indigo_gps_nmea: bug fixes
- infigo_ccd_ptp:
    - Add Canon EOS 90D
    - Sony fixes
    - capture sequence standardized
    - image caching fixed
    - Canon CRAW support added
- indigo_ccd_ica:
    - bug fixes for MacOS Catalina
    - Avoid AF property behaviour fixed
- indigo_ccd_fli: fix reopen issue
- indigo_mount_nexstar: park position can be specified by user
- indigo_mount_simulator: fix RA move when not tracking

## [2.0-108] - Wed Jan  1 2020

### Overall:
- webGUI: readonly switch support added

### Drivers:
- indigo_mount_synscan: many fixes including PC Direct mode and baud rate detection.
- indigo_mount_nexstar: support side of pier, update libnexstar.
- indigo_mount_temma: support side of pier, zenith sync and drive speed.
- indigo_ccd_gphoto2: small fixes.
- infigo_ccd_ptp: fix PTP transaction for USB 3.0.
