# Group reports

Summaries of the implementer reports, merged into the task branch. Coordinator re-verification on the merged candidate is in verification.md.

## G3 — server, HTTP, browser UI (merged 5841bca6f)
- bc8d-037, bc8d-038 — fixed in 664f00402. indigo.js `dtos`/`stod` keep the sign separately; `timestamp()` formats the offset as signed hours + minutes (`+0530`), whole-hour output unchanged. Regression `indigo_test/unit/test_indigo_js_formatting.js`, target `test-indigo-js-formatting` (in `test`): 16 pass / 0 fail; pre-fix 6 pass / 10 fail.
- 60d4-009 — fixed in e72ef3343. `indigo_uni_compress()` accepts only `Z_STREAM_END` + `deflateEnd` OK, otherwise `*out_size = 0`; the HTTP caller already falls back to uncompressed. Regressions in `test_uni_io.c` (`compress_rejects_output_buffer_too_small` failed pre-fix, `compress_incompressible_payload_round_trips`) and `test_transport_http_security.c` "incompressible gzip download is complete" (with a forced-failure hook). HTTP buffer was already enlarged by e044bf7d0.
- bc8d-040 — fixed in 246133b2f. Static UNLOAD clears `initialized` only after SHUTDOWN returns OK. Regression `test_server_static_driver.c` (target `test-server-static-driver`): 2/2; "unload then enable reinitializes" failed pre-fix.
- Reported checks: `make -C indigo_test test-unit` exit 0 (56 suites); `test-server-security-sanitize` 4 suites pass; `test-imager-download` 18/0; `test-mount-manual-motion` 22/0; `test-server-websocket` pass.

## G6 — wheel counts, aux finalizers, firmware simulators
- bc8d-023 — fixed: trutek 8→9 (79d845055), xagyl 10→11 (3f730cf2d), qhy 14→15 (57639a2cf). Count must satisfy 1..allocated_count of both slot properties, otherwise the connection is refused (not clamped). qhy also closes the port when on_connect fails. Simulator malformed-count options; regressions `malformed_filter_count_is_refused`, `largest_allocated_filter_count_is_accepted` (qhy `qhy_cfw3_*`).
- bc8d-017 — fixed in 16e17f90d, aux_fbc 10→11: impulse/exposure use `aux_light_impulse_finalizer` / `ccd_exposure_finalizer` with monotonic deadline. Regressions `intensity_is_served_during_a_long_impulse`, `disconnect_during_a_long_exposure_is_prompt`, `fractional_exposure_lasts_its_duration`.
- bc8d-018 — fixed in b975448f9, aux_flipflat 12→13: `aux_cover_finalizer`, 30 s `COVER_TIMEOUT`, ALERT on expiry. Regressions `light_is_served_while_the_cover_moves`, `disconnect_while_the_cover_moves_is_prompt`, `a_stuck_cover_times_out`.
- 3bd2-023 — fixed by inspection (811a3db42 flipflat .ino, 3527d6b86 nexdome3 .ino); not compiled, arduino-cli unavailable.
- bc8d-045 — fixed by inspection (104dfdde2 ao_sx .ino); not compiled, no harness.
- Reported runs (`run_driver_test.py <driver> --no-record`): trutek 10/10 (pre-fix 8/2), xagyl 10/10 (8/2), qhy 8/8 (6/2), fbc 13/13 (11/2), flipflat 16/16 (10/6). MIGRATION_STATUS.md Sim counts updated per driver.

