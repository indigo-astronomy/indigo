# Legacy finding migration

Source revision: `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. Retrieve original evidence with `git show <revision>:<path>`. Source locations are historical and need revalidation before repair. No application defect is fixed or closed by this migration.

Latest table dispositions override stale Open headings for DRV-085/086/087/096/097; these remain closed.

| Canonical ID | Legacy alias | Source | Legacy status |
| --- | --- | --- | --- |
| FND-000001 | DOCS-001 | `indigo_docs/REVIEW.md:36` | Open |
| FND-000002 | DOCS-002 | `indigo_docs/REVIEW.md:37` | Open |
| FND-000003 | DOCS-003 | `indigo_docs/REVIEW.md:38` | Open |
| FND-000004 | DOCS-004 | `indigo_docs/REVIEW.md:39` | Open |
| FND-000005 | DOCS-005 | `indigo_docs/REVIEW.md:40` | Open |
| FND-000006 | DRV-095 | `indigo_drivers/REVIEW.md:134` | Open (protocol confirmation needed) |
| FND-000007 | DRV-204 | `indigo_drivers/REVIEW.md:243` | Open |
| FND-000008 | EXAMPLES-001 | `indigo_examples/REVIEW.md:32` | Open |
| FND-000009 | EXAMPLES-002 | `indigo_examples/REVIEW.md:33` | Open |
| FND-000010 | EXAMPLES-003 | `indigo_examples/REVIEW.md:34` | Open |
| FND-000011 | EXAMPLES-004 | `indigo_examples/REVIEW.md:35` | Open |
| FND-000012 | EXAMPLES-005 | `indigo_examples/REVIEW.md:36` | Open |
| FND-000013 | LIB-013 | `indigo_libs/REVIEW.md:38` | Open |
| FND-000014 | LINUXDRV-001 | `indigo_linux_drivers/REVIEW.md:29` | Open |
| FND-000015 | LINUXDRV-002 | `indigo_linux_drivers/REVIEW.md:30` | Open |
| FND-000016 | LINUXDRV-003 | `indigo_linux_drivers/REVIEW.md:31` | Open |
| FND-000017 | LINUXDRV-004 | `indigo_linux_drivers/REVIEW.md:32` | Open |
| FND-000018 | LINUXDRV-005 | `indigo_linux_drivers/REVIEW.md:33` | Open |
| FND-000019 | OPTDRV-001 | `indigo_optional_drivers/REVIEW.md:30` | Open |
| FND-000020 | OPTDRV-002 | `indigo_optional_drivers/REVIEW.md:31` | Open |
| FND-000021 | OPTDRV-003 | `indigo_optional_drivers/REVIEW.md:32` | Open |
| FND-000022 | OPTDRV-004 | `indigo_optional_drivers/REVIEW.md:33` | Open |
| FND-000023 | OPTDRV-005 | `indigo_optional_drivers/REVIEW.md:34` | Open |
| FND-000024 | OPTDRV-006 | `indigo_optional_drivers/REVIEW.md:35` | Open |
| FND-000025 | OPTDRV-007 | `indigo_optional_drivers/REVIEW.md:36` | Open |
| FND-000026 | OPTDRV-008 | `indigo_optional_drivers/REVIEW.md:37` | Open |
| FND-000027 | OPTDRV-009 | `indigo_optional_drivers/REVIEW.md:38` | Open |
| FND-000028 | TEST-009 | `indigo_test/REVIEW.md:26` | Open (unblocked 2026-09-19: `DRV-195` is fixed and the binary now passes 11 of 11, so the only remaining step is adding it to `INTEGRATION_TESTS`; still absent from that list) |
| FND-000029 | TEST-010 | `indigo_test/REVIEW.md:27` | Open (intermittent; blocks a green `make -C indigo_test test`) |
| FND-000030 | TESTS-001 | `indigo_tests/REVIEW.md:29` | Open |
| FND-000031 | TESTS-002 | `indigo_tests/REVIEW.md:30` | Open |
| FND-000032 | TESTS-003 | `indigo_tests/REVIEW.md:31` | Open |
| FND-000033 | TESTS-004 | `indigo_tests/REVIEW.md:32` | Open |
| FND-000034 | TESTS-005 | `indigo_tests/REVIEW.md:33` | Open |
| FND-000035 | TESTS-006 | `indigo_tests/REVIEW.md:34` | Open |
| FND-000036 | TOOLS-001 | `indigo_tools/REVIEW.md:36` | Open |
| FND-000037 | TOOLS-002 | `indigo_tools/REVIEW.md:37` | Open |
| FND-000038 | TOOLS-003 | `indigo_tools/REVIEW.md:38` | Open |
| FND-000039 | TOOLS-004 | `indigo_tools/REVIEW.md:39` | Open |
| FND-000040 | TOOLS-005 | `indigo_tools/REVIEW.md:40` | Open |
| FND-000041 | TOOLS-006 | `indigo_tools/REVIEW.md:41` | Open |
| FND-000042 | TOOLS-007 | `indigo_tools/REVIEW.md:42` | Open |
| FND-000043 | TOOLS-008 | `indigo_tools/REVIEW.md:43` | Open |

## Scope and follow-up

Findings are nonblocking for this workflow-only adoption; they remain unresolved for affected application changes, security assessment and releases. TEST-010 continues to block a green full application test run. Legacy indigo_tests findings are retained solely to avoid loss; that directory is excluded from ordinary test commands. Security-labelled imports are historical triage, not new exploit reproduction or owner risk acceptance. The coordinator must prioritize revalidation and route confirmed security defects through security review.

## Historical review checkpoints

- `REVIEW.md`: index only; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_docs/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_drivers/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_examples/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_libs/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_linux_drivers/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_mac_drivers/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_optional_drivers/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_server/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_test/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_tests/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_tools/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `indigo_windows/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `systemd/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.
- `tools/REVIEW.md`: 017ba602857378e4aed489c065c76eacae15924c; preserved in `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. These checkpoints do not satisfy the new initial full audit.

