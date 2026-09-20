# INDIGO 3.0 refactoring record for `focuser_primaluce`

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

## Broken rotator control (2026-09-20)

Writing the rotator part of the test suite showed that the ARCO branch never worked. Four separate
defects were found and fixed; driver version is now `0x0300000C`.

- `ROTATOR_POSITION.on_change` sent `FOCUSER_POSITION_ITEM->number.target`. On a rotator device
  `FOCUSER_CONTEXT` casts the `indigo_rotator_context` to an `indigo_focuser_context`, so that macro
  actually reads the fifth pointer of the rotator context, which is `ROTATOR_ABORT_MOTION`, and then
  reads a switch item through the number member of the union. A request for 45 degrees put
  `"DEG":0` on the wire. The handler now sends `ROTATOR_POSITION_ITEM->number.target`.
- The same handler published `X_CALIBRATE_A` and `FOCUSER_POSITION` from its two error paths instead
  of `ROTATOR_POSITION`, so a refused move never reached the client and an unrelated property was
  republished. Both paths now publish `ROTATOR_POSITION`.
- `rotator_movement_finalizer` read the motor state with `CMD_MOT2_STEP`, a `res/cmd` path, from the
  reply to a `res/get` request. It could never see `stop`, so every rotator move rescheduled the
  finalizer immediately and the driver queried the controller in a tight loop for as long as the
  device stayed connected. It now reads `res/get/MOT2/STATUS/MST`, tolerates a missing field,
  repolls with a 0.2 s delay and reports a failed query as ALERT instead of leaving the property BUSY.
- `GET_MOT2_ERROR` pointed at `MOT1/ERROR`, so a rotator fault was never reported and a focuser fault
  was reported twice.

`X_CALIBRATE_A` additionally stayed BUSY forever: its `on_change` referenced a `_finalizer`, which
makes the generator suppress the final update, and the shared movement finalizer only completes
`ROTATOR_POSITION`. The controller runs the calibration on its own and only acknowledges the
request, so the handler now publishes the acknowledgement as the completion.

## Automated test coverage (2026-09-20)

`indigo_test/integration/test_focuser_primaluce_simulator.c` was extended from two smoke tests to the
full focuser and rotator class standards in `indigo_test/DRIVER_TESTING_RULES.md`. Every scenario
runs in its own forked process against a freshly started simulator, so no state leaks between cases.

The simulator `focuser_primaluce_simulator/focuser_primaluce_simulator.c` gained the fixtures the
standard needs and nothing else:

- `--profile <normal|esatto|sestosenso3|no-abs-pos|no-speed|unsupported|old-firmware|needs-calibration|external-motion>`
  selects the controller the scenario connects to.
- `INDIGO_PRIMALUCE_EVENTS` records every accepted request, so a test can assert the JSON the driver
  actually sent instead of only the resulting property state.
- `INDIGO_PRIMALUCE_FAULT` arms a one-shot `silent`, `garbage` or `close` fault on the next request
  containing a given fragment.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Capabilities and readback | `metadata`, `property_contract`, `status_polling` |
| Model dependent capabilities | `esatto_profile`, `sestosenso3_profile`, `no_abs_pos_profile`, `no_speed_profile`, `old_firmware`, `needs_calibration` |
| Initialization failures | `unsupported_device`, `handshake_timeout` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` |
| Motion, units and sign | `absolute_move`, `relative_move`, `relative_move_clamped` |
| Stop and overlapping requests | `abort_motion`, `abort_while_idle`, `overlap_rejected` |
| Command failures | `move_command_failures`, `settings_reported_failures` |
| Externally changed position | `external_motion_observed` |
| Disconnect during motion | `disconnect_during_motion` |
| Modes and controls | `controller_settings`, `run_preset`, `focuser_calibration` |
| Transport loss | `transport_loss` |
| Rotator identity and lifecycle | `rotator_metadata`, `rotator_lifecycle` |
| Shared connection and ownership | `shared_connection` |
| Rotator motion, stop and calibration | `rotator_move`, `rotator_abort`, `rotator_calibration`, `rotator_move_failure` |

### Notes and gaps

- `ROTATOR_ON_POSITION_SET` is hidden on purpose, so the shared rotator class completeness assertion
  does not apply; `rotator_metadata` asserts the concrete properties and the absence of that one.
- `FOCUSER_MODE`, `FOCUSER_COMPENSATION` and `FOCUSER_REVERSE_MOTION` stay hidden; the controller has
  no temperature compensation and no reverse switch of its own.
- `FOCUSER_STEPS.on_change` still contains the statement
  `FOCUSER_POSITION_PROPERTY->state = FOCUSER_POSITION_PROPERTY->state;`, which does nothing. It was
  left alone because no observable behaviour depends on it; the relative move is covered by
  `relative_move` and `relative_move_clamped`.
- The focuser movement finalizer reschedules itself with `indigo_execute_handler`, so a focuser move
  is polled without any delay for as long as it runs. The rotator finalizer was given a 0.2 s delay
  as part of the fix above; the focuser one was left as it is, because changing it is a behaviour
  change outside the defects listed here.
- The driver keeps no persistent settings of its own, so there is no `CONFIG` roundtrip to test.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no SestoSenso, Esatto or ARCO was
  available.

```sh
make -C indigo_test build/integration/test_focuser_primaluce_simulator
cd indigo_test && ./build/integration/test_focuser_primaluce_simulator
```

- Simulated tests run: 35; passed: 35.
- Hardware tests run: 0; passed: 0.
