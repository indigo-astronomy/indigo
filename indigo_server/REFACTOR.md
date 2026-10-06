# indigo_server refactoring notes

## Driver load and unload hang (2026-10-06)

A user reported that loading and unloading a large set of drivers sometimes hung the server.

### Test

`indigo_test/integration/test_server_driver_stress.c`, run with `make -C indigo_test test-server-driver-stress`, launches `build/bin/indigo_server` on a free loopback port with a private `HOME` and drives `Server.DRIVERS` as an ordinary client. Every request is sent as soon as the previous one was answered, as the Configuration Agent does when it loads a profile. After every request the server must answer within the timeout, still run, and answer a full enumeration, which calls into every attached device and so catches a device left attached after its library was unloaded. A request without an answer is reported as a hang and the stacks of all server threads are saved next to the server log.

| Case | What it does |
|---|---|
| `each_driver_loads_and_unloads_alone` | Every driver loaded and unloaded on its own, twice, so a defect is attributed to one driver. The server is restarted after a failure and the remaining drivers are still tried. |
| `all_drivers_load_then_unload` | The whole set switched on and then off, one request each, five cycles. |
| `random_driver_sets_load_and_unload` | Random subsets, every request loads some drivers and unloads others, 30 requests; `INDIGO_STRESS_SEED` repeats a sequence. |

The test is opt-in, it launches the server and opens loopback sockets. `INDIGO_STRESS_DRIVERS` and `INDIGO_STRESS_EXCLUDE` (regular expressions), `INDIGO_STRESS_TIMEOUT`, `INDIGO_STRESS_CYCLES`, `INDIGO_STRESS_ITERATIONS`, `INDIGO_STRESS_SEED` and `INDIGO_STRESS_VERBOSE` control it. It needs the dynamic drivers in `build/drivers`, no hardware.

### Defect: deadlock between driver unload and the bus mutex

`indigo_change_property()` holds the bus mutex while it calls the server's `change_property()`, and the server loaded and unloaded drivers right there. A driver shutdown detaches its devices, and `indigo_device_detach()` waits for the running task of the device queue (`indigo_queue_delete()`), for running timers and for the device's background handlers. A handler that publishes waits for the bus mutex, so whenever one was running the unload never finished. Stacks of the hung server:

```
Server worker:    indigo_change_property -> change_property (indigo_server.c) -> indigo_remove_driver
                  -> indigo_focuser_askar SHUTDOWN -> indigo_detach_device -> indigo_device_detach -> indigo_queue_delete
Queue Askar-WAF:  scan_ports_finalizer -> indigo_define_property -> waiting for the bus mutex
```

Large sets made it likely: handlers of drivers loaded by one request start while the request still holds the bus mutex, and the next request, sent at once, can win the mutex ahead of them and unload one of those drivers. `indigo_focuser_askar` hung even alone, because its attach starts a UDP discovery of several seconds on its queue. The Imager Agent and ASCOM Alpaca client queues were seen waiting for the mutex in the same way.

### Fix

`DRIVERS`, `LOAD` and `UNLOAD` requests are copied and queued on the server's own queue ("Server drivers"); `change_property()` publishes the property `BUSY` and returns, so no driver is loaded or unloaded with the bus mutex held. Requests are applied one after another in the order they arrived. `DRIVERS` stays `BUSY` until the last queued request is applied, so `OK` tells a client that everything it asked for is done. The queue is deleted before the server unloads its drivers at shutdown. Drivers restored from the configuration at start-up are loaded on the queue as well, instead of the main thread.

`indigo_agent_config` (3.0.0.28 to 3.0.0.29) waited at most 10 s for the server to apply a driver selection, which was enough while the server applied it inside the change request. It now keeps waiting while `DRIVERS` is `BUSY`, at most 300 s; unloading about 125 drivers takes about 15 s.

### Drivers that crashed the server on unload or reload

The stress test also found drivers that crash the whole server process when they are unloaded and loaded again. Their fixes are recorded in the drivers' own notes where they have them.

| Driver | Defect | Fix |
|---|---|---|
| `indigo_agent_solver` 3.0.0.1 to 3.0.0.2 | `INDIGO_DRIVER_INIT` never set `last_action`, so `INDIGO_DRIVER_SHUTDOWN` returned at once and the Solver Agent device stayed attached after its library was unloaded; the next enumeration jumped into unmapped code (SIGSEGV). | `INIT` sets `last_action`. |
| `indigo_ccd_atik` 3.0.0.49 to 3.0.0.50 | The pinned SDK's USB detector thread calls the driver's debug callback after the driver library was unloaded (SIGSEGV or SIGILL in whichever library was mapped there since). | The driver library is pinned as well, see `indigo_drivers/ccd_atik/REFACTOR.md`. |
| `indigo_ccd_qhy` 3.0.0.41 to 3.0.0.42 | An SDK thread that `ReleaseQHYCCDResource()` does not join outlived the unload (SIGSEGV in `pthread_mutex_lock` from `ReadImageInDDR_Titan`). | The library with the SDK is pinned, see `indigo_drivers/ccd_qhy/REFACTOR.md`. |
| `indigo_ccd_qhy2` 3.0.0.45 to 3.0.0.46 | The unload unmapped `libqhyccd.dylib` under an unjoined SDK thread, which crashed when it woke up. | The SDK library is pinned, see `indigo_drivers/ccd_qhy2/REFACTOR.md`. |

