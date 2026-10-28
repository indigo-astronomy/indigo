# FLI filter wheel refactoring and validation record

Status: migration to `indigo_generator` started 2026-09-20. This driver is migrated together with `ccd_fli` and `focuser_fli`, which share the same vendored SDK and the same hot-plug scaffolding.

## Current-state audit (2026-09-20)

### Architecture and implementation

- `indigo_wheel_fli.c` is a hand-written 516-line hot-plug driver, version `0x0300000A`, driver label `FLI Filter Wheel`, `multi_device_support` true.
- The transport is the vendored `libfli-1.999.1-180223` SDK in `externals/`. Devices are found with `FLICreateList(FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL)` followed by `FLIListFirst()`/`FLIListNext()`/`FLIDeleteList()`, and opened by file name with `FLIOpen()`.
- Hot plug is a hand-written framework: a libusb callback filtered on vendor id `0x0f18` schedules `process_plug_event()` or `process_unplug_event()` through `indigo_set_timer()`, which re-enumerate the SDK list and diff it against a 32-entry device array under a driver-wide mutex.
- Motion is one `indigo_timer` per device that writes `FLISetFilterPos()` and reads `FLIGetFilterPos()` back, guarded by a per-device `usb_mutex`.
- `WHEEL_SLOT` is the only class property the driver drives; slot count comes from `FLIGetFilterCount()`.

### Defects, risks and lifecycle findings

- `find_plugged_device()`, `find_device_slot()` and `find_unplugged_device()` dereference `PRIVATE_DATA`, which expands to `device->private_data` of the **local** `device` variable of the enclosing loop; in `find_plugged_device()` the comparison therefore reads the private data of an arbitrary already-attached device rather than of the candidate. Recorded as `DRV-161`.
- `enumerate_devices()` writes into `fli_domains[num_devices]` and the name arrays with `num_devices` already equal to `MAX_DEVICES` on the last `FLIListNext()` call of a full list, one element past the end of all three arrays. Recorded as `DRV-162`.
- The slot handler does not distinguish a failed `FLISetFilterPos()` from a mismatch, and reports `INDIGO_ALERT_STATE` from the position readback only.
- `wheel_timer_callback` runs on `indigo_set_timer` rather than the device handler queue, and each device keeps its own `usb_mutex` although the SDK is serialised per device by the driver-wide enumeration mutex only during attach and detach.
- The vendored SDK leaks 8 KiB plus 12 bytes per connected device on every enumeration; this is noted in the source and is an SDK defect, not a driver one.

### Platforms, build and packaging

Linux and macOS. `Makefile.inc` builds the vendored SDK archive. `indigo_wheel_fli.vcxproj` exists but the SDK is not built on Windows.

### Test assets

None. There is no simulator, no fake SDK and no automated test; `MIGRATION_STATUS.md` records 0 / 0.

## Baseline (2026-09-20, macOS 15 arm64, universal x86_64+arm64 build)

- `make -C indigo_drivers/wheel_fli -f ../../Makefile.drv` — succeeded, no warnings.
- No tests existed at the start of the migration, so there was no pre-existing baseline test result.
- The new characterization suite `indigo_test/integration/test_wheel_fli_sdk.c` was first run against the **unchanged** driver (built from `git show HEAD:indigo_drivers/wheel_fli/indigo_wheel_fli.c`, same fake SDK and the same `-D` symbol replacements): **13 of 14 scenarios passed**. The single expected baseline failure was `filter_count_failure`, which reproduces `DRV-163`.

## Hardware-test decision

Hardware testing will **not** be performed. No FLI filter wheel is available in this environment, and no hardware validation is claimed anywhere in this record.

## Migration plan

1. Write a fake libfli SDK shared by the three FLI drivers' tests, covering enumeration, open/close, identification and the wheel, focuser and camera entry points, with fault injection.
2. Write the wheel characterization suite against the *unchanged* driver and record any defect reproducer that cannot pass as an expected baseline failure.
3. Migrate the driver to an `sdk { hotplug = true; }` generator definition.
4. Build, re-run the suite and compare behavior.
5. Register the `.driver` file and update the status documents.

## Found defects

