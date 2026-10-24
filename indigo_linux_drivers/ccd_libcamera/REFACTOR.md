# ccd_libcamera development record

## Current state audit

`indigo_ccd_libcamera` is a new driver (task `20261024210959-0eba-ccd-libcamera-driver`), not a refactoring of an existing one. It is independent of `indigo_optional_drivers/ccd_rpi`, which stays unchanged.

- Architecture: generated driver (`indigo_ccd_libcamera.driver` → `.c`, `.h`, `_main.c`) in C, plus a C++ bridge (`indigo_ccd_libcamera_bridge.cpp`) behind the C interface `indigo_ccd_libcamera_bridge.h`. The bridge is the whole libcamera boundary; the hardware-free tests replace it with a fake.
- Discovery: generator `sdk { hotplug = false; usb = false; discover { } }` (generator extension added in the same task with owner approval). `discover` runs on the driver queue after INIT, starts the libcamera `CameraManager` and plugs one CCD device per camera, named after the sensor model in upper case, duplicates suffixed by `indigo_make_name_unique()`. CSI cameras are not hot-pluggable.
- Connection: `libcamera_open()` acquires the camera, reads pixel array size, unit cell size, exposure and analogue gain limits and the RAW stream formats, and builds `CCD_MODE` (one item per distinct RAW size, best bit depth per size, unpacked preferred). `libcamera_close()` stops and releases it. The generator owns the open/close calls; a failed open releases everything it acquired.
- Acquisition: `lcam_start()` configures a raw-only stream (falling back to raw + small viewfinder for pipelines that require a processed stream), allocates and maps frame buffers, starts the camera with manual exposure and gain (`ExposureTimeMode`/`AnalogueGainMode` on libcamera 0.5+, `AeEnable` otherwise), `AnalogueGain` and `FrameDurationLimits` long enough for the exposure, and queues one request (single exposure) or all requests (streaming). Exposures longer than 0.2 s start the sensor with the shortest exposure of the mode and request the real exposure with the first request, so the unusable frames after start are short. Completed requests are queued by the libcamera thread under a mutex; the finalizers poll `lcam_read()` every 10 ms on the device queue until the frame arrives or `exposure + 10 s` passes. Frames flagged `FrameError`, and frames (including `FrameStartup` ones) whose metadata exposure differs from the request by more than max(5 %, 100 µs), are requeued, at most 16 times; a frame is only delivered when its own exposure is the requested one, so it never started before the request.
- Exposure limits are read per mode after configuring each mode once at connect, because libcamera reports usable limits only for a configured mode.
- Image: `lcam_read()` copies each needed row of the uncached DMA frame buffer into cached memory (inside `DMA_BUF_IOCTL_SYNC` start/end) and converts unpacked 8/16-bit and MIPI CSI-2 packed 10/12-bit RAW to 16-bit little-endian samples with native values, copying only the `CCD_FRAME` rectangle. Colour sensors get `BAYERPAT`; their subframes are aligned to the 2x2 Bayer cell. Compressed PiSP formats are not offered.
- Properties: no custom properties. See `indigo_docs/PROPERTIES.md` (`ccd_libcamera`).
- Platforms: Linux only (libcamera). Built from `indigo_linux_drivers` by `Makefile.drvs` through `Makefile.drv` and `Makefile.inc`; without libcamera development files the bridge is built as a stub that reports no cameras. Not part of the Windows solution.
- Concurrency: all driver and bridge calls for a camera run on its device queue; discovery runs on the driver queue; the only cross-thread state is the completed-request queue in the bridge, protected by a mutex. `lcam_stop()` stops the camera (libcamera cancels pending requests synchronously) before buffers are unmapped and freed.

## Hardware-test decision

Hardware testing is performed: Raspberry Pi 3 Model B+ (Debian 13 trixie, kernel 6.18.50+rpt-rpi-v8, aarch64) with a Raspberry Pi Camera Module 1 (OV5647, colour, GBRG), libcamera 0.7.2+rpt20261009. Non-interactive hardware run (owner decision 2026-10-24). Scenarios: discovery and naming, every mode full frame, subframe, long exposure, gain, abort and reacquire, finite and indefinite streaming with refused geometry changes, reconnect, shutdown and reinit. Physical hot-plug is not applicable to CSI cameras.

## Plan