## Full unresolved legacy statements

### FND-000001 (DOCS-001)

The server guide embeds a 2021 `indigo_server -h` output that no longer matches the current help text. It omits `-d-` / `--disable-blob-buffering`, `-C` / `--enable-blob-compression`, `-vvb` / `--enable-trace-bus`, and conditional `-f` / `--enable-rpi-management`, while retaining old version text and typos. Refresh this block from the current server help and keep the option descriptions below it in sync.

Historical location: `INDIGO_SERVER_AND_DRIVERS_GUIDE.md:11`, `INDIGO_SERVER_AND_DRIVERS_GUIDE.md:20`, `INDIGO_SERVER_AND_DRIVERS_GUIDE.md:26`, `INDIGO_SERVER_AND_DRIVERS_GUIDE.md:28`, `indigo_server/indigo_server.c:1912`, `indigo_server/indigo_server.c:1913`, `indigo_server/indigo_server.c:1918`, `indigo_server/indigo_server.c:1923`. Status: Open.

### FND-000002 (DOCS-002)

The "Handling Properties Asynchronously" section contains two consecutive versions of the same paragraph, one typo-heavy and one corrected. This makes the guidance look contradictory or accidentally duplicated in a core developer document. Remove the obsolete first paragraph and keep one edited version.

Historical location: `CLIENT_DEVELOPMENT_BASICS.md:271`, `CLIENT_DEVELOPMENT_BASICS.md:273`. Status: Open.

### FND-000003 (DOCS-003)

Some developer-facing links point to names or paths that do not exist in the current tree. `DEVELOPMENT.md` links `indigo_libs/indigo_bus.h`, but the header is under `indigo_libs/indigo/indigo_bus.h`; `CLIENT_DEVELOPMENT_BASICS.md` displays `indigo_examples/service_ddiscovery.c` even though the actual file is `indigo_examples/service_discovery.c`. Correct the link targets and visible labels so readers can navigate locally and on GitHub.

Historical location: `DEVELOPMENT.md:27`, `CLIENT_DEVELOPMENT_BASICS.md:440`. Status: Open.

### FND-000004 (DOCS-004)

Build documentation explains generated `Makefile.inc` and generator integration only briefly and contains spelling errors in names developers will copy (`Makefie.inc`, `usuall`). It also does not explain that `Makefile.drv` invokes `$(BUILD_BIN)/indigo_generator`, so a developer must build the generator before relying on automatic `.driver` regeneration. Clarify prerequisites and correct the names.

Historical location: `MAKEFILES.md:11`, `MAKEFILES.md:17`, `MAKEFILES.md:21`, `DRIVER_GENERATOR_MIGRATION.md:87`, `Makefile.drv:112`, `Makefile.drv:113`. Status: Open.

### FND-000005 (DOCS-005)

Client documentation says the remote-server example is a working sample but only warns that the simulator driver must be loaded. The actual examples hard-code `W11` and `indigosky` service/device names and have no bounded failure path when the service is absent. Either document those defaults and failure behavior explicitly or update the docs after the examples are made parameterized and bounded.

