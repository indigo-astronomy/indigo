# Changelog

All notable changes to INDIGO framework will be documented in this file.

# [2.0-250] - 02 Thu Wed 2023
### Overall:
- indigo_ccd_driver: JPEG previews are debayered
- indigo_ccd_driver: JPEG previews use STF for stretching (also stretch parameter items changed)
- indigo_ccd_driver: add 4 stretch presets - SLIGHT, MODERATE, NORMAL & HARD
- indigo_mount_driver: add MOUNT_TARGET_INFO property to show rise/transit/set times and time to next transit of the object

### Driver fixes:
- indigo_agent_astrometry:
	- astrometry processes are killed more aggressively

- indigo_agent_scripting:
	- item_defs parameter added to indigo_on_define_property call

- indigo_focuser_primaluce:
	- support ESATTO2

- indigo_gps_nmea:
	- typo fixed
	- lost connection handled correctly

- indigo_mount_lx200:
	- OnStep: park state fixed
	- OnStep: times fix
	- OnStep: side of pier support

- indigo_mount_nexstar:
	- error messages fixed

- indigo_ccd_touptek & family:
	- increase exposure wachdog timeout
	- update SDK v.54.23640.20231022

- indigo_ccd_ptp:
	- implement new API for SONY cameras
	- ptp_property_ExposureBiasCompensation is never writable
	- all list values masked to avoid future surprises and case values translated to hex
	- forced a correct behaviour of aperture and shutter during exposure mode change

- indigo_ccd_simulator:
	- BAYERPAT added

# [2.0-248] - 09 Oct Mon 2023
### Overall:
- indigo_ccd_driver: default image directory changed (if not sandboxed) to avoid clutter in the user's home
- insigo_bus: indigo_cancel_timer_sync() crash fixed
- indigo_bus: fix indigo_dtos() rounding error that can result in DD:MM:60
- indigo_bus: indigo_trace_property shows number format

### Driver fixes:
- indigo_agent_astometry / indigo_agent_astap:
	- make AGENT_PLATESOLVER_WCS proeprty states consistent with other processes during precise goto process
	- do not fail solving in case of image failure if no solving is requested
	- abort_process_requested cleared when new process is started

- indigo_agent_imager:
	- Implemented multi-target sequences.
	- sequencer waits for guiding to settle down before starting batch

- indigo_agent_scripting:
	- fixed deadlocks
	- indigo_on_enumerate_properties params fixed
	- mapping fixed
	- AGENT_SCRIPTING_RUN_SCRIPT property added to run ad-hoc scripts without saving them

- indigo_agent_alpaca:
	- fix JSON issues, now it is more compliant to the standard

- indigo_agent_guider:
	- added some debug messages

- indigo_ccd_playerone:
	- update sdk to 3.6.0

- indigo_ccd_ogma:
	- update sdk v.54.23385.20230918


# [2.0-246] - 16 Sep Sat 2023
### Overall:
- indigo_fits: fix redefinition of FITS_HEADER_SIZE
- indigo_sdk: fix crash issue on windows
- forcing INDIGO_RW_PERM to INDIGO_RO_PERM instead of assertion crash (with same legacy INDI backends)
- indigo_raw_utils: fix Invalid free() because of uninitialised value
- indigo_server: fix occasional crash when trying to load nonexisting driver

### Driver fixes:
- indigo_ccd_playerone:
	- update sdk to 3.5.0

- indigo_ccd_mi:
	- Update MI library to 0.8.3/0.7.3

# [2.0-244] - 31 Aug Thu 2023
### Overall:
- indigo_docs: update INDIGO_SERVER_AND_DRIVERS_GUIDE.md
- remote service discovery fixed

### New Drivers:
- indigo_aux_asiair: driver for ZWO ASIAIR Power Ports

### Driver fixes:
- indigo_ccd_ppt:
	- Canon LV crash fixed
	- Canon R7, R10, R6m2, R8, R50 and R100 support added
	- Sony A7S III support added
	- Sony lens focus support added
	- disconnect race fixed

- indigo_mount_lx200:
	- OpenAstroTech dialect support added

- indigo_agent_scripting:
	- enable_blob mapping fixed

- indigo_focuser_asi:
	- EAFMove error fixed

# [2.0-242] - 26 Jul Wed 2023
### Overall:
- indigo_docs: update SCRIPTING_BASICS.md
- indigo_driver: property state fixes

### New Drivers:
- indigo_aux_uch: driver for Pegasus USB Control Hub

### Driver fixes:
- indigo_agent_mount:
	- SIDE_OF_PIER propagation fixed

- indigo_agent_astrometry & indigo_agent_astap:
	- add precise GOTO process
	- added AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY to enable and disable solving when imager agent is related
	- move states to indigo_names.h
	- fix indigo_release_property()
	- add AGENT_PLATESOLVER_EXPOSURE_SETTINGS to replce AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE item in AGENT_PLATESOLVER_PA_SETTINGS

- indigo_agent_guider:
	- fix AGENT_START_PROCESS_PROPERTY and ABORT property rule
	- do not guide close to the pole
	- typo fix

- indigo_aux_upb:
	- fix minor LED issue
	- fix firmware representation
	- power up defaults support added

- indigo_aux_ppb:
	- fix firmware representation
	- power up defaults support added

- indigo_agent_imager:
	- better focus estimation

- indigo_ccd_ptp:
	- fix uint64_t alignment issue on 32 bit ARM systems
	- add NearFar for Sony cameras, implement focusing
	- code cleanup for canon

- indigo_ccd_playerone:
	- upgrade to SDK 3.4.1
	- deadlock on config load fixed

- indigo_ccd_svb:
	- sdk updated to 1.11.4

- indigo_ccd_asi:
	- sdk updated to 1.30

- indigo_ccd_touptek & family:
	- sdk updated to 54.22993.20230723

- indigo_ccd_qhy & indigo_ccd_qhy2:
	- fix CCD_TEMPERATURE step

- indigo_ccd_atik:
	- SDK updated to 2023.07.14

- indigo_focuser_asi:
	- fix focus compensation sign

- indigo_focuser_dsd:
	- fix focus compensation sign

- indigo_focuser_mypro2:
	- fix focus compensation sign

# [2.0-240] - 08 Jun Thu 2023
### Driver fixes:
- indigo_ccd_touptek & family:
	- increase timeout for the exposure watchdog

# [2.0-238] - 07 Jun Wed 2023
### Overall:
	- imager and guider phases made public enums

