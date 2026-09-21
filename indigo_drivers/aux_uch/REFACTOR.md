# INDIGO 3.0 refactoring record for `aux_uch`

The driver's migration to `indigo_generator` predates this file and is deliberately not
reconstructed here. Only the work below is recorded, so nothing in this file is inferred history.

## Poll versus a pending change (2026-09-21)

### Defect

| Defect | Impact | Root cause | Fix | Test |
| --- | --- | --- | --- | --- |
| UCH-01 | A switched USB port silently reverts and the hub is commanded back to its old state. | The polling timer copied the hub's port states into `AUX_USB_PORT` without checking whether a change request had already been accepted. `INDIGO_COPY_*_PROCESS_CHANGE` copies the client's values and publishes BUSY before the queued handler runs, so a status frame that was already in flight overwrote the requested state, and the handler then sent that stale state on to the hub. | The poll skips `AUX_USB_PORT` while it is BUSY. | `a_change_survives_a_status_frame_in_flight` |

The defect was not found by reading this driver but by sweeping every generated driver's `on_timer`
for assignments into a writable property's items without a BUSY guard, after the same defect was
found in `aux_upb` (UPB-02) and `aux_ppb` (PPB-03). The same sweep also found it in `aux_usbdp` and
`aux_cloudwatcher`.

### Regression test

The race is only reachable while a status read is outstanding, so the simulator gained
`--slow-status <ms>`, which holds the status reply long enough for the change request to land
inside that window every time. `a_change_survives_a_status_frame_in_flight` switches the first port
four times through that window and checks that the value still holds after the in-flight poll has
published.

Verified both ways: the case fails against the driver with the guard removed and passes with it in
place.

Driver version is now `0x03000006`.

```sh
cd indigo_test && ./build/integration/test_aux_uch_simulator
```

- Simulated tests run: 11; passed: 11.
- Hardware tests run: 0; passed: 0. No USB Control Hub was available.