Historical location: `CLIENT_DEVELOPMENT_BASICS.md:509`, `remote_server_client.c:31`, `remote_server_client.c:163`, `remote_server_client_mount.c:30`, `remote_server_client_mount.c:177`. Status: Open.

### FND-000006 (DRV-095)

Direction encoding differs from upstream PHD2 (letters versus digits); device-protocol compatibility needs confirmation.

Historical location: `guider_cgusbst4/indigo_guider_cgusbst4.driver:110`. Status: Open (protocol confirmation needed).

### FND-000007 (DRV-204)

`selection subframe restore` fails deterministically: after guiding with `AGENT_GUIDER_SELECTION.SUBFRAME` set, aborting leaves `CCD_FRAME.WIDTH` at the subframe width (400) instead of restoring the full frame (1600), so the camera stays subframed after the agent stops. Reproduce with `build/integration/test_agent_guider "selection subframe restore"`. Verified pre-existing: it reproduces with `indigo_libs/` reverted to `6c28c23c6`, before this session's changes. Agents were outside the requested sweep, so it is recorded rather than fixed.

Historical location: `agent_guider/indigo_agent_guider.c`. Status: Open.

### FND-000008 (EXAMPLES-001)

The service discovery target is misspelled as `servce_discovery`, so `make all` builds a binary with the wrong name while `make clean` tries to remove `service_discovery`. The clean rule also omits `remote_server_client_mount` and uses plain `rm`, so it fails when outputs are absent. Rename the target to `service_discovery`, update `all`, and use `rm -f` for every generated example binary.

Historical location: `Makefile:20`, `Makefile:34`, `Makefile:40`. Status: Open.

### FND-000009 (EXAMPLES-002)

`driver` is uninitialized when `indigo_load_driver()` fails, but `indigo_remove_driver(driver)` is still called. A missing simulator driver or load error can therefore pass an indeterminate pointer into the driver manager. Initialize `driver = NULL` and call `indigo_remove_driver()` only after a successful load.

Historical location: `dynamic_driver_client.c:162`, `dynamic_driver_client.c:164`, `dynamic_driver_client.c:169`. Status: Open.

### FND-000010 (EXAMPLES-003)

The remote examples hard-code service/device names and never check whether `indigo_connect_server()` succeeded before waiting forever and later disconnecting `server`. If the target host is unavailable or has a different service name, the sample can hang indefinitely and then use an invalid server pointer. Accept host/service parameters or document the default clearly, check the connect result, initialize `server = NULL`, and use a bounded wait with a clear failure exit.

Historical location: `remote_server_client.c:31`, `remote_server_client.c:163`, `remote_server_client.c:164`, `remote_server_client.c:167`, `remote_server_client_mount.c:30`, `remote_server_client_mount.c:177`, `remote_server_client_mount.c:178`, `remote_server_client_mount.c:181`. Status: Open.

### FND-000011 (EXAMPLES-004)

Image-saving examples call `fopen()`, `fwrite()`, and `fclose()` without checking the file handle or write count. Running from an unwritable directory or hitting disk errors can crash or silently produce corrupt example output. Check `fopen()` before writing, verify `fwrite()` writes one complete BLOB, and log errors with `strerror(errno)`.

Historical location: `dynamic_driver_client.c:113`, `dynamic_driver_client.c:114`, `dynamic_driver_client.c:115`, `executable_driver_client.c:106`, `executable_driver_client.c:107`, `executable_driver_client.c:108`, `remote_server_client.c:109`, `remote_server_client.c:110`, `remote_server_client.c:111`. Status: Open.

### FND-000012 (EXAMPLES-005)

The executable-driver examples model abrupt child-process cleanup with `SIGKILL` and `exit()` from the client detach callback, and they do not handle `execl()` failure in the child. `client.c` also leaves the adapter attached/released only by process exit. As examples, these teach fragile lifecycle handling. Prefer graceful detach/release/stop flow, report child exec failure, close unused pipe ends in both processes, and terminate the child only from normal shutdown code.

Historical location: `client.c:93`, `client.c:95`, `client.c:96`, `client.c:130`, `client.c:131`, `client.c:133`, `executable_driver_client.c:133`, `executable_driver_client.c:135`, `executable_driver_client.c:136`, `executable_driver_client.c:164`. Status: Open.

### FND-000013 (LIB-013)

