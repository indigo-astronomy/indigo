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

## Settings commands reported as successful when they failed (2026-09-20)

Writing the full test suite showed that `X_FOCUSER_STEP_SIZE`, `FOCUSER_COMPENSATION`,
`FOCUSER_MODE`, `FOCUSER_SPEED` and `FOCUSER_LIMITS` discarded the result of every command they
sent, so a controller that did not answer, or a port that had already died, still left the property
in `INDIGO_OK_STATE`. `DRIVER_TESTING_RULES.md` forbids exactly that: a failed request must not
silently become OK. Each of those handlers now sets `INDIGO_ALERT_STATE` when its command fails.
Driver version is now `0x03000009`; `settings_reported_failures` and `transport_loss` cover the
change.

`FOCUSER_SPEED`, `FOCUSER_LIMITS` and `X_FOCUSER_STEP_SIZE` use commands the controller does not
answer, and `usbv3_command()` deliberately does not discard pending input before writing, so only a
write failure can be detected for them. That is asserted through the transport loss scenario rather
than through a lost reply.

## Automated test coverage (2026-09-20)

`indigo_test/integration/test_focuser_usbv3_simulator.c` was extended from a single smoke test to
the full focuser class standard in `indigo_test/DRIVER_TESTING_RULES.md`. Every scenario runs in its
own forked process against a freshly started simulator, so no state leaks between cases.

The simulator `focuser_usbv3_simulator/focuser_usbv3_simulator.c` gained the fixtures the standard
needs and nothing else:

- `--profile <normal|configured|no-handshake|bad-config|external-motion>` selects the controller
  state a scenario connects to.
- `INDIGO_USBV3_EVENTS` records every accepted request, so a test can assert the six character
  command and its argument exactly as the driver put them on the wire.
- `INDIGO_USBV3_FAULT` arms a one-shot `silent`, `garbage` or `close` fault on the next request with
  a given prefix.

The simulator also had to be corrected: `SGETAL` reported the signed temperature compensation, while
the real controller reports the magnitude there and the sign through `FTxxxA`. The driver parses
that field with `%u`, so a negative value was read back with the wrong sign.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Capabilities and readback | `metadata`, `property_contract`, `configured_readback` |
| Initialization failures | `handshake_rejected`, `handshake_timeout`, `config_query_failure` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` |
| Motion, units and sign | `absolute_move`, `relative_move`, `reverse_motion`, `limits_clamp` |
| Stop and overlapping requests | `abort_motion`, `abort_while_idle`, `overlap_rejected` |
| Readback failure and recovery | `motion_readback_failure` |
| Externally changed position | `external_motion_observed` |
| Disconnect during motion | `disconnect_during_motion` |
| Modes and controls | `controller_settings`, `negative_compensation`, `focuser_mode` |
| Command failures | `settings_reported_failures` |
| Settings persistence | `configuration_roundtrip` |
| Polling and failure recovery | `temperature_polling` |
| Transport loss | `transport_loss` |

### Notes and gaps

- The driver polls only the temperature while it is idle, so an externally started motion is not
  observable as progress. `external_motion_observed` asserts what is observable instead: a relative
  move issued after such a motion is sent as the requested step count and the driver publishes the
  coordinate the controller really reached.
- `FOCUSER_BACKLASH` and `FOCUSER_ON_POSITION_SET` stay hidden; the controller has neither.
- `FOCUSER_REVERSE_MOTION` is the only driver-persisted setting. The `CONFIG` roundtrip runs against
  a scratch directory through the `indigo_uni_config_folder` override, so the user profile is
  untouched.
- The motion watchdog is 600 polls of 0.1 s, so a stuck motor is only reported after a minute. That
  path is not exercised, because a one minute scenario is out of proportion to the rest of the suite.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not rerun for this change; the earlier
  hardware result recorded above still stands.

```sh
make -C indigo_test build/integration/test_focuser_usbv3_simulator
cd indigo_test && ./build/integration/test_focuser_usbv3_simulator
```

- Simulated tests run: 27; passed: 27.
- Hardware tests run: 0; passed: 0.
