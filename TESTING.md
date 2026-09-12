# Testing

All testing with physical hardware or simulators is documented in this file

Current QHY migration target: version 28 disables hot-plug for both variants by
user request. Connect cameras before initialization; startup discovery runs once.
Physical hot-plug tests are excluded. Results below describe version 27 unless
explicitly marked otherwise.

## September 12th 2026 — QHY generator migration acceptance (partial, blocked by SDK failures)

macOS 26.6 arm64 host, bundled modern QHY SDK 25.3.24.9 (svn 14950), driver
0x0300001B. Real tests began only after both generated builds and dual-SDK fake
suites passed. The user supplied QHY5III178M, QHY5LII-M and QHY5-M; all require
firmware. Use the bundled `ccd_qhy/bin_externals/qhyccd/firmware` directory as
`INDIGO_FIRMWARE_BASE` for the modern SDK. Initial sandbox/no-firmware discovery
failures are environment/setup failures, not failed exposures.

The opt-in client is `indigo_test/hardware/test_ccd_qhy_hw.c`. It requires
`INDIGO_TEST_DEVICE` (unique name substring), explicit `--run`, driver dylib and
entry symbol; `QHY_HW_CASE` selects a scenario. Each real-SDK scenario runs in a
separate process with an external time limit. Both SDKs are never loaded together.


With QHY5L-II isolated after USB reset, the corrected guide scenario passed:
all four 100 ms directions, a pulse during a 1.5 s exposure, guider operation
after CCD disconnect, reconnection and clean shutdown. This verifies SDK/property
completion, not electrical ST4 timing. The separate settings scenario also
passed gain change/restoration and advanced-settings restoration. Logs:
`/tmp/qhy-hw-5lii-guide-isolated.log` and
`/tmp/qhy-hw-5lii-isolated-settings.log`.
The next abort process crashed before selection while closing the discovery
probe (CloseQHYCCD -> QHY5IIBASE::DisConnectCamera -> QHYCAM::closeCamera,
`test_ccd_qhy_hw-2026-09-12-104725.000.ips`). Abort did not execute;
streaming/geometry were not started. Physical hot-plug remains unverified.


A minimal x86_64 program linked only the bundled legacy libqhy.a and project
libusb (with a no-op indigo_debug symbol, no INDIGO bus/driver/queues) reproduced
the QHY5L-II discovery-close crash. Sequence: InitQHYCCDResource, firmware init,
3 s wait, ScanQHYCCD, GetQHYCCDId/Model, OpenQHYCCD, query ST4/CFW, CloseQHYCCD,
ReleaseQHYCCDResource. The first process after a five-second USB reset passed;
the second identical process without USB reset crashed at CloseQHYCCD ->
QHY5IIBASE::DisConnectCamera -> QHYCAM::closeCamera (report
`qhy_probe-2026-09-12-105120.000.ips`). Logs are
`/tmp/qhy-sdk-only-probe-reset.log` and `/tmp/qhy-sdk-only-probe-repeat.log`.
This reproduces the specific discovery-close failure independently of migration;
it does not classify every QHY crash or prove physical hot-plug behavior.
A prior driver abort attempt after a short reset also crashed at discovery
(`/tmp/qhy-hw-5lii-abort-reset.log`, report 104824.000); no abort ran.


The abort scenario run first after a five-second USB reset passed: start a 5 s
exposure, request abort, then receive a fresh 1280 x 960 RAW8 image at 0.1 s,
restore settings and shut down cleanly (`/tmp/qhy-hw-5lii-abort-cold.log`).
Historical commit 074bd49a8c612d97ad5e2bdcad4bedc929e22fc6 (2020-10-03,
"ccd_qhy/ccd_qhy2: hot-plug support disabled") confirms hot-plug was deliberately
disabled. Its message does not identify the exact crash as the reason. The old
source separately documents repeated QHY5L-II open/close crashes and rescans as
a leaking workaround. Do not infer that serialized generated hot-plug fixes them.

