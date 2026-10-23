# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/20261023101445-b9b5-primaluce-needs-calibration-message` at master `f019b0047` (equal to origin/master), with the uncommitted task changes listed in handoff.md.
Review independence: not independent. The same agent implemented and verified the change. Mitigation: both cases fail without the change, and the requester checked it on hardware.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `sestosenso3_rotator_needs_calibration` (V2). Hardware H1: four moves that did not run ended ALERT with "The rotator needs calibration". |
| AC-002 | pass | `needs_calibration`, extended (V2). |
| AC-003 | pass | `sestosenso3_rotator_needs_calibration`: after an ARCO calibration the move ends OK (V2). |
| AC-004 | pass | `needs_calibration` moves that run end OK; recorded run (V4). |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` | root | 0 | regenerated, version 32 |
| V2 `PRIMALUCE_TEST_FILTER=rotator`, `=calibration`, `=needs_calibration`, `=move_not_started ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 0 | all pass |
| V3 calibration messages removed, rebuilt, `=needs_calibration`, restored | root / indigo_test | 1 | both `needs_calibration` and `sestosenso3_rotator_needs_calibration` fail; restore verified |
| V4 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 65/65 OK, recorded `2026-10-23 13:36 3.0.0.32 linux x64 simulator 65/65 OK`; TEST_SUMMARY.md regenerated |
| V5 `./build/unit/test_primalucelib` | indigo_test | 0 | 26/26 |
| V6 `./.factory/scripts/security`, `./.factory/scripts/findings check`, `git diff --cached --check` | root, Git wrapper | 0 | valid / clean |

Hardware: the requester's SESTO SENSO 3 with an ARCO flagged for calibration (`CALRESTART.MOT2` 1), using indigo_server `-vvv` and the requester's control panel; the log is kept in the session scratchpad.

| ID | Result |
| --- | --- |
| H0 (3.0.0.31) | The move to 12° was acknowledged `DEG: done`, the ARCO did not turn (HIZ 1), and ALERT was published without a message. |
| H1 (3.0.0.32) | The connect notice "ARCO needs calibration" was shown. Four moves that did not run ended ALERT with "The rotator needs calibration". Moves already at the target ended OK. The requester then started the ARCO calibration. |

## Baseline comparison and regressions
- The baseline is 64/64 at 3.0.0.31.
- Now 65/65: one new case and one extended case.
- No regression was found.

## Findings
- **FND-20261023101445-b9b5-primaluce-needs-calibration-message-001, fixed:** moves of a flagged motor ended ALERT without the reason. Retest: V2, V3, V4, H1.

## Manual checks and limitations
- The hardware check was manual; there is no `--hw` record.
- The focuser message was not seen on hardware, because the focuser was not flagged.
- Only Linux was run.

## Verification verdict rationale
The acceptance criteria pass in the simulator and on hardware, and the recorded run is clean.

## Register reconciliation
FND-20261023101445-b9b5-primaluce-needs-calibration-message-001 is archived as fixed with task reference `20261023101445-b9b5-primaluce-needs-calibration-message/verification.md`.
