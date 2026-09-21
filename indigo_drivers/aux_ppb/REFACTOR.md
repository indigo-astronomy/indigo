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

- Simulated tests run: 29; passed: 29.
- Hardware tests run: 0; passed: 0. No Pocket Powerbox was available.