The device is published into `devices[]` and the bus mutex released *before* `device->attach()` runs, so there is a window in which a device is globally visible with `DEVICE_CONTEXT == NULL`. A client enumerating in that window reaches the half-attached device and trips `assert(DEVICE_CONTEXT != NULL)` in `indigo_rotator_enumerate_properties` (`indigo_rotator_driver.c:176`); every class-level enumerate has the same assertion. Reproduced with `test_rotator_lunatico_simulator`, whose port devices are attached from an async handler while the test enumerates: `Assertion failed: (DEVICE_CONTEXT != NULL)`, SIGABRT. Not fixed. Two shapes were considered. Holding the mutex across `attach()` does **not** self-deadlock - `bus_mutex` is `PTHREAD_RECURSIVE_MUTEX_INITIALIZER` and `device_mutex`/`client_mutex` are both `#define`s of it, so a driver's `attach()` re-entering `indigo_define_property()` on the same thread is fine - but it would lock the whole bus while arbitrary driver code runs and invites the cross-thread inversion of DRV-192, where a thread holding `bus_mutex` waits on a device queue whose task blocks on `bus_mutex` in `indigo_update_property()`. It would also break an existing invariant: `indigo_detach_device` deliberately unlocks before calling `device->detach()`. The other shape, publishing the device only after `attach()` returns, takes no lock across driver code and is workable - `indigo_define_property()` walks `clients[]`, not `devices[]`, so a device that is not in the table yet can still define its properties - but the slot has to be reserved so two threads cannot claim it, every `devices[]` iteration (`indigo_enumerate_properties`, property routing, `indigo_detach_device`, `indigo_stop`) has to skip a reserved but unpublished slot, and `indigo_stop` needs a rule for a device that is mid-attach. `indigo_attach_device` is core to every driver, so this needs a deliberate decision rather than a drive-by change. The test now waits on the definitions the new device broadcasts instead of polling enumerate.

Historical location: `indigo_bus.c:552` (`indigo_attach_device`). Status: Open.

### FND-000014 (LINUXDRV-001)

PWM change handlers ignore failed `*_pwm_get()` calls, then compute frequency or duty from `period` initialized to zero. If the PWM sysfs node disappears, permission is denied, or a read fails, this can divide by zero or push nonsensical PWM values while still updating the property as successful. Check the read result, set the changed property to `INDIGO_ALERT_STATE`, and return before doing math or writes.

Historical location: `aux_asiair/indigo_aux_asiair.c:853`, `aux_asiair/indigo_aux_asiair.c:875`, `aux_rpio/indigo_aux_rpio.c:1028`, `aux_rpio/indigo_aux_rpio.c:1050`. Status: Open.

### FND-000015 (LINUXDRV-002)

PWM sysfs reads parse buffers with `atoi()` without adding a terminating NUL after `read()`. sysfs does not append NUL bytes, so parsing can read stale stack data past the bytes returned. Capture the read length, reject full-buffer reads, append `buf[n] = '\0'`, and use `strtol()` with validation.

Historical location: `aux_asiair/indigo_aux_asiair.c:282`, `aux_asiair/indigo_aux_asiair.c:298`, `aux_rpio/indigo_aux_rpio.c:319`, `aux_rpio/indigo_aux_rpio.c:335`. Status: Open.

### FND-000016 (LINUXDRV-003)

GPIO/PWM export and unexport helpers return on the first failure without rolling back or continuing cleanup. A partial connect failure can leave some GPIOs/PWMs exported, and a disconnect can stop after one unexport error while leaving the rest of the pins exported. Track successfully exported resources during connect and perform best-effort cleanup of all resources on failure and disconnect.

Historical location: `aux_asiair/indigo_aux_asiair.c:482`, `aux_asiair/indigo_aux_asiair.c:505`, `aux_rpio/indigo_aux_rpio.c:555`, `aux_rpio/indigo_aux_rpio.c:581`. Status: Open.

### FND-000017 (LINUXDRV-004)

`relay_mutex` and `port_mutex` are initialized but never destroyed before the private data is freed. This is unlikely to affect a single-process normal shutdown, but repeated driver load/unload cycles leak pthread mutex resources and complicate sanitizers. Destroy both mutexes during detach/delete after timers are cancelled and no callbacks can use them.

Historical location: `aux_asiair/indigo_aux_asiair.c:694`, `aux_asiair/indigo_aux_asiair.c:941`, `aux_rpio/indigo_aux_rpio.c:859`, `aux_rpio/indigo_aux_rpio.c:1140`. Status: Open.

### FND-000018 (LINUXDRV-005)

