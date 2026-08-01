# indigo_optional_drivers Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Complete static review for in-scope optional drivers at this commit. |

## Scope

Optional INDIGO drivers that depend on SDKs, libraries, services, or hardware integrations outside the core portable driver set.

Excluded from this review:

- `externals/` and `bin_externals/` SDK/vendor trees.
- Build products and generated binaries.
- Simulator-only driver trees.

## Coverage Manifest

| Group | Directories | Source files reviewed |
| --- | --- | --- |
| Optional CCD drivers | `ccd_andor`, `ccd_rpi` | `indigo_ccd_andor.c`, `indigo_ccd_andor.h`, `indigo_ccd_andor_main.c`, `indigo_ccd_rpi.cpp`, `indigo_ccd_rpi.h`, `indigo_ccd_rpi_main.c` |

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| OPTDRV-001 | High | `ccd_andor/indigo_ccd_andor.c:1492`, `ccd_andor/indigo_ccd_andor.c:1496`, `ccd_andor/indigo_ccd_andor.c:1503`, `ccd_andor/indigo_ccd_andor.c:1534` | Andor initialization allocates `private_data` and `device`, locks `driver_mutex`, then breaks out of the camera loop when `Initialize()` fails without unlocking the mutex or freeing the just-allocated objects. A missing or invalid SDK path can leave the global mutex permanently locked and leak the partially constructed device. Use a cleanup path that unlocks the mutex, frees partial allocations, and continues or aborts cleanly. | Open |
| OPTDRV-002 | High | `ccd_andor/indigo_ccd_andor.c:1564`, `ccd_andor/indigo_ccd_andor.c:1567`, `ccd_andor/indigo_ccd_andor.c:1573` | Andor shutdown destroys the static global `driver_mutex` inside the per-device loop after using it for the current device. With multiple cameras, the next iteration locks a destroyed mutex; after one shutdown/init cycle, future SDK operations can also use an invalid mutex. Do not destroy this static process-lifetime mutex per camera. | Open |
| OPTDRV-003 | High | `ccd_rpi/indigo_ccd_rpi.cpp:971`, `ccd_rpi/indigo_ccd_rpi.cpp:973`, `ccd_rpi/indigo_ccd_rpi.cpp:980`, `ccd_rpi/indigo_ccd_rpi.cpp:610` | Raspberry Pi exposure completion can block forever. `exposure_timer_callback()` polls `IsAnyRequestComplete()` with no timeout while `state` never changes, and then `ReadFrame()` waits on `m_cv` with no cancellation/timeout. If libcamera drops a completion, a request was never queued, or the camera is stopped during the wait, the INDIGO timer callback can hang and leave exposure properties busy. Add a bounded wait and fail the exposure with cleanup. | Open |
| OPTDRV-004 | Medium | `ccd_rpi/indigo_ccd_rpi.cpp:1364`, `ccd_rpi/indigo_ccd_rpi.cpp:1372`, `ccd_rpi/indigo_ccd_rpi.cpp:1440`, `ccd_rpi/indigo_ccd_rpi.cpp:1445` | Raspberry Pi exposure startup sets image and exposure properties to `INDIGO_BUSY_STATE`, but if `StartExposure()` fails it only logs the error. No property is moved to `INDIGO_ALERT_STATE`, no failure cleanup runs, and no client update reports completion/failure. Report the failed start through `CCD_EXPOSURE_PROPERTY` and image upload properties before returning. | Open |
| OPTDRV-005 | Medium | `ccd_rpi/indigo_ccd_rpi.cpp:187`, `ccd_rpi/indigo_ccd_rpi.cpp:204`, `ccd_rpi/indigo_ccd_rpi.cpp:1691`, `ccd_rpi/indigo_ccd_rpi.cpp:1692` | The Raspberry Pi driver copies unbounded external strings into fixed buffers: `LIBCAMERA_LOG_LEVELS` into `previousSDKLogLevel[64]`, and libcamera camera names into `name[128]` via `strcat()`. Long environment values or camera names can overflow stack/static buffers. Use bounded copies or `std::string` construction followed by `snprintf()` into INDIGO name buffers. | Open |
| OPTDRV-006 | Medium | `ccd_rpi/indigo_ccd_rpi.cpp:787`, `ccd_rpi/indigo_ccd_rpi.cpp:788`, `ccd_rpi/indigo_ccd_rpi.cpp:624`, `ccd_rpi/indigo_ccd_rpi.cpp:627` | `mmap()` results are stored without checking for `MAP_FAILED`. A failed mapping is later read as image memory in `ReadFrame()` and later passed to `munmap()`, which can crash or corrupt exposure output under memory pressure or invalid buffer FDs. Check each mapping, unwind any mappings made for the request, and fail `StartCapture()` cleanly. | Open |
| OPTDRV-007 | Medium | `ccd_rpi/indigo_ccd_rpi.cpp:1129`, `ccd_rpi/indigo_ccd_rpi.cpp:1146`, `ccd_rpi/indigo_ccd_rpi.cpp:1165`, `ccd_rpi/indigo_ccd_rpi.cpp:1184` | The Raspberry Pi connection path appends mode items for every supported format/size without checking the allocated capacity of `CCD_MODE_PROPERTY` or resizing it. Cameras that report many formats or sizes can write past the base CCD mode property item array. Bound the count or resize the property before adding items. | Open |
| OPTDRV-008 | Medium | `ccd_rpi/indigo_ccd_rpi.cpp:1212`, `ccd_rpi/indigo_ccd_rpi.cpp:1216`, `ccd_rpi/indigo_ccd_rpi.cpp:1269`, `ccd_rpi/indigo_ccd_rpi.cpp:1624` | `X_CCD_ADVANCED_PROPERTY` is deleted on disconnect but not released or nulled, then reconnect defines the stale pointer before overwriting it with a newly allocated property. Detach only releases it while connected. This leaks the advanced property on disconnect/reconnect and leaves stale property state reachable. Release and null the property on disconnect, and allocate before defining it on connect. | Open |
| OPTDRV-009 | Low | `ccd_rpi/indigo_ccd_rpi.cpp:1708`, `ccd_rpi/indigo_ccd_rpi.cpp:1710`, `ccd_rpi/indigo_ccd_rpi.cpp:1711` | The focuser device name is initialized, but the uniqueness call is accidentally applied to `device->name` again instead of `focuser->name`. Multiple RPi autofocus devices can therefore expose duplicate focuser names. Call `indigo_make_name_unique(focuser->name, ...)` for the focuser. | Open |

## Review Focus

- Build guards and optional dependency detection.
- Failure behavior when optional SDKs or services are unavailable.
- Driver lifecycle consistency with core drivers.
- Property lifecycle, connection/disconnection cleanup, and timer cancellation.
- Resource cleanup for SDK handles, mapped buffers, dynamically created child devices, and optional properties.
- Clear documentation of prerequisites and platform limitations.
- Avoiding mandatory dependencies from optional code paths.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Reviewed all in-scope optional driver source/header/main files listed in the coverage manifest. |