| Camera / SDK | Evidence so far | Limits/failures |
| --- | --- | --- |
| QHY5III178M / modern, arm64 | Full 3056 x 2048 RAW16 at 0.1, 1.5, 2.5, 16.5 s; measured 2.969/1.754/2.750/16.745 s including setup/readout. RAW8 produced a full 6,258,700-byte frame. Abort followed by a fresh image succeeded. Finite stream delivered exactly three frames. | SDK aborts in StopQHYCCDLive -> libusb_cancel_transfer (invalid mutex assertion) during stop/close/reopen. Geometry sequence stops on RAW8-to-RAW16 reopen; stream stops after three images. Reconnect/unload and full settings restoration are not passed. |
| QHY5LII-M / legacy svn r6536, x86_64 | Four 1280 x 960 RAW16 frames at 0.1/1.5/2.5/16.5 s; elapsed 2.568/3.203/4.187/18.180 s. | Reconnect followed by a short exposure hangs in QHY5LIIBASE::GetSingleFrame. The unchanged baseline reproduces it at RAW8; external 150 s limits terminated both processes. |
| QHY5-M / legacy svn r6536, x86_64 | After avoiding its unsupported bits setter, four 1280 x 1024 RAW8 frames at 0.1/1.5/2.5/16.5 s; elapsed 0.767/3.436/5.506/21.915 s. | Reuse crashes in SDK CloseQHYCCD/QHYCAM::closeCamera; another discovery probe crashes while closing QHY5L-II. Subsequent camera selection fails. These crashes remain unresolved; no full suite pass. |

The recorded baseline `fdacefa743d24669d7f33f9b6225115cbf7db730`, built in a
temporary directory against the same modern SDK and current INDIGO library,
delivered the same four exposures and reproduced the identical CloseQHYCCD
assertion on disconnect (SIGABRT). This close failure predates migration.
One separate modern-driver guide attempt crashed during discovery, before camera
selection, in QHY5LIIBASE::DisConnectCamera -> libusb darwin_reenumerate_device;
that distinct failure remains unresolved. SDK crashes prevent orderly restoration
in those processes; no persistent configuration was saved.

Physical hot-plug is **unverified**. The user replugged legacy cameras and disconnected QHY5III178 between stopped test processes to restore initial conditions; this was not a live hot-plug test.
Legacy uses the parent `bin_externals/qhyccd` firmware path because it appends
`/firmware` internally; modern uses the firmware directory itself. Initial wrong-path
runs do not establish hardware failure. Starting framework USB first suppressed
a warning but was not adopted as a production workaround. Hardware discovery
waits up to 60 seconds for slow SDK enumeration.

The remaining batch was stopped after repeated failure before camera selection;
all owned processes ended. QHY5/QHY5L-II need another USB reset before the blocked
geometry/settings/abort/stream/guide phases can continue. QHY5III178 is currently
disconnected per test preparation. No full camera suite, reliable reconnect,
driver reload or hot-plug acceptance is claimed. `QHY_HW_CASE` feature runs skip
the repeated common reconnect phase, which is exercised separately by the exposure
scenario; default execution still includes it. See REFACTOR for retained log paths.

Cooling, CFW, physical shutter, electrical pulse timing and Linux/Windows runtime
are not covered by these three cameras. The fake matrix and detailed software
checks are in `ccd_qhy/REFACTOR.md` and `indigo_test/CHANGES.md`.

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

### QHY version-28 startup follow-up (September 12th 2026)

First version-28 physical startup had both legacy cameras attached (confirmed by
the user and IOUSBHostDevice inventory). QHY5-M was attached once, without a
hot-plug callback, then detached cleanly at shutdown. QHY5L-II remained at its
cold 1618:0920 USB identity while QHY5 was 16c0:296d. SDK enumeration exposed
only QHY5-M, so the QHY5L-II guide selection timed out without connecting or
sending pulses. This is not a guide pass or a logical-capacity failure.
Log: `/tmp/qhy-v28-hw-5lii-guide.log`. Isolated QHY5L-II firmware/startup
validation follows a fresh user USB reset.

