# INDIGO 3.0 refactoring record for `aux_usbdp`

The driver's migration to `indigo_generator` predates this file and is deliberately not
reconstructed here. Only the work below is recorded, so nothing in this file is inferred history.

## Protocol coverage build-out (2026-09-21)

The suite was two smoke cases - one per model - against a driver that exposes eleven properties and
serves both the USB_Dewpoint v1 and v2 controllers. It is now a suite of 16 cases.

Simulator extensions: fault injection per command (`--fault`, `--fault-once`) with `invalid`,
`short`, `silent` and `close` modes, and `--slow-status <ms>`, which holds the `SGETAL` reply so a
change request can be made to land while a status frame is in flight.

Test coverage added: independent duty cycles for all three heater channels, dew control, the
temperature calibration offsets, both dew thresholds, the channel 2-3 link, all four heater
aggressivity levels, unknown and silent identity, a vanished port, an unparsable status frame,
repeated disconnect, refused shutdown while connected, the documented heater stop on disconnect
with polling resuming afterwards, and the race below.

### Defect

| Defect | Impact | Root cause | Fix | Test |
| --- | --- | --- | --- | --- |
| USBDP-01 | A heater duty cycle, dew mode, calibration offset, threshold, channel link or aggressivity level silently reverts, and the controller is commanded back to its old setting. | The polling timer copied the status frame into those six writable properties without checking whether a change request had already been accepted. `INDIGO_COPY_*_PROCESS_CHANGE` copies the client's values and publishes BUSY before the queued handler runs, so a frame that was already in flight overwrote them and the handler sent the stale values on. | `usbdp_adopt()` re-checks the property state at each assignment, after the reply has been read. | `a_change_survives_a_status_frame_in_flight` |

The defect was not found by reading this driver but by sweeping every driver's periodic work for
assignments into a writable property without a BUSY guard, after the same defect was found in
`aux_upb` (UPB-02) and `aux_ppb` (PPB-03). The sweep also found it in `aux_uch` and
`aux_cloudwatcher`.

Verified both ways: the regression case fails against the driver with the guards removed ("Heater
reverted to 0 after the request for 40") and passes with them in place.

### Behaviour confirmed, not a defect

`on_disconnect` stops all three heaters on purpose, so a duty cycle does not survive a reconnect.
The first version of the reconnect case asserted that it did and failed; the protocol trace showed
the deliberate `S1O000`/`S2O000`/`S3O000` sequence on disconnect, and the case now pins the real
contract instead.

Driver version is now `0x0300000B`.

```sh
cd indigo_test && ./build/integration/test_aux_usbdp_simulator
```

- Simulated tests run: 16; passed: 16.
- Hardware tests run: 0; passed: 0. No USB_Dewpoint was available.
