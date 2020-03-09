# Changelog

All notable changes to INDIGO framewark will be documented in this file.

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