If custom property allocation fails after `indigo_aux_attach()` has already succeeded, `aux_attach()` returns `INDIGO_FAILED` without releasing any properties allocated before the failed allocation and without detaching the base AUX context. Add a single cleanup path that releases partially initialized properties, destroys initialized mutexes, and calls `indigo_aux_detach()`.

Historical location: `aux_asiair/indigo_aux_asiair.c:696`, `aux_rpio/indigo_aux_rpio.c:861`. Status: Open.

### FND-000019 (OPTDRV-001)

Andor initialization allocates `private_data` and `device`, locks `driver_mutex`, then breaks out of the camera loop when `Initialize()` fails without unlocking the mutex or freeing the just-allocated objects. A missing or invalid SDK path can leave the global mutex permanently locked and leak the partially constructed device. Use a cleanup path that unlocks the mutex, frees partial allocations, and continues or aborts cleanly.

Historical location: `ccd_andor/indigo_ccd_andor.c:1492`, `ccd_andor/indigo_ccd_andor.c:1496`, `ccd_andor/indigo_ccd_andor.c:1503`, `ccd_andor/indigo_ccd_andor.c:1534`. Status: Open.

### FND-000020 (OPTDRV-002)

Andor shutdown destroys the static global `driver_mutex` inside the per-device loop after using it for the current device. With multiple cameras, the next iteration locks a destroyed mutex; after one shutdown/init cycle, future SDK operations can also use an invalid mutex. Do not destroy this static process-lifetime mutex per camera.

Historical location: `ccd_andor/indigo_ccd_andor.c:1564`, `ccd_andor/indigo_ccd_andor.c:1567`, `ccd_andor/indigo_ccd_andor.c:1573`. Status: Open.

### FND-000021 (OPTDRV-003)

Raspberry Pi exposure completion can block forever. `exposure_timer_callback()` polls `IsAnyRequestComplete()` with no timeout while `state` never changes, and then `ReadFrame()` waits on `m_cv` with no cancellation/timeout. If libcamera drops a completion, a request was never queued, or the camera is stopped during the wait, the INDIGO timer callback can hang and leave exposure properties busy. Add a bounded wait and fail the exposure with cleanup.

Historical location: `ccd_rpi/indigo_ccd_rpi.cpp:971`, `ccd_rpi/indigo_ccd_rpi.cpp:973`, `ccd_rpi/indigo_ccd_rpi.cpp:980`, `ccd_rpi/indigo_ccd_rpi.cpp:610`. Status: Open.

### FND-000022 (OPTDRV-004)

Raspberry Pi exposure startup sets image and exposure properties to `INDIGO_BUSY_STATE`, but if `StartExposure()` fails it only logs the error. No property is moved to `INDIGO_ALERT_STATE`, no failure cleanup runs, and no client update reports completion/failure. Report the failed start through `CCD_EXPOSURE_PROPERTY` and image upload properties before returning.

Historical location: `ccd_rpi/indigo_ccd_rpi.cpp:1364`, `ccd_rpi/indigo_ccd_rpi.cpp:1372`, `ccd_rpi/indigo_ccd_rpi.cpp:1440`, `ccd_rpi/indigo_ccd_rpi.cpp:1445`. Status: Open.

### FND-000023 (OPTDRV-005)

The Raspberry Pi driver copies unbounded external strings into fixed buffers: `LIBCAMERA_LOG_LEVELS` into `previousSDKLogLevel[64]`, and libcamera camera names into `name[128]` via `strcat()`. Long environment values or camera names can overflow stack/static buffers. Use bounded copies or `std::string` construction followed by `snprintf()` into INDIGO name buffers.

Historical location: `ccd_rpi/indigo_ccd_rpi.cpp:187`, `ccd_rpi/indigo_ccd_rpi.cpp:204`, `ccd_rpi/indigo_ccd_rpi.cpp:1691`, `ccd_rpi/indigo_ccd_rpi.cpp:1692`. Status: Open.

### FND-000024 (OPTDRV-006)

`mmap()` results are stored without checking for `MAP_FAILED`. A failed mapping is later read as image memory in `ReadFrame()` and later passed to `munmap()`, which can crash or corrupt exposure output under memory pressure or invalid buffer FDs. Check each mapping, unwind any mappings made for the request, and fail `StartCapture()` cleanly.

Historical location: `ccd_rpi/indigo_ccd_rpi.cpp:787`, `ccd_rpi/indigo_ccd_rpi.cpp:788`, `ccd_rpi/indigo_ccd_rpi.cpp:624`, `ccd_rpi/indigo_ccd_rpi.cpp:627`. Status: Open.

