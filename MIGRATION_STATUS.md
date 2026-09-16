# Migration status

`Automated Tests (Sim / HW)` gives the number of hardware-free integration cases and opt-in physical-hardware cases. It does not indicate whether they have passed or provide complete coverage. Shared implementation alone does not count as a test of an OEM variant.

| Driver                  | API | Windows | Generator | Async Queues | Retested | Automated Tests (Sim / HW) | Comment |
| ----------------------- | --- | ------- | --------- | ------------ | -------- | -------------------------- | ------- |
| agent_alpaca            | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | 0 / 0 | |
| agent_auxiliary         | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | 0 / 0 | |
| agent_config            | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | 56 / 0 | |
| agent_guider            | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | 88 / 0 | |
| agent_imager            | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | 39 / 0 | |
| agent_mount             | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | 69 / 0 | |
| agent_scripting         | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | 60 / 0 | |
| agent_astrometry        | 2️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Sim | 31 / 0 | |
| agent_astap             | 2️⃣ | ❌ No  | ⛔ N/A | ❌ No  | ❌ No  | 0 / 0 | ⛔ Needs fork() & pipes |
| agent_snoop             | 2️⃣ | ❌ No  | ⛔ N/A | ❌ No  | ❌ No  | 0 / 0 | 🚧 Obsolete |
| ao_sx                   | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 9 / 0 | |
| aux_arteskyflat         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 3 / 0 | |
| aux_asiair              | 2️⃣ | ⛔ N/A | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | ⛔ RPi only |
| aux_astromechanics      | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 1 / 0 | |
| aux_cloudwatcher        | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 5 / 0 | |
| aux_dragonfly           | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 5 / 0 | |
| aux_dsusb               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 10 / 0 | |
| aux_fbc                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 1 / 0 | |
| aux_flatmaster          | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 1 / 0 | |
| aux_flipflat            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 1 / 0 | |
| aux_geoptikflat         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 0 / 0 | |
| aux_joystick            | 3️⃣ | ❌ No  | ❌ No  | ✅ Yes | ✅ HW  | 14 / 2 | |
| aux_mgbox               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 32 / 0 | |
| aux_ppb                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 3 / 0 | |
| aux_rpio                | 2️⃣ | ⛔ N/A | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | ⛔ RPi only |
| aux_rts                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | 0 / 0 | |
| aux_skyalert            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 3 / 0 | |
| aux_sqm                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 3 / 0 | |
| aux_svbpowerbox         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 1 / 0 | |
| aux_uch                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 1 / 0 | |
| aux_upb                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 0 / 0 | |
| aux_upb3                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 2 / 0 | |
| aux_usbdp               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 2 / 0 | |
| aux_wbplusv3            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 1 / 0 | |
| aux_wbprov3             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 1 / 0 | |
| aux_wcv4ec              | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 1 / 0 | |
| ccd_altair              | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 1 | ➡️ Touptek |
| ccd_apogee              | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | ⏰ TODO - make boost_regex and libapogee for Windows |
| ccd_asi                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 51 / 1 | |
| ccd_atik                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 40 / 1 | |
| ccd_atik2               | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ✅ HW  | 24 / 0 | ⛔ macOS only |
| ccd_baccam              | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 0 | ➡️ Touptek |
| ccd_bresser             | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 0 | ➡️ Touptek |
| ccd_dsi                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 0 / 0 | |
| ccd_fli                 | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | |
| ccd_iidc                | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ✅ HW  | 14 / 0 | ⏰ TODO - make libdc1394 for Windows |
| ccd_mallin              | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 0 | ➡️ Touptek |
| ccd_mi                  | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ✅ HW  | 17 / 0 | ⛔ Unix and Windows SDKs are not compatible |
| ccd_ogma                | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 0 | ➡️ Touptek |
| ccd_omegonpro           | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 0 | ➡️ Touptek |
| ccd_pentax              | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | 🚧 Unfinished & stalled |
| ccd_playerone           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 48 / 1 | |
| ccd_ptp                 | 3️⃣ | ❌ No  | ❌ No  | ✅ Yes | ✅ HW  | 128 / 4 | |
| ccd_qhy                 | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ⚠️ HW  | 0 / 1 | ⏰ TODO - make libqhy for Windows |
| ccd_qhy2                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 0 / 0 | |
| ccd_qsi                 | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | ⏰ TODO - find SDK for windows |
| ccd_rising              | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 0 | ➡️ Touptek |
| ccd_sbig                | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | ⏰ TODO - find SDK for windows |
| ccd_simulator           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 19 / 0 | |
| ccd_ssag                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 13 / 2 | |
| ccd_ssg                 | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 0 | ➡️ Touptek |
| ccd_svb                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 47 / 1 | |
| ccd_svb2                | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 0 | ➡️ Touptek |
| ccd_sx                  | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 24 / 0 | |
| ccd_touptek             | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 28 / 1 | |
| ccd_uvc                 | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | ⛔ libuvc is Unix only |
| dome_baader             | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 1 / 0 | |
| dome_beaver             | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 1 / 0 | |
| dome_dragonfly          | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | |
| dome_nexdome            | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 1 / 0 | |
| dome_nexdome3           | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 1 / 0 | |
| dome_simulator          | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 7 / 0 | |
| dome_skyroof            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | 1 / 0 | |
| dome_talon6ror          | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 1 / 0 | |
| focuser_asi             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | 10 / 0 | |
| focuser_askar           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 9 / 0 | |
| focuser_astroasis       | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ✅ HW  | 0 / 0 | |
| focuser_astromechanics  | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 1 / 0 | |
| focuser_dmfc            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 1 / 0 | |
| focuser_dsd             | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ✅ HW  | 1 / 0 | |
| focuser_efa             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 56 / 0 | |
| focuser_fc3             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | 1 / 0 | |
| focuser_fcusb           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 0 / 0 | |
| focuser_fli             | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | |
| focuser_focusdreampro   | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | 1 / 0 | |
| focuser_ioptron         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 40 / 0 | |
| focuser_lacerta         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 43 / 0 | |
| focuser_lakeside        | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 16 / 0 | |
| focuser_lunatico        | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | |
| focuser_mjkzz           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 19 / 0 | |
| focuser_mjkzz_bt        | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | ⛔ macOS only |
| focuser_moonlite        | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 20 / 0 | |
| focuser_mypro2          | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | 1 / 0 | |
| focuser_nfocus          | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 10 / 0 | |
| focuser_nstep           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 12 / 0 | |
| focuser_optec           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 16 / 0 | |
| focuser_optecfl         | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ Sim | 0 / 0 | |
| focuser_primaluce       | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 2 / 0 | |
| focuser_prodigy         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 0 / 0 | |
| focuser_qhy             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 30 / 0 | |
| focuser_robofocus       | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 18 / 0 | |
| focuser_steeldrive2     | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 49 / 0 | |
| focuser_usbv3           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 1 / 0 | |
| focuser_wemacro         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 22 / 0 | |
| focuser_wemacro_bt      | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | ⛔ macOS only |
| gps_gpsd                | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | ⏰ TODO - make libgps for Windows |
| gps_nmea                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | 1 / 0 | |
| gps_simulator           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 4 / 0 | |
| guider_asi              | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ✅ Sim | 17 / 0 | ⏰ TODO - find SDK for windows |
| guider_cgusbst4         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | 0 / 0 | |
| guider_gpusb            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 0 / 0 | |
| mount_asi               | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ❌ No  | 0 / 0 | |
| mount_ioptron           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 104 / 0 | |
| mount_lx200             | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ Sim | 70 / 0 | |
| mount_nexstar           | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ✅ Sim | 13 / 0 | ⏰ TODO - make libnexstar for Windows |
| mount_nexstaraux        | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 2 / 0 | |
| mount_pmc8              | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 11 / 0 | |
| mount_rainbow           | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | 1 / 0 | |
| mount_simulator         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 16 / 0 | |
| mount_starbook          | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 11 / 0 | |
| mount_synscan           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 16 / 0 | |
| mount_temma             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 13 / 0 | |
| polaralign_simulator    | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 11 / 0 | |
| rotator_asi             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 11 / 0 | |
| rotator_falcon          | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 1 / 0 | |
| rotator_lunatico        | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 11 / 0 | |
| rotator_optec           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 10 / 0 | |
| rotator_simulator       | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 9 / 0 | |
| rotator_wa              | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 34 / 0 | |
| system_ascol            | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | |
| wheel_asi               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 5 / 0 | |
| wheel_astroasis         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 10 / 0 | |
| wheel_atik              | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ❌ No  | 0 / 0 | ⏰ TODO - make libatik for Windows |
| wheel_fli               | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | 0 / 0 | |
| wheel_indigo            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | 5 / 0 | |
| wheel_manual            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | 0 / 0 | |
| wheel_mi                | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ✅ Sim | 12 / 0 | ⛔ Unix and Windows SDKs are not compatible |
| wheel_optec             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | 1 / 0 | |
| wheel_playerone         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | 12 / 0 | |
| wheel_qhy               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | 3 / 0 | |
| wheel_quantum           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | 1 / 0 | |
| wheel_sx                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | 0 / 0 | ⏰ TODO - make hidapi for Windows |
| wheel_trutek            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | 1 / 0 | |
| wheel_xagyl             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | 1 / 0 | |

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
