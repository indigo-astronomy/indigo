# Migration status

`Automated Tests` indicates whether a driver test exists in `indigo_test/` (simulator, fake SDK or opt-in hardware harness), not whether it has passed or provides complete coverage. Shared implementation alone does not count as a test of an OEM variant.

| Driver                  | API | Windows | Generator | Async Queues | Retested | Automated Tests | Comment |
| ----------------------- | --- | ------- | --------- | ------------ | -------- | --------------- | ------- |
| agent_alpaca            | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | ❌ No  |                                                         |
| agent_auxiliary         | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | ❌ No  |                                                         |
| agent_config            | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | ❌ No  |                                                         |
| agent_guider            | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | ❌ No  |                                                         |
| agent_imager            | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | ❌ No  |                                                         |
| agent_mount             | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | ❌ No  |                                                         |
| agent_scripting         | 3️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | ❌ No  |                                                         |
| agent_astrometry        | 2️⃣ | ✅ Yes | ⛔ N/A | ✅ Yes | ✅ Yes | ❌ No  |                                                         |
| agent_astap             | 2️⃣ | ❌ No  | ⛔ N/A | ❌ No  | ❌ No  | ❌ No  | ⛔ Needs fork() & pipes                                 |
| agent_snoop             | 2️⃣ | ❌ No  | ⛔ N/A | ❌ No  | ❌ No  | ❌ No  | 🚧 Obsolete                                             |
| ao_sx                   | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| aux_arteskyflat         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_asiair              | 2️⃣ | ⛔ N/A | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⛔ RPi only                                             |
| aux_astromechanics      | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_cloudwatcher        | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ❌ No  |                                                         |
| aux_dragonfly           | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| aux_dsusb               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| aux_fbc                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| aux_flatmaster          | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_flipflat            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_geoptikflat         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_joystick            | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| aux_mgbox               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes | 																												 |
| aux_ppb                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_rpio                | 2️⃣ | ⛔ N/A | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⛔ RPi only                                             |
| aux_rts                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | ✅ Yes |                                                         |
| aux_skyalert            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_sqm                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_svbpowerbox         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| aux_uch                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_upb                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| aux_upb3                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_usbdp               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| aux_wbplusv3            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| aux_wbprov3             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| aux_wcv4ec              | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| ccd_altair              | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes | ➡️ Touptek                                              |
| ccd_apogee              | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⏰ TODO - make boost_regex and libapogee for Windows    |
| ccd_asi                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| ccd_atik                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| ccd_atik2               | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes | ⛔ macOS only                                           |
| ccd_baccam              | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes | ➡️ Touptek                                              |
| ccd_bresser             | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes | ➡️ Touptek                                              |
| ccd_dsi                 | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ❌ No  |                                                         |
| ccd_fli                 | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| ccd_iidc                | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⏰ TODO - make libdc1394 for Windows                    |
| ccd_mallin              | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes | ➡️ Touptek                                              |
| ccd_mi                  | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⛔ Unix and Windows SDKs are not compatible             |
| ccd_ogma                | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes | ➡️ Touptek                                              |
| ccd_omegonpro           | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes | ➡️ Touptek                                              |
| ccd_pentax              | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | 🚧 Unfinished & stalled                                 |
| ccd_playerone           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| ccd_ptp                 | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| ccd_qhy                 | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ⚠️ HW  | ✅ Yes | ⏰ TODO - make libqhy for Windows                       |
| ccd_qhy2                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| ccd_qsi                 | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⏰ TODO - find SDK for windows                          |
| ccd_rising              | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes | ➡️ Touptek                                              |
| ccd_sbig                | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⏰ TODO - find SDK for windows                          |
| ccd_simulator           | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ⛔ N/A | ✅ Yes |                                                         |
| ccd_ssag                | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| ccd_ssg                 | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes | ➡️ Touptek                                              |
| ccd_svb                 | 3️⃣ | ✅ No  | ❌ No  | ❌ No  | ✅ HW  | ❌ No  |                                                         |
| ccd_svb2                | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes | ➡️ Touptek                                              |
| ccd_sx                  | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| ccd_touptek             | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| ccd_uvc                 | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⛔ libuvc is Unix only                                  |
| dome_baader             | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| dome_beaver             | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| dome_dragonfly          | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| dome_nexdome            | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| dome_nexdome3           | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| dome_simulator          | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ⛔ N/A | ✅ Yes |                                                         |
| dome_skyroof            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |                                                         |
| dome_talon6ror          | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| focuser_asi             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |                                                         |
| focuser_askar           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_astroasis       | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ✅ HW  | ❌ No  |                                                         |
| focuser_astromechanics  | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_dmfc            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_dsd             | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ✅ HW  | ✅ Yes |                                                         |
| focuser_efa             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_fc3             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | ✅ Yes |                                                         |
| focuser_fcusb           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| focuser_fli             | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| focuser_focusdreampro   | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| focuser_ioptron         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_lacerta         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_lakeside        | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_lunatico        | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| focuser_mjkzz           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_mjkzz_bt        | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⛔ macOS only                                           |
| focuser_moonlite        | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_mypro2          | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| focuser_nfocus          | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_nstep           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_optec           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_optecfl         | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_primaluce       | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| focuser_prodigy         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_qhy             | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ❌ No  | ✅ Yes |                                                         |
| focuser_robofocus       | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_steeldrive2     | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_usbv3           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| focuser_wemacro         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| focuser_wemacro_bt      | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⛔ macOS only                                           |
| gps_gpsd                | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⏰ TODO - make libgps for Windows                       |
| gps_nmea                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | ✅ Yes |                                                         |
| gps_simulator           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ⛔ N/A | ✅ Yes |                                                         |
| guider_asi              | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⏰ TODO - find SDK for windows                          |
| guider_cgusbst4         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | ✅ Yes |                                                         |
| guider_gpusb            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| mount_asi               | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ❌ No  | ❌ No  |                                                         |
| mount_ioptron           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| mount_lx200             | 3️⃣ | ✅ Yes | ❌ No  | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| mount_nexstar           | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ✅ Yes | ⏰ TODO - make libnexstar for Windows                   |
| mount_nexstaraux        | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| mount_pmc8              | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| mount_rainbow           | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| mount_simulator         | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ⛔ N/A | ✅ Yes |                                                         |
| mount_starbook          | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| mount_synscan           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| mount_temma             | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| polaralign_simulator    | 3️⃣ | ❌ No  | ❌ No  | ❌ No  | ⛔ N/A | ✅ Yes |                                                         |
| rotator_asi             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| rotator_falcon          | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| rotator_lunatico        | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| rotator_optec           | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ✅ Yes |                                                         |
| rotator_simulator       | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ⛔ N/A | ✅ Yes |                                                         |
| rotator_wa              | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ✅ HW  | ✅ Yes |                                                         |
| system_ascol            | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| wheel_asi               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| wheel_astroasis         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| wheel_atik              | 3️⃣ | ❌ No  | ✅ Yes | ✅ Yes | ❌ No  | ✅ Yes | ⏰ TODO - make libatik for Windows                      |
| wheel_fli               | 3️⃣ | ✅ Yes | ❌ No  | ❌ No  | ❌ No  | ❌ No  |                                                         |
| wheel_indigo            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Sim | ✅ Yes |                                                         |
| wheel_manual            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No  | ✅ Yes |                                                         |
| wheel_mi                | 2️⃣ | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ❌ No  | ⛔ Unix and Windows SDKs are not compatible             |
| wheel_optec             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |                                                         |
| wheel_playerone         | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ HW  | ✅ Yes |                                                         |
| wheel_qhy               | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |                                                         |
| wheel_quantum           | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |                                                         |
| wheel_sx                | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ⏰ TODO - make hidapi for Windows                       |
| wheel_trutek            | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |                                                         |
| wheel_xagyl             | 3️⃣ | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |                                                         |

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
