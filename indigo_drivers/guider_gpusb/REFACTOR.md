# GPUSB guider refactoring and validation record

Status: partial fake-SDK coverage recorded on 2026-09-09. Production refactoring is not declared complete by this record.

## Current-state audit

- The generated guider uses a USB/SDK boundary. Hardware-free validation uses the fake USB/SDK implementation in `indigo_test/integration/test_usb_outputs.c`; no serial protocol is invented for this USB-only device.
- The four existing groups cover discovery, duplicate events, connection rollback, active removal and reload; direction mapping, delayed completion and errors; initialization and attach rollback; settings, abort, stop/read errors and removal during operation.
- Direction coverage includes all four directions, cross-axis pulses, reversal and zero stop. Regression coverage includes failed-replacement preservation and single-write zero-stop behavior.
- Remaining gaps recorded for this validation are the detailed timing and capacity audit.

## Validation and next steps

- The historical test-change log recorded this driver as `Partial` with fake USB/SDK coverage: lifecycle failures, four directions, cross-axis pulses, reversal, zero stop, active removal, failed-replacement preservation and single-write zero-stop regressions pass; timing and capacity audit remain pending.
- All four fake-SDK test groups passed on 2026-09-09.
- Hardware testing was not performed. No physical relay, USB timing or hardware lifecycle validation is claimed.
- Before further production refactoring: reconfirm the fake-SDK baseline; audit current coverage against the Guider Driver Test Standard; plan atomic timing, capacity, sanitizer and remaining lifecycle/concurrency work; verify and record each step before claiming completion.

## Final test summary

- Simulated/fake-SDK tests: 4 executed, 4 passed.
- Hardware tests: 0 executed, 0 passed.

## Full guider class coverage (2026-09-20)

The driver was covered only by the shared `indigo_test/integration/test_usb_outputs.c` smoke test,
which runs one set of scenarios across four unrelated USB drivers. It now has the full guider class
standard from `indigo_test/DRIVER_TESTING_RULES.md` in
`indigo_test/integration/test_guider_gpusb_sdk.c`.

The relay interface is an explicit ON/OFF mask, so the fake records every mask the driver writes
together with the time it was written. A scenario can therefore assert which relay was closed, that
the opposite axis stayed closed while the first one opened, and how long each direction was actually
held, instead of trusting the published property state. The fake also counts opens, closes, libusb
references and attached devices, and records any relay write attempted on a released device.

The libusb hot-plug boundary is faked as well, so arrivals, duplicate arrivals, removals and a
failed attachment are driven directly instead of requiring hardware.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Identity and property contract | `metadata`, `property_contract` |
| Initialization failures | `open_failure`, `init_rollback` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected` |
| Hot-plug identity and removal | `hotplug_events`, `removal_during_pulse` |
| Directions and units | `directions` |
| Replacement and axes | `simultaneous_axes`, `same_axis_replacement`, `zero_request_stops` |
| Completion and errors | `relay_write_failure`, `relay_release_failure` |
| Shared work and lifetime | `disconnect_during_pulse` |

### Notes and gaps

- `GUIDER_RATE` is not exposed; the relays have no rate of their own, so that row of the class
  standard does not apply and its absence is asserted in `property_contract`.
- `directions` prints the measured hold time of every direction and asserts it is within a loose
  band around the request. It is a liveness check, not a timing measurement: the standard keeps
  host dependent precision statistics out of the normal integration target.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not rerun for this change; the earlier
  hardware result recorded above still stands.

```sh
make -C indigo_test build/integration/test_guider_gpusb_sdk
cd indigo_test && ./build/integration/test_guider_gpusb_sdk
```

- Simulated tests run: 15; passed: 15.
- Hardware tests run: 0; passed: 0.

## Overlapping guide pulses (2026-09-20)

`GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare `accept_while_busy = true`, replacing the
earlier workaround that forced the property state back to `INDIGO_OK_STATE` in `on_change_request`
so the BUSY-guarded dispatch macro would let the request through. `on_change_request` is now only
the two item-zeroing lines, matching every other INDIGO driver that exposes a guider.

The `indigo_cancel_pending_handler(device, guider_guide_<axis>_handler)` call that used to sit in
`on_change_request` was removed. It runs on the bus thread, where `indigo_queue_remove()` does not
skip its blocking wait and therefore blocks until a running handler finishes; the same pattern hung
the mount_nexstar suite. Dropping it means two requests arriving inside the queue latency can queue
two handlers that both act on the already-overwritten item values, so the same relay mask can be
written twice. The resulting pulse is still the second request's, with a single finaliser.

Behaviour is unchanged and the existing `same_axis_replacement`, `simultaneous_axes`,
`zero_request_stops` and relay-failure cases still pass unmodified.

## Hardware acceptance run (2026-09-21)

A physical GPUSB USB Autoguide Adapter (`134a:9020`, device name `GPUSB Guider`) was connected to
a Mac mini (macOS, arm64) and driven through the real driver and the real `libgpusb` by the new
opt-in suite `indigo_test/hardware/test_guider_gpusb_hw.c`:

```sh
make -C indigo_test test-guider-gpusb-hw
```

Physical hot-plug was explicitly out of scope for this run, so no unplug/replug was performed; the
hot-plug identity, duplicate-arrival, capacity and removal cases stay covered by the faked libusb
boundary of `indigo_test/integration/test_guider_gpusb_sdk.c`. Everything else in the guider
hardware acceptance list of `indigo_test/DRIVER_TESTING_RULES.md` was exercised against the
adapter.