### FND-000025 (OPTDRV-007)

The Raspberry Pi connection path appends mode items for every supported format/size without checking the allocated capacity of `CCD_MODE_PROPERTY` or resizing it. Cameras that report many formats or sizes can write past the base CCD mode property item array. Bound the count or resize the property before adding items.

Historical location: `ccd_rpi/indigo_ccd_rpi.cpp:1129`, `ccd_rpi/indigo_ccd_rpi.cpp:1146`, `ccd_rpi/indigo_ccd_rpi.cpp:1165`, `ccd_rpi/indigo_ccd_rpi.cpp:1184`. Status: Open.

### FND-000026 (OPTDRV-008)

`X_CCD_ADVANCED_PROPERTY` is deleted on disconnect but not released or nulled, then reconnect defines the stale pointer before overwriting it with a newly allocated property. Detach only releases it while connected. This leaks the advanced property on disconnect/reconnect and leaves stale property state reachable. Release and null the property on disconnect, and allocate before defining it on connect.

Historical location: `ccd_rpi/indigo_ccd_rpi.cpp:1212`, `ccd_rpi/indigo_ccd_rpi.cpp:1216`, `ccd_rpi/indigo_ccd_rpi.cpp:1269`, `ccd_rpi/indigo_ccd_rpi.cpp:1624`. Status: Open.

### FND-000027 (OPTDRV-009)

The focuser device name is initialized, but the uniqueness call is accidentally applied to `device->name` again instead of `focuser->name`. Multiple RPi autofocus devices can therefore expose duplicate focuser names. Call `indigo_make_name_unique(focuser->name, ...)` for the focuser.

Historical location: `ccd_rpi/indigo_ccd_rpi.cpp:1708`, `ccd_rpi/indigo_ccd_rpi.cpp:1710`, `ccd_rpi/indigo_ccd_rpi.cpp:1711`. Status: Open.

### FND-000028 (TEST-009)

The rule exists but the executable is not listed in `INTEGRATION_TESTS`, so `make test` never runs it while `MIGRATION_STATUS.md` advertised 11 cases for the driver. It currently passes 7 of 11; the four failures are the port-reconfiguration cases tracked as `DRV-195`.

Historical location: `Makefile` (`test_rotator_lunatico_simulator`). Status: Open (unblocked 2026-09-19: `DRV-195` is fixed and the binary now passes 11 of 11, so the only remaining step is adding it to `INTEGRATION_TESTS`; still absent from that list).

### FND-000029 (TEST-010)

`cooler_individual_failures` and `Guiding and abort during long exposure then reacquire` fail intermittently, and only under full-suite load: 11 standalone runs of the binary were clean while the two full `make test` runs hit one each. `wait_state()` samples the property's current state every 10 ms, so a state the driver publishes and then replaces between two polls is missed; the serial harness avoids this by recording a revision per state (`property_state_revisions` in `simulator_test_common.h`). Making `wait_state()` edge triggered the same way was tried and made `cooler_individual_failures` fail more often (2 of 3 runs), so the scenario depends on the level-triggered semantics in some other way and the change was reverted. Needs diagnosis before the harness is changed.

Historical location: `integration/test_ccd_playerone_sdk.c` (`wait_state`). Status: Open (intermittent; blocks a green `make -C indigo_test test`).

### FND-000030 (TESTS-001)

The GPS script labels `UTC_TIME`, `GPS_STATUS`, and `GPS_ADVANCED` as optional, but tests them with `test_property_exists()`, which records failures when absent. The script also prints the summary without exiting based on `TESTS_FAILED`, so failed GPS compliance can still return success. Treat optional properties as skips when absent and add the same final `if [ $TESTS_FAILED -eq 0 ]; then exit 0; else exit 1; fi` block used by the other scripts.

Historical location: `gps_compliance.sh:67`, `gps_compliance.sh:79`, `gps_compliance.sh:85`, `gps_compliance.sh:189`. Status: Open.

### FND-000031 (TESTS-002)

Framework commands expand `$INDIGO_PROP_TOOL` and `$REMOTE_SERVER` unquoted and usually pipe output through `grep` without `pipefail`. Tool paths containing spaces break, remote arguments cannot be represented safely as an argv array, and command failures can be hidden when `grep` matches text from stderr. Store remote options in an array, invoke `"${INDIGO_PROP_TOOL}"`, and capture command status before filtering output.