### Driver fixes:
- insigo_agent_imager:
	- sequence phase item added
	- sequence state fixes
	- add mount coordinates property

- indigo_agent_guider:
	- add mount coordinates property (AGENT_GUIDER_MOUNT_COORDINATES)
	- no need to recalibrate after flip or on Dec change
	- add AGENT_GUIDER_FLIP_REVERSES_DEC property, to reverse or not Dec speed after meridian flip
	- README.md updated

- indigo_agent_mount:
	- AGENT_GUIDER_MOUNT_COORDINATES set to related guider agent

- indigo_agent_alpaca:
	- sensor type fixed for DSLRs
	- fixed_data offset

- indigo_ccd_touptek & family:
	- add exposure watchdog in case the pull callback is not fired, guiding ahould not stop on its own any more

- indigo_ccd_svb:
	- fix target temperature

- indigo_ccd_ptp:
	- ptp_operation_GetDevicePropDesc is executed for known properties only by default
	- typo fixed

# [2.0-236] - 17 May Wed 2023
### Driver fixes:
- indigo_agent_imager:
	- fix item descriptions
	- sequence can have up to 128 batches
	- add BATCH_INDEX to STATS proeprty
	- clear batch index on simple batch as it does not represent any of the batches in the sequence
	- update AGENT_IMAGER_BATCH accordingly during a sequence
	- restore format and upload mode on abort
	- fixed batch counting

-indigo_ccd_playerone:
	- updated SDK to v.3.3.0

- indigo_ccd_ptp:
	- use ICA transport on MacOS
	- fixed some races

- indigo_ccd_svb:
	- updated SDK to v.1.11.3

# [2.0-234] - 02 May Tue 2023
### Overall:
- indigo_ccd_driver:
	- fix FITS files saved on server regression.
	- optimize saving images on server

### Driver Fixes:
- indigo_ccd_asi:
	- updated SDK to v.1.29

# [2.0-232] - 28 Apr Thu 2023
### Overall:
- indigo_mount_driver:
	- add MOUNT_ALIGNMENT_RESET property which reset alignment data

- indigo_ccd_driver:
	- support larger headers for FITS and XIFS images
	- fix "Instrument:Camera:FocalLength" XISF proeprty

### New Drivers:
- indigo_focuser_primaluce:
	- driver for PrimaLuceLab SESTO SENSO 2 and ESATTO focusers, plus ARCO rotators

### Driver Fixes:
- indigo_mount_asi:
	- use more generic symlink as AM3 and AM5 share the same PID (AM3 is supported)
	- add park
	- clear alignment data implemented

- indigo_ccd_ssg:
	- update sdk 53.22412.20230409
	- enable apple silicon build

- indigo_ccd_ptp:
	- remove unsupported Canon EOS M6

- indigo_focuser_prodigy:
	- fix property enumeration

# [2.0-230] - 13 Apr Thu 2023
### Overall
- indigosky:
	- better wifi performance in AP mode

- indigo_bus:
	- add indigo_get_peoperty_hint() and indigo_get_item_hint()
	- documentatrion added for new hints

- indigo_xml:
	- fix property hints propagation to client

- fix logging issues

### Driver Fixes
- indigo_ccd_uvc:
	-usb rules file added

- indigo_agent_astrometry:
	- add hints to some properties

- indigo_agent_guider:
	- enforced rounding for int values

- indigo_ccd_touptek & family:
	- add camera offset
	- update SDK 53.22412.20230409 (except StarShootG)
	- native apple silicon support (except StarShootG)

# [2.0-228] - 14 Mar Tue 2023
### Overall
- logging format changes

- indigo_log_analyzer: tool to analyze indigo bus logs
- indigo_ccd_driver:
	- CCD_EXPOSURE_PROPERTY updated once a second.
	- exposure countdown fixed

### Driver Fixes
- indigo_agent_astrometry:
	- add basic index file integritiy check when downloaded

- indigo_mount_asi:
	- proper error handling os ':Te#' and ':Td#'

- indigo_ccd_svb:
	- SDK updated to v.1.11.1
	- add natve apple silicon support

- indigo_ccd_atik:
	- SDK updated to 2023_03_17

- indigo_ccd_touptek & family:
	- fix bias exposures

- indigo_focuser_asi:
	- SDK updated to v.1.6
	- show SDK version

- indigo_ccd_svb:
	- Updated MI library to 0.8.0/0.7.0
    - Added support for C2-9000 camera

- indigo_wheel_playerone:
	- add intel32 support
	- SDK updated to v.1.2.0

# [2.0-226] - 14 Mar Tue 2023
### Overall
- indigo_client:
	- fix disconnect
	- fix shutdown race
	- shutdown flag added to remote server
	- failed socket closed properly
	- 0 is alowed as a valid socket descriptor
	- service discovery API added for Linux Windows and MacOS
	- a lot better indigo_client library for Windows
	- indigo_client.dll can be built with MSVC and MinGW
	- building on windows better documented

- overall code cleanup
- indigo_docs: explain service discovery routines
- add service_discovery example

- indigo_prop_tool:
	- add discover command
	- can be built on windows with MinGW

### Driver Fixes
- indigo_ccd_touptek & family:
	- fix guider timer issue

- indigo_ccd_svb:
	- updated to sdk v.1.10.2
	- fix image buffer size

- indigo_ccd_playerone:
	- fix ROI buffer size issue
	- update to sdk 3.2.2
	- enable x86 linux build as Player One provided v86 SDK

- indigo_ccd_asi:
	- fix image buffer size

- indigo_mount_asi:
	- implement meridian limits and flip (requires yet unreleased firmware)

# [2.0-224] - 01 Mar Wed 2023
### Overall
- CCD_SET_FITS_HEADER random content issue fixed
- removed misleading log messages
- experimental client side bonjour support added (macOS only first)
- default server callback added to tcp server
- indigo_disconnect_server behaviour fixed
- tcp server: less verbose logging
- Avahi dependency moved from indigo_server to libindigo
- indigo_server_start fixed for linux
- timers: logging fixed

### Driver Fixes
- indigo_ccd_asi:
	- custom suffix changed to string to string
	- logging fixes

- indigo_focuser_asi:
	- add support for custom device name suffix
	- fix crash when unpluging connected device
	- rename custom proeprtyies to follow the driver convention
	- show device name in INFO proeprty
	- logging fixes

- indigo_wheel_asi:
	- hot plug fixes
	- show device name in INFO proeprty
	- add support for custom device name suffix

