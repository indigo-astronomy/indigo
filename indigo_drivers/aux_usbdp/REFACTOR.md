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

## Requests read from their targets (2026-09-27, 3.0.0.13)

Findings TGT-053, TGT-054, TGT-055, TGT-D01 and the aux_usbdp part of TGT-B07 of
`indigo_drivers/REVIEW_SWITCH_TARGETS.md`.

### Defects

- TGT-053 to TGT-055: the poll checks AUX_DEW_CONTROL, AUX_LINK_CH_2AND3 and AUX_HEATER_AGGRESSIVITY for
  BUSY with `usbdp_adopt()` (the USBDP-01 guard) and then writes the reported setting into the values. A
  request copied on the bus thread between that check and the write was overwritten, and the handler,
  which read `sw.value`, sent the controller its current setting and reported OK. The handlers also
  ignored a missing reply and published every request as OK.
- TGT-D01: `usbdp_adopt(P) && a != x || b != y || c != z` guarded only the first item, so a pending
  AUX_HEATER_OUTLET, AUX_CALLIBRATION or AUX_DEW_THRESHOLD request was overwritten without any guard
  whenever item 2 or 3 differed from the report, which a request for item 2 or 3 itself makes happen,
  and the handler, which read `number.value`, sent the reported values. Reproduced on 3.0.0.12: requests
  for heater #2 40, calibration #2 3 and threshold #2 5 sent from the log line of an in-flight status
  command ended as 0, 0 and 2.
- TGT-B07: the poll published every property it adopted a change for with OK at its end without checking
  BUSY again, so a request copied between its writes and its publications was shown OK before its handler
  sent it (all six writable properties).

### Fix

- The guards of the three number properties are parenthesized.
- The poll records every setting the controller reports in private data (seeded from the displayed
  settings when a v2 controller is identified) and adopts it into the values only while the property is
  not BUSY; it publishes the six writable properties only when they are still not BUSY at the end of
  the poll.
- AUX_DEW_CONTROL, AUX_LINK_CH_2AND3 and AUX_HEATER_AGGRESSIVITY (one-of-many) send the request read with
  `indigo_get_switch_target()` and apply it with `indigo_apply_switch_targets()` once the controller
  answered; otherwise they show the setting last reported with ALERT.
- AUX_HEATER_OUTLET, AUX_CALLIBRATION and AUX_DEW_THRESHOLD send `number.target` and write it into the
  value once the controller answered; otherwise value and target show the setting last reported with
  ALERT (per outlet for the heaters). Because the poll writes values only, `on_change_request` copies the
  values into the targets while not BUSY, so the items a request does not carry keep the reported
  setting instead of a stale target (the aux_upb pattern for any-of-many switches).
- The `requested_aggressivity` shadow is replaced by `aggressivity`, the level last reported or accepted.
  Its old job, keeping a pending request from being overwritten, is done by the target; what remains is
  the record a failed request is shown with, like the other settings. The poll now adopts a reported
  level when the switch does not show it instead of when it differs from the shadow; both are on the
  device queue and have no other reader, so the only visible difference is that an unchanged level is no
  longer republished on the first poll, and an unknown level is ignored instead of being published
  without a switch change.
- The order and manner of the protocol commands are unchanged (`S1O`..`S3O`, `SAUTO`, `SCA`, `STHR`,
  `SLINK`, `SAGGR`, one command per request, heaters in channel order).
- Simulator: `--set <name> <value>` starts the controller with stored settings or readings other than
  the defaults (the real controller keeps its settings in EEPROM), and `--fault-once` may be repeated.

### Verification (Linux x64)

- Unchanged 3.0.0.12 suite: 16/16.
- The TGT-053 to TGT-055 windows have no I/O or log line between check and write, so no permanent case
  can hit them. A temporary instrumented copy (debug line and 200 ms pause right after each check, the
  request sent from that line, controller starting AUTOMATIC, LINKED and aggressivity 5, not committed)
  sent `SAUTO1`, `SLINK1` and `SAGGR3` for MANUAL, NOT_LINKED and AGGRESSIVITY_1 and kept showing the
  old settings 3/3 on 3.0.0.12, and sent `SAUTO0`, `SLINK0`, `SAGGR1` and showed the requests 3/3 on
  3.0.0.13.
- New cases, each failing on 3.0.0.12 and passing on 3.0.0.13:
  - `d01_second_item_requests_survive_a_status_frame_in_flight` (TGT-D01; requests for item 2 sent from
    the driver's log line of `SGETAL`, answered 0.7 s late; item 1 must keep the reported value);
  - `request_failures_show_the_controller_state` (TGT-053 to TGT-055 and the number handlers; one silent
    reply per command, ALERT with the reported setting, then the same request succeeds);
  - `b07_requests_copied_before_the_publication_are_published_by_their_handlers` (TGT-B07; the first poll
    adopts all six settings and the requests are sent from its dew warning publication; it also fails
    with only the end-of-poll checks removed from the fix).
- Regeneration reproducible.

### Noticed, not changed

- The sensor comparison in the v2 poll tests `temp_ch1` twice instead of `temp_ch1` and `temp_ch2`, so a
  change of sensor #2 alone is published only with the next change of sensor #1. Found by reading while
  fixing the rows above; outside their scope, not reproduced. TGT-D16, fixed in 3.0.0.14, see below.

## Sensor #2 changes are published (2026-09-27, 3.0.0.14)

Finding TGT-D16 of `indigo_drivers/REVIEW_SWITCH_TARGETS.md`.

### Defect

The v2 status poll decided whether AUX_TEMPERATURE_SENSORS changed by comparing `temp_ch1` with sensor #1
twice; `temp_ch2` was never compared. A frame in which only sensor #2 changed was not written or
published, so clients kept the old sensor #2 reading until sensor #1 also changed (then both were
written). Reproduced with the simulator.

### Fix

- The second comparison uses `temp_ch2` and AUX_TEMPERATURE_SENSOR_2_ITEM. Nothing else changed: the same
  0.01 threshold, both items are still written together, no serial command changed. Regenerated with the
  unchanged generator; the only generated changes are that line and the version.
- Simulator test control `--set-after <n> <name> <value>`: from the status frame after the first `<n>`
  status replies the named setting or reading (the `--set` names) takes the value without a command,
  the way a reading changes on the controller.

### Verification (Linux x64)

- New case `d16_sensor_2_change_alone_is_published`: `--set-after 2 temp2 18.4`; sensor #1 23.5 and
  sensor #2 22.1 are published first, then sensor #2 must be published as 18.4 with sensor #1 still 23.5
  and the property OK. 3.0.0.13 failed 4/4 (sensor #2 stayed 22.10), 3.0.0.14 passed 3/3.
- Regeneration reproducible.
- Recorded run: 2026-09-27 14:54 3.0.0.14 linux x64 simulator 20/20 OK.

### Noticed, not changed

- The v1 branch and the weather comparison were checked and compare the right fields.
- Building the driver still prints the pre-existing `-Wformat-truncation` warnings for the
  AUX_DEW_WARNING and heater labels copied from the outlet names (`snprintf` of a 512-byte text into a
  128-byte label, truncated by design).

Driver version is now `0x0300000E`.

```sh
cd indigo_test && ./build/integration/test_aux_usbdp_simulator
```

- Simulated tests run: 20; passed: 20.
- Hardware tests run: 0; passed: 0. No USB_Dewpoint was available.
