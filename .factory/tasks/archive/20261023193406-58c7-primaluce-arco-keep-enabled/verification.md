# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/20261023193406-58c7-primaluce-arco-keep-enabled` at origin/master `110f87e54`, uncommitted task changes as listed in handoff.md.
Review independence: not independent; the same agent implemented and verified the change. Mitigation: both cases fail with the old disconnect, and the hardware cycles confirm the fix.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `rotator_lifecycle` (V2) fails with the old disconnect: "expected 0, got 1" (V3). Hardware H2: no `"ARCO":0` in three cycles. |
| AC-002 | pass | `rotator_lifecycle` and `rotator_metadata` (V2). Hardware H1: one switch-on, only while the port was off. |
| AC-003 | pass | `sestosenso3_arco_stays_calibrated` (V2) fails with the old disconnect (V3). Hardware H2: `CALRESTART.MOT2` stays 0 after every cycle. |
| AC-004 | pass | Recorded run 67/67 (V4). |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` | root | 0 | regenerated, version 35 |
| V2 `PRIMALUCE_TEST_FILTER=rotator`, `=shared`, `=arco ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 0 | 10, 1 and 1 cases pass |
| V3 old `on_disconnect` (`primalucelib_rotator_enable(LINK, false)`) restored, rebuilt, `=rotator_lifecycle` and `=sestosenso3_arco_stays_calibrated`, fix restored | root / indigo_test | 1 | both fail; restore verified |
| V4 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 67/67 OK, recorded `2026-10-23 22:43 3.0.0.35 linux x64 simulator 67/67 OK`; TEST_SUMMARY.md regenerated |
| V5 `./build/unit/test_primalucelib` | indigo_test | 0 | 27/27 |
| V6 `./.factory/scripts/security`, `./.factory/scripts/findings check`, `git diff --cached --check`, process check from origin/master | root, Git wrapper | 0 | valid / clean |

Hardware: the requester's SESTO SENSO 3 with ARCO on `/dev/ttyUSB0`. The requester's server (old driver) was replaced by `build/bin/indigo_server -vvv indigo_focuser_primaluce`, and the cycles were driven with `build/bin/indigo_prop_tool`. The log is kept in the session scratchpad.

| ID | Step | Result |
| --- | --- | --- |
| H0 | Earlier session logs (old driver) | Disconnect `"ARCO":0`, reconnect `"ARCO":1`, `CALRESTART.MOT2` 0 → 1 |
| H1 | First connect with the new driver | The full state showed `"ARCO":0` (left off by the old driver) and `MOT2` 0. The single switch-on gave `MOT2` 1, confirming the cause. |
| H1a | ARCO calibration (requester watching) | `CAL_STATUS` exec → stop, X_CALIBRATE_A OK |
| H2 | Rotator cycle; focuser + rotator cycle with the serial port closed and reopened; rotator cycle | Every reconnect: `"ARCO":1`, `CALRESTART.MOT2` 0. Over the whole session, `"ARCO":0` was sent 0 times and `"ARCO":1` once (H1); no calibration notice after the calibration. |

## Baseline comparison and regressions
- Baseline: 66/66 at 3.0.0.34. Now 67/67 (one new case, one updated case).
- No regression was found.

## Findings
- **FND-20261023193406-58c7-primaluce-arco-keep-enabled-001, fixed:** the ARCO calibration was lost after a disconnect and reconnect. Retest: V2, V3, V4, H2.

## Manual checks and limitations
- The hardware cycles were scripted, not a `--hw` run; there is no hardware record.
- Power-cycle retention was not tested.
- Only Linux was run.

## Verification verdict rationale
The acceptance criteria pass in the simulator and on hardware, and the recorded run is clean.

## Register reconciliation
FND-20261023193406-58c7-primaluce-arco-keep-enabled-001 is archived as fixed, with task reference `20261023193406-58c7-primaluce-arco-keep-enabled/verification.md`.
