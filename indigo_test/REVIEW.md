# indigo_test Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Initial baseline retained; scoped test-harness findings recorded. TEST-009 and TEST-010 are open. |

## Scope

Automated test harness, unit tests, integration tests, fixtures, and test documentation under `indigo_test/`.

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| TEST-001 | Medium | `integration/simulator_test_common.h:simulator_client_update_property` | The update counter counted only CONNECTION, so no-update-after-disconnect assertions missed motion and polling updates. | Closed (fixed) |
| TEST-002 | Medium | `integration/simulator_test_common.h:enumerate_simulator_device` | The enumeration selector had TEXT_VECTOR type and excluded switch/number properties. | Closed (fixed) |
| TEST-003 | High | `Makefile` (`test_guider_cgusbst4_simulator`, `test_aux_cloudwatcher_simulator`, `test_aux_dragonfly_simulator`) | Three rules linked a driver archive without listing it as a prerequisite, so a rebuilt driver did not relink the test and the run silently exercised the previous archive. Adding the prerequisites immediately surfaced a real `aux_cloudwatcher` defect that had appeared to pass. | Closed (fixed) |
| TEST-004 | Medium | `integration/test_serial_outputs.c:output_read` | The fake transport treated `length` as the buffer size (`if (size >= length) size = length - 1`), but `indigo_uni_read_section()` treats it as the payload capacity. A driver asking for exactly the reply length got 0 bytes back, so `test_guider_cgusbst4_transport` failed against a correct driver. | Closed (fixed) |
| TEST-005 | Medium | `integration/test_guider_cgusbst4_simulator.c`, `Makefile` | The `phd2_dialect_directions` scenario is a known-defect reproducer for `DRV-095` (the driver emits the INDIGO letter dialect, which a numeric PHD2-dialect device rejects), but it ran in the default suite and failed it, so `test-integration` stopped before roughly half the integration tests. It is now opt-in behind `--known-defects`, matching the `dome_baader` convention, with a `test-guider-cgusbst4-simulator-known-defects` target. | Closed (fixed) |
| TEST-006 | Medium | `integration/test_agent_imager.c` | The test used `CCD_SIMULATOR_*_NAME` macros that the generator migration removed from `indigo_ccd_simulator.h`, so `make -C indigo_test test` did not build at all. Device names are now literals, as in `test_agent_guider.c`. | Closed (fixed) |
| TEST-007 | Medium | `integration/test_wheel_playerone_sdk.c:slot_workflows` | The scenario expected a NAN slot request to reach the driver and be refused with ALERT. Since `LIB-007` the bus drops non-finite values before the handler runs, so the slot keeps its previous value and the driver reports OK. The expectation now matches the framework contract; the fractional-slot case still expects ALERT. | Closed (fixed) |
| TEST-008 | Medium | `integration/test_ccd_playerone_sdk.c:slow_initialization_and_polling` | The scenario probed `INDIGO_DRIVER_SHUTDOWN` while deliberately holding the connection handler in a gate. Generated shutdown takes `driver_queue_mutex`, which that handler holds, so the call blocked until the gate's own 5 s deadline expired and broke the deadlock; the cleanup assertion on `gate_timeouts` then failed. The shutdown probe now runs after the connection has settled. | Closed (fixed) |
| TEST-009 | Medium | `Makefile` (`test_rotator_lunatico_simulator`) | The rule exists but the executable is not listed in `INTEGRATION_TESTS`, so `make test` never runs it while `MIGRATION_STATUS.md` advertised 11 cases for the driver. It currently passes 7 of 11; the four failures are the port-reconfiguration cases tracked as `DRV-195`. | Open (wire in once `DRV-195` is fixed) |
| TEST-010 | Medium | `integration/test_ccd_playerone_sdk.c` (`wait_state`) | `cooler_individual_failures` and `Guiding and abort during long exposure then reacquire` fail intermittently, and only under full-suite load: 11 standalone runs of the binary were clean while the two full `make test` runs hit one each. `wait_state()` samples the property's current state every 10 ms, so a state the driver publishes and then replaces between two polls is missed; the serial harness avoids this by recording a revision per state (`property_state_revisions` in `simulator_test_common.h`). Making `wait_state()` edge triggered the same way was tried and made `cooler_individual_failures` fail more often (2 of 3 runs), so the scenario depends on the level-triggered semantics in some other way and the change was reverted. Needs diagnosis before the harness is changed. | Open (intermittent; blocks a green `make -C indigo_test test`) |
| TEST-011 | Medium | `integration/` wait helpers | A test that issues the next change as soon as an item's value looks right can lose it: `INDIGO_COPY_VALUES_PROCESS_CHANGE` ignores a change that arrives while the property is still `INDIGO_BUSY_STATE`, and the macro publishes the requested values with BUSY before the handler runs, so the new value is visible while the request is still in flight. A wait helper used to sequence requests must require the property to have left BUSY as well. Hit while writing the `aux_uch` suite, where it looked like a lost port switch in the driver; `test_aux_uch_simulator.c:wait_for_port()` now checks both. Hit a second time in `test_rotator_falcon2_simulator.c`, in the shape that is easiest to miss: the setup waited on the synced angle with `wait_for_number_item_value()` and then issued the goto, which the still-BUSY property dropped, so the scenario measured an abort against a rotator that had never been told to move. `sync_to()` now waits for the property to leave BUSY. Any helper that sequences two requests needs the state check, not only a value check. Related trap: properties defined OK with zeroed items at attach (`AUX_INFO`, `AUX_TEMPERATURE_SENSORS`) make `wait_for_property_state(..., OK)` return before any reading exists, so those suites wait on a value only a completed poll can write. Third trap, from `aux_wcv4ec`: where a device silently ignores an out-of-range command, the change handler has already written the requested value to the item, so a plain wait sees the value the device rejected and passes; the angle assertions there confirm the value again after more than one status frame (`wait_for_confirmed_number()`). | Closed (documented; both patterns applied in the AUX suites) |

## Finding Summaries

### TEST-001 (Closed — fixed)

The update counter counted only CONNECTION, so no-update-after-disconnect assertions missed motion and polling updates. The counter now includes every update from the selected device and is updated atomically.

Validation: affected simulator/fake transport tests rerun successfully.

### TEST-002 (Closed — fixed)

The enumeration selector had TEXT_VECTOR type and excluded switch/number properties. Initial attach broadcasts hid this error until switching the cache to a sibling device. The selector now uses wildcard type 0.

Validation: AO/guider shared connection orders and sibling survival pass.

## Review Focus

- Determinism and hardware-free execution.
- Public API coverage rather than inclusion of production `.c` files.
- Bounded waits for asynchronous simulator behavior.
- Cleanup of generated `build/` artifacts through `make -C indigo_test test-clean`.
- Alignment with `indigo_test/AGENTS.md` and `indigo_test/DRIVER_TESTING_RULES.md`.
- Updates to the relevant driver's `REFACTOR.md` for meaningful test coverage changes.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Initial review baseline only. |
| `84298256404b3aee1028d29ce152233ffd8afe2e` | working tree | 2026-09-09 | Focused shared simulator harness review; recorded and closed `TEST-001` and `TEST-002`. Verified all-property update counting and wildcard enumeration through affected simulator/fake transport tests, including AO/guider shared connection orders and sibling survival. This does not mark the entire test folder reviewed or advance its baseline. |
