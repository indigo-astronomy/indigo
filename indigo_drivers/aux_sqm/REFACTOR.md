# Unihedron SQM Coverage Completion

## Current-State Audit

- Audit baseline: branch `refactoring` at commit `5c23edc93`, macOS Darwin 25.6.0, universal arm64/x86_64 artifacts, arm64 execution.
- `indigo_aux_sqm.driver` is the authoritative generator input; `indigo_aux_sqm.c`, `.h` and `_main.c` are checked-in generated outputs. The driver is API 3, uses portable `indigo_uni_io` at 115200 baud, generator-owned serialized handlers and one serial handle per logical AUX instance.
- The device exposes `INDIGO_INTERFACE_AUX_SQM`, runtime additional instances and two read-only number properties: `AUX_WEATHER` (sky brightness, sky temperature, Bortle class) and `AUX_INFO` (`X_AUX_SENSOR_FREQUENCY`, `X_AUX_SENSOR_COUNTS`, `X_AUX_SENSOR_PERIOD`).
- Protocol reference: `SQM-LU-DL_Users_manual.pdf` in this directory. Commands are plain character strings terminated by `x`; the driver uses `ix` (unit information, table 8.6: `i,` plus protocol, model, feature and serial number as eight digits each) and `rx` (reading, table 8.3: `r,` plus the reading in mag/arcsec2 with a sign column, the sensor frequency, the period in counts, the period in seconds and the sensor temperature, CR LF terminated). The meter produces no unsolicited output.
- The driver polls `rx` every 10 seconds, parses the record with one `sscanf` covering all five fields and their unit suffixes, requires all five to be finite, and derives the Bortle class from the reading through `indigo_aux_sky_bortle()`. The handshake accepts a reply that starts with `i,`. **No production defect was found in this pass**; unlike `aux_skyalert`, the 3.0 refactoring kept the 2.0 poll and made the parse stricter.
- Before this work the hardware-free coverage was one smoke case (connect, interface bit, item presence) plus a two-case fake-transport test. The simulator answered `ix` and `rx` with two fixed strings, had no event log, no fault channel and no options, so it could not exercise polling, any parse failure, the Bortle mapping or transport loss.

## Test Work

- Simulator: added a `<ready-file>.events` command log and a one-shot `<ready-file>.control` channel taking `ACTION SELECTOR [VALUE]`, with the faults `drop`, `prefix`, `short`, `nan`, `garbage` and `close` plus `reading <mpsas>`, which pins the next record's brightness so the Bortle mapping can be driven across its thresholds. The selector aims a fault at the handshake (`i`) or at the reading (`r`) alone.
- The record now follows the manual's sign column (`" 20.70m"`, `"-05.20m"`) instead of a zero padded field, and the reading falls by 0.01 while the counts rise by one per record. That variation is not decoration: `indigo_update_property()` suppresses an update in which no value and no state changed, so a record whose every field repeats is never published and a test cannot tell a fresh reading from a stale one. One varying field per published property makes both observable.
- Suite: 1 case -> 8 cases. Faults aimed at the reading are consumed by the poll the connection handler queues immediately, so most of them cost a connect rather than a poll interval; only three cases have to outlast the driver's own 10 s cadence. Runtime is about 100 s.

## Scenario-to-Test Mapping

| Required behavior | Automated scenario |
| --- | --- |
| Driver metadata, API generation, interface bit, base inventory before connect, hidden properties, exact property counts, both properties read-only with their documented items | `identity_inventory_and_reading_contract` |
| `ix` then `rx` and nothing else, and every field of table 8.3 mapped to its item | `identity_inventory_and_reading_contract` |
| The Bortle class is derived from the reading, at both ends of the scale and across a boundary, including a negative reading through the record's sign column | `bortle_class_follows_the_reading` |
| Dropped, wrong-prefix and truncated handshake, and a port that cannot be opened; each leaves the device disconnected with nothing published and does not block a later connection | `handshake_failures_leave_the_device_disconnected` |
| Dropped, wrong-prefix, truncated, non-finite and non-numeric reading, each reported on both properties while the device stays connected; the leftover of a truncated record does not become the next one | `unparseable_readings_are_reported` |
| The poll keeps asking after connection, both published properties carry the new reading, and the handshake happens once per session | `readings_are_refreshed_by_polling` |
| A failed reading does not stop the poll, and nothing reaches the device after disconnect | `a_failed_poll_recovers_and_polling_stops_on_disconnect` |
| Meter vanishing while connected is reported, fresh meter on the same port is read again | `transport_loss_is_reported_and_reconnect_recovers` |
| Repeated disconnect, connected-property deletion and redefinition, `INDIGO_DRIVER_SHUTDOWN` refused while connected | `repeated_disconnect_and_refused_shutdown` |
| Sensor fields, malformed records, open/handshake rollback and I/O recovery at the transport boundary | `test_aux_sqm_transport` (2 cases, unchanged) |

### Non-applicable and Deferred Coverage

- The device has no writable property, no motion and no shared logical devices, so queued operations, abort races and shared-interface ordering are not applicable. The driver persists nothing, so there is no configuration roundtrip.
- The full `indigo_aux_sky_bortle()` threshold table is framework math and belongs in a unit test; the cases here check only that the driver feeds it the reading rather than another field, at five points.
- **Deferred, unchanged behavior.** The `ix` reply carries the protocol, model, feature and serial number, and the driver validates only its `i,` prefix and discards the rest. `INFO_PROPERTY->count` is left at its default, so no model, firmware or serial number is published. The 2.0 driver did the same - it only logged the record - so this is a standing gap rather than a regression, and publishing the serial number is a behavior change no simulator can validate as correct against a real meter.
- **Deferred.** The poll reschedules unconditionally, so a meter that stops answering produces an `ALERT` every 10 seconds for as long as it stays connected. The same applies to `aux_skyalert`; both preserve the 2.0 behavior and changing it is a separate decision.
- Real sensor accuracy, calibration commands (`cx`, `zcal*`), datalogger commands and interval reporting are not implemented by the driver and were not simulated.
- Real sky measurement, cable interruption and Windows runtime require environments not available for this work.

## Hardware-Test Decision

No physical Unihedron SQM is available. No hardware run was performed and none of the results below are presented as verification of real sensor behavior.

## Validation Evidence

- Strict builds: `make -B build/integration/aux_sqm_simulator build/integration/test_aux_sqm_simulator CC='clang -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter'` passed.
- `build/integration/test_aux_sqm_simulator` passed 8 of 8 cases; `build/integration/test_aux_sqm_transport` passed 2 of 2.
- `ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/integration/test_aux_sqm_simulator_asan` passed all 8 cases with the production driver source instrumented and no sanitizer report. LeakSanitizer is unavailable on this macOS runtime and is not claimed.
- The driver was not changed, so no regeneration or version bump was required; it stays at 3.0.0.20.

## Final Test Summary

- Simulated tests: **10 run, 10 passed** (8 simulator, 2 transport), plus the same 8 simulator cases under ASan/UBSan.
- Hardware tests: **0 run, 0 passed**; no compatible physical device is available.
