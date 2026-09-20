# FLI camera refactoring and validation record

Status: migration to `indigo_generator` started 2026-09-20. This driver is migrated together with `focuser_fli` and `wheel_fli`, which share the same vendored SDK and the same hot-plug scaffolding.

## Current-state audit (2026-09-20)

### Architecture and implementation

- `indigo_ccd_fli.c` is a hand-written 1158-line hot-plug driver, version taken from `DRIVER_VERSION`, driver label `FLI Camera`, `multi_device_support` true.
- Enumeration, hot plug and the 32-entry device array are the same hand-written framework as in `wheel_fli`, with `FLIDOMAIN_USB | FLIDEVICE_CAMERA`.
- On connect the driver opens the device, reads the total and visible array areas, probes RBI flood support by setting `FLI_FRAME_TYPE_RBI_FLUSH`, allocates the image buffer, enumerates the camera modes with `FLIGetCameraModeString()`, and reads pixel size, model, serial number, firmware and hardware revisions and the current temperature.
- An exposure sets binning, image area, exposure time and frame type and calls `FLIExposeFrame()`; a timer waits for the exposure and then `fli_read_pixels()` polls `FLIGetExposureStatus()` and `FLIGetDeviceStatus()` before grabbing the frame row by row with `FLIGrabRow()`.
- RBI flood adds a preliminary flooded dark exposure followed by `CCD_RBI_FLUSH_COUNT` bias frames that are read and discarded.
- Cooling is a 3-second timer that reads the temperature and cooler power and writes the set point; the SDK has no cooler on/off, so off is expressed as a +45 °C set point.
- Driver-defined properties are `FLI_NFLUSHES` and `FLI_CAMERA_MODE`, both connection-dependent and both saved by `CONFIG.SAVE`.

### Defects, risks and lifecycle findings

- `fli_read_pixels()` waits for the exposure with `do { FLIGetExposureStatus(); if (timeleft) indigo_usleep(timeleft); } while (timeleft*1000);` — `timeleft` is in milliseconds but is passed to a microsecond sleep, so the driver spins roughly a thousand times faster than intended, and the loop ignores the SDK return code entirely. Recorded as `DRV-181`.
- The same enumeration defects as in `wheel_fli` are present verbatim: the private-data confusion in `find_plugged_device()` and the one-past-the-end write in `enumerate_devices()`.
- `fli_set_cooler()` keeps the previous set point in a **function-level `static`**, so two connected cameras share one `old_target` and the second camera's set point can be silently skipped. Recorded as `DRV-182`.
- The exposure, abort and property handlers call the SDK directly from the bus thread; only the connection handler is queued.
- `FLI_CAMERA_MODE_PROPERTY` is allocated with `MAX_MODES` items and resized on connect, but never restored to `MAX_MODES` when the device disconnects, so a reconnect to a camera with more modes cannot publish them. Recorded as `DRV-183`.
- `#undef INDIGO_DEBUG_DRIVER` / `#define INDIGO_DEBUG_DRIVER(c) c` forces driver debug output on unconditionally.

### Platforms, build and packaging

Linux and macOS. `Makefile.inc` builds the vendored SDK archive. `indigo_ccd_fli.vcxproj` exists but the SDK is not built on Windows.

### Test assets

None. There is no simulator, no fake SDK and no automated test; `MIGRATION_STATUS.md` records 0 / 0.

## Baseline (2026-09-20, macOS 15 arm64, universal x86_64+arm64 build)

- `make -C indigo_drivers/ccd_fli -f ../../Makefile.drv` — succeeded, no warnings.
- No tests existed at the start of the migration, so there was no pre-existing baseline test result.
- The new characterization suite `indigo_test/integration/test_ccd_fli_sdk.c` was run against the **unchanged** driver (built from `git show HEAD:indigo_drivers/ccd_fli/indigo_ccd_fli.c`, same fake SDK and the same `-D` symbol replacements). The run **cannot complete**: the original driver deadlocks and the suite has to be killed. The last captured run reached

  ```
  PASS driver_metadata
  PASS attach_and_identify
  PASS no_rbi_flood_support
  PASS single_exposure
  PASS frame_types
  PASS binning_and_roi
  PASS frame_is_rounded
  PASS exposure_failure
  ```

  and then hung. `sample(1)` on the hung process shows the bus thread in
  `indigo_change_number_property_1 → indigo_ccd_change_property → indigo_ccd_exposure_setup → indigo_queue_remove → pthread_cond_wait`: the exposure runs on the bus thread, so `indigo_ccd_exposure_setup()` waits for a queue the bus thread itself has to drain. This is `DRV-184`. The scenario at which the hang lands varies between runs (`binning_and_roi` in one run, `readout_failure` in another), which is consistent with a race rather than a fixed scenario. **8 of 22 scenarios** is therefore the best baseline that can be recorded for the original driver.

## Hardware-test decision

Hardware testing will **not** be performed. No FLI camera is available in this environment, and no hardware validation is claimed anywhere in this record.

## Migration plan

1. Write a fake libfli SDK shared by the three FLI drivers' tests.
2. Write the camera characterization suite against the *unchanged* driver and record any defect reproducer that cannot pass as an expected baseline failure.
3. Migrate the driver to an `sdk { hotplug = true; }` generator definition.
4. Build, re-run the suite and compare behavior.
5. Register the `.driver` file and update the status documents.

## Found defects