- indigo_ccd_svb:
	- update SDK to v1.10.1
	- show warning if the camera firmware needs update

- indigo_wheel_playerone:
	- use 'model #suffix' custom device name format
	- code clenup

- indigo_ccd_playerone:
	- update SDK to v.3.2.1
	- remove USB2.0 warning as SDK is fixed
	- use gain/offset ptesets from the SDK
	- use 'name #suffix' custom device name format

- indigo_mount_ioptron:
	- protocol version can be forced manually

- indigo_mount_lx200:
	- Pegasus NYX support added
	- initialization fixed

- indigo_ccd_touptec & familly:
	- add bayer pattern in FITS header
	- binning and subframing rewrite and fix
	- add binning mode support - average, clip or expand to 16bit (for 10, 12 and 14-bit data)
	- add sensor window heater control support
	- remove odd coller conrol from setup_exposure()

# [2.0-222] - 12 Feb Sun 2023
### Overall
- add indigo_device_name_exists() and indigo_make_name_unique()
- add indigo_reschedule_timer_with_callback()

### All drivers:
- use indigo_device_name_exists() and indigo_make_name_unique() to make device names unique - prmanent where devices support it
- for hotplug drivers, the first device of a type will not have a suffix #XXX (unless it has permaned suffix added, only ASI and Player one support it)
- fixed reseting of timer reference while callback is still executed

### New Drivers:
- indigo ccd_ogma:
	- OMGA Camera driver added - toupteck clone

### Driver Fixes:
- indigo_ccd_sx:
	- long exposure fixed - regression

- indigo_ccd_fli:
	- long exposure fixed - regression
	- small fixes

- indigo_ccd_qhy:
	- long exposure fixed - regression

- indigo_ccd_dsi:
	- long exposure fixed - regression

- indigo_ccd_sbig:
	- long exposure fixed - regression

- indigo_ccd_playerone:
	- add permanent user defined camera suffix
	- exposure countdown fixed

- indigo_wheel_playerone:
	- add permanent user defined camera suffix

- indigo_ccd_asi:
	- libasicamera v.1.28
	- add permaned user defined camera suffix


# [2.0-220] - 08 Feb Wed 2023
### Overall
- fixed the timer race that we were chading for months!

# [2.0-218] - 06 Feb Mon 2023
### Overall
- revert timer issue introdiced in Januaryc + another approach to fix the race


# [2.0-216] - 02 Feb Thu 2023
### Overall
- logging cleanup, indigo_log_message_handler signature changed
- INDIGO_BUILD_COMMIT, INDIGO_BUILD_TIME added in log

### New Drivers:
- indigo_ccd_omegonpro:
	- Omegon camera driver based on Touptek

- indigo_ccd_ssg:
	- Orion StarShootG camera driver based on Touptek

- indigo_ccd_rising:
	- RisingCam camera driver based on Touptek
	- also supports Levenhuk and EHD imaging cameras
	- SDK version 53.22004.20230115

- indigo_ccd_mallin:
	- MallinCam camera driver based on Touptek

### Driver Fixes:
- indigo_ccd_touptek & family:
	- fix exposure problem on linux
	- fix camera changed (use part of serial number on mac, when sdk is fixed will be used on linux too)
	- add conversion gain control HCG, LCG and HDR

- indigo_ccd_touptek:
	- SDK version 53.22004.20230115

- indigo_wheel_asi:
	- fix error handling at connect

- indigo_wheel_playerone:
	- issues fixed
	- SDK version 1.1.1

- indigo_ccd_altair:
	- SDK version 53.22004.20230115

- indigo_ccd_playerone:
	 - update SDK to v.3.1.1
	 - many bug fixes (including frame readdout. binnig, ROIs, temperatire reading)
	 - add workaround for a bug in POAGetImageData()
	 - code refacroring
	 - added gain/offset presets property
	 - gain/offset can not be changed while exposure is in progress
	 - add warning for USB2 connection as some cameras do not work on USB2 (SDK issue)

- indigo_ccd_svbony:
	- connect bin_x = bin_y as SvBony camseras have one bin
	- fix frame size when binning
	- gain/offset can not be changed while exposure is in progress

- indigo_ccd_asi:
	- fix frame size when binning
	- gain/offset can not be changed while exposure is in progress


# [2.0-214] - 20 Jan Thu 2023

### Driver Fixes:
- indigo_agent_guider: fixed pixel scale

# [2.0-212] - 19 Jan Thu 2023
- replace usleep() with nanosleep() in indigo_usleep();
- indigo_timer: fixes race in indigo_set_timer();
- indigo_ccd_driver: use balanced approach to countdown timer - fixes potential race
- all agents:
	- selection can not be changed while list is busy - fixes connect race
	- CCD_LENS_FOV created for CCDs only

### Driver Fixes:
- indigo_ccd_ssag:
	- custom vid/pid enabled

# [2.0-210] - 16 Jan Mon 2023
### Overall
- indigo_ccd_driver: Changed API to custom add fits headers to avoid possible races via CCD_SET_FITS_HEADER/CCD_REMOVE_FITS_HEADER.
- indigo_get_utc_offset() and indigo_get_dst_state() added
- all mount drivers: use indigo_get_utc_offset() and indigo_get_dst_state()
- APTDIA and FOCALLEN added to fits headers
- device profile names are configurable

### New Drivers:
- indigo_wheel_indigo:
	- PegasusAstro Indigo filter wheel driver - looking for testers!

- indigo_focuser_prodigy:
	- PegasusAstro Prodigy Microfocuser driver - looking for testers!

- insigo_rotator_falcon:
	- PegasusAstro Falcon rotator driver added - looking for testers!

- indigo_rotator_optec:
	- Optec Pyxis camera field rotator - looking for testers!

### Driver Fixes:
- indigo_agent_astrometry:
	- use same reference RA and Dec for the 3 points in polar alignment
	- fix bug in polar error recalculation leading to offset between 0" and 20"

- indigo_agent_astap:
	- use same reference RA and Dec for the 3 points in polar alignment
	- fix bug in polar error recalculation leading to offset between 0" and 20"

- indigo_agent_guider:
	- forces upload to clinet on frame request

- indigo_ccd_playerone:
	- CCD_INFO_PIXEL_SIZE_ITEM is properly set
	- SDK updated to 3.1.0
	- several fixes

- indigo_ccd_svb:
	- CCD_INFO_PIXEL_SIZE_ITEM is properly set

