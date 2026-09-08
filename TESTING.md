# Testing

All testing with physical hardware or simulators is documented in this file

## September 8th 2026 (macOS 26.6.2, arm64)

| driver | device | Result | Comments |
| ----- | ----- | ----- | ----- |
| ccd_playerone | Mars-C II, color + ST4, 1936 × 1100 | Passed tested workflows | Generated driver 0x03000012, working tree based on 0f52fe3bc0b826767c91d3e7bd409fe8370abd69; bundled SDK 3.10.1 / API 20260430. Firmware revision not captured. |

Ran `make -C indigo_test test-ccd-playerone-hw` with real USB/SDK access, then repeated with `HW_HOTPLUG=1`. RAW images, short exposure, abort/reacquire, finite and indefinite stream/abort, guider commands, guider-first shared connection, CCD disconnect while guider remains connected, reconnect and driver shutdown/reinitialization passed. The physical hot-plug run removed the USB cable while streaming; after the user reconnected it, both exposure and guiding commands succeeded, followed by successful driver shutdown/reinitialization. The SDK library remains loaded in this harness. The pre-migration libusb mutex-destroy assertion did not recur in these generated-driver runs.

The expanded hardware run also passed RAW8/RGB24/RAW16/MONO8 acquisition, 256 × 256 ROI at origin (16, 24) with bins 1 and 2, gain/configuration SAVE–LOAD roundtrip in a temporary directory, offset changes, dark frames and a completed five-second exposure with simultaneous guider commands. A sustained stream delivered 110 frames in approximately ten seconds. Initial settings were restored; the test did not write a persistent camera suffix.

Image headers/dimensions and BLOB delivery were checked; optical scene quality, exhaustive ROI/bin combinations, cooling, persistent suffix writes and multi-camera survival were not validated. Fake SDK timing measures command entry only. See `indigo_drivers/ccd_playerone/REFACTOR.md` and `indigo_test/CHANGES.md` for the separate automated coverage and remaining matrix.

Final migration acceptance on the same Mars-C II: `indigo_test/build/hardware/test_ccd_playerone_hw --run --acceptance` exercised physical unplug/replug during streaming, idle, a 120-second exposure and a 60-second guider command; every phase recovered with a new image and guide pulse. The first wait expired without physical removal and was safely retried. The suffix subtest initially used a mistaken 17-byte fixture, correctly received ALERT, and restored the empty original. After correcting the fixture and adding a compile-time length assertion, `--run --suffix` passed writing `INDIGOTEST123456` (16 bytes), physical replug/name verification, clearing/replug verification, and restoration of the original empty suffix. Only successful SDK writes are flash writes; the oversized request was rejected in the driver.

`make -C indigo_test test-ccd-playerone-reload-hw` / the built reload executable passed with actual `dlclose`/`dlopen` of the driver, a fresh queue and a fresh exposure after reload. This executable links the shared INDIGO bus and does not perform CONFIG SAVE. The static harness remains an INIT/SHUTDOWN test; loader API success is not a claim about whether macOS physically unmaps every dependent SDK image.

Available equipment was confirmed by the user: only Mars-C II; no second/cooled camera. Other models/OEM/Saturn behavior, cooling and multi-camera survival remain unavailable. Linux/Windows execution is not covered by this macOS run.

## April 26th 2020 (macOS Catalina 10.15.4)

