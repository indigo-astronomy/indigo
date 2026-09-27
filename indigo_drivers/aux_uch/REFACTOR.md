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

## USB port requests read from their targets (2026-09-27, 3.0.0.8)

Findings TGT-052 and the aux_uch part of TGT-B07 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`.

### Defects

- TGT-052: the poll checks AUX_USB_PORT for BUSY (the UCH-01 guard) and then writes the port states
  the hub reports into the values. A request copied on the bus thread between that check and the write
  was overwritten, and the handler, which read `sw.value`, sent the hub its current state and reported
  OK. The handler also ignored a missing reply and published the request as OK.
- TGT-B07: the poll publishes AUX_USB_PORT OK at its end, after the `PC` uptime round trip, without
  checking BUSY again, so a request copied during that round trip was shown OK before its handler sent it.

### Fix

- The poll records the port states the hub reports in private data on every poll (the connect seeds
  them) and adopts them into the values only while the property is not BUSY, as before; it publishes
  AUX_USB_PORT only when it is still not BUSY at the end of the poll.
- AUX_USB_PORT (any-of-many) copies the values into the targets in `on_change_request` while not BUSY,
  because the poll writes values only and the ports a request does not carry would otherwise keep a
  stale target (the aux_upb v2 pattern). The handler sends each target in the original order
  (`U1`..`U6`) and writes an answered one into the value; a port the hub did not answer for shows the
  state last reported with ALERT.
- Simulator: `--fault-once <command>` (the hub ignores the command once, no reply, nothing changes),
  `--slow-uptime <ms>`, and `PE:bbbbbb` now sets only the power status on boot, which a reboot (`PF`)
  restores, as the command table describes, instead of switching the ports at once.

### Verification (Linux x64)

- Unchanged 3.0.0.7 suite: 11/11 passed before the change.
- The TGT-052 check-to-write window has no I/O or log line inside it, so no permanent case can hit it.
  A temporary instrumented copy of the generated driver (debug line and 200 ms pause right after the
  poll's check, PORT_1 OFF request sent from that line by a temporary case; neither committed) sent
  `U1:1`, published ON as the first OK and left the port on in 3/3 runs on 3.0.0.7, and sent `U1:0`
  with OFF as the first OK, the port off, in 3/3 runs on 3.0.0.8.
- New cases, each failing on 3.0.0.7 and passing on 3.0.0.8:
  `usb_port_failure_shows_the_hub_state` (TGT-052: ALERT with the reported state, and a later request
  for another port does not resend the refused one) and
  `b07_request_copied_after_the_check_is_published_by_its_handler` (TGT-B07: a reboot restores port 1
  off, the request is sent from the log line of the delayed `PC`; the first OK after it must follow the
  handler's `U2:0`). The B07 case also fails 3/3 on 3.0.0.8 with only the end-of-poll check removed.
- Regeneration from the `.driver` is reproducible. Recorded run through `tools/run_driver_test.py
  aux_uch`: 13/13.
- The poll publishes every port change it adopts, so there is no TGT-D15 equivalent.
- Not verified on hardware.

## Final test summary

- Simulated tests run: 13; passed: 13 (recorded run of `test_aux_uch_simulator` through
  `tools/run_driver_test.py`, driver version 8, Linux x64).
- Hardware tests run: 0; passed: 0. No USB Control Hub was available.
