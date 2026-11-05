# Runtime coverage

135 script cases;108 public methods exercised through the Scripting Agent. Passing negative cases demonstrate failure handling, not successful equipment operations. The primitive authenticated transport relay is explicit; native token propagation is tested separately and fails. Case numbers below disambiguate repeated selection/reset operation labels.

| Method | Passing runtime cases | Failing runtime cases |
| --- | --- | --- |
| `abort_on_failure` | #12 abort on missing camera property (expected Alert); #16 explicit policy transition (expected Alert) | None |
| `break_at` | #9 time break taken=true; #10 time break taken=false; #11 break without destination | None |
| `break_at_ha` | #105 HA break taken=true; #107 HA break taken=false | None |
| `break_if_solar_altitude_above` | #104 sky breaks taken=true; #106 sky breaks taken=false | None |
| `break_if_target_altitude_below` | #104 sky breaks taken=true; #106 sky breaks taken=false | None |
| `calibrate_guiding` | #112 guiding calibration real simulator | None |
| `capture_batch` | #62 capture batch named=false; #64 capture batch named=true; #66 FITS custom header present; #67 FITS custom header removed; #116 guided capture with dithering; #120 natural transit meridian flip and capture resume; #122 abort imager paused before natural transit (expected Alert); #124 pause and resume actual capture; #125 abort finite capture (expected Alert); #132 natural transit flip with native solver correction | None |
| `capture_stream` | #63 capture stream named=false; #65 capture stream named=true | None |
| `clear_focuser_selection` | #72 clear autofocus selection | None |
| `clear_guider_selection` | #117 clear guider selection | None |
| `continue_on_failure` | #13 continue on missing camera property; #16 explicit policy transition (expected Alert) | None |
| `disable_cooler` | #49 disable_cooler [] | None |
| `disable_dithering` | #61 prepare finite image batches; #116 guided capture with dithering | None |
| `disable_dome_slaving` | #86 disable_dome_slaving [] | None |
| `disable_field_derotation` | #87 disable_field_derotation [] | None |
| `disable_filter_offsets` | #78 filter offsets enabled then disabled | None |
| `disable_ha_limit` | #84 disable_ha_limit [] | None |
| `disable_joystick_control` | #88 disable_joystick_control [] | None |
| `disable_meridian_flip` | #61 prepare finite image batches; #118 meridian flip options readback; #120 natural transit meridian flip and capture resume; #123 restore flip option after transit abort; #132 natural transit flip with native solver correction | None |
| `disable_reset_loop_content_state` | #4 nested loops reset=false | None |
| `disable_time_limit` | #85 disable_time_limit [] | None |
| `disable_tracking` | #133 disable_tracking [] | None |
| `disable_verbose` | #2 messages and quoted evaluation | None |
| `dome_close` | #102 dome_close [] | None |
| `dome_open` | #99 dome_open [] | None |
| `dome_park` | #103 dome_park [] | None |
| `dome_slew` | #100 dome_slew [45] | None |
| `dome_unpark` | #98 dome unpark | None |
| `enable_cooler` | #48 enable_cooler [25] | None |
| `enable_dithering` | #116 guided capture with dithering | None |
| `enable_dome_slaving` | #81 enable_dome_slaving [] | None |
| `enable_field_derotation` | #82 enable_field_derotation [] | None |
| `enable_filter_offsets` | #78 filter offsets enabled then disabled | None |
| `enable_ha_limit` | #79 enable_ha_limit [] | None |
| `enable_joystick_control` | #83 enable_joystick_control [] | None |
| `enable_meridian_flip` | #118 meridian flip options readback; #120 natural transit meridian flip and capture resume; #122 abort imager paused before natural transit (expected Alert); #132 natural transit flip with native solver correction | None |
| `enable_reset_loop_content_state` | #3 nested loops reset=true | None |
| `enable_time_limit` | #80 enable_time_limit [] | None |
| `enable_tracking` | #95 enable_tracking [] | None |
| `enable_verbose` | #2 messages and quoted evaluation | None |
| `evaluate` | #2 messages and quoted evaluation; #20 fresh sequence after abort; #21 save private configuration; #111 prepare guider and solver relations; #119 prepare transit target 0.008; #121 prepare transit target 0.05; #128 solver setup with installed indexes; #131 prepare solver-assisted transit target; #135 reset completed sequence through bus | None |
| `focus` | #73 autofocus without focuser fails (expected Alert); #76 autofocus with real simulator | None |
| `focus_ignore_failure` | #74 autofocus ignores process failure and restores batch | None |
| `home` | #94 home mount simulator | None |
| `load_config` | #23 missing configuration fails (expected Alert) | #22 restore private configuration |
| `load_driver` | #24 missing driver fails (expected Alert); #25 load unused simulator | None |
| `make_dome_slaving_not_persistent` | #90 make_dome_slaving_not_persistent [] | None |
| `make_dome_slaving_persistent` | #89 make_dome_slaving_persistent [] | None |
| `make_field_derotation_not_persistent` | #92 make_field_derotation_not_persistent [] | None |
| `make_field_derotation_persistent` | #91 make_field_derotation_persistent [] | None |
| `park` | #134 park [] | None |
| `precise_goto` | #127 solver operations reject unconfigured source; #130 native precise goto simulator field | None |
| `recover_on_failure` | #14 recover on missing camera property; #15 recover without destination (expected Alert); #17 recover independently inside repeated blocks; #127 solver operations reject unconfigured source | None |
| `recovery_point` | #14 recover on missing camera property; #17 recover independently inside repeated blocks; #127 solver operations reject unconfigured source | None |
| `remove_fits_header` | #67 FITS custom header removed | None |
| `repeat` | #3 nested loops reset=true; #4 nested loops reset=false; #5 zero loop and trailing step; #17 recover independently inside repeated blocks | None |
| `reset_frame` | #44 binning then full sensor; #76 autofocus with real simulator; #128 solver setup with installed indexes | None |
| `resume_point` | #9 time break taken=true; #10 time break taken=false; #104 sky breaks taken=true; #105 HA break taken=true; #106 sky breaks taken=false; #107 HA break taken=false | None |
| `select_aperture` | #53 select_aperture ["f/4"] | None |
| `select_camera_mode` | #44 binning then full sensor | None |
| `select_dome` | #31 select_dome ["Dome Simulator"] | None |
| `select_filter` | #77 select_filter ["1"] | None |
| `select_filter_wheel` | #28 select_filter_wheel ["CCD Imager Simulator (wheel)"] | None |
| `select_focuser` | #29 select_focuser ["CCD Imager Simulator (focuser)"]; #73 autofocus without focuser fails (expected Alert); #75 select_focuser ["CCD Imager Simulator (focuser)"] | None |
| `select_frame_type` | #38 select_frame_type ["Light"]; #39 select_frame_type ["Dark"]; #40 select_frame_type ["Flat"]; #41 select_frame_type ["Bias"]; #61 prepare finite image batches | None |
| `select_gps` | #32 select_gps ["GPS Simulator"] | None |
| `select_guider` | #35 select_guider ["CCD Guider Simulator (guider)"] | None |
| `select_guider_agent` | #36 explicit agent selection | None |
| `select_guider_camera` | #34 select_guider_camera ["CCD Guider Simulator"] | None |
| `select_image_format` | #42 select_image_format ["RAW"]; #43 select_image_format ["FITS"]; #61 prepare finite image batches | None |
| `select_imager_agent` | #16 explicit policy transition (expected Alert); #36 explicit agent selection | None |
| `select_imager_camera` | #27 select_imager_camera ["CCD Imager Simulator"]; #50 select_imager_camera ["DSLR Simulator"]; #60 select_imager_camera ["CCD Imager Simulator"]; #128 solver setup with installed indexes | None |
| `select_iso` | #56 select_iso ["100"]; #57 select_iso ["200"]; #58 select_iso ["400"]; #59 invalid DSLR option fails (expected Alert) | None |
| `select_mount` | #30 select_mount ["Mount Simulator"] | None |
| `select_mount_agent` | #36 explicit agent selection | None |
| `select_program` | #51 select_program ["Manual"]; #52 select_program ["Bulb"]; #54 select_program ["Manual"] | None |
| `select_rotator` | #33 select_rotator ["Field Rotator Simulator"] | None |
| `select_shutter` | #55 select_shutter ["1/10"] | None |
| `select_solver_agent` | #36 explicit agent selection | None |
| `send_message` | #2 messages and quoted evaluation | None |
| `set_automatic_focuser_mode` | #71 set_automatic_focuser_mode [] | None |
| `set_directory` | #37 private image directory and names; #61 prepare finite image batches | None |
| `set_file_template` | #37 private image directory and names; #62 capture batch named=false; #63 capture stream named=false; #64 capture batch named=true; #65 capture stream named=true | None |
| `set_fits_header` | #66 FITS custom header present | None |
| `set_focuser_position` | #70 set_focuser_position [500] | None |
| `set_frame` | #44 binning then full sensor; #61 prepare finite image batches | None |
| `set_gain` | #45 set_gain [17] | None |
| `set_gamma` | #47 set_gamma [1.2] | None |
| `set_manual_focuser_mode` | #69 set_manual_focuser_mode [] | None |
| `set_object_name` | #37 private image directory and names | None |
| `set_offset` | #46 set_offset [7] | None |
| `set_rotator_angle` | #101 set_rotator_angle [10] | None |
| `set_solver_filter` | #126 solver filter current | None |
| `slew` | #96 slew mount with coordinate readback; #120 natural transit meridian flip and capture resume; #122 abort imager paused before natural transit (expected Alert); #132 natural transit flip with native solver correction | None |
| `start` | #1 empty sequence | None |
| `start_guiding` | #113 start_guiding produces frames then stops; #115 guiding reuses configured exposure; #116 guided capture with dithering | None |
| `start_guiding_exposure` | #114 start_guiding_exposure produces frames then stops | None |
| `start_preview` | #68 preview observes frames then stops | None |
| `stop_guiding` | #113 start_guiding produces frames then stops; #114 start_guiding_exposure produces frames then stops; #115 guiding reuses configured exposure; #116 guided capture with dithering | None |
| `stop_preview` | #68 preview observes frames then stops | None |
| `sync_center` | #127 solver operations reject unconfigured source; #129 native solve sync and center | None |
| `unload_driver` | #26 unload unused simulator | None |
| `unpark` | #93 unpark [] | None |
| `wait` | #2 messages and quoted evaluation; #18 pause then resume a wait; #19 abort pending wait (expected Alert); #20 fresh sequence after abort; #21 save private configuration; #68 preview observes frames then stops; #111 prepare guider and solver relations; #119 prepare transit target 0.008; #121 prepare transit target 0.05; #128 solver setup with installed indexes; #131 prepare solver-assisted transit target; #135 reset completed sequence through bus | None |
| `wait_for_gps` | #97 GPS ready | None |
| `wait_until` | #6 past numeric and UTC waits; #7 future numeric wait; #8 future UTC wait | None |
| `wait_until_solar_altitude_below` | #108 sky waits immediately satisfied; #109 abort wait_until_solar_altitude_below (expected Alert) | None |
| `wait_until_target_altitude_above` | #108 sky waits immediately satisfied; #110 abort wait_until_target_altitude_above (expected Alert) | None |

