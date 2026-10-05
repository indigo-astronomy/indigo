# INDIGO 3.0 refactoring record for `focuser_astromechanics`

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

Driver version is now `0x03000005`. Regression coverage is the existing suite in
`indigo_test/integration/test_focuser_astromechanics_simulator.c`, which was re-run after the change.

```sh
cd indigo_test && ./build/integration/test_focuser_astromechanics_simulator
```

- Simulated tests run: 1; passed: 1.
- Hardware tests run: 0; passed: 0.

## Focuser testing rules alignment (2026-10-05, version 8)

The suite was checked against the extended "Focuser Driver Test Standard" in `indigo_test/DRIVER_TESTING_RULES.md` (commit `fa5f64839`) and the protocol document `User Manual PROTOCOL.pdf` in this directory, which defines exactly three commands: `P#` (position reply `nnnn#`), `Mxxxx#` (absolute move, no reply) and `Axx#` (aperture, no reply). There is no stop command, no identification and no acknowledgement.

### Defects found and fixed (driver version 7 → 8)

Every case below failed against a build of the version 7 source kept outside the tree and passes against version 8.

| Id | Observable impact | Fix | Regression test |
| --- | --- | --- | --- |
| AM-01 | A position reply cut short by the timeout (`48` without `#`) was read as a valid position: the reader stripped and ignored `#`, so a timeout and a terminator looked the same, and it had no inter-byte timeout. Malformed and overlong replies were refused only because the number happened to be out of range. | The reply is read with explicit first-byte and inter-byte timeouts, `#` is kept and required as the terminator after one to four digits. | `bad_position_reply_refuses_connect`, `connect_reads_a_split_position_reply` |
| AM-02 | A refused connect (no or a bad position reply) left the serial port open. | `on_connect` closes the port when the handshake fails. | `bad_position_reply_refuses_connect` (open descriptors on the port are counted) |
| AM-03 | `FOCUSER_POSITION` copied the requested value on a move request, so the target was published as the measured position until the first poll, and a move whose first poll failed ended ALERT at the requested target. | `preserve_values`: the value stays the measured position, only the target is copied. | `failed_poll_during_motion_ends_alert` |
| AM-04 | A refused request turned the pending move's `FOCUSER_POSITION` ALERT, after which the next request was accepted while the first move was still queued or running, and a second move command went out. | A motion flag set by `on_change_request` for every accepted move and cleared when the move ends is part of the refusal condition. | `refused_requests_send_no_command` |
| AM-05 | A lens still moving at connect (a move of a previous session, since there is no stop command) was published OK at a position it then left. | Connect reads the position twice 0.1 s apart; a moving lens is published BUSY and polled until it settles, then OK with the target at the measured position. | `motion_running_at_connect`, `disconnect_during_motion_and_reconnect` |
| AM-06 | The stall bound counted every poll, so a long, progressing move could time out, and stall and read failures carried no reason. | The bound of 100 polls (10 s) counts polls without progress; failures publish a message. | `stalled_move_ends_alert`, `failed_poll_during_motion_ends_alert` |

The simulator gained `--position`, `--moving-to`, a runtime control file (`<ready-file>.control`, `ACTION SELECTOR` with `silent`, `malformed`, `overlong`, `partial`, `split` for `P` and `stall` for `M`, optionally `sticky_`) and an event log of every received command (`<ready-file>.events`).

### Scenario-to-test mapping (rules of `fa5f64839`)

| Rule | Cases |
| --- | --- |
| Interface, four-digit position range, aperture range, negative capability contract (no abort, speed, temperature, compensation, backlash, sync, limits, reverse, mode) | `metadata_and_property_completeness` |
| Connect publishes the lens position, also from a split reply; GOTO right after connect judged against it | `connect_reads_a_split_position_reply`, `reconnect_adopts_the_lens_position` |
| Refused connect (malformed, overlong, partial, silent reply, vanished port): ALERT, nothing defined, port released, next connect works | `bad_position_reply_refuses_connect`, `vanished_port_is_refused` |
| Absolute moves, both properties in the same state, GOTO to the current position, relative moves, clamping at both ends | `goto_moves_to_the_requested_position`, `goto_to_the_current_position_completes`, `steps_move_relative_to_the_current_position`, `steps_are_clipped_at_the_end_stops`, `out_of_range_positions_are_clamped` |
| Requests during a move refused with ALERT and no command, also after an earlier refusal; the move ends at its target with one `M` command | `refused_requests_send_no_command`, `a_request_during_a_move_does_not_divert_it` |
| Failed poll during a move ALERT with the last valid value, not arrival, fresh move works | `failed_poll_during_motion_ends_alert` |
| Stalled move ALERT within bounded time, fresh move works | `stalled_move_ends_alert` |
| Disconnect during motion sends nothing more and no poll follows; reconnect publishes the still moving lens BUSY, then OK at the real position | `disconnect_during_motion_and_reconnect`, `test_focuser_astromechanics_motion` |
| Motion running at connect BUSY then OK with target = measured, no command | `motion_running_at_connect` |
| Aperture: one `Axx` command carrying the value, no lens motion | `aperture_is_accepted_across_its_range` |
| SHUTDOWN refused while connected, `ADDITIONAL_INSTANCES` instance on its own port with its own position, sibling survives a disconnect | `shutdown_refused_while_connected`, `additional_instance` |
| Repeated disconnect | `repeated_disconnect_is_tolerated` |

Not applicable, with the reason:

- Abort and the stop before disconnect: the protocol has no stop command; `FOCUSER_ABORT_MOTION` stays hidden and a disconnect during motion sends nothing. A replacement such as a move to the last read position was not introduced, because the controller's handling of a new `M` during a move is not documented.
- Identity (`INFO.DEVICE_MODEL`, firmware): the protocol has no identification command.
- Idle position polling, hand-controller motion, requests versus polls: the driver reads the position only during a move and at connect.
- SYNC, limits, reverse, backlash, temperature, compensation, mode, speed: not in the protocol.
- Rejected or mismatched setting writes: `A` has no reply, only a transport failure can be detected.
- Shared controllers: one lens per port.
- Hardware: no lens controller available, no physical run.

The shared `integration/test_focuser_motion.c` (also built for `focuser_dmfc`, `focuser_usbv3` and `focuser_primaluce`) was not changed: its abort branch is skipped here because `FOCUSER_ABORT_MOTION` is not defined, and the driver-specific motion checks live in this driver's own suite.
