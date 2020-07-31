# Changelog

All notable changes to INDIGO framewark will be documented in this file.

## [2.0-124] - Sat Aug 01 2020
### Overal:
- Double connection issue fixed
- PSF computation fixed

### Driver Fixes:
- indigo_aux_ppb: PPB Advanced support added
- indigo_aux_sqm: crash fixed
- indigo_ccd_qsi: initial slot position fixed
- indigo_ccd_mi: SDK updated to 0.5.6
- indigo_ccd_asi: SDK updated to 1.15.0617
- indigo_wheel_asi: SDK updated to 1.5.0615
- indigo_focuser_asi: SDK updated to 0.2.0605
- indigo_mount_lx200: meridian flip control added to AvalonGO dialect
- indigo_ccd_atik: SDK updated to 2020_06_23
- indigo_ccd_qhy2: SDK updated to V2020.06.05 for macOS and  V2020.05.22 for linux

### New Drivers:
- indigo_mount_pcm8

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