- indigo_ccd_uvc:
	- CCD_INFO_PIXEL_SIZE_ITEM set to 0 as value is not known

- indigo_ccd_iidc:
	- CCD_INFO_PIXEL_SIZE_ITEM is properly set

- indigo_wheel_asi:
	- add calibration function

- indigo_mount_lx200:
	- StarGO2 support added
	- epoch support removed
	- Gemini mounts forced to accept JNow

# [2.0-208] - 04 Jan Wed 2023
### Overall
- indigo_ccd_driver: more robust exposure countdown timer implementation
- webGUI: mounts without park feature fixed
- debs: add curl dependency in the deb as it is used by astrometry agent
- global lock/unlock fixed
- shared multi_device_mutex added to device context
- all drivers: connect synchronisation fixed
- indigo_lock_master_device()/indigo_unlock_master_device() added
- INDIGO bus stop fixed

### Driver Fixes:
- indigo_agent_config:
	- add AGENT_CONFIG_LAST_CONFIG proeprty
	- replace spaces from config names with undercore
	- synchronisation improved
	- unused drivers unload is optional
	- make AGENT_CONFIG_SETUP items more descriptive
	- describe agent operation
	- AGENT_CONFIG_LOAD property waits in busy state until finished

- indigo_mount_lx200:
	- AM5 autodetection fixed

- indigo_ccd_altair:
	- SDK v.53.21849.20221208
	- fix driver shutdown
	- use better device ids in the device name
	- show cooler power
	- do not use the depricated XXX_Flush() function
	- multi device support fixed
	- hotplug/unplug fixed
	- unify labels of CCD_MODE items to match the template of other drivers

- indigo_ccd_touptek:
	- updated SDK v.53.21849.20221208
	- fix driver shutdown
	- use better device ids in the device name
	- show cooler power
	- do not use the depricated XXX_Flush() function
	- multi-device support fixed
	- hotplug/unplug fixed
	- unify labels of CCD_MODE items to match the template of other drivers

- indigo_ccd_andor:
	- small fixes

# [2.0-206] - 10 Dec Sat 2022

### Overall
- all ccd drivers: fix exposure countdown deadlock

### Driver Fixes:
- indigo_agent_config:
	- fix driver name
	- fix stale BUSY state, make save/load/delete more verbose

- indigo_ccd_altair:
	- SDK updated to 53.21749.20221121

- indigo_ccd_touptek:
	- SDK updated to 53.21749.20221121

- indigo_ccd_svb:
	- SDK uodated to v.1.10.0


# [2.0-204] - 06 Dec Tue 2022

### Overall
- all ccd drivers: fix exposure countdown race

### Driver Fixes:
- indigo_ccd_asi:
	- SDK updated to v.1.27

- indigo_ccd_svb:
	- SDK uodated to v.1.9.8
	- switch to statically inked version of the SDK on Linux
	- fix ocasional broken frames
	- fix exposure retry issue
	- swithched from sychronous to asynchrnous exposure handling
	- fix cooler issues for SV405
	- fix streaming for SV405
	- many bugfixes

## [2.0-202] - 26 Nov Sat 2022

### Overall
- property resizing fixed
- docs:
	- GUIDING_MISCONCEPTIONS.md updated
	- CCD_DRIVER_SAVED_IMAGES.md creaated - describes how file name templates work for the server side saved images.

- all ccd drivers:
	- FITS keyword OBS-DAT used with millisec precision
	- EXPTIME fixed for streaming

### New Drivers:
- indigo_agent_config:
	- new configuration agent created - it manages server condigurations

### Driver Fixes:
- indigo_agent_alpaca:
	- imagebytes support

- indigo_agent_imager:
	- AGENT_IMAGER_DOWNLOAD_FILES property is sorted by creation time and date.

- indigo_agent_scripting:
	- indigo_save_blob() added
	- indigo_populate_blob() added

- indigo_focuser_focusdreampro:
	- fix focuser limits

## [2.0-200] - 28 Oct Fri 2022

### Overall
- docs:
	- updated PLATE_SOLVING.md
- all ccd drivers:
	- saved files are numbered from 1 instead of 0
	- max file count limits for different formats replaced with single limit
	- indigo_finalize_dslr_video_stream() added

- all agents:
	- add property CCD_LENS_FOV is uses the camera data and shows FOV and pixel scale
	- related agent removal bug fixed

- indigo_client:
	- added indigo_format_number() to format sexadecimal numbers (%m format)

- indigo_server:
	- better handling of driver failure to load

### Driver Fixes
- indigo_agent_astap:
	- use pixel scale from camera as a hint

- indigo_agent_astrometry:
	- use pixel scale from camera as a hint
	- use pixel scale hint (it was not used at all) - works way faster with many indexes selected

- indigo_ccd_simulator:
	- better parameter defaults so that guiding makes more sense with 5cm FL - still not really ok but much better

- indigo_ccd_svb:
	- SDK updated to 1.9.6

- indigo_aux_joystick:
	- make analog mode more usable
	- button mapped to go home


## [2.0-198] - 09 Oct Sun 2022

### Overall
- ccd_drivers:
	- remove partially saved files
	- zero length image download fixed

### Driver Fixes
- indigo_agent_mount:
	- forward events generated by the joystick buttons to the mount driver only if item is set

- indigo_aux_joystick:
	- reset all "mount" properties to the initial state on connect

- indigo_ccd_asi:
	- set default USB Bandwidth at connect

- indigo_agent_alpaca:
	- side of pier mapping fixed

- indigo_mount_nexstar:
	- missing gps timeouts fixed


## [2.0-196] - 01 Oct Sat 2022

### Overall
- better property buffers handling to avoid memory fragmentation

### Driver Fixes
- indigo_agent_imager:
	- more relaxed buffer allocation


## [2.0-194] - 27 Sep Tue 2022

### Driver Fixes
- indigo_agent_imager:
	- fix wrong size of AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY when no files available

## [2.0-192] - 25 Sep Sun 2022

### Overall
- fix memory overrun in json parser

## [2.0-190] - 22 Sep Thu 2022
## NOTE: This version is binary incompatible with the previous version. Clients that link libindigo dynamically should be recompiled!!!

### Overall
- add indigo_get_version() call
- Properties can have unlimited number of items
- fix crash related to indigo_disconnect_server()
- add md5 hash support - useful for file transfer
- All ccd drivers:
	- server stored images use more advanced name templates

### Driver fixes
- indigo_agent_imager:
	- scandir() leeks fixed