| Step | Work | Verification | State |
| --- | --- | --- | --- |
| 1 | Generator extension `sdk { usb = false; discover {} }` | all 120 existing `.driver` files regenerate byte-identically with the old and new generator | done, 120/120 identical |
| 2 | Bridge interface and libcamera implementation with stub fallback | stub builds with clang++ (macOS); libcamera 0.7.2 build on the RPi without warnings | done (after defects 1-3) |
| 3 | `.driver`, generated outputs, `Makefile.inc` | driver builds on macOS (stub bridge) and RPi (libcamera) | done |
| 4 | Fake bridge and hardware-free suite `test_ccd_libcamera_sdk` (+ ASan/UBSan) | 15/15 pass on macOS arm64 and Linux arm64, sanitizer build clean on macOS | done |
| 5 | Hardware suite `test_ccd_libcamera_hw` | recorded run `python3 tools/run_driver_test.py ccd_libcamera --hw` on the RPi | done, 10/10 (after defects 4-7) |
| 6 | Docs, `PROPERTIES.md`, `MIGRATION_STATUS.md`, Makefile list, Xcode project | review | done |

## Scenario-to-test mapping (fake bridge)

| Area | Test case |
| --- | --- |
| Identity, metadata, property contract, CCD_INFO, limits, mode labels, disconnect releases camera | Metadata and property contract |
| Capability profile without gain / exposure limits | Capability profile without gain |
| Discovery, sensor names, duplicate names, empty model, per-device camera id | Discovery names and duplicates |
| Capacity (more cameras than `MAX_DEVICES`) | Discovery capacity |
| No cameras, libcamera unavailable | No cameras and unavailable library |
| Repeated INIT/SHUTDOWN, balanced initialize/terminate | Repeated INIT and SHUTDOWN |
| Open failure, rollback, recovery | Open failure and recovery |
| Exposure arguments (mode, exposure, gain), RAW payload content, bias shortest exposure | Exposure and image contract |
| Colour subframe alignment, edge trimming, mode change, Bayer keyword | Colour subframe, mode and Bayer |
| Mono subframe, no Bayer keyword | Mono subframe without Bayer |
| Abort before completion, no late frame, reacquire | Abort and reacquire |
| Finite stream exact count, indefinite stream, abort, no frames after abort | Finite and aborted streaming |
| Start failure, read failure, invalid frame, readout timeout, streaming failures, recovery | Bridge failures and recovery |
| Refused CCD_FRAME/CCD_MODE while BUSY, stream ignored during exposure | Busy rejection and overlap |
| Rejected shutdown while connected, disconnect during exposure, no calls after close, reconnect | Disconnect during exposure and shutdown |

Not applicable: cooling, temperature, guider, wheel, focuser (libcamera exposes none of them), hot-plug (CSI), configuration persistence of driver-specific settings (none).

## Hardware results (2026-10-25, RPi 3 B+, OV5647, libcamera 0.7.2+rpt20261009)

Recorded run: 10/10 passed. Modes: RAW 10 2592x1944, 1920x1080, 1296x972 (bin 2x2), 640x480 (bin 4x4); exposure limits 130 µs .. 3.067 s at full resolution (4.6 s at 640x480); analogue gain 1 .. 63.94. A 2 s exposure completes in about 2.7 s; 10 streamed full frames at 10 ms take about 1.5 s; abort is acknowledged at once and no frame follows; three reconnects and a SHUTDOWN/INIT cycle each deliver a fresh frame. The camera was covered (mean ≈ 15 DN), so the gain and long-exposure cases compare noise and hot-pixel maxima as well as the mean. Physical hot-plug: not applicable (CSI). Multiple cameras: only one available; the multi-camera paths are covered by the fake bridge only.

## Found defects

All found during this task, before the first commit. Defects 9-13 come from the independent read-only quality review.

