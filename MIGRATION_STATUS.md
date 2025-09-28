# Migration status

| Driver | API | Windows | Generator | Retested | Comment |
| ----- | ----- | ----- | ----- | ----- | ----- |
| agent_alpaca | 3️⃣ | ✅ Yes | ⛔ N/A | ❌ No | |
| agent_auxiliary | 3️⃣ | ✅ Yes | ⛔ N/A | ❌ No | |
| agent_config | 3️⃣ | ✅ Yes | ⛔ N/A | ❌ No | |
| agent_guider | 3️⃣ | ✅ Yes | ⛔ N/A | ❌ No | |
| agent_imager | 3️⃣ | ✅ Yes | ⛔ N/A | ❌ No | |
| agent_mount | 3️⃣ | ✅ Yes | ⛔ N/A | ❌ No | |
| agent_scripting | 3️⃣ | ✅ Yes | ⛔ N/A | ❌ No | |
| agent_astrometry | 2️⃣ | ❌ No | ⛔ N/A | ❌ No | ⛔ Needs fork() & pipes |
| agent_astap | 2️⃣ | ❌ No | ⛔ N/A | ❌ No | ⛔ Needs fork() & pipes |
| agent_snoop | 2️⃣ | ❌ No | ⛔ N/A | ❌ No | 🚧 Obsolete |
| ao_sx | 3️⃣ | ✅ Yes | ✅ Yes | ✅ HW | |
| aux_arteskyflat | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_asiair | 2️⃣ | ❌ No | ❌ No | ❌ No | ⛔ RPi only |
| aux_astromechanics | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_cloudwatcher | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| aux_dragonfly | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| aux_dsusb | 3️⃣ | ❌ No | ✅ Yes | ✅ HW | ⏰ TODO - make libdsusb for Windows |
| aux_fbc | 3️⃣ | ✅ Yes | ✅ Yes | ✅ HW | |
| aux_flatmaster | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_flipflat | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_geoptikflat | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_joystick | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| aux_mgbox | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| aux_ppb | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_rpio | 2️⃣ | ❌ No | ❌ No | ❌ No | ⛔ RPi only |
| aux_rts | 3️⃣ | ✅ Yes | ✅ Yes | ❌ No | |
| aux_skyalert | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_sqm | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_uch | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_upb | 3️⃣ | ✅ Yes | ✅ Yes | ✅ HW | |
| aux_upb3 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_usbdp | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_wbplusv3 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_wbprov3 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| aux_wcv4ec | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| ccd_altair | 3️⃣ | ✅ Yes | ❌ No | ✅ HW | |
| ccd_apogee | 2️⃣ | ❌ No | ❌ No | ❌ No | ⏰ TODO - make boost_regex and libapogee for Windows |
| ccd_asi | 3️⃣ | ✅ Yes | ❌ No | ✅ HW | |
| ccd_atik | 3️⃣ | ✅ Yes | ❌ No | ❌ No | ⏰ TODO - find SDK for arm64 macOS |
| ccd_atik2 | 3️⃣ | ❌ No | ❌ No | ❌ No | ⛔ macOS only, temporary workaround |
| ccd_bresser | 3️⃣ | ✅ Yes | ❌ No | ❌ No | ➡️ Touptek |
| ccd_dsi | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| ccd_fli | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| ccd_iidc | 2️⃣ | ❌ No | ❌ No | ❌ No | ⏰ TODO - make libdc1394 for Windows |
| ccd_mallin | 3️⃣ | ✅ Yes | ❌ No | ❌ No | ➡️ Touptek |
| ccd_mi | 2️⃣ | ❌ No | ❌ No | ❌ No | ⛔ Unix and Windows SDKs are not compatible |
| ccd_ogma | 3️⃣ | ✅ Yes | ❌ No | ❌ No | ➡️ Touptek |
| ccd_omegonpro | 3️⃣ | ✅ Yes | ❌ No | ❌ No | ➡️ Touptek |
| ccd_pentax | 2️⃣ | ❌ No | ❌ No | ❌ No | 🚧 Unfinished & stalled |
| ccd_playerone | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| ccd_ptp | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| ccd_qhy | 3️⃣ | ❌ No | ❌ No | ❌ No | ⏰ TODO - make libqhy for Windows |
| ccd_qhy2 | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| ccd_qsi | 2️⃣ | ❌ No | ❌ No | ❌ No | ⏰ TODO - find SDK for windows |
| ccd_rising | 3️⃣ | ✅ Yes | ❌ No | ✅ HW | ➡️ Touptek |
| ccd_sbig | 2️⃣ | ❌ No | ❌ No | ❌ No | ⏰ TODO - find SDK for windows |
| ccd_simulator | 3️⃣ | ✅ Yes | ❌ No | ⛔ N/A | |
| ccd_ssag | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| ccd_ssg | 3️⃣ | ✅ Yes | ❌ No | ❌ No | ➡️ Touptek |
| ccd_svb | 2️⃣ | ✅ No | ❌ No | ✅  HW | |
| ccd_svb2 | 3️⃣ | ✅ Yes | ❌ No | ❌ No | ➡️ Touptek |
| ccd_sx | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| ccd_touptek | 3️⃣ | ✅ Yes | ❌ No | ✅ HW | |
| ccd_uvc | 2️⃣ | ❌ No | ❌ No | ❌ No | ⛔ libuvc is Unix only |
| dome_baader | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| dome_beaver | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| dome_dragonfly | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| dome_nexdome | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| dome_nexdome3 | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| dome_simulator | 3️⃣ | ✅ Yes | ✅ Yes | ⛔ N/A | |
| dome_skyroof | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| dome_talon6ror | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_asi | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| focuser_astroasis | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| focuser_astromechanics | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| focuser_dmfc | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| focuser_dsd | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_efa | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_fc3 | 3️⃣ | ✅ Yes | ✅ Yes | ❌ No | |
| focuser_fcusb | 3️⃣ | ❌ No | ✅ Yes | ✅ HW | ⏰ TODO - make libfcusb for Windows |
| focuser_fli | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| focuser_focusdreampro | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| focuser_ioptron | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| focuser_lacerta | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_lakeside | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_lunatico | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_mjkzz | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_mjkzz_bt | 2️⃣ | ❌ No | ❌ No | ❌ No | ⛔ macOS only |
| focuser_moonlite | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_mypro2 | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_nfocus | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_nstep | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_optec | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_optecfl | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_primaluce | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| focuser_prodigy | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_qhy | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_robofocus | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_steeldrive2 | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_usbv3 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ HW | |
| focuser_wemacro | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| focuser_wemacro_bt | 2️⃣ | ❌ No | ❌ No | ❌ No | ⛔ macOS only |
| gps_gpsd | 2️⃣ | ❌ No | ❌ No | ❌ No | ⏰ TODO - make libgps for Windows |
| gps_nmea | 3️⃣ | ✅ Yes | ✅ Yes | ❌ No | |
| gps_simulator | 3️⃣ | ✅ Yes | ✅ Yes | ⛔ N/A | |
| guider_asi | 2️⃣ | ❌ No | ❌ No | ❌ No | ⏰ TODO - find SDK for windows |
| guider_cgusbst4 | 3️⃣ | ✅ Yes | ✅ Yes | ❌ No | |
| guider_gpusb | 3️⃣ | ❌ No | ✅ Yes | ✅ HW | ⏰ TODO - make libgpusb for Windows |
| mount_asi | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| mount_ioptron | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| mount_lx200 | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| mount_nexstar | 2️⃣ | ❌ No | ❌ No | ❌ No | ⏰ TODO - make libnexstar for Windows |
| mount_nexstaraux | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| mount_pmc8 | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| mount_rainbow | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| mount_simulator | 3️⃣ | ✅ Yes | ❌ No | ⛔ N/A | |
| mount_starbook | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| mount_synscan | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| mount_temma | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| rotator_asi | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| rotator_falcon | 3️⃣ | ✅ Yes | ✅ Yes | ✅ HW | |
| rotator_lunatico | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| rotator_optec | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| rotator_simulator | 3️⃣ | ✅ Yes | ✅ Yes | ⛔ N/A | |
| rotator_wa | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| system_ascol | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| wheel_asi | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| wheel_astroasis | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| wheel_atik | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| wheel_fli | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| wheel_indigo | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Sim | |
| wheel_manual | 3️⃣ | ✅ Yes | ✅ Yes | ❌ No | |
| wheel_mi | 2️⃣ | ❌ No | ❌ No | ❌ No | |
| wheel_optec | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| wheel_playerone | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| wheel_qhy | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| wheel_quantum | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| wheel_sx | 2️⃣ | ❌ No | ❌ No | ❌ No | ⏰ TODO - make hidapi for Windows |
| wheel_trutek | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |
| wheel_xagyl | 3️⃣ | ✅ Yes | ❌ No | ❌ No | |

# COOKBOOK for migration to API 3.0 and Windows

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