## G2 — image processing and recording
- bc8d-004 — fixed in 116286712 (AVI/SER exact writes, seeks >= 0, checked realloc, counts after complete write; `test_recording_io` 6/6, 3 failed pre-fix) and 4de1a6440 (indigo_ccd_driver.c three local-save branches compare exact byte count, ALERT + remove file; `test_ccd_file_names` case `failed_local_save_publishes_alert`, failed pre-fix). The DSLR save callers are the library `indigo_process_dslr_image` branches; no driver changed.
- bc8d-007, bc8d-008, bc8d-009 — fixed in fe7c177a6 (`k < last_star`, skip failed PSF, empty/constant map; free `mono`; AND rectangle test). Regressions in `test_raw_image` (21/21); pre-fix sanitizer heap-buffer-overflow in `indigo_make_psf_map`; `test-raw-image-sanitize` passes. PSF-failure skip verified by inspection only.
- bc8d-010 — fixed in 56f1a24f3, plus the same-family left-column blue-pixel diagonal. `debayer_uses_only_in_bounds_neighbours` in `test_image_math` (6/6), failed pre-fix.
- Reported checks: `test-unit` exit 0; ccd_simulator 26/26; agent imager 39/39, guider 95/95, imager numerics 4/4.
- Left out: stream stays open after a mid-stream write failure (lifecycle change outside scope).

## G4 — single-driver defects
- 3bd2-032 — fixed in 75cc5d1a5, mount_ioptron 66→67: HC8406 finalizers ALERT on failed stop; disconnect re-sends stop for non-OK axes. `ioptron_guider_failed_hc8406_stop_alerts` (failed pre-fix). Not covered: a stop failing while the port stays open.
- 3bd2-018 — fixed in 44d41ded0, aux_svbpowerbox 5→6: 3 s overall drain deadline. Simulator `--boot-noise`; `endless_boot_log_is_bounded` (failed pre-fix).
- 3bd2-016 — fixed in 363f75254, aux_usbdp 15→16: close on every failed identification. `failed_identity_releases_the_port` (fd count; failed pre-fix).
- 3bd2-020 — fixed in 3a5cef6b5, ccd_simulator 43→44: exact read lengths. Short payload/header cases in `simulator_file_noise_formats_and_failure` (failed pre-fix).
- bc8d-028 — fixed in 4ac99e0ec, rotator_falcon 12→13: require transport success and exact `FN:` echo, restore last confirmed flag. `refused_direction_change_keeps_the_flag` (failed pre-fix).
- bc8d-025 — fixed in 3741304cc, focuser_lakeside 13→14: unchanged P frames count toward the stall bound, `CH#` + ALERT, target kept. `frozen_motion` (failed pre-fix).
- FND-000048 — fixed in bb697e6aa, aux_upb 38→39: `focuser_moving` flag; SI no-motion poll leaves queued BUSY requests to the handler. Deterministic `focuser_poll_leaves_a_queued_request_busy` (failed pre-fix); `focuser_` filter 5×12/12. Residual sub-microsecond race between bus-thread BUSY set and poll read (no shared lock).
- bc8d-044 — fixed in 316e75b16, ccd_dsi 18→19 (libdsi.c): propagate setup/trigger failure, EXPOSING after TRIGGER, exact plane lengths. Compile only — no fake/simulator exists; hardware not tested.
- Reported runs (`--no-record`): falcon 14/14, usbdp 21/21, svbpowerbox 16/16, lakeside 43/43, upb 52/52, ioptron 137/137, ccd_simulator 26/26.

