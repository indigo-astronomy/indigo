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

## Hardware acceptance run (2026-09-22)

A physical USB_Focus v3 with firmware 1321 (`0461:0033`, USB strings `CCS` / `SERIAL DEMO`,
`/dev/cu.usbmodem2401`) was connected to a Mac mini (macOS, arm64) and driven through the real
driver by the new opt-in suite `indigo_test/hardware/test_focuser_usbv3_hw.c`:

```sh
make -C indigo_test test-focuser-usbv3-hw
```

The run exposed four defects, all fixed below; the final state passes 16 of 16 hardware cases
twice in a row and 27 of 27 simulator cases. Driver version 9 → 10.

The suite restores everything it changes - position, speed, travel limit, compensation, threshold,
step size, mode and reverse motion - in its own cleanup, so a run that stops halfway does not hand
the next one a narrowed travel limit. The first failing runs did exactly that and their follow-up
failures were all cascades of it.

### What the unit actually speaks

Recorded byte for byte with a probe outside the driver, because the simulator had been written
from the driver's parsing rather than from the device:

| Command | Reply of firmware 1321 | What the simulator sent before |
| --- | --- | --- |
| `SWHOIS` | `UFO\n\r` | `UFO\n` |
| `SGETAL` | `C=0-1-2-012-002-1321-30000\n\r`, fields zero padded | unpadded fields |
| `FPOSRO` | `P=00306\n\r`, five digits | `P=306\n` |
| `FTMPRO` | `T=+22.56\n\r`, signed, two decimals | `T=22.5\n` |
| `FTxxxA` | `A=0` or `A=1`, **no line ending at all** | `A=1\n` |
| `FMANUA` | `!\n\r` | `P=306\n` |
| `FAUTOM` | `A\n\r` | `A=1\n` |
| `FLXnnn`, `FZSIGn`, `SMAnnn` | `DONE\n\r` | `OK\n` |
| `SMOnnn`, `Mnnnnn` | `DONE\n\r` | nothing |
| `SMSTPF`, `SMSTPD` | nothing | nothing |
| `Onnnnn`, `Innnnn` | `*\n\r` on its own when the move ends | `*` in front of the next `FPOSRO` |
| `FQUITx` | `*\n\r` | `*\n` |

Every line of that table is now in `focuser_usbv3_simulator/focuser_usbv3_simulator.c`, including
the unterminated `FTxxxA` reply and the completion marker the unit sends on its own.

### Found defects

#### DRV-USBV3-101 — the compensation sign was read from a torn buffer

- **Impact**: a negative temperature compensation came back positive after a reconnect, and every
  connection stalled five seconds.
- **Cause**: `FTxxxA` is answered with a bare `A=0` or `A=1` and no line ending. The read only knew
  a terminator, so its second byte wait was the blocking one: it sat for the port's own five
  seconds, returned a failure and left the buffer unterminated, so the caller parsed the new reply
  followed by the tail of the previous one - `A=10306` out of `A=1` over `P=00306` - and read the
  sign as 10306.
- **Fix**: `usbv3_command()` reads with `indigo_uni_read_section2()` and an explicit 100 ms
  inter-byte timeout, and clears the buffer before every request.
- **Regression test**: the simulator sends `FTxxxA` unterminated, so `negative_compensation` and
  `controller_settings` now fail without the fix.

#### DRV-USBV3-102 — an unread DONE was delivered to the next command

- **Impact**: after a speed or travel-limit change, the next position readback returned `DONE` and
  the driver published a failed move; every later reply stayed one behind.
- **Cause**: the unit answers `SMOnnn` and `Mnnnnn` with `DONE`, and both were sent without
  reading a reply.
- **Fix**: both are sent with a reply now, like the compensation commands that always were.
- **Regression test**: the simulator answers both with `DONE`; `controller_settings` and
  `limits_clamp` cover them.

#### DRV-USBV3-103 — a stale completion marker ended the next move early

- **Impact**: aborting a move and moving back left the focuser about fifteen steps off, and a move
  started while the unit was already moving on its own was published as finished a tenth of a
  second in.
- **Cause**: the unit answers `FQUITx` with the same bare `*` it sends when a move ends. Nothing
  read it, so the next motion poll took it as its own completion.
- **Fix**: `usbv3_quit()` sends the quit command and reads its marker, and a new move drops
  whatever is still pending before it starts, which covers a move the driver did not start.
- **Regression test**: `external_motion_observed` and `abort_motion`, with the simulator now
  sending the marker on its own when a move ends.

#### DRV-USBV3-104 — a dropped byte turned a readback into a plausible wrong number

- **Impact**: the driver published a wrong position or a wrong configuration, silently. Observed
  as a travel limit of 3000 instead of 30000 and a compensation of 1 instead of 12.
- **Cause**: the link drops one byte of a long reply about once in twenty to forty reads. Measured
  through the driver's reader and through a plain POSIX reader with a two second collection
  window and millisecond inter-byte gaps, so the byte is lost rather than late: `28 bytes` became
  `C=0-1-2-012-0021321-30000`, one separator short. What is left still parses.
- **Fix**: the configuration is read twice and accepted only when both reads agree; a position
  reply has to be `P=` followed by the digit width the session established, and is asked again up
  to three times.
- **Regression test**: the simulator grew a `truncate` fault that drops a byte out of the middle
  of the next reply, and `truncated_reply_rejected` drives it for a position readback and for the
  configuration line. `motion_readback_failure` now also proves that one lost reply is retried
  while a persistent failure is still reported.

### Scenario to test mapping

| Hardware acceptance area | Scenario |
| --- | --- |
| Port detection, identity, configuration readback | `usbv3_reports_identity_and_capabilities` |
| Published property contract and ranges | `usbv3_publishes_the_property_contract` |
| Temperature readback | `usbv3_reads_the_temperature` |
| Relative moves in both directions | `usbv3_moves_relative_in_both_directions` |
| Absolute moves, including a move to the current position | `usbv3_moves_to_an_absolute_position` |
| Motion request refused while a move runs | `usbv3_rejects_an_overlapping_move` |
| Abort during a move, position after the abort, fresh move | `usbv3_aborts_a_move` |
| Speed written and read back over a reconnect | `usbv3_changes_the_speed` |
| Step size written, moved with, read back | `usbv3_changes_the_step_size` |
| Travel limit written, clamping a move, read back | `usbv3_changes_the_travel_limit` |
| Compensation and threshold, both signs, read back | `usbv3_changes_the_temperature_compensation` |
| Automatic and manual mode | `usbv3_switches_to_automatic_mode_and_back` |
| Reverse motion | `usbv3_reverses_the_motion` |
| Disconnect during a move | `usbv3_survives_a_disconnect_during_a_move` |
| Reconnect, repeated disconnect, identity again | `usbv3_reconnects` |
| INIT/SHUTDOWN, shutdown refused while connected | `usbv3_reinitializes` |

### Notes and remaining gaps

- That the driver polls the temperature again after manual mode is not asserted from a new
  reading: the framework drops an update that changes nothing and the unit holds the same
  hundredth of a degree for minutes, so such an assertion passes only while the room temperature
  drifts. The mode, the reading the driver holds and a move after the roundtrip are asserted
  instead.
- A damaged temperature reply is skipped rather than retried; it is a displayed value and the next
  poll replaces it.
- Physical hot-plug was not part of this run.
- The first command after a port that has been idle can take a full second to be answered, which
  is why the first-byte timeout is three seconds.

- Simulated tests run: 27; passed: 27.
- Hardware tests run: 16; passed: 16, twice in a row, against a physical USB_Focus v3 (firmware
  1321) on macOS arm64.