### Results (macOS arm64, 2026-10-06)

| Build | Drivers | Alone | All on / all off | Random |
|---|---|---|---|---|
| Before the fixes | 144 | failed: `focuser_askar` hang, `agent_solver` crash, `ccd_svb2` crash | failed: hang unloading `focuser_askar` | failed: crash in `ccd_bresser` (seed 1) |
| Server queue | 128, crashing drivers excluded | passed, Askar unload 3.0 s | passed, longest 15.4 s | passed (seed 1) |
| + `agent_solver` | 129 | passed | passed | passed (seed 1) |
| + `ccd_atik` | 131 | passed | passed | passed (seed 1) |
| + `ccd_qhy` | 132 | passed | passed | passed (seed 1) |
| + `ccd_qhy2` | 133, touptek family excluded | passed | passed | passed (seeds 1 to 4, 60 sets each for 2 to 4) |
| All fixes, nothing excluded | 144 | passed, 576 requests | failed: SIGTRAP in `ccd_baccam` | failed: SIGTRAP in `ccd_bresser` (seed 1, set 3) |

The excluded touptek family is `ccd_touptek`, `ccd_altair`, `ccd_baccam`, `ccd_bresser`, `ccd_mallin`, `ccd_meade`, `ccd_ogma`, `ccd_omegonpro`, `ccd_rising`, `ccd_ssg` and `ccd_svb2`. `ccd_qsi`, `ccd_sbig` and `guider_asi` refuse to unload without a device and stay loaded, which the test allows. Other suites run with the fixes: `test_agent_config` 56/56, Atik fake SDK 44/44, QHY fake SDK 39/39, QHY2 fake SDK 42/42. Linux and Windows were not run.

### Open

- **Touptek-family SDKs.** After a reload, `<Vendor>_EnumV2()` called from `process_plug_event_handler` / `process_unplug_event_handler` hands `IOHIDManager` a run loop that no longer exists and the process stops with SIGTRAP in `CFRunLoopAddSource` (seen with Bresser, SVBONY and BacCam, the others share the source). `INIT` registers the libusb hot-plug callback with `LIBUSB_HOTPLUG_ENUMERATE`, so every USB device present queues `process_plug_event_handler` 0.5 s later on the driver's own queue, and its first SDK call is `EnumV2()`. Single-driver sequences against a fresh server, with every image the loader maps and unmaps logged, give the same result for all three drivers:

  | Sequence | Result | SDK library |
  |---|---|---|
  | load, stay 2 s, unload, load again and stay | SIGTRAP about 0.5 s after the reload | stays mapped after the unload |
  | load, unload at once, load again and stay | passes | unmapped at the unload, mapped again |
  | load, stay 2 s, unload, stay unloaded 5 s | passes | stays mapped |

  The crash needs an `EnumV2()` call in an earlier load of the driver, an unload, and a reload that lives long enough to enumerate again. After the first enumeration the SDK library is no longer unloaded, so its state survives the driver, including the HID manager it scheduled on the run loop of the thread that called it, the driver's queue thread, which `INDIGO_DRIVER_SHUTDOWN` ends with `indigo_queue_delete()`. The reloaded driver enumerates on a new queue thread and the SDK schedules the HID devices on the run loop of the thread that is gone. Pinning does not help, the library already stays mapped; the SDK needs a thread that lives as long as the process, for example a driver queue that is never deleted, with the driver library pinned so that the reloaded driver finds it. This explains the earlier timing dependence: `each_driver_loads_and_unloads_alone` passes when the unload of the first cycle comes before the 0.5 s enumeration and crashes when it comes after it. Observed on macOS arm64 with no camera attached; the HID devices the SDK matched were not identified.
- **Client-side driver loading is not fixed.** Only the server got a queue. `indigo_load_driver()`, `indigo_add_driver()` and `indigo_remove_driver()` in `indigo_client.c`, and a driver entry point called directly, still run in the caller's thread, and the client library has no queue to move them to. An application that hosts drivers itself (serverless operation) and loads or unloads them from a bus callback, i.e. its own device's `change_property()` or `enumerate_properties()`, or a client's `define_property`, `update_property`, `delete_property` or `send_message`, all of which run with the bus mutex held, deadlocks exactly as the server did. Until the client library gets a queue such an application has to load and unload drivers from a thread that does not hold the bus mutex, for example its main thread or a queue of its own. The stress test covers only `indigo_server`.
- `ASCOM Alpaca Client` refuses to shut down for about 5 s while a discovery is in progress and then stays loaded; every device detach takes about 100 ms, which makes unloading the whole set take about 15 s. Neither blocks the server any more.
