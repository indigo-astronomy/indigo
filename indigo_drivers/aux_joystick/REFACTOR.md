# HID Joystick Driver Refactoring

## Baseline and current-state audit

- Baseline revision: `c9a2124051eb9b96e56225953bf2a0c513e4425e` plus the pre-existing working-tree changes in `indigo_aux_joystick.c`.
- Baseline environment: macOS 26.6.2, arm64.
- Baseline build: `make -C indigo_drivers/aux_joystick all` passed on 2026-09-14.
- Existing automated driver tests: none. `MIGRATION_STATUS.md` records `0 / 0`.
- Architecture: one dynamically attached AUX joystick device per discovered controller. macOS uses bundled DDHidLib callbacks; Linux scans `/dev/input/js*` and reads `struct js_event` records on an ad-hoc worker thread. A match-any libusb callback triggers rescans.
- Public behavior: common device properties before connection; after connection the driver defines `JOYSTICK_BUTTONS`, `JOYSTICK_AXES`, `JOYSTICK_MAPPING`, `JOYSTICK_OPTIONS`, and the mount park, home, slew-rate, RA/DEC motion, tracking, and abort proxy properties. Mapping and options are configurable and persistent through `CONFIG.SAVE`.
- Supported capabilities: axis, button and macOS POV input; axis/button-to-mount-property mapping; hot-plug discovery and removal; connect, disconnect and reconnect.
- Documentation and implementation sources audited: repository and driver instructions, `README.md`, `indigo_docs/DEVELOPMENT.md`, the queue/lifecycle sections of `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`, `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md`, both platform backends and bundled DDHidLib headers.
- Platforms: production implementations exist for macOS/DDHidLib and Linux joystick API. No Windows backend exists. The current environment can execute macOS production builds and a deterministic fake Linux joystick boundary, but not a real Linux kernel joystick device.
- Build/package integration: hand-written `.c` included by the macOS `.m` wrapper, driver-local Makefile, and Xcode group. There is no `.driver` generator input.
- I/O decision: retain DDHidLib and Linux `open`/`read`/`ioctl`; these HID/event APIs are not serial or socket transports and cannot be represented by `indigo_uni_io`.
- Synchronization audit: connection changes run synchronously on the bus callback; Linux starts an unowned `indigo_async` poller and frees devices without joining it; macOS dispatches device events and destruction concurrently; the libusb callback sleeps and rescans on the shared USB event thread; registry locks cover bus calls and property updates.

## Hardware-test decision

The automated suite uses a deterministic fake Linux joystick/HID boundary while preserving the real INDIGO bus, device queues, driver queue and worker thread. A user-provided Bluetooth pad and Logitech Dual Action USB controller are available on macOS. Together they cover physical DDHid discovery, connection, digital D-pad/POV input, analog axes, buttons and both Bluetooth and USB unplug/replug acceptance. Linux kernel event delivery and Windows remain platform gaps.

## Found defects

