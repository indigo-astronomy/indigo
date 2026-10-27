# INDIGO 3.0 refactoring record for `focuser_fc3`

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

## Stuck FOCUSER_STEPS after a short absolute move (2026-09-20)

Writing the full test suite uncovered a defect that made the driver unusable after the first short
`GOTO`. The absolute-move branch of `FOCUSER_POSITION.on_change` published only `FOCUSER_STEPS` as
BUSY and left the generator's `INDIGO_OK_STATE` prologue on `FOCUSER_POSITION`, so the two motion
properties were *not* published busy together, contrary to what the section above assumes. The
polling loop only settles the pair when `FOCUSER_POSITION` disagrees with the reported motion state,
so a move that finished inside one polling interval was answered with `is_running == 0` while
`FOCUSER_POSITION` already read OK. The loop did nothing and `FOCUSER_STEPS` stayed BUSY forever;
from then on every absolute move was refused with "Another motion operation is pending" until the
device was reconnected.

The fix sets `FOCUSER_POSITION` to BUSY alongside `FOCUSER_STEPS` when `FM:` is acknowledged, which
is what the relative-move branch and the connect-time status parsing already do. Driver version is
now `0x03000006`. `short_moves_settle` is the regression test; it was confirmed to fail against the
pre-fix driver and to pass against the fixed one.

## Inverted relative-move limit clamping (2026-09-20)

The relative-move branch of `FOCUSER_STEPS.on_change` clamped an inward request against
`FOCUSER_LIMITS_MAX_POSITION` and an outward request against `FOCUSER_LIMITS_MIN_POSITION`, while
`FG:` is sent negative for inward and positive for outward. Each move was therefore cut short at the
end of the travel it moves away from, so a long inward move could run past the configured minimum
while a short one was truncated for no reason. The two branch bodies were swapped so each move is
clamped at the limit it approaches. Driver version is now `0x03000007` and `limits_clamp_steps`
covers both directions and a request that stays inside the interval.

The clamp uses the last position the controller reported, not the requested target, so a relative
move issued immediately after a sync and before the next poll is clamped against the previous
coordinate. That is inherent to a polled protocol and is not treated as a defect; the test waits for
the readback.

## Automated test coverage (2026-09-20)

`indigo_test/integration/test_focuser_fc3_simulator.c` was extended from a single smoke test to the
full focuser class standard in `indigo_test/DRIVER_TESTING_RULES.md`. Every scenario runs in its own
forked process against a freshly started simulator, so no state leaks between cases.

The simulator `focuser_fc3_simulator/focuser_fc3_simulator.c` gained the fixtures the standard
needs and nothing else:

- `--profile <normal|configured|no-handshake|bad-status|external-motion>` selects the controller
  state a scenario connects to.
- `INDIGO_FC3_EVENTS` records every accepted request, so a test can assert the command, its argument
  and its sign instead of only the resulting property state.
- `INDIGO_FC3_FAULT` arms a one-shot `silent`, `garbage` or `close` fault on the next request with a
  given prefix, which is how lost acknowledgements, malformed replies and transport loss are injected.

### Scenario to test mapping

| Class standard area | Scenario |
| --- | --- |
| Capabilities and readback | `metadata`, `property_contract`, `status_readback` |
| Initialization failures | `handshake_rejected`, `handshake_timeout`, `status_query_failure`, `firmware_query_failure` |
| Lifecycle | `repeated_init_shutdown`, `shutdown_rejected_while_connected`, `reconnect` |
| Motion, units and sign | `sync_and_goto`, `relative_move`, `limits_clamp_goto`, `limits_clamp_steps`, `short_moves_settle` |
| Stop and overlapping requests | `motion_progress_and_abort`, `abort_while_idle`, `abort_failure_reported`, `overlap_rejected` |
| Externally changed position | `external_motion_observed` |
| Disconnect during motion | `disconnect_during_motion` |
| Modes and controls | `controller_settings`, `settings_reported_failures` |
| Command failures | `motion_command_failures` |
| Settings persistence | `configuration_roundtrip` |
| Polling and failure recovery | `status_polling`, `poll_failure_recovery` |
| Transport loss | `transport_loss` |