| driver | device | Result | Comments |
| ----- | ----- | ----- | ----- |
| ao_sx | arduino simulator | :white_check_mark: | |
| aux_arteskyflat | arduino simulator | :white_check_mark: | |
| aux_flatmaster | arduino simulator | :white_check_mark: | |
| aux_arteskyflat | arduino simulator | :white_check_mark: | |
| aux_flipflat | arduino simulator | :white_check_mark: | |
| aux_ppb | arduino simulator | :white_check_mark: | simulator fixed |
| aux_rts | arduino simulator | :white_check_mark: | |
| aux_sqm | arduino simulator | :white_check_mark: | |
| aux_upb | arduino simulator | :white_check_mark: | |
| aux_usbdp | arduino simulator | :white_check_mark: | |
| ccd_altair | GP224C | :white_check_mark: | |
| ccd_asi | ASI120MC-S | :white_check_mark: | |
| ccd_atik | Titan C | :white_check_mark: | |
| ccd_dsi | DSI 1 Colour | :white_check_mark: | with code change (camera can't be opened during hot plug processing) |
| ccd_iidc | Atik GP| :white_check_mark: | |
| ccd_mi | G0-300 | :white_check_mark: | |
| ccd_ptp | Canon 400D, Nikon D5600 | :white_check_mark: | |
| ccd_qhy | QHY5, QHY5-LII | :x: | hot unplug fails |
| ccd_ssag | QHY5 | :white_check_mark: | |
| ccd_sx | Lodestar | :white_check_mark: | |
| ccd_uvc | SBONY 205 | :white_check_mark: | |
| focuser_efa | arduino simulator | :white_check_mark: | |
| focuser_dmfc | arduino simulator | :white_check_mark: | |
| focuser_lakeside | arduino simulator | :white_check_mark: | |
| focuser_mjkzz | arduino simulator | :white_check_mark: | |
| focuser_moonlite | arduino simulator | :white_check_mark: | |
| focuser_nfocus | arduino simulator | :white_check_mark: | |
| focuser_nstep | arduino simulator | :white_check_mark: | |
| focuser_optec | arduino simulator | :white_check_mark: | |
| focuser_steeldrive2 | arduino simulator | :white_check_mark: | |
| mount_ioptron | arduino simulator | :white_check_mark: | just connect/disconnect |
| mount_lx200 | arduino simulator | :white_check_mark: | just connect/disconnect |
| mount_nexstar | arduino simulator | :white_check_mark: | just connect/disconnect |
| mount_synscan | arduino simulator | :white_check_mark: | just connect/disconnect |
| wheel_optec | arduino simulator | :white_check_mark: | just connect/disconnect |
| wheel_quantum | arduino simulator | :white_check_mark: | just connect/disconnect |
| wheel_trutek | arduino simulator | :white_check_mark: | just connect/disconnect |
| wheel_xagyl | arduino simulator | :white_check_mark: | just connect/disconnect |
| ccd_ica | Canon 400D, Nikon D5600 | :x: | hot unplug fails |

## April 27th 2020 (Linux)

| driver | device | Result | Comments |
| ----- | ----- | ----- | ----- |
| focuser_dsd | DSD AF1 | :white_check_mark: | - |
| focuser_asi | ZWO EAFocuser | :white_check_mark: | - |
| wheel_asi   | ZWO EFW mini  | :white_check_mark: | - |
| ccd_dsi     | Meade DSI Pro II | :white_check_mark: | fixed crash on hotplug |
| ccd_qhy     | QHY6-M | :x: | hot-plug almost does not work |
| ccd_qhy2     | QHY6-M | :x: | unstable but works somehow |
| ccd_asi     | ZWO ASI224MC | :white_check_mark: | if unplugged while reading the image timeout is > 60s, everything else works |
| dome_nexdome | arduino simulator | :white_check_mark: | - |
| dome_nexdome3 | arduino simulator | :white_check_mark: | - |
| aux_cloudwatcher | AAG CloudWatcher | :white_check_mark: | - |
| aux_dragonfly | Dragonfly Controller | :white_check_mark: | - |
| aux_dome_dragonfly | Dragonfly Controller | :white_check_mark: | - |
| aux_focuser_lunatico | Armadillo Controller | :white_check_mark: | - |
| aux_rotator_lunatico | Armadillo Controller | :white_check_mark: | - |
| indigo_gps_nmea | Ublox GPS mouse | :white_check_mark: | There was a bug in the driver -> indigo_set_timer(device, 0, gps_connect_callback, &PRIVATE_DATA->timer_callback) should be  indigo_set_timer(device, 0, gps_connect_callback, NULL) |
