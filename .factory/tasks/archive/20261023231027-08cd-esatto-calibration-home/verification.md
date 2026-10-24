# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-24
Revision and working-tree state: `task/20261023231027-08cd-esatto-calibration-home` at master `ba09a1367`, uncommitted task changes as listed in handoff.md.
Review independence: not independent; the same agent implemented and verified the change. Mitigations: each new case was run against the unchanged driver and failed, and a supervised hardware check was done.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `esatto_profile` (V2, V4). Hardware H0: no X_CALIBRATE on the ESATTO2. The SESTO SENSO calibration cases pass in V4. |
| AC-002 | pass | `esatto_needs_calibration` (V2, V4). Not reproducible on the unit, whose focuser is calibrated. |
| AC-003 | pass | `esatto_home_reset` (V2, V4). Hardware H1: 1000 to 500 ended ALERT at 0 with the reason; H2: 0 to 400 ended OK. |
| AC-004 | pass | Diff review: the driver comment, PROTOCOL.md and PROPERTIES.md. |
| AC-005 | pass | Recorded run 71/71 (V4). |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `build/bin/indigo_generator indigo_focuser_primaluce.driver`; `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` | driver dir / root | 0 | regenerated, `DRIVER_VERSION 0x03000027` |
| V2 `PRIMALUCE_TEST_FILTER=esatto ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 1, then 0 | first run against a stale driver library and with a too-long move (see quality-review.md); then 3/3 |
| V3 the same filter with the driver files stashed (unchanged driver) | indigo_test | 1 | all three cases fail at their new assertions |
| V4 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 71/71 OK, recorded `2026-10-24 02:22 3.0.0.39 linux x64 simulator 71/71 OK`; TEST_SUMMARY.md regenerated |
| V5 `./.factory/scripts/security`, `git diff --check`, process check | root, Git wrapper | see handoff.md | |

An earlier full run was stopped before it finished, because the simulator was changed after the hardware check (the ESATTO ARCO acknowledgement under `DEG`). V4 ran on the final sources; only REFACTOR.md and PROTOCOL.md changed after it.

Hardware: the requester's ESATTO2 (application firmware 3.05.28) with ARCO on `/dev/ttyUSB0`. The server was `build/bin/indigo_server -vvv indigo_focuser_primaluce`, and the steps were driven with `build/bin/indigo_prop_tool` while the requester watched. The logs are kept in the session scratchpad.

| ID | Step | Result |
| --- | --- | --- |
| H0 | Focuser connect | ESATTO2; no X_CALIBRATE; position 1000 |
| H1 | 1000 to 500 | readback 1270, 0 (`NOPCMD` 1), 1000, 0; ALERT at 0 with "The focuser reached its home sensor and reset its position to 0" |
| H2 | 0 to 400 | OK at 400 |
| H3 | 400 to 60000, abort after 5 s | finished before the abort (about 12000 steps/s); OK at 60000 |
| H4 | 60000 to 300000, abort after 4 s | `MOT_ABORT` done; ALERT at 107855 |
| H5 | Rotator connect | the controller's `CalRestart_MaxPosEqualMinPos` and "ARCO needs calibration"; no `ARCO` write |
| H6 | 10° move, uncalibrated | `MOVE_ABS` acknowledged under `DEG`, no movement; ALERT "The rotator needs calibration" |
| H7 | ARCO calibration, at the requester's request | `CAL_STATUS` `exec` done, `stop` after about 66 s; OK with "ARCO calibration complete" |
| H8 | 10° move, calibrated | OK at 10, at about 3°/s |
| H9 | Rotator reconnect | `CALRESTART` `MOT2` 0, no notice, 10°; no `"ARCO":0` sent in the session |

## Baseline comparison and regressions
- Baseline: 69/69 at 3.0.0.38.
- Now: 71/71, with two new cases.
- No regression was found.

## Findings
None registered. The defects are fixed within this task.

## Manual checks and limitations
- The hardware check was scripted, not a `--hw` run, so there is no hardware record.
- AC-002 was verified in the simulator only.
- Only Linux was run.

## Verification verdict rationale
The acceptance criteria pass in the simulator and, where the unit allows it, on hardware. The recorded run is clean.

## Register reconciliation
No finding.
