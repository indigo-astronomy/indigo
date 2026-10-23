# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/20261023091538-dfa4-primaluce-calibration-position-check` at master `8989126b5` (equal to origin/master), with the uncommitted task changes listed in handoff.md.
Review independence: not independent. The same agent implemented and verified it. Mitigation: the new case fails without the check.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `sestosenso3_calibration_lost_position` (V2): ALERT with "run the calibration again", position 1000000 published. Without the check, X_CALIBRATE never turns ALERT (V3). |
| AC-002 | pass | The same case starts a new calibration right after the failure (a second Init). |
| AC-003 | pass | `sestosenso3_calibration`, `focuser_calibration` (V2); recorded run (V4). |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` | root | 0 | regenerated, version 31, no new warnings |
| V2 `PRIMALUCE_TEST_FILTER=calibration ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 0 | 7/7, including the new case |
| V3 check replaced by `if (false)`, rebuilt, `=sestosenso3_calibration_lost_position`, restored | root / indigo_test | 1 | X_CALIBRATE ALERT never seen; restore verified |
| V4 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 64/64 OK, recorded `2026-10-23 12:18 3.0.0.31 linux x64 simulator 64/64 OK`; TEST_SUMMARY.md regenerated |
| V5 `./build/unit/test_primalucelib` | indigo_test | 0 | 26/26 |
| V6 `./.factory/scripts/security`, `./.factory/scripts/findings check`, `git diff --cached --check` | root, Git wrapper | 0 | valid / clean |

Hardware evidence for the condition: the session logs of the requester's SESTO SENSO 3 contain `ABS_POS_STEP` 1000000 with HIZ 1 (218 replies in five sessions): after connect, after Init, and after StoreAsMinPos. The requester's report was OK, target 13995, value 1000000. The behaviour was not reproduced on demand, so the fix is not verified on hardware.

## Baseline comparison and regressions
- The baseline is 63/63 at 3.0.0.30.
- Now 64/64, with one new case.
- No regression was found.

## Findings
- **FND-20261023091538-dfa4-primaluce-calibration-position-check-001, fixed:** a calibration was published OK with a lost coordinate. Retest: V2, V3, V4.

## Manual checks and limitations
- No hardware reproduction.
- The 1000000 reading outside calibration is not handled (out of scope by the requester's choice).
- Only Linux was run.

## Verification verdict rationale
The acceptance criteria pass in the simulator, and the recorded run is clean.

## Register reconciliation
FND-20261023091538-dfa4-primaluce-calibration-position-check-001 is archived as fixed, with task reference `20261023091538-dfa4-primaluce-calibration-position-check/verification.md`.
