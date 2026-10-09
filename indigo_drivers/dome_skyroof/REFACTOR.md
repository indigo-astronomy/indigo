# INDIGO 3.0 refactoring record for `dome_skyroof`

The driver's migration to `indigo_generator` predates this file and is not reconstructed here. Only the work below
is recorded.

## 2.0 behaviour restored (2026-10-09, 3.0.0.11)

Compared with the 2.0 driver (`indigo_dome_skyroof.c` before the generator migration), the generated driver had
lost or changed the following; all of it is restored:

- `X_MOUNT_PARK_STATUS` was gone. The connection again reads `Parkstatus#` and publishes the mount park sensor
  (`0#` lights `STATUS`, `1#` leaves it idle); an answer that is neither refuses the connection, as in 2.0.
- The heater switch was renamed from `X_HEATER_CONTROL` to `HEATER_CONTROL`, which broke clients and saved
  configurations; it is `X_HEATER_CONTROL` again.
- The model "Interactive Astronomy SkyRoof" and the roof labels ("Roof state", "Roof opened", "Roof closed",
  at-most-one rule) were missing.
- The connection accepted any answer to `Status#`; 2.0 refused anything but `RoofOpen#`, `RoofClosed#` and
  `Safety#`, and so does the driver again.
- A roof move whose `Status#` went unanswered stayed BUSY for good, and an unknown status was polled forever; 2.0
  ended both in ALERT. Both end in ALERT again, and a move that does not arrive within 300 s too.
- No command discarded pending input, so an unexpected byte shifted every later reply; each command now discards
  first. The shutter request is read from its target.

The C simulator gained `--park-status 0|1` and `--status-fault silent|garbage` (the first `Status#` after the first
move). New cases `mount_park_status_is_read_at_connect` and `lost_or_unknown_status_alerts_the_roof`; the
metadata and heater cases check the restored names. Against 3.0.0.10 four cases fail (metadata, heater, park
status, lost status); against 3.0.0.11 all 13 pass (recorded, macOS arm64).

New hardware suite `indigo_test/hardware/test_dome_skyroof_hw.c` (`make test-dome-skyroof-hw`, `SKYROOF_HW_PORT`)
on `dome_hw_common.h`: model, labels and the park sensor, open and close, an aborted move closed from where it
stopped, the heater, reconnect and INIT/SHUTDOWN. Recorded run against `dome_skyroof_simulator.ino` on an ESP32-S3:
7/7. No physical SkyRoof is available.
