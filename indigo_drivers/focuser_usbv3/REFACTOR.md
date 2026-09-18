# INDIGO 3.0 refactoring record for `focuser_usbv3`

This record was created together with the 2026-09-18 `reject_change` work. The driver's earlier
migration to `indigo_generator` predates this file and is deliberately not reconstructed here; only
the change below is recorded, so nothing in this file is inferred history.

## Rejected-change regression coverage (2026-09-18)

A change request that arrives while a motion is already running is now refused with the generator's
`reject_change` block on `FOCUSER_POSITION` and `FOCUSER_STEPS`, with the condition that the other motion property is BUSY and the message
shown in the generated driver. The generated guard marks every item for update, sets
`INDIGO_ALERT_STATE` and publishes the property, so the client receives the actual driver-side values
instead of an update that carries no items.

Both motion properties are published BUSY together at motion start, so `INDIGO_COPY_*_PROCESS_CHANGE` dropped a concurrent request silently: the client received no update at all and kept showing the refused target. The guard runs before that macro and turns the silent drop into an explicit refusal.

Driver version is now `0x03000007`. Regression coverage is the existing suite in
`indigo_test/integration/test_focuser_usbv3_simulator.c`, which was re-run after the change.

```sh
cd indigo_test && ./build/integration/test_focuser_usbv3_simulator
```

- Simulated tests run: 1; passed: 1.
- Hardware tests run: 0; passed: 0.

## Reconnect defect found and fixed (2026-09-18)

`test_focuser_usbv3_motion` failed roughly one run in three at the reconnect step with
`connect_serial_device()` returning false. `on_disconnect` sends `FQUITx`, which the device answers
with `*`, and `usbv3_command()` is called there with `response = false`, so nothing reads that
reply. The bytes stay in the port buffer across close/open, and on reconnect the `SWHOIS` handshake
read consumed a stale line instead of `UFO`. The driver tolerates exactly one leading `*`, so a
single stale line was absorbed, but the abort path can leave a second one.

`usbv3_open()` now discards pending input before the identity handshake. A discard inside
`usbv3_command()` was rejected deliberately: motion completion is signalled by an asynchronous `*`
that the driver detects in the `FPOSRO` reply, and discarding before every command would drop it on
real hardware. Tracked as `DRV-197`.

Verified on macOS arm64/x86_64: 12 consecutive `test_focuser_usbv3_motion` runs passed after the
fix, against 2 of 3 before it. Driver version incremented to 8; the same change also reserves space
for the terminating NUL in both `indigo_uni_read_section()` calls (`DRV-198`).