- indigo_mount_lx200:
	- :RG# :RC# etc does not work any more on AM5 - using :R1# :R3# instead
	- checksum crash fixed
	- fix gemini issues

- indigo_mount_asi:
	- :RG# :RC# etc does not work any more - using :R1# :R3# instead

- indigo_ccd_mi:
	- Update MI library to version 0.7.2/0.6.2
	- Fix bug in GPS exposure time info

## [2.0-188] - 02 Sep Fri 2022
### Overall
- indigo_ccd_failure_cleanup() call added
- documentation updates
- indigo_io.c: Fix wrong report of failed connection
- All ccd drivers:
	- set CCD_IMAGE state to ALERT when CCD_EXPOSURE is ALERT
	- use indigo_ccd_failure_cleanup() to cleanup the state at error

### New Drivrs
- indigo_ccd_playerone:
	- Driver for PlayerOne cameras
	- 32-bit Intel not supported

- indigo_focuser_prodigy:
	- supports focuser part only, power box not implemented yet

### Driver fixes
- indigo_agent_imager:
	- AGENT_PAUSE_PROCESS_WAIT_ITEM added to AGENT_PAUSE_PROCESS_PROPERTY
	- frame counting with wait & pause fixed
	- breakpoint handling fixed

- indigo_agent_astap:
	- kill pending children at exit
	- fix process failure if exposure fails

- indigo_agent_astrometry:
	- kill pending children at exit
	- fix process failure if exposure fails

- indigo_ccd_asi:
	- better cooler error handling
	- update sdk to v.1.26

- indigo_ccd_svb:
	- code refactored
	- SDK updated to version 1.9.4

- indigo_ccd_mi:
	- add support for new C5 cameras and optional GPS module attachment
	- update SDK with some new PIDs and support for new binning parameters

- indigo_ccd_ptp:
	- fix LIBUSB_ERROR_OVERFLOWs

- indigo_mount_asi:
	- typo fixes
	- Handling of TCP disconnections

- indigo_mount_lx200:
	- typo fixes
	- Handling of TCP disconnections

- indigo_mount_ioptron:
	- fix TZ/DST issues

- indigo_mount_starbook:
	- fix getstatus (starbook < 4.20)


## [2.0-186] - 25 Jul Mon 2022
### Overall
- indigo_docs: SCRIPTING_BASICS added (Thanks to Johan Bakker)
- indigo_align: fix indigo_equatorial_to_hotizontal() to work with poles
- indigocat_jnow_to_j2k() helper added
- EXPTIME format in FITS changed to %20.4f for values < 1
- mount_driver: All alignment points can be deleted at once

### Driver fixes
- indigo_agent_guider:
	- added weighted selection guiding method
	- increased guide stars to 24
	- use smaller step for several items
	- add Error or Warning prefix to the messages
	- select guide stars in wider area (the central 90% of the frame)

- indigo_agent_mount:
	- mount movement doesn't abort preview in imager and guider agents

- indigo_ccd_uvc:
	- auto exposure turned off

- indigo_mount_lx200:
	- high precision format used for Sg/St on Avalon
	- bug fixes

- indigo_ccd_svb:
	- SDK updated to 1.7.3
	- Y16 support added

- indigo_ccd_atik:
	- SDK updated to 2022-07-13

- indigo_ccd_qhy2:
	- SDK updated to V2022.07.06

- indigo_aux_dsusb:
	- autofocus made configurable

- indigo_mount_asi:
	- added "e9" error text, some messages fixed

- mount_synscan:
	- assertion fixed for Hour angle 0h and 12h

## [2.0-184] - 01 Jul Fri 2022
### Overall
- all guider drivers can handle simultaneous RA and Dec guiding
- agent configuration load regression fixed

### Driver fixes
- indigo_agent_guider:
	- RA and Dec guiding pulses are issued simultaneously

- indigo_agent_scripting:
	- AGENT_SCRIPTING_DELETE_SCRIPT handling and agent_send_message fixed

- indigo_guider_cgusbst4:
	- cancel guider timer on detach

- indigo_guider_gpusb:
	- cancel guider timer on detach

- indigo_mount_simulator:
	- cancel guider timer on detach

## [2.0-182] - 28 Jun Tue 2022
### Overall
- add libindigocat - library to calculate planetary positions and stellar positions
- indigo_property_match_changeable() - add new call to match only properties that can be changed (writable and defined)
- indigo_drivers: simplify all drivers by using indigo_property_match_changeable()
- indigo_docs: add PLATE_SOLVING.md
- indigo_ccd_driver: CCD_LENS property step fix
- indigo_raw_utils: indigo_find_stars_precise() - fix luminescence calculation
- indigo_raw_utils: better star detection using stddev as a threshold
- libusb: fixed regression rendering QHY cameras more unstable then they are.

- indigo_server:
	- Web UI: add solar system objects to the star map
	- Web UI: fix Lambda and Phi designations (lambda - longitude, Phi - latitude)

### New drivers
- indigo_mount_asi:
	- driver optimized for the lx200 dialect of ZWO AM mounts (AM mounts can still be used with the generic lx200 driver)

### Driver fixes
- indigo_mount_lx200:
	- handshake fails if auto detection is not successful
	- optimizations for ZWO AM mount
	- fix am5 guide rate
	- fix mount move
	- remove PARK property for ZWO AM mount
	- fix set time on ZWO AM
	- better driver responsiveness - flush input buffer timeout reduced
	- code cleanup
	- minor fixes

- indigo_mount_synscan:
	- fix N/S and E/W move issue.

- indigo_agent_astrometry:
	- large index selection issue fixed

- indigo_agent_guider:
	- add RA, Dec drifts and RMSE in arc seconds
	- fix automatic guide star selection
	- better RA speed estimation during calibration - take periodic error effect in to account

- indigo_ccd_ptp:
	- dependency on ptp_property_nikon_LiveViewAFFocus removed

- indigo_ccd_simulator:
	- random star field shift fixed with amplitude 0.2px

## [2.0-180] - 06 Jun Mon 2022
### Overall
- Remove novas dependency
- fix 11sec offset in LST
- indigo_mount_driver: alt and az are now computed for the current epoch

