# INDIGO 3.0 refactoring record for `focuser_fc3`

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

## Stuck FOCUSER_STEPS after a short absolute move (2026-09-20)

Writing the full test suite uncovered a defect that made the driver unusable after the first short
`GOTO`. The absolute-move branch of `FOCUSER_POSITION.on_change` published only `FOCUSER_STEPS` as
BUSY and left the generator's `INDIGO_OK_STATE` prologue on `FOCUSER_POSITION`, so the two motion
properties were *not* published busy together, contrary to what the section above assumes. The
polling loop only settles the pair when `FOCUSER_POSITION` disagrees with the reported motion state,
so a move that finished inside one polling interval was answered with `is_running == 0` while
`FOCUSER_POSITION` already read OK. The loop did nothing and `FOCUSER_STEPS` stayed BUSY forever;
from then on every absolute move was refused with "Another motion operation is pending" until the
device was reconnected.

The fix sets `FOCUSER_POSITION` to BUSY alongside `FOCUSER_STEPS` when `FM:` is acknowledged, which
is what the relative-move branch and the connect-time status parsing already do. Driver version is
now `0x03000006`. `short_moves_settle` is the regression test; it was confirmed to fail against the
pre-fix driver and to pass against the fixed one.

## Automated test coverage (2026-09-20)

`indigo_test/integration/test_focuser_fc3_simulator.c` was extended from a single smoke test to the
full focuser class standard in `indigo_test/DRIVER_TESTING_RULES.md`. Every scenario runs in its own
forked process against a freshly started simulator, so no state leaks between cases.

The simulator `focuser_fc3_simulator/focuser_fc3_simulator.c` gained the fixtures the standard
needs and nothing else:

- `--profile <normal|configured|no-handshake|bad-status|external-motion>` selects the controller
  state a scenario connects to.
- `INDIGO_FC3_EVENTS` records every accepted request, so a test can assert the command, its argument
  and its sign instead of only the resulting property state.
- `INDIGO_FC3_FAULT` arms a one-shot `silent`, `garbage` or `close` fault on the next request with a
  given prefix, which is how lost acknowledgements, malformed replies and transport loss are injected.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Capabilities and readback | `metadata`, `property_contract`, `status_readback` |
| Initialization failures | `handshake_rejected`, `handshake_timeout`, `status_query_failure`, `firmware_query_failure` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` |
| Motion, units and sign | `sync_and_goto`, `relative_move`, `limits_clamp_goto`, `short_moves_settle` |
| Stop and overlapping requests | `motion_progress_and_abort`, `abort_while_idle`, `abort_failure_reported`, `overlap_rejected` |
| Externally changed position | `external_motion_observed` |
| Disconnect during motion | `disconnect_during_motion` |
| Modes and controls | `controller_settings`, `settings_reported_failures` |
| Command failures | `motion_command_failures` |
| Settings persistence | `configuration_roundtrip` |
| Polling and failure recovery | `status_polling`, `poll_failure_recovery` |
| Transport loss | `transport_loss` |

### Notes and gaps

- `FOCUSER_MODE` and `FOCUSER_COMPENSATION` stay hidden; the controller has no temperature
  compensation of its own, so the compensation rows of the class standard do not apply. Their absence
  is asserted in `property_contract`.
- The polling loop republishes both motion properties every second, so a state the driver publishes
  in answer to a request can be overwritten before a test looks at it. Every request assertion is
  therefore made against the property state revisions rather than the current state.
- The relative-move branch clamps `FOCUSER_STEPS` against `FOCUSER_LIMITS_MAX_POSITION` when moving
  inward and against `FOCUSER_LIMITS_MIN_POSITION` when moving outward, while `FG:` is sent negative
  for inward and positive for outward. The clamp is therefore applied against the opposite end of the
  travel from the one the move approaches. Only the absolute-move clamp is covered by
  `limits_clamp_goto`; the relative clamp is left untested on purpose, because asserting the present
  behaviour would lock the inversion in. This is a separate defect and was not fixed here.
- `FOCUSER_DIRECTION` and `FOCUSER_LIMITS` are the driver-persisted settings. The `CONFIG` roundtrip
  runs against a scratch directory through the `indigo_uni_config_folder` override, so the user
  profile is untouched.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no FocusCube 3 was available.

```sh
make -C indigo_test build/integration/test_focuser_fc3_simulator
cd indigo_test && ./build/integration/test_focuser_fc3_simulator
```

- Simulated tests run: 27; passed: 27.
- Hardware tests run: 0; passed: 0.
