# indigo_mac_drivers Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Complete expanded scoped static baseline review recorded findings and coverage manifest. Build products were excluded. |

## Scope

macOS-specific INDIGO drivers and platform integration code under `indigo_mac_drivers/`.

For the 2026-08-01 scoped baseline pass, build products and SDK/vendor subtrees named `externals`, `bin_externals`, `build`, `DerivedData`, or `.build` were intentionally excluded. The pass covered 8 C-family and Objective-C source/header files in 3 top-level driver directories.

## Coverage Manifest

| Group | Directories |
| --- | --- |
| CCD/camera | `ccd_atik2` |
| Bluetooth focusers | `focuser_mjkzz_bt`, `focuser_wemacro_bt` |

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| MACDRV-001 | High | `ccd_atik2/indigo_ccd_atik2.c:183` | CCD, guider, and wheel connection paths acquire the INDIGO global lock before `libatik_open()`, but if `libatik_open()` fails they only decrement `device_count` and leave the global lock held. A failed open can block later driver/device access until process restart. Track whether the lock was acquired and release it on every failed-open path. | Closed (fixed) |
| MACDRV-002 | Medium | `ccd_atik2/indigo_ccd_atik2.c:310` | The Atik2 detach handlers call `indigo_global_unlock()` whenever the detached device is the master device, independent of `device_count`. If CCD, guider, and wheel logical devices share the same physical camera, detaching the master while siblings are still connected can release the global lock early; if disconnect already released it, detach can also unlock a second time. Let the shared disconnect path own the lock lifecycle. | Closed (fixed) |
| MACDRV-003 | Medium | `ccd_atik2/indigo_ccd_atik2.c:577` | The hotplug arrival path allocates shared private data and one or more logical `indigo_device` objects, then searches the fixed `devices[MAX_DEVICES]` table. If the table is full, the newly allocated objects and the `libusb_ref_device()` reference are not released. Reserve slots before allocation or add cleanup when no slot is available. | Open |
| MACDRV-004 | Medium | `focuser_mjkzz_bt/indigo_focuser_mjkzz_bt.m:420`, `focuser_wemacro_bt/indigo_focuser_wemacro_bt.m:468` | Bluetooth driver shutdown only sets the static delegate to `nil`. It does not stop scanning, disconnect a connected peripheral, clear the CoreBluetooth delegate, or call `deleteDevice`, so an attached INDIGO device and malloc-backed private data can outlive driver shutdown. Add explicit delegate shutdown that disconnects, stops scans, detaches the device, and clears references. | Open |
| MACDRV-005 | Medium | `focuser_mjkzz_bt/indigo_focuser_mjkzz_bt.m:135`, `focuser_wemacro_bt/indigo_focuser_wemacro_bt.m:153` | Both Bluetooth drivers call `CFBridgingRetain(peripheral)` when reusing or discovering peripherals. MJKZZ never releases the retained object, and WeMacro's release path is unreachable because `hc08` is set to `nil` before the `if (hc08)` check. Under ARC this is still a manual retain leak. Remove the explicit retain or balance it before clearing the peripheral reference. | Open |
| MACDRV-006 | Medium | `focuser_mjkzz_bt/indigo_focuser_mjkzz_bt.m:210`, `focuser_wemacro_bt/indigo_focuser_wemacro_bt.m:236` | CoreBluetooth characteristic update callbacks assume the payload is exactly the expected protocol frame and immediately read 8 bytes for MJKZZ or byte 2 for WeMacro. A short or empty notification can read past `characteristic.value.bytes` and crash the driver. Validate `characteristic.value.length` before parsing. | Open |
| MACDRV-007 | Low | `focuser_mjkzz_bt/indigo_focuser_mjkzz_bt.m:27` | The MJKZZ Bluetooth focuser defines `DRIVER_NAME` as `"indigo_ccd_mjkzz_bt"`, while the entry point and public device name are focuser-specific. This reports incorrect driver metadata through `indigo_focuser_attach()` and log messages. Rename it to the focuser driver name. | Open |
| MACDRV-008 | Medium | `focuser_wemacro_bt/indigo_focuser_wemacro_bt.m:316` | WeMacro attach allocates three custom properties in sequence, but returns `INDIGO_FAILED` immediately if the second or third allocation fails. Any earlier custom property, plus resources allocated by `indigo_focuser_attach()`, are not released on those partial-failure paths. Add cleanup before returning failure or centralize custom-property allocation before base attach succeeds. | Open |
| MACDRV-009 | Medium | `ccd_atik2/indigo_ccd_atik2.c:516` | Atik2 wheel movement schedules `wheel_timer_callback` with a `NULL` timer handle, and the callback reschedules itself the same way. Disconnect and detach have no stored timer pointer to cancel, so a pending wheel timer can run after the logical wheel has been disconnected or freed by hotplug removal. Store the timer in shared private data and cancel it during wheel disconnect/detach. | Closed (fixed) |

