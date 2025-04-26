# Migrated to code generator

ao_sx

aux_arteskyflat
aux_astromechanics
aux_dsusb
aux_fbc - needs retest with physical hw
aux_flatmaster
aux_flipflat
aux_geoptikflat
aux_upb3

focuser_fcusb

gps_simulator
gps_nmea

guider_gpusb
guider_cgusbst4

rotator_simulator

wheel_manual

# Migrated to Windows


agent_alpaca
agent_auxiliary
agent_config
agent_guider
agent_imager
agent_mount
agent_scripting

aux_mgbox
aux_ppb
aux_rts
aux_skyalert
aux_sqm
aux_uch
aux_upb
aux_usbdp
aux_wbplusv3
aux_wbprov3
aux_wcv4ec

ccd_altair
ccd_asi
ccd_atik
ccd_bresser
ccd_dsi
ccd_fli
ccd_mallin
ccd_ogma
ccd_omegonpro
ccd_playerone
ccd_qhy2
ccd_rising
ccd_simulator
ccd_ssag
ccd_ssg
ccd_sx
ccd_touptek

dome_simulator

focuser_asi
focuser_astroasis
focuser_astromechanics
focuser_dmfc
focuser_fc3
focuser_fli
focuser_focusdreampro
focuser_ioptron
focuser_primaluce

mount_asi
mount_ioptron
mount_lx200
mount_rainbow
mount_simulator

rotator_falcon

wheel_asi
wheel_astroasis
wheel_indigo
wheel_fli
wheel_optec
wheel_playerone
wheel_qhy
wheel_quantum
wheel_trutek
wheel_xagyl

# TODO

agent_astap
agent_astrometry
agent_snoop

aux_cloudwatcher
aux_dragonfly
aux_joystick

ccd_apogee
ccd_iidc
ccd_mi
ccd_pentax
ccd_ptp
ccd_qhy
ccd_qsi
ccd_sbig
ccd_svb
ccd_svb2
ccd_uvc

dome_baader
dome_beaver
dome_dragonfly
dome_nexdome
dome_nexdome3
dome_skyroof
dome_talon6ror

focuser_dsd
focuser_efa
focuser_lacerta
focuser_lakeside
focuser_lunatico
focuser_mjkzz
focuser_moonlite
focuser_mypro2
focuser_nfocus
focuser_nstep
focuser_optec
focuser_optecfl
focuser_prodigy
focuser_qhy
focuser_robofocus
focuser_steeldrive2
focuser_usbv3
focuser_wemacro

gps_gpsd

guider_asi

mount_nexstar
mount_nexstaraux
mount_pmc8
mount_starbook
mount_synscan
mount_temma

rotator_asi
rotator_lunatico
rotator_optec
rotator_wa

system_ascol

wheel_atik
wheel_mi
wheel_sx

# COOKBOOK

On Linux or macOS for files, serial ports and TCP and UDP streams can be used uniform approach: handle is int, last error is in errno, for all kinds of communication can be used select/read pattern for reading with timeout. On Windows there are different kinds of handles for files, serial ports and network sockets. Also last error must be retrieved differently. That's why indigo_io must be replaced by something more sophisticated.

On Linux or macOS either ASCI or UTF-8 is used for strings, while on Windows may be used UTF-16, so special handling is necessary for communication with Windows SDKs.

Use Visual Studio 2022 with Desktop C++ development option installed and MSC. Porting with various GCC clones and Linux compatibility layers may look easier, but makes no sense, don't waste time on it. You will sooner or later hit compatibility issues between such binaries and standard Windows SDKs and applications.

## Serial communication drivers

1. You will probably need to remove some includes which are not used anyway (<unistd.h>, <sys/time.h>, <sys/termios.h>, <libusb.h> etc.), if driver needs libusb, include <indigo/indigo_usb_utils.h> before including SDK headers.

2. Replace <indigo/indigo_io.h> with <indigo/indigo_uni_io.h>.

3. Replace "int handle" declaration with "indigo_uni_handle *handle".

4. Replace occurences of "PRIVATE_DATA->handle = indigo_open_serial_...(...)" with "PRIVATE_DATA->handle = indigo_uni_open_serial_...(..., INDIGO_LOG_DEBUG)".

