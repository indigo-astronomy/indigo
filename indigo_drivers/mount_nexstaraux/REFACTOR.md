# INDIGO 3.0 refactoring record for `mount_nexstaraux`

This record was created with the 2026-09-20 test coverage work. The driver's earlier migration to
`indigo_generator` predates this file and is deliberately not reconstructed here; only the changes
below are recorded, so nothing in this file is inferred history.

## Defects found while writing the test suite (2026-09-20)

Five defects were found and fixed. Driver version is now `0x0300000C`.

- **One lost reply killed the connection.** `nexstaraux_command()` read the answer with
  `indigo_uni_read()`, which on a socket with a receive timeout reports the timeout through
  `read_data()`. That records the timeout in `handle->last_error`, and every later read *and write*
  on the handle then fails immediately without touching the socket. A single unanswered request
  therefore silenced the mount for the rest of the session: polling stopped, no further command was
  sent, and the device still reported itself connected. The reads now wait for data first, so a
  missing answer is a recoverable timeout. The same rewrite removed a path through the reply loop
  that neither returned nor made progress when a header byte could not be read.
- **Losing the connection crashed the driver.** `nexstaraux_validate_handle()` passed
  `device->master_device` to `indigo_execute_handler()`. The mount is its own master and that pointer
  is NULL, so a dropped socket dereferenced NULL. It now falls back to the device itself.
- **Unparking parked the mount.** `MOUNT_PARK.on_change` ran the park slew for both items, so
  selecting UNPARKED slewed to the pole and parked. Unparking now only releases the mount and
  publishes the result, which the handler has to do itself because its slew branch references a
  finalizer and the generator suppresses the final update.
- **The guider stopped the wrong axis.** `guider_guide_ra_finalizer()` stopped the altitude axis and
  published `GUIDER_GUIDE_DEC`, and `guider_guide_dec_finalizer()` did the opposite, so every pulse
  stopped the axis it had not started. Each finalizer now ends its own axis.
- **Pulses shorter than a second were not timed, and failures were never reported.** The guide
  handlers scheduled the finalizer with `duration / 1000`, an integer division, so a 300 ms pulse
  expired immediately and a 1500 ms pulse lasted one second. They also set `INDIGO_ALERT_STATE`
  without publishing it, and the generator suppresses the final update for a handler that references
  a finalizer, so a refused pulse stayed BUSY forever. Both handlers now use the fractional duration
  and publish their own start and error states.

## Automated test coverage (2026-09-20)

`indigo_test/integration/test_mount_nexstaraux_simulator.c` was extended from two smoke tests to the
full mount and guider class standards in `indigo_test/DRIVER_TESTING_RULES.md`. Every scenario runs
in its own forked process against a freshly started simulator, so no state leaks between cases.

The simulator `mount_nexstaraux_simulator/mount_nexstaraux_simulator.c` was given a real motion
model, because it previously completed every slew instantly and could not report a mount in motion:

- A slew and a rate move are both a signed speed in encoder units per second, so `MC_SLEW_DONE`
  reports a motion that really takes time and a slew can be interrupted.
- `--profile <normal|no-version|slow-slew>` selects an unresponsive motor controller or a slow mount.
- `INDIGO_NEXSTARAUX_EVENTS` records the destination, command and payload of every accepted request,
  so a test can assert the bytes the driver put on the wire.
- `INDIGO_NEXSTARAUX_FAULT` arms a one-shot `silent`, `garbage` or `close` fault on the next request
  to a given destination and command. The `garbage` action answers with a reply for a command nobody
  asked about, which the driver has to skip.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Identity and property contract | `metadata`, `property_contract` |
| Initialization failures | `handshake_rejected`, `handshake_timeout` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` |
| GOTO and SYNC | `sync_coordinates`, `slew_to_coordinates`, `slew_command_failure`, `sync_command_failure` |
| Stop and abort | `abort_slew`, `abort_while_idle`, `abort_command_failure` |
| Manual motion and rates | `manual_motion`, `manual_motion_rates`, `manual_motion_failure` |
| Tracking and rates | `tracking`, `tracking_failure`, `guide_rate` |
| Park and home | `park_and_unpark`, `abort_park` |
| Polling and stale replies | `coordinate_polling`, `stale_reply_skipped` |
| Transport loss | `transport_loss` |
| Guider | `guider_metadata`, `guider_pulses`, `guider_pulse_failure`, `guider_rate` |
| Shared connection and ownership | `shared_connection` |

### Notes and gaps

- The mount has no park position, home command or side-of-pier report of its own, so those rows of
  the class standard do not apply; their absence is asserted in `property_contract`.
- `MOUNT_HORIZONTAL_COORDINATES` is published by the base driver from the equatorial readback, so it
  is only checked for visibility.
- The network autodiscovery branch of `nexstaraux_open()` is not exercised: the test points
  `DEVICE_PORT` straight at the simulator, and a passive discovery broadcast is not hardware free.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no SkyPortal module was available.

```sh
make -C indigo_test build/integration/test_mount_nexstaraux_simulator
cd indigo_test && ./build/integration/test_mount_nexstaraux_simulator
```

- Simulated tests run: 30; passed: 30.
- Hardware tests run: 0; passed: 0.