Version 28 with only QHY5L-II attached passed the isolated guide scenario:
startup enumeration, CCD/guider connection, four 100 ms guide directions,
guide during a 1.5 s exposure, one valid 1280 x 960 RAW8 image, guider survival
after logical CCD disconnect, CCD reconnect, settings restoration and clean
shutdown. Exit 0, log `/tmp/qhy-v28-hw-5lii-guide-isolated.log`.
This confirms the static-discovery path on this camera; it is not a full camera
suite or an electrical ST4 measurement. Modern-SDK static startup is next.

Version 28, isolated QHY5III178M / modern SDK: static startup and both logical
connections succeeded, all four 100 ms guide directions completed, a pulse ran
during a 1.5 s exposure, and a valid 3056 x 2048 RAW16 frame arrived. Guiding
also completed after the logical CCD disconnected. Final guider disconnect
(last shared close) aborted in CloseQHYCCD -> StopQHYCCDLive ->
QHY5IIIBASE::StopLiveExposure -> libusb_cancel_transfer -> usbi_mutex_lock.
Report `test_ccd_qhy_hw-2026-09-12-111241.ips`, log
`/tmp/qhy-v28-hw-178-guide.log`, exit -6. This matches the stop/close assertion
already reproduced with the unchanged baseline; the complete guide scenario
is not a pass. Static startup now has physical evidence for both SDK variants.

The final physical checkpoint is complete with partial acceptance, not a full
camera-suite pass. Existing SDK-blocked streaming/geometry/reconnect/unload
limitations remain as documented. Hot-plug is intentionally disabled and is
excluded by user request. No persistent configuration was saved; final settings
restoration could not complete after the modern SDK abort. All owned HW
processes have ended.

### QHY5III178 retest with modern SDK 26.06.04.16 — 2026-09-12

Working-tree generated driver v28, macOS arm64, isolated QHY5III178M after
user USB reset. Static discovery, CCD/guider connection, four 100 ms guide
directions, guiding during a 1.5 s exposure, and a 3056 x 2048 RAW16 image
completed. A guide pulse after logical CCD disconnect also completed. Final
guider disconnect aborted (SIGABRT/exit -6). The crash stack is CloseQHYCCD ->
StopQHYCCDLive -> QHY5IIIBASE::StopLiveExposure ->
QHYBASE::StopAsyQCamLiveClearLibUsb -> StopAsyQCamLive ->
libusb_cancel_transfer -> usbi_mutex_lock. Thus the supplied modern update
does not resolve the previously observed last-close failure. The full guide
scenario remains failed, not passed. No persistent configuration was saved.

Log: `/tmp/qhy-modern-260604-178-guide.log`; crash report:
`test_ccd_qhy_hw-2026-09-12-150436.ips`. The process has ended.

Legacy comparison preparation exposed two test-build issues before camera
access: the first temporary build omitted INDIGO_MACOS (so skipped firmware
initialization); the corrected arm64 build hit the driver's existing Intel-only
macOS architecture guard. Neither is a camera/SDK acceptance result. Both
processes ended. A full patched x86_64 SDK and driver have now been built for
Rosetta. The legacy source intentionally lacks SetQHYCCDLogLevel; only the
temporary test link supplies a no-op for it, leaving existing logging intact.

### QHY5III178 patched legacy SDK retest — 2026-09-12

Generated driver v28, macOS Intel/Rosetta, SDK built from the separate legacy
source checkout with the experimental transfer/close fixes. The source-built
SDK expects INDIGO_FIRMWARE_BASE to name the firmware directory directly, unlike
the previously bundled legacy binary. An initial path error was corrected
before opening a camera. Firmware then loaded and USB identity became
1618:0179 / Titan178U, but startup-only enumeration missed the re-enumeration
window. That test exited normally with no camera selected. A subsequent process
with firmware already loaded discovered QHY5III178M and its guider.

CCD/guider connection and all four 100 ms guide directions completed. A guide
pulse also completed after requesting a 1.5 s exposure. No image arrived.
The exposure timed out; a process sample showed the device queue blocked in
GetQHYCCDSingleFrame -> QHY5IIIBASE::GetSingleFrame, repeatedly attempting
readout. Abort could not execute on the blocked queue. The owned process was
terminated with SIGTERM after capturing evidence. No persistent configuration
was saved; normal disconnect/settings restoration could not complete.

