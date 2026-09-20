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