Historical location: `indigo_test_framework.sh:104`, `indigo_test_framework.sh:170`, `indigo_test_framework.sh:239`, `indigo_test_framework.sh:396`, `indigo_test_framework.sh:479`, `indigo_test_framework.sh:507`, `indigo_test_framework.sh:576`, `indigo_test_framework.sh:726`, `indigo_test_framework.sh:754`, `indigo_test_framework.sh:790`. Status: Open.

### FND-000032 (TESTS-003)

Most compliance scripts save `WAS_CONNECTED` from `test_connection_battery()` and then ignore it, always disconnecting the target device at the end. Manual runs against already-connected hardware will leave devices disconnected and may disrupt an active setup. Restore the original connection state like `gps_compliance.sh` attempts to do, and fix the framework comment at `indigo_test_framework.sh:527` because the function returns original state, not final state.

Historical location: `ao_compliance.sh:53`, `ao_compliance.sh:133`, `filter_wheel_compliance.sh:70`, `filter_wheel_compliance.sh:196`, `focuser_compliance.sh:53`, `focuser_compliance.sh:377`, `guider_compliance.sh:53`, `guider_compliance.sh:207`, `rotator_compliance.sh:53`, `rotator_compliance.sh:438`. Status: Open.

### FND-000033 (TESTS-004)

Abort-motion tests assume a long-running move is still `BUSY` after a fixed two-second sleep. Fast devices can finish before abort is sent and fail the required `BUSY -> OK` abort transition even though the driver is valid; slow devices can also leave a background `indigo_prop_tool` process running in the rotator test. Use a generated long move known to remain busy, wait until the motion property reports `BUSY`, send abort immediately, and wait/reap any background helper process.

Historical location: `focuser_compliance.sh:199`, `focuser_compliance.sh:201`, `focuser_compliance.sh:206`, `rotator_compliance.sh:231`, `rotator_compliance.sh:233`, `rotator_compliance.sh:238`. Status: Open.

### FND-000034 (TESTS-005)

Property and range parsing is based on unanchored regular expressions over human-readable `indigo_prop_tool` output. Item checks can match substrings from unrelated items, and range extraction takes the first bracketed expression from all output instead of the requested item line. Parse exact item names from structured tool output if available, or filter to the exact property item with fixed-string matching before extracting state, values, and ranges.

Historical location: `indigo_test_framework.sh:720`, `indigo_test_framework.sh:726`, `indigo_test_framework.sh:737`, `indigo_test_framework.sh:765`, `indigo_test_framework.sh:801`. Status: Open.

### FND-000035 (TESTS-006)

Framework documentation is out of sync with the implementation: `print_test_summary()` now counts skipped tests, but the early example and prose still describe only passed/failed counts. Update the documentation so manual test operators understand skipped optional properties and do not mistake skips for missing coverage.

Historical location: `INDIGO_TEST_FRAMEWORK.md:47`, `INDIGO_TEST_FRAMEWORK.md:592`, `indigo_test_framework.sh:71`, `indigo_test_framework.sh:72`, `indigo_test_framework.sh:75`. Status: Open.

### FND-000036 (TOOLS-001)

The property parsers build `sscanf()` formats with field widths equal to destination buffer sizes, which still leaves room for a terminating NUL beyond the array. The remote-server parser also uses `%[^:]:%s` with no widths for `hostname[255]` and `port_str[100]`. Exact-size or long command-line arguments can overflow stack/global buffers before validation. Use bounded widths of `sizeof(buffer) - 1`, check `item_count < MAX_ITEMS` before appending parsed items, and reject overlong fields.

Historical location: `indigo_prop_tool.c:181`, `indigo_prop_tool.c:202`, `indigo_prop_tool.c:214`, `indigo_prop_tool.c:224`, `indigo_prop_tool.c:251`, `indigo_prop_tool.c:262`, `indigo_prop_tool.c:937`. Status: Open.

### FND-000037 (TOOLS-002)

`indigo_raw_crop` trusts the RAW header and ROI dimensions. It does not validate the header read, RAW signature, positive crop width/height, multiplication overflow, allocation success, or payload read size. A truncated or crafted RAW file, or a crop like `0,0,-1,10`, can drive undersized allocation and out-of-bounds reads/writes. Validate the header and type, compute byte counts with overflow-checked `size_t`, require `width > 0 && height > 0`, and fail if reads or allocations are incomplete.

Historical location: `indigo_raw_crop.c:41`, `indigo_raw_crop.c:44`, `indigo_raw_crop.c:45`, `indigo_raw_crop.c:68`, `indigo_raw_crop.c:76`. Status: Open.

### FND-000038 (TOOLS-003)