Result: failed guide/exposure acceptance, not a successful workaround. This
run did not reach final close and therefore neither validates nor disproves
the close fix. It also does not isolate an SDK baseline defect from an effect
of the experimental patch. A clean USB reset is needed before another HW run.
Logs: `/tmp/qhy-legacy-fixed-178-guide.log` (firmware/startup) and
`/tmp/qhy-legacy-fixed-178-guide-loaded.log` (connected test). Stack sample:
`/tmp/qhy-legacy-fixed-178-readout-sample.txt`. All owned test processes ended.

### Legacy SDK experiment reverted at user request — 2026-09-12

The five experimental source changes in the separate legacy SDK checkout and
its added transfer regression test have been reverted. Pre-existing user
Makefile edits and library files were preserved. Temporary patched SDK/driver
builds and the custom retest launcher were removed. INDIGO's bundled legacy
SDK headers and libraries were never replaced and remain unchanged. Previous
experimental findings/results above are historical evidence, not active fixes.
The generated driver migration and the separately requested modern SDK update
remain in place. No further legacy SDK workaround testing is planned.

## QHY5III178M hardware retest — SDK 26.7.21.5, driver v29 (2026-09-12)

User confirmed QHY5III178 connected and authorized resuming physical tests. macOS arm64, modern `ccd_qhy2`, SDK 26.7.21.5 and libusb 1.0.29.11990. Seven sequential isolated scenarios all exited 0 without an intervening USB reset:

- `switching`: two rounds of RAW8 and RAW16; each format runs single/live/single, 20 valid full-frame images in total (3056 x 2048).
- `exposure`: 0.1, 1.5, 2.5 and 16.5 s requested exposures; logical disconnect/reconnect, driver shutdown/dlclose/dlopen/init and a new exposure all succeed. First exposure includes mode-initialization overhead; this is not optical shutter timing validation.
- `abort`: interrupt a 5 s exposure and acquire a new 0.1 s exposure.
- `guide`: four 100 ms directional commands, guiding during a 1.5 s exposure, guider survival after CCD disconnect and reconnect after final guider disconnect. Command completion is verified, not physical mount motion.
- `geometry`: RAW8/RAW16, 128 x 128 ROI and all exposed modes (1x1 and 2x2).
- `settings`: gain/advanced settings restoration and available read modes with exposure.
- `stream`: three-frame acquisition, continuous stream, abort and return to single exposure.

Every scenario also completed final physical camera close and driver shutdown without a crash. This supersedes the earlier close-failure result for this camera under this exact driver/SDK combination; it does not establish which part of the combined update fixed it or guarantee other SDK/platform/camera combinations. Hot-plug remains disabled and was not tested. Legacy QHY5/QHY5L-II, cooling, camera-connected CFW, other platforms and optical image quality remain outside this retest. No configuration was saved, and test processes ended. Hardware test binaries were cleaned afterward.

Logs: `/tmp/qhy178-sdk26721-{switching,exposure,abort,guide,geometry,settings,stream}.log`.

### Physical hot-plug outcome: four phases passed

Experimental QHY2 v30, SDK 26.7.21.5, macOS arm64, one QHY5III178M. The user physically unplugged and replugged the camera in each phase: (1) logically disconnected, (2) connected idle, (3) during a 60 s exposure, (4) during continuous streaming. All four detach/rearrival cycles completed; firmware bootloader 1618:0178 became 1618:0179, camera and guider were recreated, and a fresh 0.1 s RAW16 exposure succeeded after every replug. Final close, SDK shutdown and callback deregistration completed, exit 0. During streaming removal the SDK rejected resetting stream mode after USB removal; cleanup and the subsequent replug/exposure still succeeded.

