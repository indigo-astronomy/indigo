# INDIGO 3.0 refactoring record for `focuser_fcusb`

This record was created with the 2026-09-20 test coverage work. The driver's earlier migration to
`indigo_generator` predates this file and is deliberately not reconstructed here; only the change
below is recorded, so nothing in this file is inferred history.

## Automated test coverage (2026-09-20)

The driver was covered only by the shared `indigo_test/integration/test_usb_outputs.c` smoke test,
which runs one set of scenarios across four unrelated USB drivers. It now has the full focuser class
standard from `indigo_test/DRIVER_TESTING_RULES.md` in
`indigo_test/integration/test_focuser_fcusb_sdk.c`.

The focuser has no encoder: a move is a direction driven for the requested number of milliseconds.
The fake SDK therefore records the power, the frequency and the direction the driver wrote and
timestamps the start and the stop of the motor, so a scenario can assert what the device was told
and how long it was actually driven, instead of trusting the published property state. The fake also
counts opens, closes, libusb references and attached devices, and records any call attempted on a
released device.

The libusb hot-plug boundary is faked as well, so arrivals, duplicate arrivals, removals and a
failed attachment are driven directly instead of requiring hardware.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Capabilities and property contract | `metadata`, `property_contract` |
| Initialization failures | `open_failure`, `init_rollback` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected` |
| Hot-plug identity and removal | `hotplug_events`, `removal_during_motion` |
| Motion, units and direction | `timed_move` |
| Controls | `power_and_frequency` |
| Start and stop failures | `move_command_failures`, `stop_failure` |
| Abort | `abort_motion`, `abort_while_idle`, `abort_command_failure` |
| Cancellation and ownership | `disconnect_during_motion` |
| Settings persistence | `configuration_roundtrip` |

### Notes and gaps

- `FOCUSER_POSITION`, `FOCUSER_ON_POSITION_SET`, `FOCUSER_BACKLASH` and `FOCUSER_TEMPERATURE` are
  hidden, because the device has no encoder and no probe. Their absence is asserted in
  `property_contract`, and the absolute motion, sync, limit and compensation rows of the class
  standard therefore do not apply.
- `FOCUSER_SPEED` drives the motor power rather than a step rate; the suite asserts the relabelled
  item, its range and the value that reaches the device.
- A zero length move is refused, which `move_command_failures` records as the driver's documented
  no-op behaviour.
- `timed_move` prints the measured drive time of both directions and asserts it is within a loose
  band around the request. It is a liveness check, not a timing measurement: the standard keeps host
  dependent precision statistics out of the normal integration target.
- `X_FOCUSER_FREQUENCY` is the only driver-persisted setting. The `CONFIG` roundtrip runs against a
  scratch directory through the `indigo_uni_config_folder` override, so the user profile is
  untouched.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no FCUSB was available.

```sh
make -C indigo_test build/integration/test_focuser_fcusb_sdk
cd indigo_test && ./build/integration/test_focuser_fcusb_sdk
```

- Simulated tests run: 17; passed: 17.
- Hardware tests run: 0; passed: 0.

## Hardware acceptance run (2026-09-21)

A physical FCUSB USB Focuser Control Adapter (`134a:9023`, device name `FCUSB Focuser`) was
connected to a Mac mini (macOS, arm64) and driven through the real driver and the real `libfcusb`
by the new opt-in suite `indigo_test/hardware/test_focuser_fcusb_hw.c`:

```sh
make -C indigo_test test-focuser-fcusb-hw
```

All ten cases passed on the first run; no defect was found and the driver is unchanged by this
record. The suite redirects `HOME` with `indigo_test_use_private_home()` for the persistence case,
as `indigo_test/AGENTS.md` requires. Physical hot-plug was explicitly out of scope, so no
unplug/replug was performed; the hot-plug identity, capacity and removal cases stay covered by the
faked libusb boundary of `test_focuser_fcusb_sdk.c`.

The controller is open loop and has no encoder, so the focuser hardware acceptance list of
`indigo_test/DRIVER_TESTING_RULES.md` applies only in part: absolute moves, SYNC, position
readback and travel limits do not exist for this device, and their absence is asserted instead.
Every move runs the motor for 300 ms and is followed by the opposite move of the same length, so a
drawtube wired to the controller ends up where it started.

### Scenario to test mapping

| Hardware acceptance area | Scenario |
| --- | --- |
| Discovery, identity and connection | `fcusb_reports_identity_and_capabilities` |
| Published property contract, absent position/sync/temperature/backlash | `fcusb_publishes_the_property_contract` |
| Moves in both directions, BUSY/completion | `fcusb_moves_in_both_directions` |
| Abort during a move, abort while idle, fresh move | `fcusb_aborts_a_move` |
| Motor power changed and restored | `fcusb_changes_the_motor_power` |
| PWM frequency over all three settings, each with a move | `fcusb_changes_the_pwm_frequency` |
| Driver setting written, saved and restored over a reconnect | `fcusb_restores_the_setting_after_a_reconnect` |
| Disconnect during a move | `fcusb_survives_a_disconnect_during_a_move` |
| Reconnect and repeated disconnect | `fcusb_reconnects` |
| INIT/SHUTDOWN, shutdown refused while connected | `fcusb_reinitializes` |

Measured against the controller: a 300 ms move completed in 328 ms in both directions, and an
abort was answered in 7.4 ms.

Unlike the sibling Shoestring libraries, `libfcusb_write()` writes two bytes and compares the
result with two, so it reports its writes correctly. The defect recorded in
`guider_gpusb/REFACTOR.md` and `aux_dsusb/REFACTOR.md` does not affect this driver, which is why
this run needed no production change.

- Simulated tests run: 17; passed: 17 (unchanged by this record).
- Hardware tests run: 10; passed: 10, against a physical FCUSB Focuser on macOS arm64, without
  physical hot-plug.