`fits_to_raw` does not check CFITSIO status after reading image parameters or pixels, accepts any non-8-bit FITS image as 16-bit output, and allocates image buffers from unchecked axis multiplication. Bad or unsupported FITS inputs can produce corrupt RAW output or memory faults. Check `status` after every FITS call, explicitly accept only supported `bitpix` values, validate `naxis/naxes`, and verify allocation/write results.

Historical location: `fits_to_raw.c:49`, `fits_to_raw.c:65`, `fits_to_raw.c:69`, `fits_to_raw.c:70`, `fits_to_raw.c:77`, `fits_to_raw.c:80`. Status: Open.

### FND-000039 (TOOLS-004)

The RAW-to-FITS utility has fragile file handling. `write()` is treated as a one-shot full write, input allocation/reallocation is unchecked, `fseek()`/`ftell()` and `fread()` failures are ignored, and each glob pattern is evaluated repeatedly in separate `glob()` calls before `globfree()`. This can mis-handle partial I/O, leave uninitialized data in conversions, or leak glob state. Use checked `size_t` file sizes, loop until all output bytes are written, call `glob()` once per pattern, and check all I/O results.

Historical location: `indigo_raw_to_fits.c:46`, `indigo_raw_to_fits.c:64`, `indigo_raw_to_fits.c:68`, `indigo_raw_to_fits.c:70`, `indigo_raw_to_fits.c:72`, `indigo_raw_to_fits.c:160`, `indigo_raw_to_fits.c:161`, `indigo_raw_to_fits.c:165`. Status: Open.

### FND-000040 (TOOLS-005)

Metadata tools copy paths into fixed buffers or pass paths through a shell command unsafely. `indigo_driver_metadata` uses `strcpy()` from arbitrary CLI arguments into `name[128]`; `indigo_metadata_extractor` builds `clang -E %s` for `popen()` without quoting and then dereferences `pipe` without checking for failure. Long paths can overflow, and specially named driver paths can change shell behavior. Use `snprintf()`/length checks and fork/exec-style invocation or strict shell quoting plus `popen()` failure checks.

Historical location: `indigo_driver_metadata.c:15`, `indigo_driver_metadata.c:19`, `indigo_metadata_extractor.c:83`, `indigo_metadata_extractor.c:84`, `indigo_metadata_extractor.c:85`. Status: Open.

### FND-000041 (TOOLS-006)

The generator assumes allocations, reallocations, file redirects, and path copies always succeed. `allocate()` immediately `memset()`s a possibly NULL allocation, `realloc()` results overwrite the only pointer, `freopen()` results are ignored, and `strcpy(source_file, definition_file)` can overflow `PATH_MAX`. Failures can crash the generator, read from the wrong stream, or emit partial files. Add checked allocation helpers, temporary pointers for `realloc()`, bounded path copies, and explicit `freopen()` error handling before generating output.

Historical location: `indigo_generator.c:123`, `indigo_generator.c:1035`, `indigo_generator.c:2152`, `indigo_generator.c:2162`, `indigo_generator.c:2724`, `indigo_generator.c:2737`, `indigo_generator.c:2741`, `indigo_generator.c:2745`, `indigo_generator.c:2751`, `indigo_generator.c:2765`, `indigo_generator.c:2766`. Status: Open.

### FND-000042 (TOOLS-007)

The repair tools read whole files and then inspect or rewrite fixed offsets without first proving the file is large enough, and they do not check write results. A small input can make `fix_fits` read past the loaded buffer at offset 5760, while `fix_xisf` can rewrite from `data + 2880` even when the file is shorter. Validate minimum sizes before fixed-offset access and check every `fwrite()`/`fputc()` result before reporting success.

Historical location: `fix_fits.c:58`, `fix_fits.c:63`, `fix_fits.c:74`, `fix_xisf.c:67`, `fix_xisf.c:75`, `fix_xisf.c:90`, `fix_xisf.c:97`. Status: Open.

### FND-000043 (TOOLS-008)

The tools build target produces `indigo_drivers`, `indigo_driver_metadata`, and `indigo_scan_drivers`, but `install`, `uninstall`, and `clean` omit some of those binaries. This can leave stale build artifacts and makes packaging behavior differ from `all`. Decide which helper tools are intentionally private; then either exclude them from `all` or handle them consistently in install/uninstall/clean rules.

Historical location: `indigo_tools/Makefile:33`, `indigo_tools/Makefile:35`, `indigo_tools/Makefile:42`, `indigo_tools/Makefile:48`. Status: Open.