| Id | Severity | Description | Reproducer | Resolution |
| --- | --- | --- | --- | --- |
| `DRV-161` | High | `find_plugged_device()`, `find_device_slot()` and `find_unplugged_device()` expand `PRIVATE_DATA` to `device->private_data` of the **local** loop variable, so the candidate is compared against an arbitrary already-attached device's private data. A second wheel could therefore be rejected as a duplicate, or an unplug could match the wrong device. | `two_wheels`, `duplicate_arrival`, `hot_unplug` exercise the paths; the mismatch is latent for the enumeration orders the fake SDK produces, so it is a code defect found by inspection rather than a failing baseline scenario. | Removed. The generator owns device discovery and the plug/unplug diff; the driver only supplies `fli_enumerate()` and `fli_is_enumerated()`, which take the name to compare as a parameter. |
| `DRV-162` | High | `enumerate_devices()` writes `fli_domains[num_devices]` and both name arrays while `num_devices` is already `MAX_DEVICES`, one element past the end of all three arrays, whenever the SDK reports a full list. | Out-of-bounds write; not reachable with the 2-device fake SDK, found by inspection and confirmed against the original source. | Removed. `fli_enumerate()` bounds every write with `count < FLI_MAX_DEVICES` before storing. |
| `DRV-163` | Medium | A wheel whose `FLIGetFilterCount()` fails or reports 0 slots was still reported as connected, with `WHEEL_SLOT_ITEM->number.max` left at whatever the previous device had; the SDK handle stayed open. | `filter_count_failure` — fails against the original driver (connection reports `OK`), passes against the generated one. | Fixed. `fli_open()` is transactional: filter count, position and identity are all read while the handle is open, and any failure closes the handle and fails the connection with `ALERT`. |
| `DRV-164` | Low | `wheel_timer_callback()` ran on an `indigo_set_timer()` timer, so a slot change queued while another was in flight ran concurrently with it under a per-device `usb_mutex` rather than being serialised. | Structural; covered by `select_slots`. | Removed. The slot change runs on the generated device handler queue; the mutex and the timer are gone. |
| `DRV-165` | Low | The slot handler reported `ALERT` from the readback only, so a failed `FLISetFilterPos()` whose readback happened to succeed was reported as `OK`. | `select_failure` | Fixed. The set and the readback are checked separately and each failure is reported with its own message. |

The vendored SDK's 8 KiB-per-enumeration leak is an SDK defect and is left unchanged; it is documented in the source.

## Final test summary

| Suite | Scenarios | Result |
| --- | --- | --- |
| `test_wheel_fli_sdk` (simulated, fake libfli) | 14 | 14 passed |
| `test_wheel_fli_sdk_asan` (ASan + UBSan) | 14 | 14 passed, no sanitizer reports |
| Hardware | 0 | not performed — no FLI filter wheel available |

Coverage map:

| Scenario | Covers |
| --- | --- |
| `driver_metadata` | version `0x0300000B`, label, `INDIGO_DRIVER_INFO` |
| `attach_and_identify` | attach on arrival, `INFO` model/firmware, `WHEEL_SLOT` count and bounds |
| `select_slots` | slot change, readback, `OK` state, wrap to slot 1 |
| `select_failure` | `FLISetFilterPos()` failure → `ALERT` (`DRV-165`) |
| `readback_failure` | `FLIGetFilterPos()` failure → `ALERT` |
| `uninitialised_wheel_is_homed` | position `-1` from an unhomed wheel is normalised to slot 1 |
| `open_failure` | `FLIOpen()` failure → connection `ALERT`, no handle leaked |
| `filter_count_failure` | `DRV-163` |
| `two_wheels` | two devices attach, connect and select independently (`DRV-161`) |
| `hot_unplug` | unplug while connected detaches the device and releases the handle |
| `duplicate_arrival` | a second arrival event for a present device attaches nothing (`DRV-161`) |
| `arrival_without_device` | an arrival event with an empty SDK list attaches nothing |
| `reconnect` | disconnect/reconnect cycle re-reads the wheel state |
| `shutdown_releases_devices` | `INDIGO_DRIVER_SHUTDOWN` detaches every device and unrefs every USB device |
