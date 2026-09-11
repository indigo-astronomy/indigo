# Testing

All testing with physical hardware or simulators is documented in this file

## September 11th 2026 — ASI generator migration acceptance

macOS 26.6.2 arm64, bundled ASI SDK 1.41.0.0, final driver 0x0300003A in the migration worktree. Opt-in target: `make -C indigo_test test-ccd-asi-hw`, with `INDIGO_TEST_DEVICE` selecting the exact camera name. All 50 fake SDK tests and ASan/UBSan also pass.

| Device | Result | Evidence |
| --- | --- | --- |
| ASI120MC-S | PASS | All four pixel formats and five frame types, ROI/bin/configuration, controls/presets, fractional/long/short exposures, exact finite and sustained streams, abort/restart, all ST4 directions, simultaneous axes, BUSY reversal rejection, zero requests and sibling-preserving disconnect. 102 frames/~10 s at 0.1 s; long-stream abort 2.502 s, following short snapshot recovered in 2.645 s including one retry. |
| ASI294MC Pro | PASS | Same applicable CCD suite; cooler target 21 °C settled at 21.4 °C with 5% power, original target/cooler restored. 108 frames/~10 s; long-stream abort 0.345 s, subsequent short snapshot passed. ST4 is not present. |

Both passed physical USB removal/replug while idle, exposing and streaming; ASI120 also passed pending-ST4 removal/replug. Every cycle restored a fresh image/pulse as applicable, and the other connected camera could acquire while the selected camera was absent. Both passed eight-byte suffix write/replug/name verification, clear/replug and original empty suffix restoration. Final-version driver dlclose/dlopen and fresh image passed on both, in addition to shutdown/reinitialization. All modified settings were restored, logical devices disconnected and test builds cleaned.

The unchanged d373999 driver reproduces ASI120 ASI_EXP_FAILED after long-stream abort; the same sequence passes on ASI294. Final ASI120 recovery permits up to three queued retries. Separate direct-SDK diagnosis measured 0.958/1.673 s short-video frame intervals after a long snapshot abort; ASI120 video readout now allows at least five seconds per frame with 20 ms SDK transactions. Exhausted retries and genuine SDK failures retain ALERT.

See `indigo_drivers/ccd_asi/REFACTOR.md` for atomic checkboxes, H01–H08 mapping and detailed limits. Exact physical unplug inside a brief SDK readout is covered deterministically by gated fake SDK tests, not claimed as synchronized cable testing. Linux/Windows and Intel runtime, unavailable models, optical calibration and electrical ST4 timing were not validated.

## September 10th 2026 — Player One 0.1 s abort latency

Physical Mars-C II, full 1936 × 1100 RAW8 frames, local macOS hardware client, driver 0x03000012 at `b250fa179` (URGENT abort). Production code and driver versions were not changed for this measurement.

| Acquisition path | Trials | Extra frames after abort send / camera BUSY | Terminal latency min / median / max |
| --- | --- | --- | --- |
| Indefinite CCD_STREAMING, 0.1 s | 20 | 0 / 0 | 12.818 / 59.522 / 106.024 ms |
| Client-managed repeated CCD_EXPOSURE, 0.1 s | 20 | 0 / 0 | 13.082 / 51.496 / 126.896 ms |
| Imager Agent exposure batch, 0.1 s, delay 0, dithering disabled | 20 | 0 / 0 | 0.505 / 60.081 / 133.656 ms (agent terminal state) |

Each trial first delivered at least three frames. Direct abort phases ranged from 0 to 90 ms in 10 ms increments; client-series aborts were explicitly issued after requesting the next exposure. Agent phases ranged from 0 to 270 ms in 30 ms increments, covering inter-frame processing and the next exposure. Delivery was observed for 600 ms after terminal abort publication. No frames arrived after terminal publication, and a fresh 0.1 s exposure succeeded after each set. Initial image-format/upload settings were restored and the camera disconnected. No guiding commands, hot-plug cycles or camera suffix writes were requested by these modes. An initial additional 20-stream run and an initial 20-agent run also delivered zero extra frames; the table contains the final controlled sets.

