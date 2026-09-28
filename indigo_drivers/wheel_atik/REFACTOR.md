# INDIGO 3.0 refactoring record for `wheel_atik`

This record was created with the 2026-09-20 test coverage work. The driver's earlier migration to
`indigo_generator` predates this file and is deliberately not reconstructed here; only the changes
below are recorded, so nothing in this file is inferred history.

## Leaked HID handle on a refused connection (2026-09-20)

`atik_open()` only opened the HID device and returned, and the wheel was then probed from the
`on_connect` block, which gave up after ten seconds if the wheel never reported a slot. The
generated connection handler does not close a device whose `on_connect` fails, so every refused
connection leaked the open HID handle, and a wheel that was slow to find its first slot could not be
connected again afterwards. This violates the connection contract in the repository guidelines: the
open helper must return true with every resource acquired, or false after releasing everything that
attempt acquired.

The probe moved into `atik_open()`, which now closes the handle when the wheel never reports a slot.
`on_connect` only publishes the slot count and the current slot, exactly like `wheel_sx`. Driver
version is now `0x03000006`; `unknown_initial_slot` and `initial_query_failure` cover it, both
asserting that the number of opens matches the number of closes.

## Automated test coverage (2026-09-20)

The driver was only covered by the shared `indigo_test/integration/test_usb_outputs.c` smoke test,
which exercises four unrelated USB drivers through one set of scenarios. It now has the full wheel
class standard from `indigo_test/DRIVER_TESTING_RULES.md` in
`indigo_test/integration/test_wheel_hid.c`.

That file is a shared suite for the two hot-plugged HID filter wheels, `wheel_atik` and `wheel_sx`.
Above the transport both drivers have the same shape, so the scenarios are shared and `TEST_KIND`
selects which boundary is faked: the `libatik` calls for this driver, the raw two byte HID protocol
for the other. The fake models a wheel with a configurable slot count and a move that takes a
configurable number of readbacks, so a turning wheel is observable, and it counts opens, closes,
transfers, libusb references and attached devices. A transfer attempted on a closed wheel is counted
separately, which is how the suite proves the driver stopped talking to a device it had released.

The libusb hot-plug boundary is faked as well, so arrivals, duplicate arrivals, removals and a
failed attachment are driven directly instead of requiring hardware.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Discovery and slots | `metadata`, `property_contract`, `slot_count_rebuilt` |
| Initialization failures | `unknown_initial_slot`, `open_failure`, `initial_query_failure`, `init_rollback` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected` |
| Hot-plug identity and removal | `hotplug_events`, `removal_while_connected` |
| Positioning | `positioning`, `already_selected_slot`, `motion_progress` |
| Errors and recovery | `move_command_failure`, `poll_failure` |
| Cancellation and ownership | `disconnect_during_motion` |

### Notes and gaps

- The wheel has no calibration or reset command, so that row of the class standard does not apply.
- `WHEEL_SLOT_NAME` and `WHEEL_SLOT_OFFSET` are maintained entirely by the framework; the suite only
  asserts that both follow the slot count the wheel reports.
- The motion watchdog gives up after 120 readbacks half a second apart, so a wheel that never
  arrives is only reported after a minute. That path is not exercised, because a one minute scenario
  is out of proportion to the rest of the suite.
- The driver is single device by construction: `MAX_DEVICES` is one wheel, so the capacity and
  multiple device rows of the hot-plug standard do not apply. A second arrival is asserted not to
  attach a second device.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no Atik wheel was available, and
  the driver README already states it was written without access to the hardware.

```sh
make -C indigo_test build/integration/test_wheel_atik_hid
cd indigo_test && ./build/integration/test_wheel_atik_hid
```

- Simulated tests run: 17; passed: 17.
- Hardware tests run: 0; passed: 0.