## G5 — motion completion deadlines (bc8d-024)
Monotonic operation deadlines checked by the finalizer; on expiry the protocol stop (where one exists), last measured position kept, ALERT, polling stops. Fake-SDK builds shorten deadlines through `#ifndef` constants set in indigo_test/Makefile.
- wheel_quantum 2ea396fe4 7→8 (synchronous loop → `wheel_slot_finalizer`, 20 s; connect homing same deadline): `stuck_wheel_alerts_after_the_move_deadline`, `disconnect_during_a_stuck_move_is_served`; 10/10.
- wheel_trutek 9cd5c5437 8→9 (20 s): same two cases; 10/10.
- wheel_xagyl 854d3a2eb 10→11 (20 s, wrong slot): `wrong_slot_alerts_after_the_move_deadline`, disconnect case; 10/10.
- wheel_asi 930a51de5 17→18 (move 30 s, calibration 60 s; test 3/4 s): "endless motion ends at the deadline"; 6/6.
- wheel_playerone 3d795bb4d 15→16 (15 s for every move; test 3 s): "endless ordinary move ends at the deadline"; 13/13.
- mount_pmc8 43c48ebd0 17→18 (park uses `pmc8_point_deadline`, ESPt stop): `pmc8_mount_fails_a_stalled_park`; 37/37.
- mount_temma 20538c1cf 26→27 (TEMMA_SLEW_TIMEOUT 600 s for GOTO and park, PS stop): no timeout regression (600 s, no shortening hook); 28/28.
- focuser_nfocus abc7c2e94 12→13 (1 s/step + 10 s, `:F11000#`): `endless_motion_is_abortable_and_times_out`; 17/17.
- focuser_optec 566f24791 14→15 (10 ms/step + 15 s, existing `motion_failed` recovery): `blocking_move_endless_reports`; suite reported `descriptor_cleanup` failing, reproduced at pristine HEAD (pre-existing), and a 43/42 count anomaly in run_driver_test.py — see verification.md.
- mount_nexstar 28e655064 49→50 (park 300 s, abort motion): no timeout regression; 38/38. GOTO completion still has no overall deadline (not named in the finding; existing test expects stalled GOTO to stay BUSY).
- All pre-fix failures captured except temma/nexstar. Merge with G6 conflicted only in the trutek/xagyl simulator option lists (union resolved by coordinator); both drivers keep a single bump (trutek 9, xagyl 11).

## G1 — core library, test runner
- bc8d-003 — fixed in 417f06e0b: POSIX HID close no longer reaches fd/socket cleanup. `test_uni_io`/`hid_close_leaves_aliased_descriptor_open` (failed pre-fix).
- bc8d-016 — fixed in 417f06e0b: legacy `indigo_printf` rejects format errors and reformats long output into a heap buffer. `legacy_printf_sends_output_longer_than_its_buffer` (failed pre-fix), `legacy_printf_sends_short_output_unchanged`. Format-error branch not regressed (vsnprintf failure not portably inducible).
- bc8d-035 — fixed in 4b2091e9e: `>=` in driver/subprocess/server registries; `indigo_query_slave_devices` bounds before writing, returns at most `max`. `driver_registry_refuses_entry_past_capacity`, `server_registry_refuses_entry_past_capacity`, `slave_query_stores_at_most_max_devices` (all failed pre-fix). Subprocess registry by inspection.
- bc8d-036 — fixed in 59c2eac71: seed both items from the requesting client. `session_feature_change_keeps_unnamed_feature_of_its_client` (failed pre-fix).
- bc8d-029 — fixed in 63615d85a: ASSERT_NEAR requires finite non-negative tolerance; equal-sign infinities pass; other non-finite values fail. `test_test_runner` 5 cases (3 failed against old header). 16 macOS integration binaries using ASSERT_NEAR rerun; no hidden defect.
- bc8d-011 — fixed in 71fae40ef: proper motion applied when either component is nonzero. `proper_motion_in_one_axis_is_applied` (failed pre-fix).
- bc8d-013 — fixed in 637e56c56: pole sign, `-atan2(y, x)` longitude, `atan2(z, hypot)` latitude. `cartesian_axes_convert_to_their_own_direction` (failed pre-fix), round-trip case.
- 3bd2-015 — fixed in 66ca2066a: MULTI_POINT translations pass coordinates through like CONTROLLER (no model exists; property shape unchanged). `multi_point_translations_pass_coordinates_through` (unit), `mount_multi_point_alignment_passes_coordinates_through` (mount_simulator); both failed pre-fix. Limitation: SYNC still stores points that have no effect until a model exists.
- Reported: `make -C indigo_test test-unit` exit 0, 372 PASS / 0 FAIL. mount_simulator `mount_abort_allows_fresh_goto` intermittent (also pre-fix, 2/5) — see verification.md.
