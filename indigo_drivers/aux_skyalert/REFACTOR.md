# Interactive Astronomy SkyAlert Coverage Completion

## Current-State Audit

- Audit baseline: branch `refactoring` at commit `21873b796`, macOS Darwin 25.6.0, universal arm64/x86_64 artifacts, arm64 execution.
- `indigo_aux_skyalert.driver` is the authoritative generator input; `indigo_aux_skyalert.c`, `.h` and `_main.c` are checked-in generated outputs. The driver is API 3, uses portable `indigo_uni_io` at 115200 baud, generator-owned serialized handlers and one serial handle per logical AUX instance.
- The device exposes `INDIGO_INTERFACE_AUX_SQM`, runtime additional instances and two read-only number properties: `AUX_WEATHER` (temperature, humidity, pressure, wind speed, dampness, sky temperature) and `AUX_INFO` (sky brightness, power).
- Protocol: the device answers the request `send` with a ten line record terminated by CR - the literal `Data`, seven numbers, the firmware string and the pressure in pascal. It produces no unsolicited output, so every reading is one request. No manufacturer document is present in the repository; the 2.0 driver and the bundled simulator are the protocol reference.
- Before this work the hardware-free coverage was one smoke case (`skyalert_passes_serial_compliance_checks`: connect, interface bit, item presence) plus a two-case fake-transport test. The simulator answered every `send` with the same fixed record, had no event log, no fault channel and no options, so it could not exercise polling, any parse failure, or transport loss.

## Defect Found and Fixed (driver version 6 -> 7)

**The 3.0 refactoring dropped the poll.** The 2.0 driver ran `aux_timer_callback()` every 10 seconds
(`indigo_reschedule_timer(device, 10, &PRIVATE_DATA->timer_callback)` in commit `95905474e`). The
generated 3.0 driver has no `on_timer` block at all: `skyalert_read_record()` is called only from
`skyalert_open()`, so `AUX_WEATHER` and `AUX_INFO` were populated once at connection and then never
changed again for the whole session. A weather station that reports the same temperature forever is
not a subtle failure, but nothing tested it, because the old simulator returned a constant record
and the old case asserted only that the items exist.

The `.driver` source now carries an `on_timer` block that re-reads the record, publishes both
properties with `INDIGO_OK_STATE` or `INDIGO_ALERT_STATE`, and reschedules itself 10 seconds later,
matching the 2.0 cadence. The generator queues the first run at the end of the connection handler
and `indigo_cancel_pending_handlers()` in the disconnect path stops it.

Confirmed before the fix: against version 6 the new suite fails `readings_are_refreshed_by_polling`,
`a_failed_poll_is_reported_and_polling_stops_on_disconnect` and
`transport_loss_is_reported_and_reconnect_recovers` - exactly the three cases that depend on a
reading arriving after connection - and passes all eight after it.

## Test Work

- Simulator: added a `<ready-file>.events` command log, a one-shot `<ready-file>.control` fault
  channel (`drop`, `header`, `truncate <n>`, `garbage <n>`, `infinite <n>`, `close`), a `--firmware`
  option, and per-record variation of the temperature (+0.1 C) and the sky brightness (+1). The
  variation is not decoration: `indigo_update_property()` suppresses an update in which no value and
  no state changed, so a record whose every field repeats is never published and a test cannot tell
  a fresh reading from a stale one. One varying field per published property makes both observable.
- Suite: 1 case -> 8 cases, listed below. The `--known-defects` convention is not used; every case
  runs in the default target.
- Runtime is about 95 s, dominated by three cases that have to outlast the driver's own 10 s poll.
  That is inherent to the cadence, not to the harness.

## Scenario-to-Test Mapping

| Required behavior | Automated scenario |
| --- | --- |
| Driver metadata, API generation, interface bit, base inventory before connect, hidden properties, exact property counts, both properties read-only with their documented items | `identity_inventory_and_record_contract` |
| Every record field mapped to its item, and pascal converted to hectopascal | `identity_inventory_and_record_contract` |
| Firmware string from the ninth record line, cleared on disconnect, restored on reconnect | `firmware_is_reported_from_the_record` |
| Dropped record, non-`Data` header, record truncated before and after the firmware line, non-numeric field, non-finite field; each refuses the connection and publishes nothing; the leftover of a truncated record does not become the next one | `unparseable_records_are_refused` |
| The poll keeps asking after connection and both published properties carry the new reading | `readings_are_refreshed_by_polling` |
| A failed reading is reported on both properties, the poll survives it and recovers, and nothing reaches the device after disconnect | `a_failed_poll_is_reported_and_polling_stops_on_disconnect` |
| Failed serial open, nothing published, later connection succeeds | `failed_open_leaves_the_device_disconnected` |
| Device vanishing while connected is reported, fresh device on the same port is read again | `transport_loss_is_reported_and_reconnect_recovers` |
| Repeated disconnect, connected-property deletion and redefinition, `INDIGO_DRIVER_SHUTDOWN` refused while connected | `repeated_disconnect_and_refused_shutdown` |
| Record units and firmware, open/write failure and every per-field read failure at the transport boundary | `test_aux_skyalert_transport` (2 cases, unchanged) |

### Non-applicable and Deferred Coverage

- The device has no writable property, no motion, no abort and no shared logical devices, so queued operations, abort races and shared-interface ordering are not applicable.
- The driver persists nothing, so there is no configuration roundtrip to test.
- **Deferred.** The poll reschedules unconditionally, so a device that stops answering produces an `ALERT` every 10 seconds for as long as it stays connected. `aux_mgbox` stops its own poll behind a `transport_failed` flag instead. The 2.0 driver rescheduled unconditionally too, so this preserves the historical behavior rather than choosing one; changing it is a separate decision.
- **Deferred.** The 10 s cadence is hard-coded, as it was in 2.0. Other weather drivers in the tree expose no refresh-interval property either, so none was added.
- Real sensor accuracy, cable interruption and Windows runtime require environments not available for this work.

## Hardware-Test Decision

No physical Interactive Astronomy SkyAlert is available. No hardware run was performed and none of the results below are presented as verification of real sensor behavior.

## Validation Evidence

- `indigo_generator indigo_aux_skyalert.driver` run twice leaves the generated `.c`, `.h` and `_main.c` unchanged.
- Strict builds: `make -B build/integration/aux_skyalert_simulator build/integration/test_aux_skyalert_simulator CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` passed.
- `build/integration/test_aux_skyalert_simulator` passed 8 of 8 cases; `build/integration/test_aux_skyalert_transport` passed 2 of 2 against the polling driver.
- `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/integration/test_aux_skyalert_simulator_asan` passed all 8 cases with the production driver source instrumented and no sanitizer report. LeakSanitizer is unavailable on this macOS runtime and is not claimed.

## Final Test Summary

- Simulated tests: **10 run, 10 passed** (8 simulator, 2 transport), plus the same 8 simulator cases under ASan/UBSan.
- Hardware tests: **0 run, 0 passed**; no compatible physical device is available.