5. Replace occurences of "close(PRIVATE_DATA->handle); PRIVATE_DATA->handle = 0;" with "indigo_uni_close(&PRIVATE_DATA->handle);"

6. Search for "if (PRIVATE_DATA->handle > 0)...", it must be replaced by "if (PRIVATE_DATA->handle != NULL)...".

7. Refactor xxx_command(...) function to the pattern like this:

```
static bool xxx_command(indigo_device *device, char *command, char *response, int max) {
  // discard pending input with short timeout
  if (indigo_uni_discard(PRIVATE_DATA->handle) >= 0) {
     // write command
    if (indigo_uni_write(PRIVATE_DATA->handle, command, (long)strlen(command)) > 0) {
      if (response != NULL) {
        // read input terminated with \n, don't copy \r and \n with 1 second timeout.
        if (indigo_uni_read_section(PRIVATE_DATA->handle, response, max, "\n", "\r\n", INDIGO_DELAY(1)) > 0) {
          return true;
        }
      }
    }
  }
  return false;
}
```

8. There are functions like indigo_uni_set_dtr(), indigo_uni_set_rts(), indigo_uni_set_cts() etc. defined in indigo_uni_io.h if the driver needs special handling for signal bits.

9. Log level used for indigo_uni_open_serial_... or any other function creating handle will be used in all subsequent I/O operation in uniform manner so don't use any other logging for it. Use -INDIGO_LOG_DEBUG instead of INDIGO_LOG_DEBUG to make logging for "binary" protocols.

10. Make Visual Studio project by copying some existing project, e.g. aux_rts/indigo_aux_rts.vcxproj, indigo_aux_rts.vcxproj.filters, indigo_aux_rts.vcxproj.user and rename these files to match the current driver name.

11. Open these files with some text editor and replace all occurences of "aux_rts" with the current driver name.

12. Open indigo_windows.sln in Visual Studio and add just created project.

13. Try to build...

14. Pls. cast double to int explicitly, don't use unsigned ints if it is not really necessary, because all this produces lots and lots of warnings in MSC.

15. If the driver can be build, add project as dependency to indigo project.

16. Move the driver from TODO section to ALREADY_DONE section in this file.

## SDK based drivers

Make sure that Windows SDK uses the same API as Linux/macOS SDK, it is not necessary true (e.g. Moravian Instruments SDK). To test the driver you will need "system driver" for Windows, if you don't have it, you may try to use Zadig (https://zadig.akeo.ie) to create a generic one.

1. Copy SDK to <DRIVER_BASE>bin_external/<SDK>/lib/windows/Win32 and  <DRIVER_BASE>bin_external/<SDK>/lib/windows/x64 ("Win32" and "x64" matches architecture in Visual Studio, don't use other names), copy .DLL and .LIB files.

2. Make Visual Studio project by copying some existing project, e.g. ccd_playerone/indigo_ccd_playerone.vcxproj, indigo_ccd_playerone.vcxproj.filters, indigo_ccd_playerone.vcxproj.user and rename these files to match the current driver name.

3. Open these files with some text editor and replace all occurences of "indigo_ccd_playerone" with the current driver name, "bin_externals\libplayeronecamera" with the path to SDK, "PlayerOneCamera.lib" with SDK library.

4. Open indigo_windows.sln in Visual Studio and add just created project.

5. Try to build...

6. You will probably need to remove some includes which are not used anyway (<unistd.h>, <sys/time.h>, <libusb.h> etc.), if driver needs libusb, include <indigo/indigo_usb_utils.h> before including SDK headers.

7. Pls. cast double to int explicitly, don't use unsigned ints if it is not really necessary, because all this produces lots and lots of warnings in MSC.

8. Make sure SDK API doesn't use wide chars. If it does, use INDIGO_WCHAR_TO_CHAR() or INDIGO_CHAR_TO_WCHAR() macros defined in <indigo/indigo_uni_io.h> to handle them correctly.

9. If the driver can be build, add project as dependency to indigo project.

10. Move the driver from TODO section to ALREADY_DONE section in this file.