## Individual cases

| # | Case | Verdict | Expected / observed | Seconds |
| --- | --- | --- | --- | --- |
| 1 | empty sequence | pass | Ok / Ok | 0.106 |
| 2 | messages and quoted evaluation | pass | Ok / Ok | 0.419 |
| 3 | nested loops reset=true | pass | Ok / Ok | 1.362 |
| 4 | nested loops reset=false | pass | Ok / Ok | 1.360 |
| 5 | zero loop and trailing step | pass | Ok / Ok | 0.417 |
| 6 | past numeric and UTC waits | pass | Ok / Ok | 0.212 |
| 7 | future numeric wait | pass | Ok / Ok | 3.192 |
| 8 | future UTC wait | pass | Ok / Ok | 3.216 |
| 9 | time break taken=true | pass | Ok / Ok | 0.212 |
| 10 | time break taken=false | pass | Ok / Ok | 0.211 |
| 11 | break without destination | pass | Ok / Ok | 0.208 |
| 12 | abort on missing camera property | pass | Alert / Alert | 0.209 |
| 13 | continue on missing camera property | pass | Ok / Ok | 0.211 |
| 14 | recover on missing camera property | pass | Ok / Ok | 0.211 |
| 15 | recover without destination | pass | Alert / Alert | 0.206 |
| 16 | explicit policy transition | pass | Alert / Alert | 0.211 |
| 17 | recover independently inside repeated blocks | pass | Ok / Ok | 0.525 |
| 18 | pause then resume a wait | pass | Ok / Ok | 1.132 |
| 19 | abort pending wait | pass | Alert / Alert | 0.420 |
| 20 | fresh sequence after abort | pass | Ok / Ok | 0.734 |
| 21 | save private configuration | pass | Ok / Ok | 1.143 |
| 22 | restore private configuration | fail | Ok / Alert | 1.667 |
| 23 | missing configuration fails | pass | Alert / Alert | 0.106 |
| 24 | missing driver fails | pass | Alert / Alert | 0.102 |
| 25 | load unused simulator | pass | Ok / Ok | 0.211 |
| 26 | unload unused simulator | pass | Ok / Ok | 0.627 |
| 27 | select_imager_camera ["CCD Imager Simulator"] | pass | Ok / Ok | 0.211 |
| 28 | select_filter_wheel ["CCD Imager Simulator (wheel)"] | pass | Ok / Ok | 0.211 |
| 29 | select_focuser ["CCD Imager Simulator (focuser)"] | pass | Ok / Ok | 0.211 |
| 30 | select_mount ["Mount Simulator"] | pass | Ok / Ok | 0.211 |
| 31 | select_dome ["Dome Simulator"] | pass | Ok / Ok | 0.211 |
| 32 | select_gps ["GPS Simulator"] | pass | Ok / Ok | 0.209 |
| 33 | select_rotator ["Field Rotator Simulator"] | pass | Ok / Ok | 0.206 |
| 34 | select_guider_camera ["CCD Guider Simulator"] | pass | Ok / Ok | 0.209 |
| 35 | select_guider ["CCD Guider Simulator (guider)"] | pass | Ok / Ok | 0.211 |
| 36 | explicit agent selection | pass | Ok / Ok | 0.208 |
| 37 | private image directory and names | pass | Ok / Ok | 0.211 |
| 38 | select_frame_type ["Light"] | pass | Ok / Ok | 0.207 |
| 39 | select_frame_type ["Dark"] | pass | Ok / Ok | 0.210 |
| 40 | select_frame_type ["Flat"] | pass | Ok / Ok | 0.211 |
| 41 | select_frame_type ["Bias"] | pass | Ok / Ok | 0.211 |
| 42 | select_image_format ["RAW"] | pass | Ok / Ok | 0.211 |
| 43 | select_image_format ["FITS"] | pass | Ok / Ok | 0.208 |
| 44 | binning then full sensor | pass | Ok / Ok | 0.211 |
| 45 | set_gain [17] | pass | Ok / Ok | 0.212 |
| 46 | set_offset [7] | pass | Ok / Ok | 0.210 |
| 47 | set_gamma [1.2] | pass | Ok / Ok | 0.211 |
| 48 | enable_cooler [25] | pass | Ok / Ok | 3.531 |
| 49 | disable_cooler [] | pass | Ok / Ok | 0.210 |
| 50 | select_imager_camera ["DSLR Simulator"] | pass | Ok / Ok | 0.206 |
| 51 | select_program ["Manual"] | pass | Ok / Ok | 0.211 |
| 52 | select_program ["Bulb"] | pass | Ok / Ok | 0.207 |
| 53 | select_aperture ["f/4"] | pass | Ok / Ok | 0.207 |
| 54 | select_program ["Manual"] | pass | Ok / Ok | 0.208 |
| 55 | select_shutter ["1/10"] | pass | Ok / Ok | 0.210 |
| 56 | select_iso ["100"] | pass | Ok / Ok | 0.211 |
| 57 | select_iso ["200"] | pass | Ok / Ok | 0.211 |
| 58 | select_iso ["400"] | pass | Ok / Ok | 0.206 |
| 59 | invalid DSLR option fails | pass | Alert / Alert | 0.104 |
| 60 | select_imager_camera ["CCD Imager Simulator"] | pass | Ok / Ok | 0.209 |
| 61 | prepare finite image batches | pass | Ok / Ok | 0.208 |
| 62 | capture batch named=false | pass | Ok / Ok | 0.314 |
| 63 | capture stream named=false | pass | Ok / Ok | 0.417 |
| 64 | capture batch named=true | pass | Ok / Ok | 0.421 |
| 65 | capture stream named=true | pass | Ok / Ok | 0.414 |
| 66 | FITS custom header present | pass | Ok / Ok | 0.317 |
| 67 | FITS custom header removed | pass | Ok / Ok | 0.311 |
| 68 | preview observes frames then stops | pass | Ok / Ok | 0.726 |
| 69 | set_manual_focuser_mode [] | pass | Ok / Ok | 0.211 |
| 70 | set_focuser_position [500] | pass | Ok / Ok | 0.725 |
| 71 | set_automatic_focuser_mode [] | pass | Ok / Ok | 0.211 |
| 72 | clear autofocus selection | pass | Ok / Ok | 0.207 |
| 73 | autofocus without focuser fails | pass | Alert / Alert | 0.209 |
| 74 | autofocus ignores process failure and restores batch | pass | Ok / Ok | 0.419 |
| 75 | select_focuser ["CCD Imager Simulator (focuser)"] | pass | Ok / Ok | 0.206 |
| 76 | autofocus with real simulator | pass | Ok / Ok | 6.666 |
| 77 | select_filter ["1"] | pass | Ok / Ok | 0.211 |
| 78 | filter offsets enabled then disabled | pass | Ok / Ok | 0.210 |
| 79 | enable_ha_limit [] | pass | Ok / Ok | 0.207 |
| 80 | enable_time_limit [] | pass | Ok / Ok | 0.212 |
| 81 | enable_dome_slaving [] | pass | Ok / Ok | 0.211 |
| 82 | enable_field_derotation [] | pass | Ok / Ok | 0.210 |
| 83 | enable_joystick_control [] | pass | Ok / Ok | 0.209 |
| 84 | disable_ha_limit [] | pass | Ok / Ok | 0.211 |
| 85 | disable_time_limit [] | pass | Ok / Ok | 0.210 |
| 86 | disable_dome_slaving [] | pass | Ok / Ok | 0.207 |
| 87 | disable_field_derotation [] | pass | Ok / Ok | 0.210 |
| 88 | disable_joystick_control [] | pass | Ok / Ok | 0.204 |
| 89 | make_dome_slaving_persistent [] | pass | Ok / Ok | 0.206 |
| 90 | make_dome_slaving_not_persistent [] | pass | Ok / Ok | 0.207 |
| 91 | make_field_derotation_persistent [] | pass | Ok / Ok | 0.204 |
| 92 | make_field_derotation_not_persistent [] | pass | Ok / Ok | 0.211 |
| 93 | unpark [] | pass | Ok / Ok | 0.625 |
| 94 | home mount simulator | pass | Ok / Ok | 0.827 |
| 95 | enable_tracking [] | pass | Ok / Ok | 0.628 |
| 96 | slew mount with coordinate readback | pass | Ok / Ok | 10.251 |
| 97 | GPS ready | pass | Ok / Ok | 0.211 |
| 98 | dome unpark | pass | Ok / Ok | 0.622 |
| 99 | dome_open [] | pass | Ok / Ok | 6.696 |
| 100 | dome_slew [45] | pass | Ok / Ok | 5.504 |
| 101 | set_rotator_angle [10] | pass | Ok / Ok | 1.350 |
| 102 | dome_close [] | pass | Ok / Ok | 6.619 |
| 103 | dome_park [] | pass | Ok / Ok | 5.898 |
| 104 | sky breaks taken=true | pass | Ok / Ok | 0.209 |
| 105 | HA break taken=true | pass | Ok / Ok | 0.205 |
| 106 | sky breaks taken=false | pass | Ok / Ok | 0.206 |
| 107 | HA break taken=false | pass | Ok / Ok | 0.209 |
| 108 | sky waits immediately satisfied | pass | Ok / Ok | 0.210 |
| 109 | abort wait_until_solar_altitude_below | pass | Alert / Alert | 0.517 |
| 110 | abort wait_until_target_altitude_above | pass | Alert / Alert | 0.518 |
| 111 | prepare guider and solver relations | pass | Ok / Ok | 0.628 |
| 112 | guiding calibration real simulator | pass | Ok / Ok | 55.553 |
| 113 | start_guiding produces frames then stops | pass | Ok / Ok | 0.726 |
| 114 | start_guiding_exposure produces frames then stops | pass | Ok / Ok | 0.829 |
| 115 | guiding reuses configured exposure | pass | Ok / Ok | 0.723 |
| 116 | guided capture with dithering | pass | Ok / Ok | 2.383 |
| 117 | clear guider selection | pass | Ok / Ok | 0.211 |
| 118 | meridian flip options readback | pass | Ok / Ok | 0.204 |
| 119 | prepare transit target 0.008 | pass | Ok / Ok | 0.626 |
| 120 | natural transit meridian flip and capture resume | pass | Ok / Ok | 27.371 |
| 121 | prepare transit target 0.05 | pass | Ok / Ok | 0.621 |
| 122 | abort imager paused before natural transit | pass | Alert / Alert | 1.135 |
| 123 | restore flip option after transit abort | pass | Ok / Ok | 0.208 |
| 124 | pause and resume actual capture | pass | Ok / Ok | 1.979 |
| 125 | abort finite capture | pass | Alert / Alert | 0.938 |
| 126 | solver filter current | pass | Ok / Ok | 0.209 |
| 127 | solver operations reject unconfigured source | pass | Ok / Ok | 14.409 |
| 128 | solver setup with installed indexes | pass | Ok / Ok | 0.728 |
| 129 | native solve sync and center | pass | Ok / Ok | 5.709 |
| 130 | native precise goto simulator field | pass | Ok / Ok | 8.611 |
| 131 | prepare solver-assisted transit target | pass | Ok / Ok | 0.612 |
| 132 | natural transit flip with native solver correction | pass | Ok / Ok | 77.236 |
| 133 | disable_tracking [] | pass | Ok / Ok | 0.619 |
| 134 | park [] | pass | Ok / Ok | 10.259 |
| 135 | reset completed sequence through bus | pass | Ok / Ok | 0.316 |