The reported four or five extra exposures were not reproduced. These measurements count delivered CCD_IMAGE frames, not unobservable sensor integrations in continuous SDK mode. The test used an in-process client and the actual Imager Agent; network/UI delivery and the reporting user's original application/configuration are not reproduced. The timings combine queue wait, SDK stop and property publication; they do not separately measure SDK stop time or prove that priority was the original cause.

A separate defect was reproduced: when CCD_EXPOSURE and CCD_STREAMING are no longer BUSY, the Player One abort handler sets ALERT and clears the switch without publishing the result. The request's initial BUSY therefore remains visible to clients. The generator suppresses the epilogue because the on_change block references acquisition_finalizer. The initial inter-exposure client-series trial received no terminal CCD_ABORT_EXPOSURE update within five seconds. In the final agent run, 9/20 trials had no camera abort terminal update even though AGENT_ABORT_PROCESS completed and no extra frames arrived; the remaining 11 camera terminal updates took 35.047–128.043 ms. This status-publication defect is fixed in the follow-up below; it did not establish four or five extra frames.

Reproduce: build `make -C indigo_test build/hardware/test_ccd_playerone_hw`; explicitly run `indigo_test/build/hardware/test_ccd_playerone_hw --run --abort-latency` and `--run --abort-agent`. ABORT_LATENCY lines report frame counts and monotonic camera timestamps; AGENT_LATENCY lines report process completion. A `done_ms=-1` means no terminal camera update was observed, not zero latency. Hardware runs require USB access. Test configuration is isolated in a temporary directory; the agent selects/connects the previously disconnected camera itself.

### Terminal-publication fix and preview validation

The idle abort branch now resets the switch and explicitly publishes ALERT. Active abort still publishes through the existing cleanup. No new locks or driver version change. The new fake-SDK regression failed before the fix (client remained BUSY) and passed afterward; all five abort-filtered cases passed. Both hardware executables compile.

Physical Mars-C II validation of the fixed driver (`ecf5e90cf`), using `--run --abort-previews`, passed idle abort before acquisition and after a fresh completed exposure, plus these 0.1 s agent processes:

| Process | Trials | Extra delivered frames after send | Camera terminal min / median / max | Agent terminal min / median / max |
| --- | --- | --- | --- | --- |
| PREVIEW | 20 | 0 in every trial | 14.896 / 51.786 / 111.194 ms | 0.738 / 4.693 / 11.511 ms |
| STREAMING, count -1 | 20 | 1 in one trial, otherwise 0 | 15.498 / 59.019 / 107.792 ms | 16.085 / 70.225 / 115.020 ms |

All 40 trials delivered the camera terminal state and no frame after either camera or agent terminal completion. Streaming trial 18 (210 ms phase) delivered one frame after request/BUSY but before completion (90.145 ms); the four/five-frame report remains unreproduced. Observation continued for 600 ms after agent completion. Subsequent acquisition, settings restoration and disconnection passed. These remain local in-process hardware tests, not validation of the original reporting client's UI/network behavior.

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

ASI294MC Pro original-driver cross-check: unchanged d373999 baseline passed 1.5 s/2.5 s snapshots, a 100-frame 2.5 s stream aborted after three received frames, and the subsequent 0.1 s snapshot. Original abort delivered a fourth streaming frame and completed in 3.058 s. Unlike ASI120MC-S, this sequence passes on ASI294MC Pro with both original and migrated drivers.

ASI294MC Pro physical USB removal/replug passed while idle, streaming at 0.1 s, and exposing for 120 s. Every cycle recovered with a fresh 0.1 s image; removal-related SDK read/stop/close errors did not prevent cleanup or rediscovery. The user performed the cable cycles.

ASI120MC-S resumed physical tests at version 0x03000038 passed idle, active 120 s exposure, active streaming and 60 s ST4 pulse USB removal/replug. Each restored a fresh 0.1 s image and guide pulse; ASI294 stayed connected and acquired during each ASI120 absence. Eight-byte INDIGOT1 suffix write/replug verified both logical names, then clear/replug restored the original empty suffix and names. ASI294 passed the same suffix workflow while ASI120 acquired during its absence. Both cameras were restored and disconnected after these cycles. The earlier long-exposure cycle whose exposure finished before unplug counts only as an idle cycle; the dedicated active-exposure test supplies active-removal evidence.