### Notes and gaps

- `FOCUSER_MODE` and `FOCUSER_COMPENSATION` stay hidden; the controller has no temperature
  compensation of its own, so the compensation rows of the class standard do not apply. Their absence
  is asserted in `property_contract`.
- The polling loop republishes both motion properties every second, so a state the driver publishes
  in answer to a request can be overwritten before a test looks at it. Every request assertion is
  therefore made against the property state revisions rather than the current state.
- `FOCUSER_DIRECTION` and `FOCUSER_LIMITS` are the driver-persisted settings. The `CONFIG` roundtrip
  runs against a scratch directory through the `indigo_uni_config_folder` override, so the user
  profile is untouched.
- Hardware acceptance from `DRIVER_TESTING_RULES.md` was not run; no FocusCube 3 was available.

## Focuser testing rules alignment (2026-10-05)

The suite was checked against the "Focuser Drivers" chapter of `indigo_test/DRIVER_TESTING_RULES.md` (commit fa5f64839) and extended where a relevant rule was not verified. The simulator fault file gained an optional argument and the actions `value <reply>` (replaced reply, e.g. a mismatched echo or malformed status), `slow` (status reply held 0.5 s, inside the driver timeout), `split`, `overlong`, `truncated`, `stall` (move accepted, motor never turns, cleared by `FH`) and `external <target>` (hand-controller move). The runner now plans its forked cases.

Driver version 3.0.0.8 → 3.0.0.9.

### Defects found and fixed

