# Development plan

Status: implemented

## Approach and affected components
1. Add the `Motor_Stop` constant to primalucelib.
2. Teach the ALTO simulator the documented step.
3. Write the tests against the original driver first, recording the expected failures.
4. Change the driver's calibration STOP and STORE, and regenerate.
5. Update `PROTOCOL.md`, then make the recorded simulator run.

## Baseline
- Commands and environment: see verification.md "Executed checks" (Linux x64).
- Existing failures: see verification.md.
- Repository revision / working-tree state: master 86d3e884e, clean task worktree.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Baseline: build the driver, run the ALTO simulator suite and the primalucelib unit suite | AC-008 | None | Baseline recorded in verification.md | done: master 86d3e884e, primalucelib unit 38/38, ALTO simulator 34/34 (verification.md) |
| 2 | primalucelib: add `PRIMALUCE_COVER_CALIBRATION_MOTOR_STOP` (`primalucelib.h`) and unit-test the request (`test_primalucelib.c`) | AC-005 | 1 | Unit suite passes | done: constant added; `cover_commands` checks request 5 `CAL_ALTO Motor_Stop`; unit 38/38 |
| 3 | Simulator: accept `Motor_Stop` during a calibration as a decelerated stop (`alto_simulator.c`) | AC-006 | 1 | Simulator suite still passes against the original driver | done: simulator version 3, `Motor_Stop` shares the `MOT_STOP` decelerated stop (`decelerated_stop()`); applied after the baseline run |
| 4 | Tests: the calibration case checks `Motor_Stop` for STOP and before each STORE, and `MOT_STOP` for CANCEL; add a refusal case. Run against the original driver and record the expected failures | AC-001..AC-004, AC-006 | 3 | Expected failures recorded | done: `calibration` extended, `calibration_stop_refused` added; against the original driver both fail as expected (no `Motor_Stop` sent), 33 others pass |
| 5 | Driver: STOP and STORE use `Motor_Stop`; version 10; regenerate | AC-001..AC-003, AC-007 | 2, 4 | New cases pass; regeneration clean | done: version 10; regenerated `.c` differs only in the three calls, the comment and DRIVER_VERSION; `.h`/`_main.c` unchanged; 35/35 |
| 6 | `PROTOCOL.md`: sources and calibration row | — | 5 | Text matches behaviour | done: DOC (ALTO) source class, calibration row, notes; also MIGRATION_STATUS.md sim count 34 → 35 (deviation: not listed in the plan) |
| 8 | Interactive hardware run on the ALTO1: 9-case suite, guided calibration | AC-012 | 7 | Suite passes; calibration completes | done: 9/9 (11:20, version 10); calibration with 37 `CAL_ALTO` requests, all "done", no `MOT_STOP`; travel 237189; found the stale travel and the meaningless position (verification.md) |
| 9 | Fix the stale travel (`calibration_end_finalizer`, `primalucelib_cover_get_travel`) and hide the position during a calibration; version 11; simulator v4 models both; tests first against version 10 | AC-009, AC-010 | 8 | New cases fail on version 10, pass on 11 | done: on version 10 `calibration` and `calibration_travel_not_reported` fail; on 11 the suite is 36/36 after a simulator modelling fix (stale-travel profile reported 0 before any store) |
| 10 | Opt-in hardware case `alto_survives_the_usb_unplug`, Makefile `HW_HOTPLUG`; MIGRATION_STATUS 36 / 10 | AC-011 | 9 | Hot-plug run passes | done: `--hot-plug` 10/10 |
| 11 | Docs: `PROTOCOL.md`, `PROPERTIES.md`, primalucelib comments; full rerun | AC-008, AC-012 | 10 | All recorded runs clean | done: simulator 36/36 (12:00), hardware 9/9 (12:04), hot-plug 10/10 (12:06), unit 38/38 |
| 12 | Second-round review refinements (owner approved) and reruns | AC-009, AC-010 | 11 | Simulator and hardware suites clean | done: simulator 36/36 (12:23, after a test-race fix), repeated twice clean; hardware 9/9 (12:33) |
| 7 | Recorded run `python3 tools/run_driver_test.py aux_alto`, unit suite, reviews, archive | AC-008 | 6 | Script run clean; reviews recorded | done: final recorded runs simulator 36/36 (12:23), hardware 9/9 (12:33), hot-plug 10/10 (12:06); unit 38/38; two review rounds; archived |

## Test strategy
Each risk is tested at the lowest layer that catches it:
- the wire request, in the primalucelib unit test;
- request order, refusal and property states, in the simulator integration test (request journal).

No hardware.

## Security risks and mitigations
None new; the change uses constant strings only.

## Dependencies and architecture decisions
No new dependencies; no generator change.

## Migration, rollout, and rollback
Revert the product commit to restore `MOT_STOP`.

## Deviations and owner decisions
- Step 3 was edited before the baseline run; the edit was set aside (`git diff` saved, file restored), the baseline taken on the unchanged tree, then re-applied.
- The task was archived for delivery, then reopened (records moved back, completion record deleted, nothing committed) when the owner asked for the hardware run; steps 8–11 were added and step 7 repeated afterwards.
- `MIGRATION_STATUS.md` automated-test count updated (34 → 35 simulator cases) as required by `indigo_drivers/AGENTS.md`; Comment column preserved.
