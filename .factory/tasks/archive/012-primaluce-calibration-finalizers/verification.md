# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/012-primaluce-calibration-finalizers` at `d37d4392d`, with the uncommitted task changes listed in handoff.md. Tasks 010 and 011 are not integrated into master `51e14099c`.
Review independence: not independent. The same agent implemented and verified it. Mitigations: the new cases fail against the previous code, and the requester ran a supervised hardware check.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `sestosenso3_calibration_restart` (V3). Fails with the 3.0.0.27 `.driver` (V2). On hardware, LEDs were refused within 0.3 s during the restart (H2). |
| AC-002 | pass | `sestosenso3_calibration`: a LED change completes while X_CALIBRATE is BUSY during the range poll (V3). Fails with 3.0.0.27 (V2). |
| AC-003 | pass | `sestosenso3_abort_during_restart` (V3). Fails with 3.0.0.27 (V2). On hardware the abort acted in the same millisecond (H2). |
| AC-004 | pass | Same step order and 1 s spacing on hardware (H2: Init, StoreAsMinPos and GoOutToFindMaxPos 1.0 s apart). END polled past the stale maximum (H2). Existing calibration cases pass (V5). |
| AC-005 | pass | Recorded run (V5). |
| AC-006 | pass | Unit case (V4); simulator cases (V3); hardware (H1 reproduction, H2 fixed). |
| AC-007 | pass | `sestosenso3_calibration_restart` with a 5 s restart fails without the drain (V6) and passes with it (V3). Hardware H2: 11 late replies dropped, then Init succeeded. |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` | root | 0 | Regenerated. The X_CALIBRATE and abort handlers have no generated OK prologue or final update, as required for `_finalizer` references. No new warnings. |
| V2 3.0.0.27 `.driver` from HEAD, rebuilt, `PRIMALUCE_TEST_FILTER=sestosenso3`, then restored (cmp of the regenerated `.c`) | root / indigo_test | 1 | `sestosenso3_calibration` and `sestosenso3_calibration_restart` failed (X_CALIBRATE not BUSY), as did `sestosenso3_abort_during_restart`. Restore verified with `cmp`. |
| V3 `PRIMALUCE_TEST_FILTER=sestosenso3`, `=calibration`, `=abort ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 0 | All pass on the final code. |
| V4 `./build/unit/test_primalucelib` | indigo_test | 0 | 24/24, including `a_restarting_link_refuses_requests_until_probed` |
| V5 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 61/61 OK, recorded as `2026-10-23 04:02 3.0.0.28 linux x64 simulator 61/61 OK`; TEST_SUMMARY.md regenerated |
| V6 library drain removed, rebuilt, `PRIMALUCE_TEST_FILTER=sestosenso3_calibration_restart`, then restored | root / indigo_test | 1 | No GoOutToFindMaxPos after the restart, which reproduces H1b |
| V7 `./.factory/scripts/security` | root, Git wrapper | 0 | Factory security boundaries valid |
| V8 `git diff --check HEAD` | root | 0 | clean |
| V9 `python3 .factory/scripts/findings.py check` | root, Git wrapper | 0 | 47 active, 13 archived |

An intermediate recorded run (`2026-10-23 03:06 3.0.0.28 … 61/61 OK`, before the restart gate) was replaced by V5.

Hardware: the requester's SESTO SENSO 3 (firmware 1.0.9, `/dev/ttyUSB1`), driven by the requester through indigo_server and the control panel. The logs are kept in the session scratchpad and not committed.

| ID | Build | Result |
| --- | --- | --- |
| H1 | finalizers only | The restart still delayed everything. The idle position poll waited 5 s on the silent controller and the abort queued behind it (03:31:35 → 03:31:39). The LED change waited until the restart was over. |
| H1b | with the restarting gate | The LED was refused at once, but the calibration failed: the controller answered 12 queued probes after the restart, and Init exceeded `PRIMALUCE_MAX_STALE_REPLIES`. |
| H2 | with the drain (final) | START with a direction change: LED ALERT in 0.28 s, 11 late replies dropped, Init 10 s after the CAL_DIR write, then the steps 1 s apart. END published 6977 after polling past the stale 18250. START_INVERTED followed by an abort during the restart: "Calibration aborted" in the same millisecond. A move and an abort afterwards worked. |

## Baseline comparison and regressions
- The task 011 baseline is 60/60 at 3.0.0.27.
- Now 61/61: one new case, two cases extended, one wait helper.
- No regression.

## Findings
- **FND-000060, fixed:** the calibration held the device queue during its waits. Retest: V2, V3, V5, H2.
- H1 and H1b were defects of this task's own intermediate code, found on hardware before delivery. They are fixed and documented in REFACTOR.md, not registered separately.

## Manual checks and limitations
- The hardware session was manual and supervised; there is no `--hw` record.
- Linux only.
- No dedicated case covers a restart that never answers. That path shares the failure helper.

## Verification verdict rationale
All acceptance criteria pass in the simulator and on hardware. The recorded run is clean.

## Register reconciliation
FND-000060 is in `.factory/findings/archive/`, kind fixed, task reference `012-primaluce-calibration-finalizers/verification.md`. next_id is 61.
