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

## Rejected-change regression coverage (2026-09-18)

Change requests refused by a busy guard are now declared with the generator's `reject_change` block. The generated guard marks every item for update, sets `INDIGO_ALERT_STATE` and publishes the property with the message, so the client receives the actual driver-side values instead of an `INDIGO_OK_STATE` update carrying no items, which left the refused value visible in the client.

Covered by the `rejected_change` scenario in `indigo_test/integration/test_focuser_askar_simulator.c`: while `FOCUSER_POSITION` is BUSY, `FOCUSER_STEPS` ends in ALERT with unchanged value and target, no second move command is issued, and the property is accepted again after the abort.

```sh
cd indigo_test && ASKAR_TEST_FILTER=rejected_change ./build/integration/test_focuser_askar_simulator
```

## Focuser testing rules alignment (2026-10-05, version 9)

The suite was checked against the extended "Focuser Driver Test Standard" in `indigo_test/DRIVER_TESTING_RULES.md` (commit `fa5f64839`) and against `Commands_Focuser_CDC_EN.md`, including the two decisions for all focusers: an aborted move ends `FOCUSER_POSITION` and `FOCUSER_STEPS` ALERT at the stopped position, and a disconnect during motion sends `FS#` before the port closes (already implemented, now tested).

### Defects found and fixed (driver version 8 → 9)

Every case below failed against a build of the version 8 source kept outside the tree and passes against version 9.

| Id | Observable impact | Fix | Regression test |
| --- | --- | --- | --- |
| ASK-01 | An aborted move ended OK; an abort of a move still queued published ALERT and then OK. | The aborted move ends ALERT with value = target = the position read after `FS#`. | `abort_motion`, `rejected_change`, `refused_stop_keeps_move` |
| ASK-02 | An abort while idle sent `FS#`. | Nothing moving and nothing uncertain answers OK without a command. | `abort_motion` |
| ASK-03 | An idle position poll whose reply was in flight when a GOTO was accepted overwrote the copied target with the polled position, so the GOTO became a no-op and no `FP` was sent. | The guard is repeated after the reply, right before any assignment. | `poll_in_flight_does_not_complete_request` (simulator `slow` reply) |
| ASK-04 | A failed idle position read left `FOCUSER_POSITION` ALERT for good; motion started by another controller (Wi-Fi application) or running at connect was published only as changing values with an OK state. | The poll also reads `FQ#`: external motion is published BUSY and ends OK with target = measured position; a poll failure that turned the property ALERT is cleared by the next good read; connect reads `FQ#` as well. | `idle_poll_and_external_motion`, `motion_running_at_connect` |
| ASK-05 | A refused (`FE#`) or unacknowledged `FP` left the driver "uncertain", so every following move was refused until an abort; the refusal reason was lost because the reader kept waiting for the expected prefix after `FE#`. | `FE#` ends the exchange at once and is reported in the message; a refused move needs nothing more, an unacknowledged one sends `FS#`, and only a failed stop leaves the state uncertain. | `refused_and_lost_move` |
| ASK-06 | A failed SYNC made the driver uncertain, so the next SYNC was refused until a reconnect. | A failed SYNC re-reads and keeps the real position, ends ALERT with the reason and the next SYNC is accepted; a successful `FY` (which halts the focuser) clears an uncertain state. | `sync_and_poll_failure_recovery` |
| ASK-07 | A move request while a move was queued was dropped without an answer; limits, backlash, reverse and motor mode requested during a move were answered ALERT from the handler with the requested value left visible (limit and backlash targets, the switch items). | `reject_change` with a motion condition that includes a pending flag set by `on_change_request`; refused or failed setting writes restore the device value and a readback that differs from the request ends ALERT. | `refusals_during_motion`, `command_failure_recovery` |
| ASK-08 | On firmware without the backlash commands (before 1.0.3, `Fb#` answers `FE#`) `FOCUSER_BACKLASH` was offered anyway and kept the value of a previous session; reverse and motor mode behaved the same way when their reads failed. | A setting whose read fails at connect is not defined for that session, so its write command is never sent. | `firmware_without_backlash` |

The simulator gained start-state options (`--position`, `--max-step`, `--backlash`, `--reverse`, `--motor-mode`, `--moving-to`), firmware-dependent backlash support (`--firmware` below 1.0.3 answers `FE#` to `Fb`/`FB`, as the protocol document states), `sticky_` faults and the fault actions `partial`, `overlong`, `split`, `slow`, `stall` (the motor accepts a move and does not turn until a stop) and `move=<n>` (motion started by another controller).

### Scenario-to-test mapping (rules of `fa5f64839`)

| Rule | Cases |
| --- | --- |
| Protocol commands against the simulator | `protocol` |
| Interface, ranges, hidden speed/temperature/compensation/mode, custom motor mode | `capabilities` |
| Connect publishes the device state (position, travel, backlash, reverse, motor mode); GOTO to the position read at connect without a move; SYNC equal to the published value reaches the device | `connect_publishes_device_state`, `external_position_and_reconnect` |
| Firmware variant without backlash: property not offered, `FB` never sent | `firmware_without_backlash` |
| Failed optional identity query keeps the placeholder | `identity_fallback` |
| Refused connect (malformed, partial, overlong position reply): ALERT, nothing defined, port released, next connect works; split reply reassembled | `rejected_connection`, `position_reply_framing` |
| Absolute and relative moves, zero step, clamping at the travel end, limits written and read back, limit below the position refused | `absolute_and_relative_motion`, `limits_and_sync` |
| Move and setting requests during a move refused with ALERT and no command, values kept, move ends at its target with one `FP` | `refusals_during_motion`, `rejected_change` |
| Mid-move abort ALERT at the stopped position, two idle readbacks, idle abort and OFF request without `FS#`, fresh move | `abort_motion` |
| Refused stop keeps the move BUSY, retry stops it | `refused_stop_keeps_move` |
| Refused and unacknowledged move: ALERT with the reason, next move works | `refused_and_lost_move` |
| Stalled move (accepted, motor does not turn): stopped within 100 polls without progress, ALERT with the reason, fresh move works | `stalled_move_ends_alert` |
| SYNC failure keeps the real position, next SYNC accepted | `sync_and_poll_failure_recovery` |
| Failed idle read ALERT with the last value, recovery OK; external motion BUSY then OK; relative move from it | `idle_poll_and_external_motion`, `sync_and_poll_failure_recovery` |
| Motion running at connect | `motion_running_at_connect` |
| Poll reply in flight when a move is accepted | `poll_in_flight_does_not_complete_request` |
| Rejected and mismatched setting writes keep the device value, immediate retry works | `command_failure_recovery` |
| Disconnect during motion: one `FS#`, no poll afterwards, reconnect OK at the real position; idle disconnect without `FS#` | `disconnect_during_motion` |
| SHUTDOWN refused while connected, `ADDITIONAL_INSTANCES` instance with its own state, sibling survives | `shutdown_refused_while_connected`, `additional_instance` |

Not applicable, with the reason:

- Temperature, compensation, automatic mode and speed: not in the CDC protocol.
- Wi-Fi (`askar://`, UDP discovery, TCP): the hardware-free suite does not open network sockets; the TCP path shares the command layer with the serial path.
- Hardware: no Askar-WAF available, no physical run.

The case runner now records the plan in the parent before it forks the cases, as `indigo_test/AGENTS.md` requires.
