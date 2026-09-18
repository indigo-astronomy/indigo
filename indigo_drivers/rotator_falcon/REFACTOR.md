# INDIGO 3.0 refactoring record for `rotator_falcon`

This record was created together with the 2026-09-18 `reject_change` work. The driver's earlier
migration to `indigo_generator` predates this file and is deliberately not reconstructed here; only
the change below is recorded, so nothing in this file is inferred history.

## Rejected-change regression coverage (2026-09-18)

A change request that arrives while a motion is already running is now refused with the generator's
`reject_change` block on `ROTATOR_POSITION` and `ROTATOR_RELATIVE_MOVE`, with the condition that the other motion property is BUSY and the message
shown in the generated driver. The generated guard marks every item for update, sets
`INDIGO_ALERT_STATE` and publishes the property, so the client receives the actual driver-side values
instead of an update that carries no items.

Before this change `ROTATOR_POSITION` published BUSY only for itself, so `ROTATOR_RELATIVE_MOVE` stayed idle during an absolute goto and a relative move request reached the controller in the middle of the running motion. This was a genuine concurrent-command defect, not only a reporting problem.

Driver version is now `0x03000007`. Regression coverage is the existing suite in
`indigo_test/integration/test_rotator_falcon2_simulator.c`, which was re-run after the change.

```sh
cd indigo_test && ./build/integration/test_rotator_falcon2_simulator
```

- Simulated tests run: 1; passed: 1.
- Hardware tests run: 0; passed: 0.