### Scenario to test mapping

| Hardware acceptance area | Scenario |
| --- | --- |
| Discovery, identity and connection | `gpusb_reports_identity_and_capabilities` |
| Published property contract | `gpusb_publishes_the_property_contract` |
| All four directions, BUSY/completion | `gpusb_pulses_in_all_four_directions` |
| Overlapping axes | `gpusb_pulses_both_axes_at_once` |
| Same-axis replacement | `gpusb_replaces_a_pulse_on_the_same_axis` |
| Zero request and relay release | `gpusb_stops_on_a_zero_request` |
| Pulse duration measurement | `gpusb_measures_guide_pulse_duration` |
| Disconnect during a pulse, fresh pulse after reconnect | `gpusb_survives_a_disconnect_during_a_pulse` |
| Reconnect and repeated disconnect | `gpusb_reconnects` |
| INIT/SHUTDOWN, shutdown refused while connected | `gpusb_reinitializes` |

### Guiding-pulse duration measurement

Requested 20, 50, 100, 200 and 500 ms in all four directions, four samples each with the first
discarded, 60 samples in total, on an idle adapter with no other workload. Measured endpoints are
the public `GUIDER_GUIDE_RA` / `GUIDER_GUIDE_DEC` request and the OK completion the client
observes, so this is **software completion timing of the real USB command path**, not an
electrical measurement of the relay output: the ON and OFF writes happen inside the driver and the
adapter gives no feedback. Every direction ran long by a roughly constant amount, which is the two
HID writes plus the handler queue latency, not a proportional error:

| Requested | Mean error | Error in percent |
| --- | --- | --- |
| 20 ms | +11.3 … +17.2 ms | +57 … +86 % |
| 50 ms | +13.7 … +14.2 ms | +27 … +28 % |
| 100 ms | +14.4 … +17.1 ms | +14 … +17 % |
| 200 ms | +16.0 … +16.5 ms | +8 % |
| 500 ms | +12.4 … +20.0 ms | +2.5 … +4 % |

Worst absolute error over all 60 samples: 21.8 ms. Standard deviation per cell stayed between 0.7
and 6.0 ms.

## Found defects

### DRV-GPUSB-001 — every guide pulse reported ALERT on real hardware

- **Observable impact**: with a physical GPUSB connected, every `GUIDER_GUIDE_RA` and
  `GUIDER_GUIDE_DEC` request went straight to `INDIGO_ALERT_STATE` although the relay was closed
  correctly. A guiding agent treats that as a failed pulse, so the adapter was unusable through the
  3.0 driver. The first hardware run failed 9 of its 10 cases on this.
- **Root cause**: the vendored `libgpusb` reports failure for every successful write.
  `libgpusb_write()` hands `hid_write()` a single byte (`mov w2, #0x1`) and then compares the
  result with two (`cmp w0, #0x2; cset`), so `libgpusb_set()` returns false whenever the write
  succeeds. The same code is in the macOS arm64 and x86_64 slices. A direct probe against the
  adapter shows `libgpusb_open()` returning 1 and every `libgpusb_set()` returning 0 while the HID
  layer logs `Success`. The pre-3.0 driver never looked at the return value, so the defect only
  became visible when the refactoring started deriving the property state from it.
- **Production fix**: `gpusb_open()` asks the library once, right after the open that has already
  released the relays, whether it can report a success at all, and the new `gpusb_set()` helper
  reports a failure only for a build that can. A fixed `libgpusb` re-enables the error reporting
  with no further change here. Driver version 9 → 10.
- **Regression test**: `library_never_reports_success` in
  `indigo_test/integration/test_guider_gpusb_sdk.c`. The fake takes the write, records the mask and
  still answers false, exactly like the shipped library; the pulse has to complete OK and the relay
  has to be held for the requested time. `relay_write_failure` and `relay_release_failure` still
  prove that a library which can report success keeps producing ALERT.
- **Not fixed here**: the library itself. `bin_externals` holds only the binary, and repository
  rules forbid editing vendored SDKs, so `libgpusb_write()` has to be corrected in its own source
  tree.

### DRV-GPUSB-002 — the relay mask survived a disconnect

- **Observable impact**: disconnecting during a pulse left the interrupted direction in
  `PRIVATE_DATA->relay_mask`. The adapter itself was released by `gpusb_close()`, but the first
  pulse after a reconnect rebuilt its mask from the stale value and closed the old relay again on
  the other axis.
- **Root cause**: `gpusb_close()` released the relays on the device without clearing the mask the
  driver keeps for the session.
- **Production fix**: `gpusb_close()` now clears `PRIVATE_DATA->relay_mask`.
- **Regression test**: found by source audit while investigating DRV-GPUSB-001, and covered
  end-to-end by `gpusb_survives_a_disconnect_during_a_pulse`, which disconnects mid pulse,
  reconnects and pulses the other axis. The published properties cannot show which relay is
  closed, so the mask itself stays asserted by the fake-SDK suite.

## Final test summary (2026-09-21)

- Simulated/fake-SDK tests: 20 executed, 20 passed (16 in `test_guider_gpusb_sdk`, 4 in the shared
  `test_usb_outputs` GPUSB configuration).
- Hardware tests: 10 executed, 10 passed, against a physical GPUSB Guider on macOS arm64, without
  physical hot-plug.