The hardware harness now exposes this manual-only scenario as explicit `QHY_HW_CASE=hotplug`; it is excluded from the default full run. It requires an experimental hot-plug driver, not the checked-in startup-only driver. The exact experiment sources, fake test and generated build remain in `/tmp/qhy178-hotplug`; authoritative physical log: `/tmp/qhy178-hotplug/hardware-early-usb.log`. The hardware build is arm64; the fake test compiled for both macOS architectures and ran natively. All test processes have ended.

Production hot-plug remains disabled. This experiment deliberately matched only QHY5III178 and did not verify discovery while a different camera remains open, SDK-id to physical-USB association with multiple cameras, other QHY models, Windows/Linux, or the legacy SDK. Permanent modern-only activation also needs a shared-source build/DSL decision because `sdk.hotplug` currently accepts a generation-time boolean rather than a QHY2 preprocessor condition. No generator modifications were made.

### QHY5LII-M hot-plug experiment — four phases passed (2026-09-12)

The user connected a camera described as QHY5II; USB bootloader 1618:0920 and SDK discovery identified QHY5LII-M, running firmware 1618:0921. The first process selected the wrong name substring QHY5II and was terminated while waiting for a matching device, before a logical connection; this was a harness-selection error, not an SDK crash. A new process with QHY5LII selection completed all four manual unplug/replug phases: logically disconnected, connected idle, during a 60 s exposure, and during continuous streaming. Each replug loaded firmware, recreated CCD/guider devices and produced a new valid 1280 x 960 RAW8 image. Final camera close and SDK/driver shutdown succeeded, exit 0.

This used the modern SDK 26.7.21.5, macOS arm64 and an isolated QHY2 v30 hot-plug build, with USB handling started before SDK initialization and matching limited to 0920/0921. It does not validate legacy SDK hot-plug or multi-camera scanning. As with QHY5III178, stream-mode reset after physical removal returned an SDK error but cleanup and the following acquisition succeeded. Production hot-plug remains disabled. No persistent camera configuration was saved; all processes ended. Source/build and log: `/tmp/qhy5ii-hotplug`, `/tmp/qhy5ii-hotplug/hardware-qhy5lii.log`.


### QHY production hot-plug promotion (2026-09-12)

The user accepted the two four-phase modern-SDK hot-plug experiments above. Independent version-30 definitions now enable hot-plug in `ccd_qhy2` and retain startup-only discovery in legacy `ccd_qhy`. The manual `QHY_HW_CASE=hotplug` harness can use the standard modern driver; an experimental build is no longer required. Physical acceptance remains specific to QHY5III178 and QHY5LII-M on macOS arm64 with SDK 26.7.21.5; it does not imply legacy or multi-camera hardware validation.


### Legacy QHY5LII-M hot-plug experiment (2026-09-12)

An isolated x86_64/Rosetta build of legacy v30 enabled SDK hot-plug only for QHY5LII boot/running USB IDs 1618:0920/0921, with firmware loading on boot arrival and early USB event handling. Production legacy hot-plug remains disabled; SDK binaries are unchanged. The first preparation run used the modern firmware-directory convention and detected no camera; the corrected run used the legacy parent directory `bin_externals/qhyccd`.

The corrected run loaded firmware, attached CCD and guider, acquired a 1280 x 960 RAW8 image at 0.1 s, and disconnected successfully. Physical removal while logically disconnected detached both devices without a crash. Replug loaded firmware (0920 -> 0921); ScanQHYCCD returned one camera and GetQHYCCDId/GetQHYCCDModel identified QHY5LII-M, but the discovery probe did not attach it. The last log was immediately before OpenQHYCCD; no close-error log followed, and a process sample showed the driver queue idle, consistent with OpenQHYCCD returning NULL rather than hanging. No fresh image after replug was obtained. The waiting test was terminated with SIGTERM after diagnosis, not an SDK crash. Connected-idle, exposure and streaming removal phases were not reached.

Artifacts: `/tmp/qhy-legacy-hotplug/hardware-correct-firmware.log`, `/tmp/qhy-legacy-hotplug/sample-replug.txt`, temporary definition and binaries in the same directory. This experiment does not validate legacy hot-plug.
