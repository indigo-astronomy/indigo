# INDIGO 3.0 refactoring record for `focuser_dmfc`

This record was created together with the 2026-09-18 `reject_change` work. The driver's earlier
migration to `indigo_generator` predates this file and is deliberately not reconstructed here; only
the changes below are recorded, so nothing in this file is inferred history.

## Rejected-change regression coverage (2026-09-18)

A change request that arrives while a motion is already running is now refused with the generator's
`reject_change` block on `FOCUSER_POSITION` and `FOCUSER_STEPS`, with the condition that the other motion property is BUSY and the message
shown in the generated driver. The generated guard marks every item for update, sets
`INDIGO_ALERT_STATE` and publishes the property, so the client receives the actual driver-side values
instead of an update that carries no items.

Both motion properties are published BUSY together at motion start, so `INDIGO_COPY_*_PROCESS_CHANGE` dropped a concurrent request silently: the client received no update at all and kept showing the refused target. The guard runs before that macro and turns the silent drop into an explicit refusal.

Driver version is now `0x03000011`.

## Automated test coverage (2026-09-20)

`indigo_test/integration/test_focuser_dmfc_simulator.c` was extended from a single smoke test to the
full focuser class standard in `indigo_test/DRIVER_TESTING_RULES.md`. Every scenario runs in its own
forked process against a freshly started simulator, so no state leaks between cases.

The simulator `focuser_dmfc_simulator/focuser_dmfc_simulator.c` gained the fixtures the standard
needs and nothing else:

- `--profile <normal|configured|no-handshake|bad-status|external-motion>` selects the controller
  state a scenario connects to.
- `INDIGO_DMFC_EVENTS` records every accepted request, so a test can assert the command, its
  argument and its sign instead of only the resulting property state.
- `INDIGO_DMFC_FAULT` arms a one-shot `silent`, `garbage` or `close` fault on the next request with a
  given prefix, which is how lost acknowledgements, malformed replies and transport loss are injected.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Capabilities and readback | `metadata`, `property_contract`, `status_readback` |
