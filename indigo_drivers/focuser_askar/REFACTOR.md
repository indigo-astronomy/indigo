# Askar-WAF generated migration

Status: complete, 2026-09-12. Baseline version 0x03000003. Generated version 0x03000004. No generator implementation changes.

## References and decisions

Read repository, generator and automated-test rules; `README.md`; `Commands_Focuser_CDC_EN.md`; original `indigo_focuser_askar.c`; the host PTY simulator; and the existing one-scenario simulator test. The CDC protocol uses `F...#` frames, `#\r\n` replies, logical positions from 0 to device max travel, `FP` absolute moves, `FY` sync, `FS` stop, `Fm/FM` max travel, `Fb/FB` backlash, `Fr/FR` reverse motion and `Fo/FO` motor mode. Unsupported temperature, compensation, automatic mode and speed properties stay hidden.

The driver keeps both USB CDC serial ports and existing `askar://` TCP/Wi-Fi connection support. UDP broadcast discovery is preserved as existing platform-specific code inside the generated driver's custom block because there is no generator or shared INDIGO discovery helper for this device-specific protocol. Hardware-free automated validation uses the serial CDC path; Wi-Fi broadcast/TCP electrical/network behavior remains hardware/network acceptance.

## Atomic plan

1. Inventory original property surface, connection flow, protocol helpers, async movement and simulator behavior. Record migration boundaries in this file.
2. Create `indigo_focuser_askar.driver` as the source of truth. Preserve single focuser device, additional instances, visible serial port and baud-rate properties, Askar-specific `X_FOCUSER_MOTOR_MODE`, limits/backlash/reverse controls, and hidden unsupported base properties.
3. Regenerate checked-in `indigo_focuser_askar.c`, `.h` and `_main.c`. Replace ad hoc timer/callback boilerplate with generator-owned lifecycle, explicit movement finalizer, transactional open rollback and disconnect/abort cleanup.
4. Extend the PTY simulator with test-only event/fault hooks while preserving its protocol model and manual operation.
5. Expand `test_focuser_askar_simulator.c` from smoke coverage to named scenarios for protocol commands, capability/property inventory, absolute/relative moves, SYNC, limits clipping/rejection, abort, command failures, polling recovery and reconnect.
6. Synchronize Xcode project, `indigo_docs/PROPERTIES.md`, `indigo_test/CHANGES.md` and `MIGRATION_STATUS.md`. Run generation/build plus the Askar simulator suite outside the sandbox before commit.

## Coverage boundaries

Simulator validation is sufficient for this migration because no hardware is available. It does not prove actual USB CDC firmware timing, saved NVS persistence across power cycles, Wi-Fi UDP broadcast discovery, TCP reconnect behavior or physical motor reversal. The simulator covers protocol command mapping and driver-owned property state transitions, not generic INDIGO numeric validation or configuration storage.

## Current results

Step 1 is complete. The original driver exposed one focuser, visible `DEVICE_PORT`, `DEVICE_PORTS` and baud-rate controls, hidden unsupported focuser base properties, asynchronous position/step motion with polling, abort by `FS#`, and Askar-specific backlash, reverse and motor-mode settings. The protocol document and simulator agree on the core CDC command set; Wi-Fi discovery exists only in the driver/README path.

Step 2 is complete. `indigo_focuser_askar.driver` now owns the driver source, keeps the public device name and additional-instance support, raises the version to 0x03000004, preserves the custom `X_FOCUSER_MOTOR_MODE` property and maps all visible inherited focuser properties to device commands.

Step 3 is complete at build level. Regeneration produced `indigo_focuser_askar.c`, `.h` and `_main.c`; the generated driver builds with `make -C indigo_drivers/focuser_askar -f ../../Makefile.drv`. `DEVICE_PORTS` uses `pass_through_change` so Wi-Fi refresh augmentation can coexist with the base serial-port selection handler. Motion completion compares against the accepted target before publishing final OK/ALERT, and TCP command failure preserves the previous unexpected-disconnect behavior.

Step 4 is complete. The PTY simulator keeps its normal protocol behavior and gains test-only `INDIGO_ASKAR_EVENTS` and `INDIGO_ASKAR_FAULT` hooks for command tracing, malformed/error/silent replies, transport close and external position changes.

Step 5 is complete. `test_focuser_askar_simulator.c` now has nine isolated scenarios covering direct protocol behavior, property/capability inventory, absolute and relative movement, limit changes and clipping, SYNC, abort, failed commands with recovery, polling failure recovery, external position polling and reconnect. The unsandboxed final run passed 9/9 scenarios with 0 failing scenarios.

Step 6 is complete. `indigo_docs/PROPERTIES.md`, `indigo_test/CHANGES.md`, `MIGRATION_STATUS.md` and the Xcode project are updated for the generated driver and new files. `plutil -lint indigo.xcodeproj/project.pbxproj` passes. The driver build and simulator integration test were run outside the sandbox before commit.

## Reproduction

```sh
cd indigo_drivers/focuser_askar
../../build/bin/indigo_generator indigo_focuser_askar.driver
make -f ../../Makefile.drv
cd ../..
make -C indigo_test build/integration/test_focuser_askar_simulator
cd indigo_test
./build/integration/test_focuser_askar_simulator
```

Final automated result: 9/9 named scenarios passed, 0 failing scenarios. The scenarios were `protocol`, `capabilities`, `absolute_and_relative_motion`, `limits_and_sync`, `abort_motion`, `rejected_connection`, `command_failure_recovery`, `sync_and_poll_failure_recovery` and `external_position_and_reconnect`.