| Id | Severity | Description | Reproducer | Resolution |
| --- | --- | --- | --- | --- |
| `DRV-161` | High | The `PRIVATE_DATA`-of-the-loop-variable confusion in `find_plugged_device()`, `find_device_slot()` and `find_unplugged_device()`, verbatim as in `wheel_fli`. | Inspection; the hot-plug paths are covered by `two_cameras_cool_independently`, `hot_unplug`. | Removed — device discovery is generator-owned; the driver supplies only `fli_enumerate()` / `fli_is_enumerated()`. |
| `DRV-162` | High | The one-past-the-end write in `enumerate_devices()`, verbatim as in `wheel_fli`. | Inspection. | Removed — `fli_enumerate()` bounds every write. |
| `DRV-184` | Critical | The exposure was driven from `indigo_ccd_change_property()` on the **bus thread**, so `indigo_ccd_exposure_setup()` blocked in `indigo_queue_remove()` waiting for work only the bus thread could dispatch. The driver deadlocks, taking the whole bus with it. | The original driver hangs partway through `test_ccd_fli_sdk` and the suite never finishes (see the baseline above). | Fixed. Exposure start, readout and the RBI flood phase all run on the generated device handler queue; the bus thread only validates and queues. |
| `DRV-181` | Medium | `fli_read_pixels()` took the **millisecond** time-left reported by `FLIGetExposureStatus()` and passed it straight to `indigo_usleep()`, which takes microseconds, so the wait was 1000× too short and the readout loop busy-spun on the SDK for the whole exposure. | Inspection of `fli_read_pixels()`; the readout path is covered by `single_exposure` and `readout_failure`. | Fixed. The exposure wait is a `FLI_POLL_DELAY` handler on the device queue rather than a spin, and no millisecond value is passed to a microsecond sleep. |
| `DRV-182` | High | `fli_set_cooler()` kept the previous set point in a **function-level `static`**, shared by every camera in the process. With two cameras, one camera's set point suppressed the other's SDK write. | `two_cameras_cool_independently` | Fixed. `previous_set_point` is per-device private data. |
| `DRV-183` | Medium | `FLI_CAMERA_MODE_PROPERTY->count` was shrunk to the number of modes the camera reports but never restored on the next connect, so a camera with fewer modes permanently truncated the property for every later camera and every later connect. | `camera_mode_count_restored` | Fixed. `fli_read_camera_modes()` restores `count` to the number of declared items before re-counting. |
| `DRV-185` | Medium | A failed `FLIGetVisibleArea()` / `FLIGetPixelSize()` left the geometry at zero and the camera still reported connected. | `geometry_failure` | Fixed. `fli_open()` is transactional — a geometry failure logs `Camera geometry could not be read`, closes the handle and fails the connection with `ALERT`. |
| `DRV-186` | Low | `FLI_CAMERA_MODE` and `FLI_NFLUSHES` could be changed while an exposure was running, reconfiguring the camera mid-frame. | `camera_mode_failure`, `advanced_settings` | Fixed. Both carry a `reject_change` guard while `CCD_EXPOSURE` is `BUSY`. |

## Final test summary

| Suite | Scenarios | Result |
| --- | --- | --- |
| `test_ccd_fli_sdk` (simulated, fake libfli) | 22 | 22 passed |
| `test_ccd_fli_sdk_asan` (ASan + UBSan) | 22 | 22 passed, no sanitizer reports |
| Hardware | 0 | not performed — no FLI camera available |

Coverage map:

| Scenario | Covers |
| --- | --- |
| `driver_metadata` | version `0x0300000B`, label, `INDIGO_DRIVER_INFO` |
| `attach_and_identify` | attach on arrival, `INFO`, `CCD_INFO` geometry and pixel size |
| `no_rbi_flood_support` | `FLI_RBI_FLUSH*` stay hidden on a camera without RBI support |
| `single_exposure` | full exposure → readout → `CCD_IMAGE`, `OK` (`DRV-184`) |
| `frame_types` | light/dark/bias/flat map to the right shutter mode |
| `binning_and_roi` | `CCD_BIN` and `CCD_FRAME` reach the SDK visible area (`DRV-184`) |
| `frame_is_rounded` | odd frame origins and sizes are rounded to the binning |
| `exposure_failure` | `FLIExposeFrame()` failure → `ALERT` |
| `readout_failure` | `FLIGrabRow()` failure → `ALERT`, no partial image published |
| `abort_exposure` | `CCD_ABORT_EXPOSURE` cancels the pending handlers and stops the camera |
| `disconnect_during_exposure` | disconnect while exposing finalizes cleanly and closes the handle |
| `rbi_flood` | RBI flood frame is exposed, discarded and followed by `CCD_RBI_FLUSH_COUNT` flushes before the real frame |
| `cooling` | `CCD_COOLER`, `CCD_TEMPERATURE` set point and power reporting |
| `cooling_failure` | `FLISetTemperature()` failure → `ALERT` |
| `two_cameras_cool_independently` | `DRV-182` |
| `advanced_settings` | `FLI_NFLUSHES` and `FLI_CAMERA_MODE` round-trip (`DRV-186`) |
| `camera_mode_failure` | `FLISetCameraMode()` failure → `ALERT` |
| `camera_mode_count_restored` | `DRV-183` |
| `geometry_failure` | `DRV-185` |
| `hot_unplug` | unplug while connected detaches the device and releases the handle |
| `reconnect` | disconnect/reconnect cycle re-reads geometry and modes |
| `shutdown_releases_devices` | `INDIGO_DRIVER_SHUTDOWN` detaches every device and unrefs every USB device |
