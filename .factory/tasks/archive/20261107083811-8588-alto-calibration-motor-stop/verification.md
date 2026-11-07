# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Scope of the verdict: simulator, unit and ALTO1 hardware runs (interactive, with guided calibration and hot-plug)
Reviewer: Claude Code (claude-opus-5-5), the implementer
Date: 2026-11-07
Revision and working-tree state: task branch at base 86d3e884e with the uncommitted task diff
Review independence: not independent; the implementer verified. The diff review was made by a separate agent context (quality-review.md).

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `calibration`: the journal holds `{"req":{"cmd":{"MOT1":{"CAL_ALTO":"Motor_Stop"}}}}` after STOP; X_CALIBRATION_MOTION ends OK |
| AC-002 | pass | `calibration`: `requests_adjacent()` shows `Motor_Stop` immediately before `StoreAsClosedPos` and before `StoreAsMaxOpenPos`; 3 `Motor_Stop` in total; `MOT_STOP` count unchanged through the calibration; "Calibration complete, travel", then a move to 50 % uses the new travel |
| AC-003 | pass | `calibration_stop_refused`: STOP and STORE end ALERT with "Error: invalid command"; `StoreAs` count 0; the next STORE stores the closed position |
| AC-004 | pass | `calibration`: CANCEL adds exactly one `MOT_STOP` |
| AC-005 | pass | `test_primalucelib` `cover_commands`: request 5 matches the documented request, accepted `"done"` |
| AC-006 | pass | Version 10: after STOP the polled position stayed strictly between the ends over two polls. Version 11 hides the position during a calibration (AC-010), so the stop is now proven by the travel: 2 s of opening at 48000 steps/s, STOP, 1.5 s wait, STORE gives 85000–130000 steps (about 98000); without the stop it would be about 170000. `Motor_Stop` outside a calibration stays refused (`calibration_step()` returns INVALID). On hardware every `Motor_Stop` stopped the arm: the ALTO reported `MST` "stop" and an unchanged position |
| AC-007 | pass | `DRIVER_VERSION 0x0300000B` (was `0x03000009` at the base); regenerated outputs match the `.driver`; `.h` and `_main.c` byte-identical to the base |
| AC-008 | pass | `python3 tools/run_driver_test.py aux_alto`: 36/36 OK, final record `2026-11-07 12:23 3.0.0.11 linux x64 simulator 36/36 OK`; primalucelib unit 38/38 |
| AC-009 | pass | `calibration`: X_CALIBRATION BUSY on the second STORE, then "Calibration complete, travel N steps" with N about 98000 (not the simulator's stale 2097136), at least 3 `CAL_MAXPOS` reads; `calibration_travel_not_reported` (profile stale-travel): ALERT "did not report its new travel" after the polls, no completion message, position OK, moves accepted again; unit `cover_commands` checks `{"req":{"get":{"MOT1":{"CAL_MAXPOS":""}}}}` → 237189 |
| AC-010 | pass | `calibration`: after START X_COVER_POSITION and AUX_COVER are IDLE and the value stays 100 % (the simulator's provisional 50 % is never published); CANCEL restores OK |
| AC-011 | pass | Hardware `alto_survives_the_usb_unplug`: HOTPLUG_READY → owner pulled the cable → port gone, X_COVER_POSITION ALERT ("Failed to write (Input/output error)"), clean disconnect → owner replugged → reconnect at 100 %, moves to 90 % and back; recorded `2026-11-07 12:06 3.0.0.11 linux x64 ALTO1 (hot-plug) 10/10 OK` |
| AC-012 | pass | Hardware suite 9/9 on version 10 (11:20) and on the final version 11 (`2026-11-07 12:33 3.0.0.11 linux x64 ALTO1 9/9 OK`); guided calibration on version 10 completed (see "Hardware calibration run") |

## Executed checks
Environment: Linux 6.8.0-147 x64, worktree `/home/rumen/work/indigo-task-alto-motor-stop`.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make all -j8` | worktree root, base 86d3e884e | 0 | Baseline build |
| `make build/integration/test_aux_alto_simulator build/unit/test_primalucelib`, then both executables | `indigo_test`, unchanged tree | 0 / 0 | Baseline: unit 38 PASS; ALTO simulator 34 PASS, 0 failing |
| Same, with the new simulator and tests, original driver | `indigo_test` | 0 / 1 | Unit passes; ALTO: `calibration` fails at `wait_for_requests(...Motor_Stop...)` (journal holds Init, Close_Fast, no Motor_Stop); `calibration_stop_refused` fails: "X_CALIBRATION_MOTION never published state 3 (current 1)"; 33 others pass |
| `../../build/bin/indigo_generator indigo_aux_alto.driver` | `indigo_drivers/aux_alto` | 0 | Wrote `.h`, `_main.c`, `.c`; only `.c` changed |
| `make -C indigo_drivers/shared/primalucelib all`; `make -C indigo_drivers/aux_alto -f ../../Makefile.drv all` | worktree root | 0 | No warnings |
| Both test executables after the driver change | `indigo_test` | 0 / 0 | Unit 38 PASS; ALTO 35 PASS, 0 failing |
| `python3 tools/run_driver_test.py aux_alto` | worktree root | 0 | `aux_alto: 35/35 OK` at 10:56; README record and `TEST_SUMMARY.md` regenerated |
| Review fixes (3 s poll margin in `calibration`, simulator store comment), then `python3 tools/run_driver_test.py aux_alto` and the unit suite | worktree root | 0 / 0 | `aux_alto: 35/35 OK`, recorded `2026-11-07 11:02 3.0.0.10 linux x64 simulator 35/35 OK`; unit 38 PASS |
| `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all`; same for `aux_giotto` | worktree root | 0 | Other primalucelib drivers build without warnings |
| `git diff --check` | worktree root | 0 | No whitespace errors |
| `./.factory/scripts/findings check` | worktree root | 0 | "Findings valid: 25 active, 292 archived; task evidence checks passed." |
| `python3 tools/run_driver_test.py aux_alto --hw --port /dev/ttyUSB0` (version 10) | worktree root, ALTO1 on USB | 0 | 9/9 OK, recorded 11:20 |
| Direct read before the calibration: `{"req":{"get":{"MOT1":{"CAL_MAXPOS":"",...}}}}` with a pyserial script | scratchpad, `/dev/ttyUSB0` | 0 | `CAL_MAXPOS` 236010, `CAL_MINPOS` 0, `POSITION` 100, `CALRESTART.MOT1` 0, SWAPP 1.03.11, SWWEB 1.01 |
| Guided calibration through a private `indigo_server -p 7625 -vvv` with `indigo_prop_tool` (version 10) | task build, ALTO1 | 0 | See "Hardware calibration run" |
| New tests against version 10 (index copy of the `.driver`/`.c`), new simulator v4 | `indigo_test` | 1 | `calibration` fails at `message_seen("the position is shown again when the calibration ends")`; `calibration_travel_not_reported` fails at `message_seen_within("did not report its new travel", 15)` |
| Version 11, first run | `indigo_test` | 1 | 35 pass; `calibration_travel_not_reported` fails: a simulator defect, the stale-travel profile reported `CAL_MAXPOS` 0 before any store, so the stale 2097136 looked new. Fixed in `reported_travel()` (stale only after a store) |
| Version 11, after the simulator fix | `indigo_test` | 0 | 36 PASS |
| `python3 tools/run_driver_test.py aux_alto` | worktree root | 0 | 36/36 OK, recorded 12:00; unit 38/38 |
| `python3 tools/run_driver_test.py aux_alto --hw --port /dev/serial/by-id/usb-Silicon_Labs_CP2102N_..._b00016cac822f011929b15d936263638-if00-port0` | worktree root | 0 | 9/9 OK, recorded 12:04 |
| `python3 tools/run_driver_test.py aux_alto --hot-plug --port <same>` | worktree root, owner unplugging/replugging on request | 0 | 10/10 OK, recorded 12:06 |
| Second review round refinements (owner approved): motion steps refused while the travel is awaited, `poll_alert` after a failed `cover_show_position()` read, provisional travel never taken (`PRIMALUCE_COVER_PROVISIONAL_TRAVEL`), simulator time-compression comment, hardware test comment; `calibration_travel_not_reported` extended (garbage fault on the pre-read, motion refused) | worktree root | — | Regenerated; outputs reproduced by the reviewer |
| `python3 tools/run_driver_test.py aux_alto` | worktree root | 1 | 36/35: `calibration_travel_not_reported` "expected 5, got 7", a test race (BUSY is published before the STORE handler sends its requests); the test now waits for `StoreAsMaxOpenPos` before counting |
| `python3 tools/run_driver_test.py aux_alto`, then the suite binary twice more | worktree root | 0 | 36/36 OK, recorded 12:23; both repeats 0 failing |
| `python3 tools/run_driver_test.py aux_alto --hw --port <same>` | worktree root | 0 | 9/9 OK, recorded 12:33 (final code) |

## Baseline comparison and regressions
Simulator: baseline 34/34 → 36/36. The new cases are `calibration_stop_refused` and `calibration_travel_not_reported`;
`calibration` was strengthened twice. Hardware: 9/9 before (2026-10-24 record) and after, plus the new opt-in hot-plug
case. No regression in the primalucelib unit suite.

## Hardware calibration run
Driver version 10, ALTO1 firmware 1.03.11, owner watching the arm (270° from open to closed) and reporting its angle;
full protocol log retained in the session scratchpad only (`alto-calibration-run.log`, not checked in).
- `Init` → "done"; the next poll reported `POSITION` 50 with the arm still fully open (before: 100).
- Timed bursts, each ended with `X_CALIBRATION_MOTION.STOP` = `CAL_ALTO Motor_Stop`: Close_Fast 10 s (owner: 45°, the
  driver showed 48 %), 35 s, 25 s; Close_Slow 5, 3, 0.5, 0.5, 1, 1 s; Open_Slow 0.5 s; owner: "dead on".
- STORE: `Motor_Stop` → "done", `StoreAsClosedPos` → "done"; position 0.
- Open_Fast 60 s, 12 s; Open_Slow 3, 1, 0.5, 0.3 s; STORE: `Motor_Stop` → "done", `StoreAsMaxOpenPos` → "done".
- The full state the driver read 170 ms after the store still held `CAL_MAXPOS` 2097136; after a reconnect the ALTO
  reported `CAL_MAXPOS` 237189, `POSITION` 100.
- 37 `CAL_ALTO` requests, all acknowledged "done"; no `MOT_STOP`. Measured speeds: fast about 3.3–3.6°/s, slow about 3°/s.
- The cover was left fully open (100 %), as found; its calibration is now 237189 steps (was 236010).

## Found defects
| Defect | Observable impact | Root cause | Fix | Regression test | Reproduction |
| --- | --- | --- | --- | --- | --- |
| Stale travel after the calibration (version ≤ 10) | After `StoreAsMaxOpenPos` the driver took the travel from a state read 170 ms later, which still held the provisional `CAL_MAXPOS` 2097136: preset percentages and new preset positions were wrong (50 % → 1048568 steps) until a reconnect, and the completion message showed 2097136 | The ALTO stores the new travel a moment after acknowledging the store; the driver read once | `calibration_end_finalizer` polls `CAL_MAXPOS` (new `primalucelib_cover_get_travel`) until it differs from the value read before the store, BUSY meanwhile, ALERT after 20 polls; X_PRESETS updated | `calibration`, `calibration_travel_not_reported`, unit `cover_commands` | Reproduced on the ALTO1 (hardware run above), modelled in the simulator (`STALE_TRAVEL_TIME`, profile stale-travel) and reproduced against version 10 |
| Meaningless position during a calibration (version ≤ 10) | X_COVER_POSITION showed 48 % with the arm at 45° from open, and values relative to a 2097136-step range throughout the calibration | `CAL_ALTO Init` switches to a provisional range with the counter in its middle; the idle poll kept publishing it | No cover poll while `calibration_phase` is set; AUX_COVER/X_COVER_POSITION IDLE from START; `cover_show_position()` after the calibration and on CANCEL | `calibration` | Reproduced on the ALTO1, modelled in the simulator (`PROVISIONAL_TRAVEL`, Init moves the count to the middle) and reproduced against version 10 |
| Calibration stop uses `MOT_STOP` | STOP and STORE during a calibration send `MOT1 MOT_STOP`; the PrimaLuce firmware may not complete the calibration step, so a store could be refused or wrong (unverified) | Sequence copied from INDI `alto.cpp` without a PrimaLuce source | `.driver` X_CALIBRATION STORE (both phases) and X_CALIBRATION_MOTION STOP call `primalucelib_cover_calibration_step(LINK, PRIMALUCE_COVER_CALIBRATION_MOTOR_STOP)` | `calibration`, `calibration_stop_refused`, unit `cover_commands` | Found by source audit against the PrimaLuce protocol v1.3, PLAY 1.9.8, web firmware 1.01 and firmware 1.03.11; the failure is reproduced against the simulator's documented model, not on hardware |

## Findings
None registered: all three defects are fixed in this task (above).

## Hardware observations modelled in the simulator
| Observation on the ALTO1 | Simulator change (version 4) | Test |
| --- | --- | --- |
| `Init` reads `POSITION` 50 with the arm open | `Init` sets `calibration_max` to 2097136 and syncs the count to its middle | `calibration` (position hidden; would publish 50 otherwise) |
| `CAL_MAXPOS` 2097136 170 ms after `StoreAsMaxOpenPos`, 237189 later | `reported_travel()` returns the previous travel for 1 s after the store; profile stale-travel keeps it | `calibration`, `calibration_travel_not_reported` |
| `Motor_Stop` stops the arm, acknowledged "done" | already modelled (version 3) | `calibration` (travel range) |
| Calibration speeds about 3.3–3.6°/s fast and 3°/s slow (about 2900–3100 and 2600 steps/s) | not modelled: the simulator keeps 48000 and 12000 steps/s so the suite stays fast; no driver logic depends on the speed | — |
| USB unplug: write fails with an I/O error, port disappears and returns under the same by-id name | not modelled hardware-free (physical USB removal); `transport_loss` covers a closed link | hardware `alto_survives_the_usb_unplug` |

## Manual checks and limitations
- Document evidence for `PROTOCOL.md` (from the owner-supplied PDF, text extracted with `pdftotext -layout`):
  - title page and footer "Document Revision 1.3 (18th Mar 2023)"; revision history row "1.3  18th Mar 2023  1.3.3  WiFi-AP status. Enable/Disable WiFi-AP";
  - calibration: "2) "Close_Fast", "Close_Slow" and "Motor_Stop" ... The "Motor_Stop" command will be send when the arm will be arrived in the right close position", followed by "send the "StoreAsClosedPos" command"; example `{"req":{"cmd": {"MOT1": {"CAL_ALTO": "Motor_Stop"}}}}`;
  - `POSITION` set example reply `{"res":{"get": {"MOT1": {"POSITION": "done"}}}}`; `POSITION_STEP` set example `{"req":{"set": {"MOT1": {"POSITION_STEP": 60000}}}}` with reply `{"res":{"get": {"MOT1": {"POSITION": "done"}}}}`.
- PrimaLuce software evidence: PLAY 1.9.8 `TelescopeManager.exe` holds the eight `CAL_ALTO` requests including `{"req":{"cmd": {"MOT1": {"CAL_ALTO": "Motor_Stop"}}}}`; ALTO web firmware 1.01 `custom.js` sends `{ MOT1: { CAL_ALTO: "Motor_Stop" } }` from `.clBtMotStop` and only `StoreAsClosedPos` / `StoreAsMaxOpenPos` from its store buttons; firmware 1.03.11 strings `Calib_Handler_ALTO`, `Motor_Stop`, " ALTO Calibration - Motor_Stop".
- Sources were inspected by string extraction of the downloaded PrimaLuce packages; no PrimaLuce program was executed and
  the firmware was not disassembled.
- Hardware: the guided calibration ran on version 10; the stale-travel and hidden-position fixes of version 11 are
  verified on the simulator and by the hardware suite and hot-plug runs, but no second calibration was run on version 11
  (it would have to be judged by eye again). Version 11's calibration path on hardware is therefore unverified.
- Not tested: the ALTO's range after a calibration cancelled following `Init`.
- The hot-plug run (12:06) used version 11 before the second-round refinements; they do not touch the unplug path and
  the owner agreed not to repeat it. The refinements' new assertions were not run against the pre-refinement build.
- `PROTOCOL.md` and `PROPERTIES.md` were edited after some recorded runs (text only).
- macOS and Windows not available.

## Verification verdict rationale
All acceptance criteria pass, with expected pre-fix failures recorded for every fix. `Motor_Stop` and the store sequence
are confirmed on the ALTO1; the version 11 calibration fixes are confirmed on the simulator only.

## Final test summary
- Simulated tests: 36 ALTO simulator cases run, 36 passed (recorded run, version 11); 38 primalucelib unit cases run, 38 passed.
- Hardware tests: 9 ALTO1 cases run, 9 passed (final version 11); 10 ALTO1 hot-plug-run cases run, 10 passed (version 11 before the refinements); 9 run, 9 passed on version 10; guided calibration on version 10 completed (manual scenario, not an automated case).

## Register reconciliation
Canonical FND IDs and evidence references: none; no finding opened or closed by this task.