## [2.0-178] - 02 Jun Thu 2022
### Overall
- DRIVER_DEVELOPMENT_BASICS: documentation updates
- POLAR_ALIGNMENT: documentation updates
- add libraw to externals
- indigo_names: fix CCD_RBI_FLUSH_PROPERTY_NAME
- indigo_ccd_driver: add CCD_ADVANCED_GROUP and use it in drivers, moved RBI properties to CCD_ADVANCED_GROUP
- indigo_platesolver: fix memory leaks
- indigo_platesolver: add WCS state to be used by clients (WAITING_FOR IMAGE, SOLVING, SYNCING etc)
- rotator property names mapped for legacy protocol
- indigosky: can now work at 5GHz Wifi up to 200Mbit/s

### New drivers
- indigo_mount_starbook:
	- Vixen StarBook mount controller driver added

### Driver fixes
- indigo_mount_lx200:
	- code refacored, made asynchronous
	- ZWO AM5 support completed and tested
	- added udev rules to give adequate names to some mounts on linux

- indigo_ccd_touptek:
	- SDK updated to 20220424

- indigo_ccd_altair:
	- SDK updated to 20220424

- indigo_ccd_gphoto2:
	- make it compile against the global libraw

- indig_ccd_ptp:
	- add XISF conversion
	- add FITS conversion
	- add indigo RAW conversion

- indigo_ccd_fli:
	- fix CCD_TEMPERATURE_ITEM step
	- fix CCD_RBI_FLUSH_COUNT_ITEM defaults

- indigo_ccd_asi:
	- if bin_x != bin_y exposure fails, so force bin_x = bin_y

- indigo_agent_astrometry:
	- solve camera raw images
	- use catalog name in index labels in USE_INDEX property
	- USE_INDEX property items are sorted

- indigo_agent_astap:
	- USE_INDEX property items are sorted

## [2.0-176] - 20 Apr Wed 2022
### Overall
- Rotator lists added to filter agent base code
- POLAR_ALIGNMENT: add notes that 3PPA will not work when looking at the pole
- deb_package: added indigo-server.service and create indigo user on install

### New drivers
- indigo_rotator_optec:
	- Optec Pyxis rotator driver created

### Driver fixes
- indigo_agent_imager:
	- rotator support added

- indigo_agent_guider:
	- disable dithering in Dec when Dec guiding is not 'North and South'
	- changing declination guiding to/from 'Notrh and South' while guiding is prohibited

- indigo_agent_astrometry:
	- JPEG detection fixed

- indigo_agent_astap:
	- JPEG detection fixed

- indigo_mount_rainbow:
	- reader thread fixed

- indigo_mount_rainbow:
	- reader thread fixed

- indigo_ccd_altair:
	- SDK updated to 20210912_50.19561

- indigo_ccd_svb:
	- fix udev rules
	- retry counter reset after processed image for streaming
	- do not update target temperature if not needed

- indigo_ccd_andor:
	- remove clear_reg_timer_callback()

- indigo_ccd_apogee:
	- remove clear_reg_timer_callback()

- indigo_rotator_lunatico:
	- ROTATOR_DIRECTION_PROPERTY made persistent in driver instead of base code

- indigo_ao_sx:
	- port closed on unsuccessful handshake

- indigo_wheel_trutek:
	- port closed on unsuccessful handshake

- indigo_wheel_xagyl:
	- port closed on unsuccessful handshake

- indigo_focuser_optec:
	- CRLF handling fixed

- indigo_wheel_optec:
	- CRLF handling fixed

- indigo_ccd_asi:
	- add some debug messages
	- do not update target temperature if not needed

- mount_pmc8:
	- slew to target fixed

## [2.0-174] - 04 Apr Mon 2022
### Overall
- indigo_ccd_driver: fix exposure timer counter stall if you abort and start new exposure before the previous timer is hit

### Driver fixes
- indigo_ccd_asi: fix race condition in exposure timer handler

## [2.0-172] - 29 Mar Tue 2022
### Overall
- indigo_query_slave_devices() signature changed
- indigo_stop() fixed
- indigo_detach_device()/indigo_detach_client() return value fixed
- auto save of the port property is disabled (it is saved with the profile)
- indigo_try_global_lock() fixed
- many warnings silenced and unused variables removed
- Astro-TIFF image format support added
- device name added to message
- libusb updated to 1.0.25
- convenience wrappers for indigo_precess() added

- all lx200 mount drivers:
	- any coordinate epoch can be used for driver-device communication

- indigo_platesolver:
	- AGENT_PLATESOLVER_PA_STATE is never Idle

- all focuser drivers:
	- fix race in which STEPS/POSITION may not become busy during move

- all drivers:
	- support for multiple devices added
	- fix BUSY status handling
	- fix race in deivce attach

### Driver fixes
- indigo_agent_imager:
	- AGENT_IMAGER_BREAKPOINT, AGENT_IMAGER_RESUME_CONDITION and AGENT_IMAGER_BARRIER_STATE properties added for inter-agent synchronization
	- documentation fixed
	- disable temp compensation when focusing is started
	- process state fixed
	- filter names snooping fixed
	- change some messages to match 'warning' and 'complete'
	- AGENT_IMAGER_DOWNLOAD_FILES is sorted
	- DOWNLOAD_MAX_COUNT reset back to 127

- indigo_agent_guider:
	- change some messages to match 'warning' and 'complete'
	- fix backlash calculation
	- Declination backlash can be corrected

- indigo_agent_mount:
	- HA and local time limits are evaluated even if MOUNT_PARK_UNPARKED_ITEM is not defined
	- EPOCH added to lx200 server configuration properties
	- initialization fixed

- indigo_mount_lx200:
	- custom parking position enabled for OnStep

- indigo_agent_lx200:
	- EPOCH added to server configuration properties

- indigo_ccd_asi:
	- update SDK v.1.22
	- add message to streaming failure
	- make sure there is no exposure in progress before we start another one

- ccd_ptp:
	- add Canon EOS R3
	- add Nikon Z9
	- media set explicitly for exposure and live view on Nikons
	- update ptp_fuji_fix_property

- indigo_focuser_dsd:
	- fix handle initialization
	- check if connected for some property changes

- indigo_focuser_lunatico / indigo_rotator_lunatico:
	- check if connected for some property changes

- indigo_ccd_simulator
	- superfluous locking removed

- indigo_ccd_svb:
	- better handling of failed exposures

- mount_ioptron:
	- park/unpark ignored for parked/unparked mount

- mount_simulator:
	- superfluous park/unpark fixed