| ID | Evidence | Observable impact and root cause | Planned fix | Regression coverage |
| --- | --- | --- | --- | --- |
| AUX-JOY-001 | Source audit | `CONNECTION` performs lifecycle work directly in `change_property`, blocking the bus thread and providing no serialized transition state. | Publish BUSY and execute a connection handler on the dedicated driver queue. | BUSY-to-OK connect/disconnect, queued ordering and reconnect cases. |
| AUX-JOY-002 | Source audit | The Linux poller is launched without ownership or join. Disconnect/reconnect or unplug can leave a worker using a closed, reused or freed device/fd. | Store thread ownership and stop state; close/wake and join before reuse or free. | Disconnect during held read, unplug while connected, rapid reconnect and no-calls-after-close cases. |
| AUX-JOY-003 | Source audit | macOS event blocks and destruction use a global concurrent queue; listener stop is asynchronous and shutdown returns before release completes. | Use a per-wrapper serial event queue; stop listening and drain queued events before detach/free. | Architecture/source checks plus the shared fake event/removal ordering cases; physical DDHidLib execution remains unavailable. |
| AUX-JOY-004 | Source audit | The libusb callback sleeps 500 ms and performs full attach/detach work on the USB event thread. | Queue a delayed, coalesced rescan on a dedicated driver queue and drain it during shutdown. | Non-blocking callback, duplicate-event coalescing, plug/unplug and queued-shutdown cases. |
| AUX-JOY-005 | Source audit | Linux accepts any filename parsed by `sscanf("js%d")` and writes `found[index]` without range validation. | Require an exact `jsN` filename and `0 <= N < MAX_DEVICES`. | Out-of-range and suffixed-name discovery cases. |
| AUX-JOY-006 | Source audit | Axis, POV and button callbacks index property items without checking the actual property count. | Reject invalid event indices before accessing property items. | Invalid axis/button events leave properties unchanged and do not crash under ASan/UBSan. |
| AUX-JOY-007 | Source audit | The Linux `void *` poll routine reaches the end without returning a value. | Return `NULL`. | Strict warnings build. |
| AUX-JOY-008 | Source audit | Reported buttons, physical axes and POV-derived axes could exceed the driver's fixed internal capacities. | Clamp non-negative button counts and the combined physical/POV axis count before allocating properties. | Oversized fake descriptor counts produce exactly 64 buttons and 16 axes. |
| AUX-JOY-009 | Source audit | macOS rescan removed an existing wrapper from the same mutable array currently being fast-enumerated. | Match against a separate mutable copy and remove the match only after the inner iteration completes. | Strict Objective-C compile plus shared multi-device attach/removal lifecycle coverage; physical DDHid enumeration remains unavailable. |
| AUX-JOY-010 | Physical Bluetooth unplug | macOS removed the BT HID device, but INDIGO continued to publish it as Connected because Bluetooth changes do not trigger the libusb callback. | Schedule a recurring delayed macOS rescan on the serialized driver queue and remove/drain it during shutdown. | With the fixed driver, the pad disappeared after connected BT unplug and reappeared after reconnect; the server remained stable. |

## Atomic plan

1. **Complete — baseline and audit.** The existing macOS arm64 driver built successfully; no driver-specific test executable existed. Hardware-free and physical test counts were both zero.
2. **Complete — add deterministic fake HID integration harness.** Added `test_aux_joystick_hid.c` plus minimal fake Linux compatibility headers. The real Linux driver source is compiled against fake filesystem, joystick-event and libusb discovery calls while the real INDIGO bus, device queues, driver queue and pthread poller remain active. The focused normal and ASan/UBSan executions both pass 14/14 cases, and every new persistent file is registered in Xcode.
3. **Complete — queue connection and hot-plug lifecycle.** `CONNECTION` now publishes BUSY and queues its handler on the driver queue. Hot-plug callbacks return immediately and coalesce a 0.5-second delayed rescan. macOS also schedules a one-second recurring queue rescan because Bluetooth devices do not emit libusb hot-plug events. Shutdown stops the producer, removes pending rescans, drains and deletes the queue. The fake verifies BUSY publication, callback latency, coalescing, rejected connected shutdown and reinitialization; physical BT unplug/replug verifies the macOS fallback.
4. **Complete — make backend teardown safe.** Linux opens transactionally before starting a stored pthread, stops it atomically and joins it before reconnect or free. HID events are serialized with connection and hot-plug lifecycle work through the dedicated driver queue without heap-owned task payloads. macOS listener start/stop is synchronous on the main queue, callbacks feed the same driver queue, and removal stops listening before synchronous detach/free. The production universal macOS build passes; the fake disconnect/reconnect and connected-unplug cases pass under ASan/UBSan.
5. **Complete — validate inputs and recovery.** Discovery now accepts only exact, in-range `jsN` entries, validates descriptor ioctls and continues after a failed probe. Reported counts and incoming events are bounded, open/fcntl failures roll back, and the poller returns `NULL`. Corresponding fake failure, invalid-discovery, invalid-event and recovery cases pass.
6. **Complete — final integration and documentation.** The focused normal and ASan/UBSan suites, strict source builds, universal production build and physical Bluetooth acceptance pass. Xcode registration, migration status, review findings and driver version `0x0300000A` are synchronized; the final diff passed whitespace and project-file checks and test artifacts were cleaned.

