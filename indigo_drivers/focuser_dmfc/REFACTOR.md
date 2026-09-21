# INDIGO 3.0 refactoring record for `focuser_dmfc`

This record was created together with the 2026-09-18 `reject_change` work. The driver's earlier
migration to `indigo_generator` predates this file and is deliberately not reconstructed here; only
the changes below are recorded, so nothing in this file is inferred history.

## Rejected-change regression coverage (2026-09-18)

A change request that arrives while a motion is already running is now refused with the generator's
`reject_change` block on `FOCUSER_POSITION` and `FOCUSER_STEPS`, with the condition that the other motion property is BUSY and the message
shown in the generated driver. The generated guard marks every item for update, sets
`INDIGO_ALERT_STATE` and publishes the property, so the client receives the actual driver-side values
instead of an update that carries no items.

Both motion properties are published BUSY together at motion start, so `INDIGO_COPY_*_PROCESS_CHANGE` dropped a concurrent request silently: the client received no update at all and kept showing the refused target. The guard runs before that macro and turns the silent drop into an explicit refusal.

Driver version is now `0x03000011`.

## Automated test coverage (2026-09-20)

`indigo_test/integration/test_focuser_dmfc_simulator.c` was extended from a single smoke test to the
full focuser class standard in `indigo_test/DRIVER_TESTING_RULES.md`. Every scenario runs in its own
forked process against a freshly started simulator, so no state leaks between cases.

The simulator `focuser_dmfc_simulator/focuser_dmfc_simulator.c` gained the fixtures the standard
needs and nothing else:

- `--profile <normal|configured|no-handshake|bad-status|external-motion>` selects the controller
  state a scenario connects to.
- `INDIGO_DMFC_EVENTS` records every accepted request, so a test can assert the command, its
  argument and its sign instead of only the resulting property state.
- `INDIGO_DMFC_FAULT` arms a one-shot `silent`, `garbage` or `close` fault on the next request with a
  given prefix, which is how lost acknowledgements, malformed replies and transport loss are injected.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Capabilities and readback | `metadata`, `property_contract`, `status_readback` |
| Initialization failures | `handshake_rejected`, `handshake_timeout`, `status_query_failure` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` |
| Motion, units and sign | `sync_and_goto`, `goto_no_op`, `relative_move`, `limits_clamp_goto` |
| Stop and overlapping requests | `motion_progress_and_abort`, `abort_while_idle`, `overlap_rejected` |
| Externally changed position | `external_motion_observed` |
| Disconnect during motion | `disconnect_during_motion` |
| Modes and controls | `controller_settings`, `settings_reported_failures` |
| Settings persistence | `limits_configuration_roundtrip` |
| Polling and failure recovery | `temperature_polling`, `poll_failure_recovery` |
| Transport loss | `transport_loss` |

### Notes and gaps

- `FOCUSER_MODE` and `FOCUSER_COMPENSATION` stay hidden; the controller has no temperature
  compensation of its own, so the compensation rows of the class standard do not apply. Their absence
  is asserted in `property_contract`.
- The driver publishes `INFO` while opening the port and parses the model and firmware from the `A`
  status line afterwards without republishing, so `metadata` reads the parsed identity through a
  fresh enumeration. A client that connects and then enumerates sees the correct values; a client
  watching only updates keeps the handshake placeholder until it re-enumerates.
- `FOCUSER_LIMITS` is the only driver-persisted setting. The `CONFIG` roundtrip runs against a
  scratch directory through the `indigo_uni_config_folder` override, so the user profile is untouched.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no DMFC or FocusCube controller was
  available.

```sh
make -C indigo_test build/integration/test_focuser_dmfc_simulator
cd indigo_test && ./build/integration/test_focuser_dmfc_simulator
cd indigo_test && ./build/integration/test_focuser_dmfc_motion
```

- Simulated tests run: 25; passed: 25.
- Hardware tests run: 0; passed: 0.

## Regenerated for the shared refusal and connect macros (2026-09-21)

No behaviour change and no version bump. `indigo_generator` stopped emitting the eight-line refusal
block and the five-line CONNECTION admission block inline and now emits `INDIGO_REJECT_CHANGE_IF()`
and `INDIGO_PROCESS_CONNECT()` / `INDIGO_PROCESS_QUEUED_CONNECT()` instead, so this driver was
regenerated along with the other 114 generator inputs. The macros expand to exactly the statements
that were written out before; for a sample of four drivers the preprocessed translation unit is
byte-identical apart from the new `indigo_reject_change()` declaration and shifted `assert()` line
numbers. Background and the defect that motivated the refusal macro are in `indigo_drivers/REVIEW.md`
(DRV-213, DRV-214) and `indigo_libs/REVIEW.md` (LIB-015, LIB-016).

Verification: the complete simulator suite was re-run after regeneration —
`indigo_test/build/integration/test_focuser_dmfc_simulator`, macOS arm64, 24/24 passed on 2026-09-21 12:44.