## [2.0-170] - 05 Mar Sat 2022
### Overall
- indigo_platesolver: do not fail the whole process if platesolve during recalculate fails
- indigo_platesolver: implement refraction compensation for polar alignment
- indigo_platesolver: start_exposure() checks for available camera
- indigo_platesolver: better error handling
- indigo_platesolver: start_exposure() checks if the camera is available
- indigo_platesolver: better AGENT_PLATESOLVER_PA_STATE property state transitions
- Makefile: libhidapi-libhidraw.a replaced with libhidapi-libusb.a

### Driver fixes
- indigo_astrometry_agent:
	- see overall: indigo_platesolver

- indigo_astap_agent:
	- see overall: indigo_platesolver

- indigo_ccd_atik:
	- EFW1/2 SDK support added to the CCD driver

- indigo_wheel_atik:
	- usb rules fixed

- indigo_ccd_simulator:
	- set more reasonable value for default image in Guider Camera

- indigo_ccd_svb:
	- initialization fixed
	- workatound for prematurely ended exposures

## [2.0-168] - Mon Feb 21 2022
### Overall
- added WRITE_ONLY BLOBs
- indigo_platesolver: added 3 point polar alignment
- indigo_platesolver: local files can be uploaded for solving
- indigo_platesolver: code refactored
- indigo_platesolver: add processes
- indigo_prop_tool: add support for WRITE_ONLY BLOBs
- indigo_libs: libnovas fixed for arm64 linux machines
- indigo_libs: HIDAPI switched from libraw to libusb on linux
- indigo_docs: POLAR_ALIGNMENT.md added
- ccd_driver: byte_order_rgb interpreted correctly for TIFF format

### New drivers
- indigo_astap_agent:
	- plate solver agent using ASTAP
	- has all the features as Astrometry agent

- indigo_ccd_svb:
	- driver for SvBony cameras

### Driver fixes
- indigo_astrometry_agent:
	- solving can be triggered by a processes
	- added 3-point polar alignment
	- local files can be uploaded for solving
	- solution can be transformed to JNow
	- fix abort function
	- fixed cleanup of the temporary files

- indigo_agent_imager:
	- dithering can skip frames

- indigo_agent_scripting:
	- race fixed

- indigo_ccd_simulator:
	- Hipparcos data is used for real sky images from "CCD Guider Simulator"
	- add polar error simulation
	- images from "CCD Guider Simulator" can be in JNow or J2000
	- "CCD Guider Simulator" frame size is user defined

- indigo_ccd_asi:
	- use SDK v1.21
	- fix array overrun
	- number.target vs. number.value cleanup

- indigo_gps_nmea:
	- support for Glonass messages

- indigo_ccd_qhy:
	- provde hack for missing pthread_yield() call

- indigo_mount_synscan:
	- add support for "StarSeek" mounts

- indigo_ccd_ptp:
	- Fuji driver improved, tested with XT1
	- add Canon EOS 250D
	- add Fujifilm X-S10
	- generalised image download code for Fujifilm
	- Sony A7R4 compatibility fixes

- indigo_mount_lx200:
	- add experimental support for ZWO AM5 mount
	- 10micron bugfixes
	- guiding commands made synchronous
	- mount type override fixed

- indigo_mount_ioptron:
	- guiding commands made synchronous

- indigo_mount_pmc8:
	- new firmware support added
	- rounding issues fixed

- indigo_focuser_fcusb:
	- rules file added

## [2.0-166] - Tue Nov 30 2021
### Overall
- indigo_raw_utils: new API for RMS contrast estimator
- IMAGING_AF_TUNING: updated to reflect the latest AF changes
- indigo_process_image(): fix sending of a wrong error message

### Driver fixes
- indigo_agent_imager:
	- complete rewrite of RMS contrast estimator
	- RMS AF supports RGB images
	- RMS AF is not confused by saturated areas any more

- indigo_ccd_touptek:
	- SDK updated to 50.19728.20211022

- indigo_ccd_atik:
	- SDK updated to SDK_2021_11_02

- indigo_ccd_asi:
	- SDK updated to 1.20.3

- indigo_ccd_qhy2:
	- SDK updated to V2021.10.12 (x86 driver uses the old version - QHY dropped 32bit Linux support)

- indigo_ccd_qhy/indigo_ccd_qhy2:
	- camera firmware update

- indigo_ccd_uvc:
	- image buffer overrun fixed

- indigo_ccd_mi:
	- Update ibgxccd for MacOS to version 0.5.1 (fixes issue with Monterey when camera did not connect the first time after power on)

- indigo_mount_simulator:
	- fix MOUNT_TRACKING property state transitions

## [2.0-164] - Fri Nov 05 2021
### Overall
- indigo_dtos(): call fixed formatting issues
- indigo_stod(): fix for values [-1, 0] represented as positive
- libusb: updated

### Driver fixes
- indigo_agent_solver:
	- more reliable "Sync and center"
	- FOV unit parsing fixed

- indigo_agent_imager:
	- Peak/HFD focus failed criteria fixed
	- BEST_FOCUS_DEVIATION item added as a measure of the deviation of the final focus quality compared to the best of the run
	- return to initial on failure fixed
	- fix memory leak in capture_raw_frame()
	- prevent abort_process() from aborting all devices on the bus when no focuser selected
	- do not evaluate RMS contrast on frame restoration as contrast changes dramatically when frame is changed

- indigo_agent_guider:
	- fixed calibration near the poles

- indigo_ccd_ptp:
	- Fuji camera BULB exposure fixed
	- Fuji X-T2 support

- indigo_focuser_mypro2:
	- fix proeprty handling

- indigo_mount_lx200:
	- OnStep compatibility issue fixed

- indigo_mount_ioptron:
	- search for mechanical zero position added for CEM45, CEM70 and CEM120
	- MOUNT_PARK_SET implemented for 2.5 and 3.0 protocols

- indigo_ccd_asi:
	- lower the camera stress at exposure start
	- fix ASI120 issues
	- SDK updated v.1.20.2.1103

- indigo_ccd_simulator:
	- backlash simulation added
	- add declination to the drift to simulate high declination guiding
	- change GUIDER_IMAGE to SIMULATION_SETUP property

- indigo_ccd_uvc:
	- libuvc updated

## [2.0-162] - Fri Oct 15 2021
### Overall
- indigo_filter: device and related device validation callback added
- indigo_docs: updated IMAGING_AF_TUNING.md
- indigo_framework: add indigo_contrast() call to calculate Root Mean Square (RMS) contrast of the image

