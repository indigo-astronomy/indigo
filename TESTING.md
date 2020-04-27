# Testing

All testing with physical hardware or simulators is documented in this file

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


