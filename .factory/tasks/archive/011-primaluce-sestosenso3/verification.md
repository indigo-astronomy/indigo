# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/011-primaluce-sestosenso3` at `4a32f088d` (contains task 010, which is not yet integrated into master `51e14099c`), with the uncommitted task changes listed in handoff.md. The owner's unrelated untracked files are untouched.
Review independence: not independent. The same agent implemented and verified. Mitigations: hardware evidence from the owner's unit, a mutation check, and library cases that fail against the previous library.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | Library cases V1. Two of them fail against the previous library. `sestosenso3_calibration_restart` passes with a simulator that answers late after the restart (V4). On hardware the driver logged "Dropped a reply that answers another request" after each restart and continued (H2, H4). |
| AC-002 | pass | `sestosenso3_uncalibrated` (V4). On hardware the uncalibrated unit refused moves with "The focuser is not calibrated" (H1). |
| AC-003 | pass | `sestosenso3_calibration`, `sestosenso3_calibration_restart` (V4). Mutation V3. Hardware H4: restart message, Init, StoreAsMinPos, GoOutToFindMaxPos, END with StopMotor and StoreAsMaxPos; CAL_MAXPOS changed from 17197 to 30975 about 0.65 s after the acknowledgement and was published without reconnecting. |
| AC-004 | pass | `sestosenso3_calibration`, `sestosenso3_calibration_restart`, `calibration_end_without_run` (V4). Hardware H3 to H4: one calibration per START, END accepted during the run. |
| AC-005 | pass (simulator) | `sestosenso3_calibration_restart`: the abort sends StopMotor and stop_calib against a simulator that ignores the ordinary stop (V4). Not exercised on hardware. |
| AC-006 | pass | `move_not_started` (V4). The hardware case it models is H2 (HIZ, no message before the fix). |
| AC-007 | pass | Recorded run V5: 60/60, with every SESTO SENSO 2 case unchanged and passing. |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `make build/unit/test_primalucelib && ./build/unit/test_primalucelib` | indigo_test | 0 | 23/23 PASS. Earlier in the task, against the previous library, `a_duplicate_reply_is_discarded` and `a_late_reply_is_skipped` failed. |
| V2 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` | root | 0 | The generator regenerated the `.c` from the `.driver`. No new warnings (the pre-existing format-truncation warning remains). |
| V3 range-poll mutation (`&& max_position != previous_max` removed), rebuild, `PRIMALUCE_TEST_FILTER=sestosenso3_calibration`, restore | root / indigo_test | 1 | `Calibrated to 100000, stopped at 2028`, FAIL. After restore, PASS and the `.c` was regenerated (grep count 1). |
| V4 `PRIMALUCE_TEST_FILTER=sestosenso3`, `=calibration`, `=move_not_started ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 0 | All new cases pass, after one fix: the SESTO SENSO 3 abort branch returned before the generated FOCUSER_ABORT_MOTION update, which `sestosenso3_calibration_restart` caught. |
| V5 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 60/60 OK, recorded as `2026-10-23 02:17 3.0.0.27 linux x64 simulator 60/60 OK`; TEST_SUMMARY.md regenerated. |
| V6 `python3 tools/run_driver_test.py focuser_primaluce --no-record`, twice, after the calibration guard and the range poll | root | 0, 0 | 54 + 1 cases each, before the new cases were added. |
| V7 `./.factory/scripts/security` | root, Git wrapper (see handoff) | 0 | Factory security boundaries valid. |
| V8 `git diff --check HEAD -- . ':!*.vcxproj*'` | root | 0 | clean |
| V9 `python3 .factory/scripts/findings.py check` | root, Git wrapper | 0 | 47 active, 12 archived; task evidence valid |

Hardware sessions on the owner's SESTO SENSO 3 (firmware 1.0.9, `/dev/ttyUSB0` and later `/dev/ttyUSB1`), using `build/bin/indigo_server -vvv indigo_focuser_primaluce` with indigo_control_panel. The owner drove each session; the server logs are kept in the session scratchpad and are not committed.

| ID | Session | Result |
| --- | --- | --- |
| H0 | Probe of the delivered unit | CAL_MINPOS = CAL_MAXPOS = 0, CALRESTART.MOT1 1, GOTO acknowledged without motion; CAL_DIR write followed by a restart and late replies; `[mLogs_DefaultSetup]` before LOGLEVEL; DIMLEDS acknowledged under `cmd` |
| H1 | Library fix, uncalibrated | Moves refused with a reason. The first calibration's START failed on the CAL_DIR restart (FND-000055). After the fix, START ran, but END read CAL_MAXPOS 0 and moves were refused (FND-000056, first form). |
| H2 | Range read fix, not yet guarded | Moves were OK. START_INVERTED waited about 10 s; the owner's extra clicks queued two more calibrations and END was refused. The run went to 29697 with motor fault flags and the owner unplugged the focuser (FND-000057). After replug: HIZ 1, GOTO acknowledged without motion, ALERT without a reason (FND-000059). |
| H3 | Calibration guard | START_INVERTED once, with the restart message. END was accepted, but the driver published the stale maximum 15237 while the position was about 17000 (FND-000056, second form). |
| H4 | Stale maximum fix | Reconnect showed 17197. START (direction normal, with restart), END: the driver polled through 17197 until 30975 and published it. Moves afterwards were OK. |

## Baseline comparison and regressions
- The task 010 baseline is 55/55 on Linux x64 at `4a32f088d`.
- After this task, the 55 earlier cases still pass, together with the 5 new ones (V5).
- The hardware defects were reproduced from the session logs before each fix.
- No regression was found.

## Findings
All are fixed, archived and listed in the central register.

- **FND-000053:** link stayed shifted after a late reply. Retest: V1, V4, H2 to H4.
- **FND-000054:** moves were accepted on an uncalibrated SESTO SENSO 3. Retest: V4, H1.
- **FND-000055:** START failed because of the CAL_DIR restart. Retest: V4, H3, H4.
- **FND-000056:** range read before the controller stored it. Retest: V3, V4, H4.
- **FND-000057:** X_CALIBRATE requests were accepted during a step, and END was refused. Retest: V4, H3, H4.
- **FND-000058:** abort did not end a SESTO SENSO 3 calibration run. Retest: V4 only.
- **FND-000059:** a move acknowledged but never run gave no reason. Retest: V4.

FND-000052 (WiFi credential escaping) stays open and out of scope.

## Manual checks and limitations
- The hardware sessions were manual and supervised, not a `--hw` scripted run, so no hardware record was written.
- Hot-plug was not part of this task. The one unplug was the owner's emergency stop in H2.
- macOS and Windows builds were not run.
- The SESTO SENSO 3 abort during calibration is verified with the simulator only.

## Verification verdict rationale
Every acceptance criterion passes. AC-005 passes on the simulator only, as the limitations above say. The recorded run is clean.

## Register reconciliation
FND-000053 to FND-000059 are in `.factory/findings/archive/`, kind fixed, task reference `011-primaluce-sestosenso3/verification.md`. next_id is 60. FND-000052 stays active.