### Driver fixes
- indigo_agent_imager:
	- add new backlash clearing strategy "Backlash overshoot"
	- add new autofocus estimator "RMS contrast"
	- remove FWHM from original focus quality estimator - tests show it does not work well.
	- user can select autofocus estimator: "Peak/HFD" or "RMS contrast"
	- sequencer doesn't fail if autofocus fails, autofocus restores initial position if it fails
	- AGENT_IMAGER_FOCUS_FAILURE property added with the option to return to starting position (Peak/HFD estimator only)
	- add possibility to retry focus on failure (user configurable)
	- focusing statistics are cleared on focus start
	- better autofocus error handling
	- manual focusing fixed
	- fix ABORT_PROCESS ending in endless loop with message 'Failed to evaluate quality'
	- the error 'FOCUSER_STEPS_PROPERTY didn't become OK' is not shown if process is aborted or no move us performed.
	- external shutter support added
	- Changed AF max move limits to:
		- 20 * Initial step for Peak/HFD estimator
		- 40 * Initial step for RMS contrast estimator

- indigo_aux_flipflat:
	- command delimiters fixed
	- fix default linux serial port name

- indigo_aux_joystick:
	- macOS deadlocks fixed

- indigo_ccd_ptp:
	- fix A7R4 capture issues
	- external shutter support added for Nikon

- indigo_mount_ioptron:
	- some more protocol 3.1 firmware versions added
	- more Fuji models added

## [2.0-160] - Wed Sep 15 2021
### Driver fixes
- indigo_ccd_asi:
	- updated SDK to 1.20
		- adds support for ASI482 and ASI485
		- fixes the problem that ASI2600 sometimes fails to obtain data
		- fixes the problem of image tearing after ASI2600 switching resolution
		- the ASI183 ROI function is rolled back to obtain a higher frame rate
		- fixes an occasional failures to open the camera

- indigo_ccd_atik:
	- update SDK to SDK_2021_09_06

## [2.0-158] - Sun Aug 29 2021
### Overall:
- indigo_raw_to_fits(): user provideds keyword can be added
- indigo_raw_to_fits: the tool can add BAYERPAT, CCD-TEMP and EXPTIME keywords
- TIFF/JPEG buffer size fixed
- dome drivers: DOME_DIMENSION property made persistent
- indigo docs: IMAGING_AF_TUNING - guidelines for auto focus tuning

### New Drivers
- indigo_aux_geoptikflat: Geoptik flat field generator driver

### Driver fixes
- indigo_agent_imager:
	- fix backlash application if it is handled by the driver
	- fix CCD_UPLOAD_MODE_PROPERTY and CCD_IMAGE_FORMAT_PROPERTY restoration in preview process

- indigo_agent_guider:
	- multi-star guiding now works with less than required stars
	- change some messages

- indigo_agent_mount: AGENT_SET_HOST_TIME_PROPERTY made persistent

- indigo_ccd_atik:
	- window heater support added

- indigo_ccd_asi:
	- RREADME.md updated

## [2.0-156] - Sun Jul 26 2021
### Overall:
- j2k coordinates added to indigo_topo_planet()

### Driver fixes
- indigo_agent_astrometry: index download or solving can be aborted

- indigo_agent_guider:
	- settling limit made configurable (AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM)
	- dec-only calibration fixed
	- guiding will fail if RA speed = 0

- indigo_ccd_mi:
	- driver made multiplatform on macOS
	- add support for Moravian C1×, C3 and C4 cameras

- indigo_ccd_asi:
	- update sdk to 1.19.2

## [2.0-154] - Sat Jul 03 2021
### Overall:
- indigo_server: by default blob compression is disabled

- indigo_framework:
	- add indigo_azimuth_distance() and used in dome drivers
	- define new PWM related properties

- indigo_solver: fix Sync & Center for some mounts with lazy sync operation

- indigo_prop_tool:
	- accept timeout 0
	- timeout countdown is started only if the connection is established

### New Drivers
- indigo_dome_beaver: Driver for NexDome domes with beaver controllers

- indigo_aux_astromechanics: ASTROMECHANICS Light Pollution Meter driver - untested

- indigo_focuser_astromechanics:  ASTROMECHANICS focuser driver - untested

### Driver fixes
- indigo_ccd_asi:
	- updated to SDK v.1.19.1 - fixes multi camera issue
	- fix unity gain calculation for cameras with full well < ADC resolution
	- show firmware version in the device info property

- indigo_focuser_asi:
	- updated to SDK v.1.4 - fixes 5v EAF crashes.

- indigo_whell_asi:
	- updated to SDK v.1.7
	- show firmware version in the device info property

- indigo_aux_rpio: add PWM support (see driver README.md)

## [2.0-152] - Sun Jun 13 2021
### Overall:
- indigo_client: fix http timeout for blob transfer - RPi is slow at compressing large images
- fix typo leading to server self lock.

### Driver fixes
- indigo_ccd_altair: macOS library architecture fixed

## [2.0-150] - Wed Jun 09 2021
### Overall:
- do not exit on libjpeg errors
- libjpeg error handling race fixed
- added indigo_raw_to_fits() call
- new tool added "indigo_raw_to_fits"
- indigo_platesolver: add step to some items and use degree sign in the labels
- MOUNT_EPOCH uses J2000 istead of JNOW
- added full NGC/IC catalog
- fix random behavior while removing related agent

### Driver Fixes
- indigo_aux_ppb: PPB Micro support added
- indigo_ccd_simulator: battery level property added

- indigo_ccd_asi:
	- revert SDK to v.1.16.3 for arm (because of regressions)
	- update SDK to v.1.18 for Intel Linux and macOS
	- default bandwidth set to 45 to allow 2 cameras to work at the same time

- indigo_agent_astrometry:
	- FOV property added
	- index download synchronized
	- index sizes shown in labels
	- fix missing sync before center
	- pix potential race
	- better error handling
	- make AGENT_PLATESOLVER_WCS property busy while mount is slewing
	- update AGENT_PLATESOLVER_WCS when ready but remain busy while slewing
	- pixel scale fixed

- indigo_ccd_ptp:
	- Nikon Z6II support
	- Nikon Z7II support
	- Canon M50mk2 support
	- ExposureDelayMode fixed for D780
	- NULL pointer exception handled

## [2.0-148] - Thu Apr 29 2021
### Overall:
- better memory management - allocated buffers reuse
- fixed some memory leaks
- fixed telescope alignment
- wifi channel can be selected for indigo sky in Access Point mode

### Driver Fixes
- indigo_mount_ioptron: add firmware version 3.1
- indigo_wheel_qhy: fix typo

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
- indigo_property_match_writable() added
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
