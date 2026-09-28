# ZWO AM mount refactoring and validation record

Status: first automated coverage added on 2026-09-18. The driver is already on the 3️⃣ API with
async queues and Windows support, but had no automated tests and was not retested; this record
covers the new simulator suite, not a completed refactoring.

## Current-state audit

- `indigo_mount_asi.c` is hand-written (no `.driver` source). It speaks the LX200 dialect plus a
  small set of AM-series commands: `:GTa#` / `:STa...#` (meridian settings), `:GRl#` / `:SRl...#`
  (maximum slew speed), `:GAT#` (tracking status with error codes), `:NSC#` (clear alignment),
  `:R<n>#` (numeric slew rates) and `:SBu<n>#` (buzzer).
- `asi_detect_mount()` accepts the device only when `:GVP#` returns an `AM<digit>` product string,
  and `asi_init_mount()` gates the meridian properties and `MOUNT_ALIGNMENT_RESET` on firmware
  `>= 1.2.4`. `MOUNT_PARK` is deliberately hidden because parking leaves AM mounts in a state only
  the vendor application recovers from.
- The driver exposes two logical devices: the mount and a guider that shares the mount's connection.

## Test harness

`indigo_test/integration/test_mount_asi_simulator.c` drives the driver over a real PTY against the
shared LX200 simulator (`indigo_test/simulator_common/mount_lx200_simulator.c`), using a new `asi`
profile. That profile is the existing `zwo` profile plus:

- the AM-series commands listed above, all gated on `MODEL_ASI`;
- `:GV#` reporting `1.2.4` instead of `1.0.0`, so the firmware-gated properties are exercised.

`indigo_mount_lx200` never sends those commands, and the two pre-existing `MODEL_ZWO` comparisons
were replaced by a `model_is_zwo()` helper that covers both models, so the `zwo` profile behaves
exactly as before. The LX200 suite still passes 67 of 67 scenarios with the extended simulator.

## Scenario-to-test mapping

All 10 scenarios pass; three consecutive runs were clean.

| Class standard scenario | Test case |
| --- | --- |
| Mount interface bit, base and driver-specific visible properties, `MOUNT_INFO` vendor/model/firmware | `metadata_and_visible_properties` |
| Sync, then goto reporting BUSY and reaching the target | `slew_reports_busy_then_reaches_the_target` |
| Abort stops a running slew short of the target | `abort_stops_a_running_slew` |
| Tracking on/off, all track rates, all slew rates, both motion axes started and stopped | `tracking_rates_and_motion` |
| Guide rate, buzzer and maximum slew speed | `guide_rate_buzzer_and_max_slew_speed` |
| Meridian auto-flip, track-past and limit round trip through `:GTa#` / `:STa...#` | `meridian_settings_round_trip` |
| `MOUNT_ALIGNMENT_RESET` accepted | `alignment_reset_is_accepted` |
| Guider pulses on all four directions through the shared connection | `guider_pulses_complete` |
| A non-AM product string is refused with `INDIGO_ALERT_STATE` | `foreign_mount_is_refused` |
| Disconnect followed by reconnect re-runs initialization | `reconnect_restores_the_session` |

The abort scenario syncs to declination -80 before slewing to +80. The simulator slews at 45 deg/s
and the driver only learns its position from the poll, so a short slew finishes before the abort is
observable; over the full range the abort lands consistently near -49.

## Deferred work

- `MOUNT_PARK` and `MOUNT_HOME` motion are not exercised. Park is hidden by design; homing drives the
  simulator's reference motion and was left out of this pass.
- `:GAT#` tracking error codes are modelled by the simulator but no scenario injects one, so the
  driver's `asi_error_string()` mapping is not covered.
- Meridian limits outside -15..+15, `MOUNT_MODE` alt-az reporting, `MOUNT_SIDE_OF_PIER`,
  `MOUNT_TRACK_RATE` custom rates, epoch conversion and multi-instance behaviour are not covered.
- Transport loss mid-slew and reconnect during motion are not covered.
- Hardware testing was not performed. No hardware validation is claimed.

## Fixes applied in this pass

- `asi_command()` call sites passed `sizeof(response)` as the payload capacity for
  `indigo_uni_read_section2()`, which stores the terminating NUL at `buffer[length]`. All 34 call
  sites now reserve that byte. Tracked as `DRV-198`; driver version incremented to `0x0300001E`.

## Final test summary

- Simulated tests: 10 executed, 10 passed.
- Hardware tests: 0 executed, 0 passed.
