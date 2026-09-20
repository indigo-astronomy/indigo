# FLI focuser refactoring and validation record

Status: migration to `indigo_generator` started 2026-09-20. This driver is migrated together with `ccd_fli` and `wheel_fli`, which share the same vendored SDK and the same hot-plug scaffolding.

## Current-state audit (2026-09-20)

### Architecture and implementation

- `indigo_focuser_fli.c` is a hand-written 698-line hot-plug driver, version `0x0300000A`, driver label `FLI Focuser`, `multi_device_support` true.
- Enumeration, hot plug and the 32-entry device array are the same hand-written framework as in `wheel_fli`, with `FLIDOMAIN_USB | FLIDEVICE_FOCUSER`.
- On connect the driver opens the device, reads the model, homes it with `FLIHomeDevice()`, polls `FLIGetDeviceStatus()` in a blocking loop until the moving bit clears, takes the resulting stepper position as the zero reference, reads the focuser extent, serial number and firmware and hardware revisions.
- Motion is relative only at SDK level: `FLIStepMotorAsync()` with at most `MAX_STEPS_AT_ONCE` = 4000 steps per call, and a polling timer that issues the remaining steps in further chunks.
- `FOCUSER_POSITION` is writable and is converted into a relative move against the cached position. `FOCUSER_SPEED` is hidden.

### Defects, risks and lifecycle findings

- The connect handler blocks the caller in `do { indigo_usleep(100000); FLIGetDeviceStatus(); } while (value & FLI_FOCUSER_STATUS_MOVING_MASK);` with no bound, so a focuser that never reports the end of homing hangs the connection forever. Recorded as `DRV-171`.
- The same enumeration defects as in `wheel_fli` are present verbatim: the private-data confusion in `find_plugged_device()` and the one-past-the-end write in `enumerate_devices()`.
- `focuser_change_property()` performs SDK calls directly on the bus thread for `FOCUSER_STEPS`, `FOCUSER_POSITION` and `FOCUSER_ABORT_MOTION` instead of dispatching to the device handler queue.
- `FOCUSER_ABORT_MOTION` cancels the polling timer but does not stop the motor, so the focuser keeps running to the target it was given.
- The driver reports `FOCUSER_STEPS` as the number of steps still outstanding, which is a driver-specific reinterpretation of the property.

### Platforms, build and packaging

Linux and macOS. `Makefile.inc` builds the vendored SDK archive. `indigo_focuser_fli.vcxproj` exists but the SDK is not built on Windows.

### Test assets

None. There is no simulator, no fake SDK and no automated test; `MIGRATION_STATUS.md` records 0 / 0.

## Baseline (2026-09-20, macOS 15 arm64, universal x86_64+arm64 build)

- `make -C indigo_drivers/focuser_fli -f ../../Makefile.drv` — succeeded, no warnings.
- No tests existed at the start of the migration, so there was no pre-existing baseline test result.
- The new characterization suite `indigo_test/integration/test_focuser_fli_sdk.c` was first run against the **unchanged** driver (built from `git show HEAD:indigo_drivers/focuser_fli/indigo_focuser_fli.c`, same fake SDK and the same `-D` symbol replacements): **10 of 17 scenarios passed**. The 7 expected baseline failures were `homing_failure` (`DRV-172`), `extent_failure` (`DRV-173`), `absolute_moves`, `relative_moves`, `long_move_is_chunked`, `limits_are_respected` (all four: `FOCUSER_POSITION` reports outstanding steps rather than the position, `DRV-175`) and `move_failure` (`DRV-174`).

## Hardware-test decision

Hardware testing will **not** be performed. No FLI focuser is available in this environment, and no hardware validation is claimed anywhere in this record.

## Migration plan

1. Write a fake libfli SDK shared by the three FLI drivers' tests.
2. Write the focuser characterization suite against the *unchanged* driver and record any defect reproducer that cannot pass as an expected baseline failure.
3. Migrate the driver to an `sdk { hotplug = true; }` generator definition.
4. Build, re-run the suite and compare behavior.
5. Register the `.driver` file and update the status documents.

## Found defects