## Scenario-to-test mapping

| Scenario | Test |
| --- | --- |
| Metadata, AUX interface, disconnected and connected property sets | `joystick_exposes_complete_properties` |
| Oversized device-reported button/axis counts | `reported_counts_are_bounded` |
| Non-blocking queued connection and BUSY-to-OK transition | `connection_is_queued_and_reports_busy` |
| A second connection request while BUSY cannot replace the active request | `connection_busy_guard_preserves_the_active_request` |
| Open and nonblocking-configuration failure rollback and recovery | `open_and_configuration_failures_recover` |
| Axis/button delivery, public properties and mapped mount output | `axis_and_button_events_drive_public_properties` |
| Mapping conflict validation, options and RA direction swap | `mapping_validation_and_axis_swap_work` |
| Out-of-range axis/button events | `invalid_events_are_ignored` |
| Disconnect joins the old poller and reconnect starts a clean one | `disconnect_reconnect_stops_the_old_poller` |
| Hot-plug callback latency and duplicate-event coalescing | `hotplug_is_nonblocking_and_coalesced` |
| Invalid/suffixed/out-of-range discovery entries and ioctl failure recovery | `invalid_discovery_entries_and_descriptor_failure_recover` |
| Maximum five-device attach, one-device removal and survivor operation | `multiple_devices_attach_remove_and_survive` |
| Connected unplug closes and joins before detach/free | `unplug_connected_device_is_quiescent` |
| Connected shutdown rejection, pending-rescan drain and reinitialization | `shutdown_rejects_connected_device_and_drains_hotplug` |

Real DDHid listener delivery was physically exercised with a Bluetooth digital pad and a Logitech Dual Action USB controller. The Logitech supplied four smooth analog axes plus two discrete D-pad/POV axes, so the macOS axis, POV and button paths are physically covered. Linux kernel delivery and Windows remain unverified.

## Final test summary

- Simulated/fake-HID tests run: 14; passed: 14.
- Physical-hardware tests run: 2; passed: 2.
- `make -C indigo_test test-aux-joystick-hid`: 14/14 passed.
- `make -C indigo_test test-aux-joystick-hid-asan`: 14/14 passed with ASan/UBSan and leak detection disabled because the shared INDIGO process keeps global allocations.
- Strict arm64 macOS Objective-C driver compile: passed with `-Wall -Wextra -Werror -Wno-unused-parameter`.
- Strict arm64 fake-Linux driver and harness compile: passed with the same warning policy; `-Wno-unused-function` was added only for unused static helpers from the shared test header.
- `make -C indigo_drivers/aux_joystick clean all`: universal macOS archive, dylib and executable passed; existing warnings originate only in bundled DDHidLib.
- LLVM source coverage for the production fake-Linux/common path: 20/21 functions (95.24%), 494/685 lines (72.12%), 1,129/1,611 regions (70.08%) and 199/401 branches (49.63%).
- Physical macOS Bluetooth pad: DDHid discovery, version `3.0.0.10`, 8 published axes, 14 buttons, D-pad full-range axis events, multiple button ON/OFF events, queued disconnect/reconnect and connected unplug/replug passed. The initial unplug failed before AUX-JOY-010; the repeated cycle passed after the periodic queue rescan fix.
- Physical Logitech Dual Action USB controller: hot-plug discovery, 6 axes, 12 buttons and queued connection passed. Axes 1–4 delivered smooth intermediate values and both `-65536/65536` endpoints; D-pad/POV axes 5–6 delivered discrete endpoints and neutral zero. Multiple buttons delivered ON/OFF transitions. Connected unplug removed the device without destabilizing the server; replug preserved identity/version and reconnect/disconnect passed.