## Finding Summaries

### MACDRV-009 (Closed — fixed)

`wheel_timer_callback` in `indigo_ccd_atik2.c` was scheduled with a `NULL` timer
handle (line 516 in `wheel_change_property`, line 450 in the callback itself when
the wheel is still moving). With no stored pointer, the wheel disconnect path had
no way to cancel an in-flight timer, allowing it to fire after the wheel was
disconnected or after hotplug removal had freed the device.

The fix adds `indigo_timer *wheel_timer` to `atik_private_data` (line 53) and
passes `&PRIVATE_DATA->wheel_timer` in both `indigo_set_timer` calls so the handle
is always tracked. `indigo_cancel_timer(device, &PRIVATE_DATA->wheel_timer)` is
called at the start of the wheel disconnect path (`CONNECTION_DISCONNECTED_ITEM`
branch), before `libatik_close`, ensuring any pending callback is cancelled before
the device context is torn down.

### MACDRV-002 (Closed — fixed)

All three detach handlers in `indigo_ccd_atik2.c` (`ccd_detach` line 308,
`guider_detach` line 433, `wheel_detach` line 531) called `indigo_global_unlock()`
whenever `device == device->master_device`, independent of `device_count`. The
disconnect path already calls `indigo_global_unlock()` when `device_count` reaches
0. If the master CCD device is detached while the guider or wheel sibling is still
connected, the unlock fires early. If all devices were disconnected first, detach
unlocks a second time.

The fix removes the `if (device == device->master_device) { indigo_global_unlock(device); }`
block from all three detach handlers. The shared disconnect path exclusively owns
the global lock lifecycle: lock acquired on first connection (`device_count == 0`),
released when the last sibling disconnects (`device_count == 0` again).

### MACDRV-001 (Closed — fixed)

All three connection handlers in `indigo_ccd_atik2.c` (CCD line 176, guider line
358, wheel line 476) follow the pattern: when `device_count++ == 0`, acquire the
global lock, then call `libatik_open()`. On failure the outer `else` branch only
decremented `device_count`, leaving the lock permanently held.

The fix adds `indigo_global_unlock(device)` immediately inside the `else` arm of
the `libatik_open()` call, within the `device_count++ == 0` block. The global lock
is now released as soon as the failed open is detected, before control reaches the
shared failure branch. The `result = 0` → `result = false` change is also applied
for type consistency.

## Review Focus

- Driver lifecycle: `INDIGO_DRIVER_INIT`, `INDIGO_DRIVER_SHUTDOWN`, and `INDIGO_DRIVER_INFO`.
- Attach/detach ordering and resource cleanup.
- Connection/disconnection behavior and property visibility.
- Property state transitions, especially `BUSY -> OK` and `BUSY -> ALERT`.
- Handler queue, timer, and async usage.
- Generated driver source synchronization with `.driver` inputs.
- Simulator coverage under `indigo_test/integration/`.
- macOS framework/API usage and build guards.
- Objective-C/C interoperability, ARC behavior, and manual CoreFoundation ownership.
- Device discovery, hotplug behavior, and resource cleanup.
- Error handling when hardware or frameworks are unavailable.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Initial review baseline only. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Exhaustive scoped static pass over all 3 macOS driver directories and 8 C-family/Objective-C source/header files; recorded `MACDRV-001` through `MACDRV-007`. |
| `017ba602857378e4aed489c065c76eacae15924c` | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Expanded pass over standalone XML adapter mains, partial attach failures, property allocation cleanup, property state publication, and timer ownership; recorded `MACDRV-008` and `MACDRV-009`. |
