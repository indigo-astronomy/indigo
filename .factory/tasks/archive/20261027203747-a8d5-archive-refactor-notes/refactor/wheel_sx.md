# INDIGO 3.0 refactoring record for `wheel_sx`

This record was created with the 2026-09-20 test coverage work. The driver's earlier migration to
`indigo_generator` predates this file and is deliberately not reconstructed here; only the change
below is recorded, so nothing in this file is inferred history.

## Automated test coverage (2026-09-20)

The driver was only covered by the shared `indigo_test/integration/test_usb_outputs.c` smoke test,
which exercises four unrelated USB drivers through one set of scenarios. It now has the full wheel
class standard from `indigo_test/DRIVER_TESTING_RULES.md` in
`indigo_test/integration/test_wheel_hid.c`.

That file is a shared suite for the two hot-plugged HID filter wheels, `wheel_sx` and `wheel_atik`.
Above the transport both drivers have the same shape, so the scenarios are shared and `TEST_KIND`
selects which boundary is faked: the raw two byte HID protocol for this driver, the `libatik` calls
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
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no SX wheel was available.

```sh
make -C indigo_test build/integration/test_wheel_sx_hid
cd indigo_test && ./build/integration/test_wheel_sx_hid
```

## Hardware acceptance (2026-09-20)

### Decision and device

Hardware testing is performed. The device is the SX filter wheel reported by the host as USB
`0x1278:0x0920` "SxFilterWh", attached to a macOS 15 arm64 host. It is the only Starlight Xpress
wheel available, so the multiple-device row of the hot-plug standard still cannot be exercised.

### Baseline

Before any production change the checked-in driver was rebuilt and the existing hardware-free suite
was run against it.

```sh
cd indigo_drivers/wheel_sx && make -f ../../Makefile.drv
make -C indigo_test build/integration/test_wheel_sx_hid
cd indigo_test && ./build/integration/test_wheel_sx_hid
```

Result: 17 of 17 cases passed, the generator reproduced the checked-in `.c`, `.h` and `_main.c`
without a diff. The repository-wide `make all` fails earlier, in `indigo_driver_metadata` on
`ccd_atik`; that is unrelated to this driver and the driver was therefore built through
`Makefile.drv`.

### Plan

1. Write `indigo_test/hardware/test_wheel_sx_hw.c` covering the wheel hardware acceptance
   checklist, wire it into `indigo_test/Makefile` as the opt-in `test-wheel-sx-hw` target and add
   both to the Xcode project. — **done**, the test builds warning free and registers 12 cases plus
   the two opt-in transport loss cases.
2. Run the suite against the wheel, record every failure. — **done**, see the run below. Two cases
   failed, `sx_disconnects_while_turning` on a real driver defect and `sx_reconnects` as its
   cascade.
3. Fix each defect in `indigo_wheel_sx.driver`, regenerate, rerun the hardware suite and the
   hardware-free suite. — **done**, see the found defects section. Version 5 → 6.
4. Record the run in the driver `README.md`, regenerate `TEST_SUMMARY.md` and reconcile
   `MIGRATION_STATUS.md`. — **done**.

### Environment defect that blocked the run

The first run of the hardware test crashed in `libusb_init()` before reaching any scenario, with a
jump to address 0 in `usbi_create_event()`. The cause is outside this driver: libusb's `configure`
detects `pipe2()` on macOS because the link test succeeds against the weakly linked 10.10
deployment target, while this machine's libSystem exports no `pipe2`, so libusb calls a null
pointer. Every libusb based INDIGO driver crashes on connect, and the repository `make all` fails
in `indigo_driver_metadata` for the same reason. Fixed by passing `ac_cv_func_pipe2=no` to the
macOS libusb `configure` invocation in `indigo_libs/Makefile`; `indigo_libs/externals/libusb/config.h`
is untracked build output and is regenerated by that rule.

### Found defects

**WSX-001 — the wheel refuses to connect while it is turning.** Reproduced on hardware by
`sx_disconnects_while_turning`: after disconnecting during a move, the next connect failed with
"Failed to connect to SX Filter Wheel", leaving the wheel unusable until it had come to a stop.

Root cause: `sx_open()` required `PRIVATE_DATA->current_slot > 0` from the initial query. A probe
of the real protocol (5 slot wheel, query `{0,0}` answered with `{slot, count}`) shows that a
turning wheel answers `{0, 5}` for the whole move, roughly four seconds, and only then reports the
slot it reached. Slot 0 is the documented "turning" answer, not a failure, and a client hits it
whenever it connects during a move or shortly after power up.

Fix, in `indigo_wheel_sx.driver`:

- `sx_open()` now only requires the query itself to succeed. `sx_message()` still rejects an
  invalid slot count or an out of range slot, so a genuinely bad reply still fails the open.
- `wheel.on_connect` publishes `WHEEL_SLOT` as `INDIGO_BUSY_STATE` and schedules
  `wheel_move_finalizer` when the wheel reports slot 0, with the same 120 poll watchdog a commanded
  move gets. The slot count is known immediately, so `WHEEL_SLOT`, `WHEEL_SLOT_NAME` and
  `WHEEL_SLOT_OFFSET` are complete from the start.
- `wheel_move_finalizer()` treats a zero target as "any arrival": it adopts the slot the wheel
  stops on as both value and target, and its completion test now also requires a non zero slot so
  that an unknown slot can never satisfy `current_slot == target_slot`.

Regression tests: `sx_disconnects_while_turning` in `indigo_test/hardware/test_wheel_sx_hw.c`
asserts that the reconnect succeeds, publishes `WHEEL_SLOT` busy, settles on the slot the wheel was
travelling to and sends no select command of its own. The hardware free reproducer is
`unknown_initial_slot` in `indigo_test/integration/test_wheel_hid.c`, which previously asserted the
defective behaviour, that the connection is refused. That scenario is now split per driver: for
`wheel_sx` it drives the fake wheel through three polls of slot 0 and asserts the connection
succeeds, the slot count is published, the wheel settles by itself and no select was commanded. The
Atik branch keeps the old expectation, because `atik_open()` deliberately probes for up to ten
seconds and refuses a wheel that never reports a slot.

### Hardware run

```sh
make -C indigo_test test-wheel-sx-hw
```

Wheel `SX Filter Wheel #01010101`, 5 slots, on slot 1 at the start and restored to it at the end.

| Run | Result |
| --- | --- |
| Driver 3.0.0.5, before the fix | 12 cases, 10 passed, `sx_disconnects_while_turning` and `sx_reconnects` failed |
| Driver 3.0.0.6, after the fix | 12 cases, 12 passed |

The hardware free suite was rerun against the fixed driver for both HID wheels:
`test_wheel_sx_hid` 17 of 17 passed, `test_wheel_atik_hid` 17 of 17 passed, so the shared scenario
split did not regress the Atik driver.

### Not covered

- The two transport loss cases, `sx_survives_transport_loss` and
  `sx_survives_transport_loss_while_turning`, are implemented and wired to `HW_HOTPLUG=1` but were
  not run: they require an operator to pull and reconnect the USB cable, and the operator declined
  for this session. They are not counted in the totals below.
- Calibration and reset are not applicable; the protocol has only the query and the select command.
- Only one SX wheel was available, so the multiple device row of the hot-plug standard is still
  uncovered. The driver is single device by construction.
- The 60 second motion watchdog, 120 polls half a second apart, is still not exercised on hardware
  or in the simulator; a wheel that never arrives cannot be produced without disabling the device.
- Windows and Linux were not exercised; the run was macOS 15 arm64 only.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Discovery and connection | `sx_reports_identity_and_capabilities`, `sx_publishes_the_property_contract` |
| Actual slot count | `sx_publishes_the_property_contract`, `sx_selects_every_slot` |
| Representative slots and readback | `sx_selects_every_slot`, `sx_selects_the_slot_boundaries` |
| Already-selected slot | `sx_reselects_the_current_slot` |
| Repeated selection | `sx_repeats_a_selection` |
| BUSY/poll/OK sequence | `sx_publishes_busy_while_turning` |
| Overlapping request | `sx_ignores_an_overlapping_request` |
| Interrupted operation | `sx_disconnects_while_turning` |
| Disconnect/reconnect and fresh operation | `sx_reconnects` |
| INIT/SHUTDOWN | `sx_rejects_shutdown_while_connected`, `sx_reinitializes` |
| Transport loss, idle and active | `sx_survives_transport_loss`, `sx_survives_transport_loss_while_turning` (opt-in, `HW_HOTPLUG=1`) |

Calibration and reset are not applicable: the SX wheel protocol has only the query and the select
command, so there is no calibration to exercise and no abort command to require.

```sh
make -C indigo_test test-wheel-sx-hw
make -C indigo_test test-wheel-sx-hw HW_HOTPLUG=1
```

- Simulated tests run: 34; passed: 34. (`test_wheel_sx_hid` 17 and `test_wheel_atik_hid` 17, both
  against the fixed driver.)
- Hardware tests run: 12; passed: 12. (Driver 3.0.0.6 against an SX filter wheel; the pre-fix run of
  the same 12 cases had 2 failures, both fixed.)