| Initialization failures | `handshake_rejected`, `handshake_timeout`, `status_query_failure` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` |
| Motion, units and sign | `sync_and_goto`, `goto_no_op`, `relative_move`, `limits_clamp_goto` |
| Stop and overlapping requests | `motion_progress_and_abort`, `abort_while_idle`, `overlap_rejected` |
| Externally changed position | `external_motion_observed` |
| Disconnect during motion | `disconnect_during_motion` |
| Modes and controls | `controller_settings`, `settings_reported_failures` |
| Settings persistence | `limits_configuration_roundtrip` |
| Polling and failure recovery | `temperature_polling`, `poll_failure_recovery` |
| Transport loss | `transport_loss` |

### Notes and gaps

- `FOCUSER_MODE` and `FOCUSER_COMPENSATION` stay hidden; the controller has no temperature
  compensation of its own, so the compensation rows of the class standard do not apply. Their absence
  is asserted in `property_contract`.
- The driver publishes `INFO` while opening the port and parses the model and firmware from the `A`
  status line afterwards without republishing, so `metadata` reads the parsed identity through a
  fresh enumeration. A client that connects and then enumerates sees the correct values; a client
  watching only updates keeps the handshake placeholder until it re-enumerates.
- `FOCUSER_LIMITS` is the only driver-persisted setting. The `CONFIG` roundtrip runs against a
  scratch directory through the `indigo_uni_config_folder` override, so the user profile is untouched.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no DMFC or FocusCube controller was
  available.

```sh
make -C indigo_test build/integration/test_focuser_dmfc_simulator
cd indigo_test && ./build/integration/test_focuser_dmfc_simulator
cd indigo_test && ./build/integration/test_focuser_dmfc_motion
```

- Simulated tests run: 25; passed: 25.
- Hardware tests run: 0; passed: 0.

## Regenerated for the shared refusal and connect macros (2026-09-21)

No behaviour change and no version bump. `indigo_generator` stopped emitting the eight-line refusal
block and the five-line CONNECTION admission block inline and now emits `INDIGO_REJECT_CHANGE_IF()`
and `INDIGO_PROCESS_CONNECT()` / `INDIGO_PROCESS_QUEUED_CONNECT()` instead, so this driver was
regenerated along with the other 114 generator inputs. The macros expand to exactly the statements
that were written out before; for a sample of four drivers the preprocessed translation unit is
byte-identical apart from the new `indigo_reject_change()` declaration and shifted `assert()` line
numbers. Background and the defect that motivated the refusal macro are in `indigo_drivers/REVIEW.md`
(DRV-213, DRV-214) and `indigo_libs/REVIEW.md` (LIB-015, LIB-016).

Verification: the complete simulator suite was re-run after regeneration —
`indigo_test/build/integration/test_focuser_dmfc_simulator`, macOS arm64, 24/24 passed on 2026-09-21 12:44.

## Queued position request overwritten by the poll (3.0.0.19, 2026-09-27)

- **Defect:** the poll set FOCUSER_POSITION and FOCUSER_STEPS OK whenever `I` reported no motion, also while a
  position or steps request was copied (BUSY) but its handler had not sent `M:` / `G:` yet. The request was shown
  OK until the handler set BUSY again, and a second motion request arriving in that window was accepted instead of
  refused as overlapping. Seen as the intermittent `overlap_rejected` failure (FOCUSER_STEPS did not reach ALERT).
- **Fix:** `PRIVATE_DATA->moving` records a motion the controller runs: set when `M:` or `G:` was sent, when `I` or
  the connect status reports motion, cleared by an idle `I` and by `H`. The poll ends BUSY only for such a motion.
- **Simulator:** the fault action `slow` answers the next matching command 0.5 s late with the state from before
  the delay.
- **Regression test:** `request_survives_idle_status_read` sends a position request while the poll waits for a
  slow idle `I` reply and checks that FOCUSER_POSITION is never published OK before the abort and that a relative
  move is refused. Version 18 failed 3 of 3 runs, version 19 passed 5 of 5; the whole suite (25 cases) and
  `test_focuser_dmfc_motion` passed 3 of 3 runs on macOS arm64.

## Focuser testing rules alignment (3.0.0.20, 2026-10-05)

The suite was checked against the "Focuser Drivers" chapter of `indigo_test/DRIVER_TESTING_RULES.md` (commit fa5f64839) and the bundled `DMFC-Serial-Command-Table.pdf`. The simulator fault file gained `value=<reply>`, `ignore` (a command without reply taken but not applied), `stall` and `external=<target>`; the simulator answers `B` and follows the command table for `L` (`L:2` on, `L:1` off, reply `L:1`/`L:0`). The runner now plans its forked cases.

### Defects found and fixed

- `DMFC-1` An aborted move ended OK, and an abort with nothing moving sent `H` and republished the motion properties OK. User decision: an aborted move ends both motion properties ALERT at the stopped position (read back after `H`) with value equal to target; an idle abort, or one with the item OFF, sends nothing. Test: `motion_progress_and_abort`, `abort_while_idle`, `overlap_rejected`.
- `DMFC-2` Disconnecting during a move let the controller finish it. User decision: the disconnect sends `H` before the port closes. Test: `disconnect_during_motion` (one `H`, nothing afterwards, the reconnect finds the focuser stopped short and OK).
- `DMFC-3` The generator migration of 2025-05-18 opened the port at 9600 baud and sent `L:1`/`L:0` for the LED, while the command table and the driver before the migration use 19200 baud and `L:2` (on) / `L:1` (off). Both are restored. Test: `controller_settings`, `settings_readback` (the baud rate is not observable on a PTY).
- `DMFC-4` A failed status query at connect was ignored and the controller was published with driver defaults, and the speed was never read. The status line and `B` are mandatory now; a failure refuses the connection, resets the identity and releases the port. Test: `status_query_failure`, `connect_query_refused`, `status_readback`.
- `DMFC-5` Failed or malformed `P`, `I` and `T` replies were ignored or parsed as 0 (`atoi`, `indigo_atod`), and a reply cut off by the timeout was accepted. Replies must end with their line end and parse completely; an idle failure is ALERT with the last valid value and the next good poll restores OK; an implausible temperature is ALERT as well. Test: `poll_failure_recovery`, `temperature_failures`.
- `DMFC-6` A failed poll or a stalled motor during a move kept the move BUSY forever. One lost poll is retried, a second one, or five polls without progress, send `H` and end the move ALERT, which later idle polls keep. Test: `motion_poll_failures`, `stalled_move`.
- `DMFC-7` A move the driver did not command ended OK with the target left at the old value. It ends with the target at the measured position, and a hand-controller move during the session is followed the same way. Test: `external_motion_observed`, `external_motion_during_session`.
- `DMFC-8` `R`, `E`, `L` and `N` were accepted on any reply, `C` and `S` were never confirmed, and a failure left the requested value shown. The replies are checked against the request, `C` is confirmed by the status line and `S` by `B`; a failure shows the value the controller holds. Test: `settings_reported_failures`, `settings_readback`.
- `DMFC-9` `W` was never confirmed, so a sync the controller did not apply ended OK. The position is read back after `W`; a mismatch ends ALERT with the real position, idle polls keep it, and the next sync is accepted. Test: `sync_contract`.
- `DMFC-10` A relative move ignored `FOCUSER_LIMITS`, a GOTO to the current position sent `M`, and `FOCUSER_LIMITS` accepted an empty interval or one excluding the focuser without bounding `FOCUSER_POSITION`/`FOCUSER_STEPS`. Relative moves are sent to the limit they approach (nothing at the limit), a GOTO to the current position ends OK without a command, and accepted limits set both ranges (republished) while other changes are refused with the old limits kept. Test: `moves_without_travel`, `goto_no_op`, `limits_contract`, `limits_clamp_goto`.
- `DMFC-11` Backlash, reverse, motor type, encoder, limits and a sync were accepted during a move, and a refused motion property opened the guard for the other one. They are refused without a command now, and `moving` joins the motion guards. Test: `settings_refused_during_motion`, `overlap_rejected`.

Against the pre-fix 3.0.0.19 driver (built from a copy of its sources) 22 of the 36 scenarios fail. `test_focuser_dmfc_motion`, built from the shared `test_focuser_motion.c`, is unchanged and passes with 3.0.0.20.

### Rules not applicable

- Relative-only profile, model variants, temperature sentinel, compensation and mode: the controller is absolute, documents no "no sensor" value and has no compensation.
- Zero step: `FOCUSER_STEPS` has a minimum of 1.
- An unacknowledged stop: `H` has no reply; only a write failure is detectable, which leaves the move BUSY and the next abort resends `H` (`stop_pending`). After transport loss an idle abort has nothing to stop and ends OK.
- Shared controllers: single device; `ADDITIONAL_INSTANCES` is covered by `additional_instance`.

- Simulated tests run: 36 in the recorded run (see README `## Testing`), with `test_focuser_dmfc_motion` alongside.
- Hardware tests run: 0; passed: 0.
