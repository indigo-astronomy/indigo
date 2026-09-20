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