- `FC3-1` An aborted move was published ALERT, but the next status poll turned it OK because the poll settled every non-OK `FOCUSER_POSITION` whose motor was idle, and the target stayed at the requested position. The abort now reads the stopped position, publishes both motion properties ALERT with the value at the stopped position and the target still the requested one (3.0.0.11, `FC3-14`), and the poll completes only a move the driver is following. Test: `motion_progress_and_abort` (two later polls keep the ALERT and the stopped position).
- `FC3-2` An abort while idle sent `FH` and published the motion properties ALERT. Now OK without a command. Test: `abort_while_idle` (also an abort request with the item OFF).
- `FC3-3` Any reply counted as an acknowledgement: a garbled or mismatched echo of `FM:`, `FG:`, `FN:`, `SP:`, `BL:` or `FD:` was published as success, and a setting failure left the requested value shown. Commands are now acknowledged only by their exact echo; a failure shows the value the controller last confirmed, a mismatched echo the value it applied. Test: `settings_reported_failures`, `motion_command_failures`.
- `FC3-4` A relative move published `FOCUSER_STEPS` OK while the motor ran (only `FOCUSER_POSITION` was set BUSY). Both are BUSY now. Test: `relative_move`.
- `FC3-5` A status poll whose reply arrived after a move request was accepted, but before its handler ran, published the request OK (the poll settled any non-OK state while the motor was idle). The poll now leaves a request the bus accepted alone and completes only moves it follows. Test: `request_survives_poll`.
- `FC3-6` A failed, malformed, overlong or truncated status line was silently ignored (or partly parsed: `atoi`/`indigo_atod` turned garbage into 0). The status line is validated as a whole; an idle failure publishes `FOCUSER_POSITION` and `FOCUSER_TEMPERATURE` ALERT with the last valid values and the next good poll restores OK; replies must end in a line end. Test: `poll_failure_recovery`, `temperature_failures`.
- `FC3-7` A failed status read or a stalled motor during a move kept the move BUSY forever. One lost status line is retried, a second one, or five polls without progress, send `FH` and end the move ALERT. Test: `motion_poll_failures`, `stalled_move`.
- `FC3-8` A hand-controller move started during the session was published BUSY/OK but its end left the target at the old value. External moves now end with the target at the measured position. Test: `external_motion_during_session`, `external_motion_observed`.
- `FC3-9` A failed status or speed query at connect was ignored and the controller published with driver defaults. Both are mandatory now; a failure refuses the connection, resets the identity and releases the port. Test: `status_query_failure`, `connect_query_refused`.
- `FC3-10` `FOCUSER_LIMITS` accepted an empty interval (the base handler swapped it) or one excluding the focuser, and `FOCUSER_POSITION`/`FOCUSER_STEPS` ranges ignored the limits. Such changes are now ALERT with the old limits kept; accepted limits set both ranges (republished). Test: `limits_contract`, `limits_clamp_goto` (a sync is now inside the limits too, because the framework clamps to the published range).
- `FC3-11` A GOTO to the current position sent `FM:` and waited for a poll. It now ends OK at once without a command; a relative move at the limit it approaches sends nothing. Test: `sync_and_goto`, `moves_without_travel`.
- `FC3-12` Backlash, reverse, limits and a sync were accepted during a move. They are refused without a command now, and both motion properties refuse a second move while any move runs (`motion_active` keeps the guard while a refused property shows ALERT; progress polls keep that ALERT visible until the move ends). Test: `settings_refused_during_motion`, `overlap_rejected` (the running move ends at its own target with one `FM:`).
- `FC3-13` A failed sync left the requested value as target. The target is restored to the real position. Test: `motion_command_failures`. Superseded in 3.0.0.11 (`FC3-14`): the target keeps the requested value, the value stays the real position.
- `FC3-14` (3.0.0.11) An aborted, stalled or poll-failed move, and a refused `FM:`/`FG:`, overwrote the requested target with the reached position, and the idle poll rewrote the target whenever the position changed. A move now publishes the reached position as the value only; the target changes only at connect, with a new request (a relative move's computed target, also when refused), with a successful sync and for motion the driver did not command; a failed sync keeps the requested sync value as target (a change the next poll finds is attributed to the sync). After a failed or aborted move the idle polls follow the stopping motor without touching the target (`settling`) until the controller reports it stopped. Test: `motion_progress_and_abort` (two polls after the abort), `abort_failure_reported` (two polls after the retried abort), `stalled_move`, `motion_poll_failures`, `motion_command_failures`.

All new and changed cases were run against the pre-fix driver (built from a copy of the 3.0.0.8 sources): 21 of 38 fail there.

### Rules not applicable

- Relative-only profiles, model or firmware variants: one controller, one protocol; `FV` is the only optional identity query and is covered by `firmware_query_failure`.
- Zero step: `FOCUSER_STEPS` has a minimum of 1 (moves without travel are covered instead).
- Temperature sentinel: the protocol documents no "no sensor" value; implausible readings outside -55…125 °C and malformed ones are ALERT.
- `FOCUSER_MODE`, `FOCUSER_COMPENSATION`: the controller has no compensation; their absence is asserted.
- Homing, calibration, other momentary switches, refusal reasons: not in the protocol; the controller answers with echoes only.
- Shared controllers: single device; `ADDITIONAL_INSTANCES` is covered by `additional_instance`.
- Legacy names: the driver defines no driver-specific properties.

### Open

- After transport loss an abort with nothing moving ends OK without a command (it has nothing to stop); every other request ends ALERT.

### Scenario additions

| Rule area | New or changed scenario |
| --- | --- |
| Refused connect | `status_query_failure`, `connect_query_refused` |
| Additional instance | `additional_instance` |
| Zero travel, limits and ranges | `moves_without_travel`, `limits_contract`, `limits_clamp_goto` |
| Abort contract | `motion_progress_and_abort`, `abort_while_idle` |
| Overlap and settings during motion | `overlap_rejected`, `settings_refused_during_motion` |
| External motion | `external_motion_during_session`, `external_motion_observed` |
| Failures during motion | `stalled_move`, `motion_poll_failures`, `motion_command_failures` |
| Requests versus polls | `request_survives_poll` |
| Readback and temperature failures | `poll_failure_recovery`, `temperature_failures`, `settings_reported_failures` |

```sh
make -C indigo_test build/integration/test_focuser_fc3_simulator
cd indigo_test && ./build/integration/test_focuser_fc3_simulator
```

- Simulated tests run: 38 in the recorded run (see README `## Testing`); the same 38 against the pre-fix 3.0.0.8 driver failed 21.
- Hardware tests run: 0; passed: 0.

## Stopped-short completion repair (2026-10-24)

Finding FND-20261023101753-bc8d-complete-repository-audit-027: idle is not sufficient to confirm arrival of a commanded move. Preserve the measured value and requested target; compare with the physical command endpoint, including software backlash or reversed raw travel. Protocol/SDK calls and their order remain unchanged. Existing generated inputs and checked-in outputs remain synchronized. No new properties, dependencies or platform-specific code. Existing simulator/fake SDK covers arrival, abort, external motion and transport failures; add an idle-early-stop regression. No hardware testing is planned or claimed; Linux/Windows and real device mechanics remain unverified.

Atomic plan:
1. Build the original driver and existing hardware-free suite; record baseline results and reference behavior.
2. Add a deterministic early-stop regression against the original implementation and retain its expected failure.
3. Compare completion with the issued endpoint, preserve external/abort behavior, increment driver version, regenerate with unchanged generator.
4. Rerun affected suites and hand results to the coordinator for final recorded runs and review.

Evidence and delivery: parent task 20261024183351-e8e1-fix-motion-image-alpaca; see repair verification below.

Found defect: commanded motor stops before its physical endpoint yet reports OK. The regression requires ALERT, measured value and retained requested target. Fix compares the measured endpoint at idle; DSD stores the software-backlash endpoint, USBv3 stores signed raw travel after reversal and controller limit clipping, and Lunatico compares the quantized raw step endpoint. External motion remains independent and aborts keep their established ALERT behavior. No arbitrary tolerance is added to integral step reports.

Plan progression: early-stop reproducer added and failed before the production change; comparison and version increment implemented, followed by unchanged-generator regeneration and focused verification (details below).

### Repair verification and handoff

Baseline source: `463eb538daf440fa0f459b53a5f02fa949e244ed`. Isolated checkout `/private/tmp/indigo-motion-20261024`, macOS arm64, universal driver archives (arm64/x86_64), existing parent build library used at `/Users/polakovic/Development/indigo/build/lib`. Original suite: `./build/integration/test_focuser_fc3_simulator` from `indigo_test/`: 39/39 passed, exit 0. Serial/shared original and fixed drivers built with `make -C indigo_drivers/<driver> -f ../../Makefile.drv all`; ASI compiled through the fake-SDK target. Test targets built with `make -C indigo_test build/integration/test_<driver>_<kind>`.

Early-stop regression: 1 run, 0 passed against the original implementation (expected defect reproduction), then 1 run, 1 passed after the fix. Both Lunatico exports have separate executable rotator regressions; the focuser export uses its Exp rotator. The second-export baseline executable was compiled from retained original generated/shared sources. No tests or expectations were weakened.

Selected post-fix simulator/fake-SDK checks: 3 run, 3 passed. Full commands, filters, exit codes and output are retained in the parent task verification evidence. The focused run covers this driver's early-stop case and relevant existing external/hand-controller or geometry cases. DSD software-backlash overshoot, USBv3 reversed absolute arrival, and fractional Lunatico angle quantization pass. Final recorded full suites, platform checks and quality review remain assigned to the coordinator. `git diff --check` passed. No property inventory changed.

Atomic plan status: baseline build and all eight original existing-suite executions completed; defect reproducer completed; production repair, driver version increment and unchanged-generator output regeneration completed; focused regression verification completed. Coordinator owns final recorded full suites and review.

### Repair test summary

- Simulated tests for the selected post-fix checks: 3 executed, 3 passed. Expected failing original early-stop reproduction: 1 executed, 0 passed, intentionally retained as defect evidence.
- Hardware tests for this repair: 0 executed, 0 passed. Linux/Windows, real SDK/device behavior and hardware mechanics remain unverified.

Final combined candidate recorded software run: unchanged `tools/run_driver_test.py` main with coordinated record locking passed 40/40 on macOS arm64. README Testing and TEST_SUMMARY were regenerated by the runner. Full command/result and prior attempts are retained in task verification; generator reproduction was byte-identical. No physical hardware run: 0 executed, 0 passed. Existing hardware-case availability is retained separately. These results supersede the delegated handoff's pending full-run assignment; final diff review is task evidence.
