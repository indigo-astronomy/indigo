# INDIGO 3.0 refactoring record for `aux_ppb`

The driver's migration to `indigo_generator` predates this file and is deliberately not
reconstructed here. Only the work below is recorded, so nothing in this file is inferred history.

## Protocol coverage build-out (2026-09-21)

The suite was three smoke cases - one per model, each setting a handful of properties and checking
only that they reached `INDIGO_OK_STATE` - against a driver that serves four models over ten
properties. It is now a protocol suite of 29 cases.

Simulator (`aux_ppb_simulator.c`) extensions, audited against the driver's protocol handling: the
missing `PPBM` model, configurable input voltage, temperature, humidity and dewpoint, the power
alert flag, the initial auto dew state, the initial outlet states and the initial DSLR voltage, and
fault injection per command (`--fault`, `--fault-once`, `--fault-after N`) with `invalid`, `short`,
`silent` and `close` modes, so a fault can be aimed at the connect or at a later poll.

Test coverage added: the outlet inventory and hidden properties of all four models, the reported
model and firmware, every sensor of the status frame, the load dependent current, per outlet
switching with readback, the single outlet of the Smart Powerbox, the power alert light, all five
DSLR voltages and the one adopted from the device, independent heater duty cycles, manual and
automatic dew control including the Advance's non-echoing `PD` reply, heater renaming, the
momentary reboot and save-as-default switches, reconnect, repeated disconnect, refused shutdown
while connected, a second instance with its own model, unknown and silent identity, a vanished
port, a truncated and an unparsable status frame while polling, and transport loss.

### Defects found

| Defect | Impact | Root cause | Fix | Test |
| --- | --- | --- | --- | --- |
| PPB-01 | `AUX_POWER_OUTLET` never follows the device. An outlet switched off at the box, or an outlet state restored at power-up, is parsed and stored but never published, so a client keeps showing the wrong switch position. On the PPB and the Smart Powerbox nothing is published at all, because the property the code did update is hidden on those models. | The two status fields that carry the outlet switches set `updatePowerOutletState`, which publishes `AUX_POWER_OUTLET_STATE`; `updatePowerOutlet` was declared and tested but never assigned. | The outlet fields publish `AUX_POWER_OUTLET`; `AUX_POWER_OUTLET_STATE` keeps the power alert field only. | `current_falls_to_zero_without_a_load`, `each_power_outlet_switches_on_its_own`, `reconnect_resumes_polling` |
| PPB-02 | The identified model and firmware are never published. `INFO` keeps the "Unknown" placeholders the attach handler seeded for the whole session, and the only `INFO` update a client ever sees is the reset published on disconnect. | `ppb_open()` fills `INFO_DEVICE_MODEL_ITEM` and `INFO_DEVICE_FW_REVISION_ITEM` but never calls `indigo_update_property()` for `INFO`, unlike the sibling `aux_upb` driver. | Publish `INFO` after a successful identification. | `ppb_model_inventory`, `ppba_model_inventory`, `ppbm_model_inventory`, `spb_model_inventory`, `firmware_version_reaches_info` |
| PPB-03 | A switched outlet, heater level, dew mode or DSLR voltage can silently revert, and the device is commanded back to its old state. | The polling timer copies the device state into the properties without checking whether a change request has already copied the client's values and published BUSY; the queued handler then sends the stale values on. The same defect was found and fixed in `aux_upb` (UPB-02). | The poll skips a writable property that is BUSY. | `each_power_outlet_switches_on_its_own`, `heaters_hold_independent_duty_cycles`, `dslr_voltage_round_trips` |

Driver version is now `0x0300001D`.

```sh
cd indigo_test && ./build/integration/test_aux_ppb_simulator
```

## Switch requests read from their targets (2026-09-27, 3.0.0.31)

Findings TGT-043 to TGT-045 and the aux_ppb part of TGT-B07 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`.

### Defects

- TGT-043 to TGT-045: the status poll checks AUX_POWER_OUTLET, AUX_DSLR_POWER and AUX_DEW_CONTROL for BUSY
  and then writes the state the box reports into the values. A request copied on the bus thread between
  that check and the write was overwritten, and the handler, which read `sw.value`, sent the box its
  current state and reported OK. The handlers also ignored a missing reply and published the request as OK.
- TGT-B07: the end-of-poll publications of these three properties did not re-check BUSY, so the poll
  published the overwritten state (or, with the fix above alone, the state the request replaces) as OK
  while the request was still queued, and reopened the BUSY guard.
- The `updatePowerOutlet` flag that aux_upb recorded as TGT-D15 is set by the outlet fields here (PPB-01),
  so aux_ppb does not have that finding.

### Fix

- The poll records the outlet, DSLR voltage and dew states the box reports in private data on every poll
  (seeded from the displayed switches in `ppb_open()`, so a request failing before the first poll keeps
  them), adopts them into the values only while the property is not BUSY, as before, and publishes the
  three properties only when they are still not BUSY at the end of the poll.
- AUX_DSLR_POWER and AUX_DEW_CONTROL (one-of-many) send the target read with `indigo_get_switch_target()`,
  apply it with `indigo_apply_switch_targets()` once the box answered and otherwise show the state last
  reported with ALERT. The DSLR items are named after the voltages, so the poll and the handler use the
  item name for the `P2:<volts>` field; the bytes sent are unchanged.
- AUX_POWER_OUTLET (any-of-many) copies the values into the targets in `on_change_request` while not BUSY,
  because the poll writes values only and the item a request does not carry would otherwise keep a stale
  target (the mount_lx200 TGT-025 pattern, as in aux_upb). The handler sends each target in the original
  order (`P1`, then `P2` except on the Saddle box, whose property has one item) and writes an accepted one
  into the value; an outlet the box did not answer for shows the state last reported with ALERT.
- The AUX_HEATER_OUTLET publication (number property) is unchanged.

### Verification (Linux x64)

- Unchanged 3.0.0.30 suite: 29/29 passed before the change.
- The check-to-write windows have no I/O or log line inside them, so no permanent case can hit them. A
  temporary instrumented copy of the generated driver (debug line and 200 ms pause right after each poll
  check, request sent from that log line by a temporary case that records the first OK publication; neither
  committed) lost every request on 3.0.0.30 in 3/3 runs: OUTLET_1 OFF published ON and the outlet stayed
  powered, 12 V published and kept 5 V, MANUAL published and kept AUTOMATIC. On 3.0.0.31 every request was
  sent and published as requested in 3/3 runs, and the only OK after each request was the handler's.
  Without the end-of-poll re-checks the same copy published the overwritten state as OK before the
  handler's OK 3/3 for each property.
- New cases, each failing on 3.0.0.30 (no ALERT) and passing on 3.0.0.31:
  `power_outlet_failure_shows_the_box_state` (TGT-043, also a request for the other outlet after the
  failure does not resend the refused one), `dslr_power_failure_shows_the_box_state` (TGT-044),
  `dew_control_failure_shows_the_box_state` (TGT-045).
- Regeneration from the `.driver` is reproducible. Recorded run through `tools/run_driver_test.py
  aux_ppb`: 32/32.
- Not verified on hardware.

## Final test summary

- Simulated tests run: 32; passed: 32 (recorded run of `test_aux_ppb_simulator` through
  `tools/run_driver_test.py`, driver version 31, Linux x64). The earlier 29/29 run was on driver version 29.
- Hardware tests run: 0; passed: 0. No Pocket Powerbox was available.