| # | Observation | Root cause | Fix | Regression evidence |
| --- | --- | --- | --- | --- |
| 1 | Bridge did not compile against libcamera 0.7.2 (`std::span` errors) | libcamera 0.7 headers need C++20 | `Makefile.inc` builds with `-std=gnu++20`; `FrameDurationLimits` set from an initializer list | RPi build |
| 2 | `properties::PixelArraySize` missing, `Model` is `std::string_view` (libcamera 0.7) | API change | use `PixelArrayActiveAreas[0]` (present in all versions), copy `Model` through `std::string` | RPi build; hardware "Discovery and identity" (2592 x 1944) |
| 3 | Manual exposure not guaranteed on libcamera 0.5+ | `AeEnable` replaced by `ExposureTimeMode`/`AnalogueGainMode` | set both manual modes when the camera offers them (guarded by `LIBCAMERA_VERSION`) | hardware exposure metadata matches the request |
| 4 | Every exposure ended in ALERT "Frame capture failed" (hardware) | the Raspberry Pi pipeline returns the first frames after start as `FrameError` and then `FrameStartup` | requeue such frames; accept `FrameStartup` when its metadata exposure is the requested one | hardware suite (all acquisition cases). Not reproducible with the fake bridge: the behaviour is inside `indigo_ccd_libcamera_bridge.cpp`, which the fake replaces |
| 5 | CCD_EXPOSURE limited to 66 ms although the sensor exposes 3 s (hardware) | ControlInfo before `configure()` belongs to no mode | read limits per mode after configuring each mode at connect; driver applies the mode's limits and redefines CCD_EXPOSURE/CCD_STREAMING on mode change | fake profile now carries the measured per-mode limits; "Colour subframe, mode and Bayer" asserts the 4.6 s limit of the binned mode and a 3.2 s exposure; hardware "Discovery and identity" |
| 6 | One full frame took 1.2 s to copy; 10 streamed frames took 12.3 s (hardware) | per-sample reads from uncached DMA memory | copy rows into cached memory first, with `DMA_BUF_IOCTL_SYNC` | hardware "Finite streaming" asserts < 5 s (1.5 s measured). Not reproducible with the fake bridge (memory type) |
| 7 | A 2 s exposure took 7.6 s (hardware) | the unusable start frames each lasted the full exposure | start long exposures with the shortest exposure and request the real one with the first request | hardware "Long exposure" asserts < exposure + 2 s (2.7 s measured). Not reproducible with the fake bridge (inside the bridge) |
| 8 | Invalid `usb = false` combinations were reported but generation continued | block parsers cannot signal a fatal error to `parse_driver_block()` | generator exits with status 1 | `test_generator_architecture` "SDK discovery without USB" (6 refused definitions) |
| 9 | An exposure requested during streaming (or the reverse) was dropped without an answer (independent review, source audit) | `on_change_request` returned without replying | `reject_change` blocks answer with ALERT/BUSY message | fake "Busy rejection and overlap" uses `assert_rejected_number_change_on` |
| 10 | Disconnect during streaming left the video stream open and image properties BUSY (review, source audit) | `on_disconnect` only stopped the camera | finalize the stream and run `indigo_ccd_failure_cleanup()` when an acquisition was BUSY | fake "Disconnect during exposure and shutdown" streams to a local SER file and checks the frame count `indigo_ser_close()` writes; it fails with the finalization removed |
| 11 | Packed RAW row size too small for widths not divisible by the CSI-2 group size, heap over-read (review, latent: all current modes are multiples of 4) | row size computed from bits instead of whole groups | whole 5-byte/3-byte groups | source audit only; not reachable through the fake bridge or the OV5647 modes |
| 12 | A long exposure the sensor quantizes outside 5 % would be dropped 16 times and time out (review, source audit) | count-based discard limit vs exposure + 10 s deadline | accept the first frame that no longer carries the short start exposure. Trade-off: a sensor that ramps the exposure over several frames would deliver an intermediate exposure; the frame metadata exposure is logged with every frame | source audit; hardware "Long exposure" still passes (2.7 s) |
| 13 | Mode change kept stale CCD_FRAME limits in clients (review) | CCD_FRAME only updated | CCD_FRAME redefined with the exposure properties | fake "Colour subframe, mode and Bayer" (frame values after the mode change) |

## Final test summary

- Simulated (fake bridge) tests: 15 run, 15 passed (macOS arm64, recorded 2026-10-25 00:08); 15 run, 15 passed (Linux arm64, recorded 2026-10-25 00:08); ASan/UBSan build 15/15 on macOS arm64.
- Hardware tests: 10 run, 10 passed (Raspberry Pi 3 B+, OV5647, recorded 2026-10-25 00:04).
- Generator: 120/120 existing definitions regenerate identically; `test_generator_architecture` passes, including the new USB-free case.