| Id | Severity | Description | Reproducer | Resolution |
| --- | --- | --- | --- | --- |
| `DRV-161` | High | The `PRIVATE_DATA`-of-the-loop-variable confusion in `find_plugged_device()`, `find_device_slot()` and `find_unplugged_device()`, verbatim as in `wheel_fli`. | Inspection; the hot-plug paths are covered by `two_focusers`, `hot_unplug`. | Removed — device discovery is generator-owned; the driver supplies only `fli_enumerate()` / `fli_is_enumerated()`. |
| `DRV-162` | High | The one-past-the-end write in `enumerate_devices()`, verbatim as in `wheel_fli`. | Inspection. | Removed — `fli_enumerate()` bounds every write. |
| `DRV-171` | High | The connect handler waits for homing in an **unbounded** `do { indigo_usleep(100000); FLIGetDeviceStatus(); } while (status & FLI_FOCUSER_STATUS_MOVING_MASK);` loop on the bus thread. A focuser that never clears the moving bit hangs the connection, and with it the bus, forever. | `homing_waits_for_status` with the fake SDK's `moving_polls` held high. | Fixed. `fli_open()` polls at `FLI_POLL_DELAY` (0.5 s) for at most `FLI_HOME_TIMEOUT_CYCLES` (300) cycles — 150 s — and fails the connection with `ALERT` when the focuser never settles. The wait runs on the device handler queue, not the bus thread. |
| `DRV-172` | Medium | The return value of `FLIHomeDevice()` was ignored, so a focuser that refused to home was still reported as connected, with an arbitrary position. | `homing_failure` — fails against the original driver, passes against the generated one. | Fixed. `fli_open()` checks the result, logs `Focuser home position not found`, closes the handle and fails the connection. |
| `DRV-173` | Medium | `FLIGetStepperPosition()` / `FLIGetFocuserExtent()` failures were ignored; the extent silently defaulted to 1000 steps, so `FOCUSER_POSITION->number.max` was wrong for the whole session. | `extent_failure` — fails against the original driver, passes against the generated one. | Fixed. Both reads are checked in the transactional `fli_open()`; a failure logs `Focuser position or extent could not be read` and fails the connection. |
| `DRV-174` | Medium | When `FLIStepMotorAsync()` failed, the driver still published the requested target as the current `FOCUSER_POSITION` and left the property `OK`, so a rejected move looked successful. | `move_failure` — fails against the original driver, passes against the generated one. | Fixed. `fli_step()` reports the failure, the motion finalizer restores the real position from `FLIGetStepperPosition()` and the property settles `ALERT`. |
| `DRV-175` | Medium | `FOCUSER_POSITION` was updated with the number of steps still **outstanding** rather than with the focuser's position, a driver-specific reinterpretation of a class property. | `absolute_moves`, `relative_moves`, `long_move_is_chunked`, `limits_are_respected` — all four fail against the original driver. | Fixed. `FOCUSER_POSITION` now always carries the stepper position read back from the SDK; `FOCUSER_STEPS` carries the requested relative move. |
| `DRV-176` | Low | `FOCUSER_ABORT_MOTION` cancelled the polling timer but never stopped the motor, so the focuser kept running to its old target while the driver reported the motion aborted. | `abort_during_motion` | Fixed. Abort issues `FLIStepMotorAsync(dev, 0)`, cancels the pending motion handlers and resyncs the position. |
| `DRV-177` | Low | A zero-step move still issued an SDK command and a full poll cycle. | `relative_moves` (zero-step leg) | Fixed. `fli_start_motion()` settles immediately when the clamped target equals the current position. |

## Final test summary

| Suite | Scenarios | Result |
| --- | --- | --- |
| `test_focuser_fli_sdk` (simulated, fake libfli) | 17 | 17 passed |
| `test_focuser_fli_sdk_asan` (ASan + UBSan) | 17 | 17 passed, no sanitizer reports |
| Hardware | 0 | not performed — no FLI focuser available |

Coverage map:

| Scenario | Covers |
| --- | --- |
| `driver_metadata` | version `0x0300000B`, label, `INDIGO_DRIVER_INFO` |
| `attach_and_identify` | attach on arrival, `INFO`, `FOCUSER_POSITION` bounds from the SDK extent |
| `homing_waits_for_status` | bounded homing wait (`DRV-171`) |
| `homing_failure` | `DRV-172` |
| `extent_failure` | `DRV-173` |
| `absolute_moves` | `FOCUSER_POSITION` targets, readback, `OK` (`DRV-175`) |
| `relative_moves` | `FOCUSER_STEPS` inward/outward and the zero-step no-op (`DRV-175`, `DRV-177`) |
| `long_move_is_chunked` | moves longer than `FLI_MAX_STEPS_AT_ONCE` (4000) are split (`DRV-175`) |
| `limits_are_respected` | targets are clamped to `[0, extent]` (`DRV-175`) |
| `move_failure` | `DRV-174` |
| `progress_failure` | `FLIGetStepsRemaining()` failure → `ALERT`, motion finalized |
| `abort_during_motion` | `DRV-176` — motor stopped, handlers cancelled, position resynced |
| `abort_while_idle` | abort with no motion in flight settles `OK` and leaves the focuser usable for the next move |
| `two_focusers` | two devices attach, connect and move independently (`DRV-161`) |
| `hot_unplug` | unplug while connected detaches the device and releases the handle |
| `reconnect` | disconnect/reconnect cycle re-homes and re-reads the extent |
| `shutdown_releases_devices` | `INDIGO_DRIVER_SHUTDOWN` detaches every device and unrefs every USB device |
