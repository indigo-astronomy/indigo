# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: task commit `7ce125f54` on master `b0c5cf377`, merged with origin/master `0cf556f8f` (Factory v2 upgrade, repository audit and its PrimaLuce repair: exact reply key matching, ARFLAGS, driver 28 to 29). The merge renumbers this task and its findings to the v2 format and sets the driver to 30.
Review independence: not independent. The same agent implemented and verified. Mitigations: every new case fails without its fix, and the requester supervised a hardware check.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `sestosenso3_rotator_move` and `rotator_move` (V2). Without the DEG fallback the SESTO SENSO 3 case fails (V3). Hardware H3: moves end OK. |
| AC-002 | pass | `sestosenso3_rotator_move` with the simulator angle 0.00006° short of the target (V2). With the exact comparison it settles at 44.9999 in ALERT (V4). Hardware H2/H3: 15° read back as 14.99994 and ended OK. |
| AC-003 | pass | `sestosenso3_rotator_sync` and `rotator_sync` (V2). Fails without the DEG sync acknowledgement (V5). Hardware H3: three syncs in a row ended OK. |
| AC-004 | pass | `debug_text_before_a_reply_is_skipped` (V1) and `sestosenso3_rotator_sync`; both fail without the prefix skip (V5). Hardware H3: the sync whose reply came behind `mJsonAPI_ParseCmdReq_Motor` text ended OK. |
| AC-005 | pass | `rotator_abort` (V2). Hardware H2/H3: abort sends MOT_STOP, acknowledged under MOT_STOP, and ends ALERT. Recorded run (V6). |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `./build/unit/test_primalucelib` | indigo_test | 0 | 25/25 |
| V2 `PRIMALUCE_TEST_FILTER=rotator ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 0 | 9/9 rotator cases, including the two SESTO SENSO 3 cases |
| V3 DEG move fallback removed, rebuilt, `=sestosenso3_rotator_move`, restored | root / indigo_test | 1 | `rotator_position_is(45)` failed |
| V4 exact comparison restored in the `.driver`, rebuilt, `=sestosenso3_rotator_move`, restored | root / indigo_test | 1 | "Rotator did not settle at 45 (value 44.9999, state 3)" |
| V5 DEG sync acknowledgement removed, then prefix skip removed (each rebuilt), `=sestosenso3_rotator_sync` and the unit tests, restored | root / indigo_test | 1, 1 | both `sestosenso3_rotator_sync` and `debug text before a reply is skipped` failed each time |
| V6 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 63/63 OK, recorded `2026-10-23 05:01 3.0.0.30 linux x64 simulator 63/63 OK`; TEST_SUMMARY.md regenerated |
| V7 `./.factory/scripts/security` | root, Git wrapper | 0 | Factory security boundaries valid |
| V8 `git diff --check HEAD` | root | 0 | clean |
| V9 `python3 .factory/scripts/findings.py check` | root, Git wrapper | 0 | registry valid (run again on the merged tree, see V12) |
| V10 merged tree: `./build/unit/test_primalucelib`; `PRIMALUCE_TEST_FILTER=rotator ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 0, 0 | 26/26 (including the audit's exact-key case); 9/9 rotator cases |
| V11 merged tree: `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 63/63 OK, recorded `2026-10-23 11:07 3.0.0.30 linux x64 simulator 63/63 OK`; TEST_SUMMARY.md regenerated |
| V12 merged tree: `./.factory/scripts/findings check`, `./.factory/scripts/security`, `git diff --cached --check` | root, Git wrapper | 0 | see the delivery record in handoff.md |

Hardware: the requester's SESTO SENSO 3 with an ARCO (firmware 1.0.9, `/dev/ttyUSB1`), driven by the requester through indigo_server `-vvv`. The logs are kept in the session scratchpad and are not committed.

| ID | Build | Result |
| --- | --- | --- |
| H1 | 3.0.0.28 | Move to 10°: acknowledged as `{"res":{"cmd":{"MOT2":{"DEG":"done"}}}}`, ALERT at once while the ARCO turned (FND-20261023080442-7930-primaluce-arco-sestosenso3-001). |
| H2 | DEG move acknowledgement, 0.01° tolerance | Moves ended OK and the abort worked. The requester asked for a 0.1° tolerance. Sync was acknowledged under DEG and refused (FND-20261023080442-7930-primaluce-arco-sestosenso3-001), and one reply arrived behind debug text (FND-20261023080442-7930-primaluce-arco-sestosenso3-003). |
| H3 | final | Three syncs to 16° in a row ended OK, including one reply behind debug text. Moves to 10° and 100° worked; the 100° move was aborted (ALERT, MOT_STOP done). A further move to 10° ended OK. |

## Baseline comparison and regressions
- The task 012 baseline is 61/61.
- Now 63/63: two new SESTO SENSO 3 cases.
- One readback check in `rotator_sync` now allows 0.01°, which the SESTO SENSO 3 profile's rounding requires. A moved angle is still detected.
- No regression was found.

## Findings
- **FND-20261023080442-7930-primaluce-arco-sestosenso3-001, fixed:** the SESTO SENSO 3 ARCO move and sync acknowledgements were not accepted. Retest: V2, V3, V5, H3.
- **FND-20261023080442-7930-primaluce-arco-sestosenso3-002, fixed:** the exact angle comparison. Retest: V2, V4, H2, H3.
- **FND-20261023080442-7930-primaluce-arco-sestosenso3-003, fixed:** a debug-text prefix broke the reply. Retest: V1, V5, H3.

## Manual checks and limitations
- The hardware session was manual and supervised; there is no `--hw` record. It ran before the merge, so the audit's exact key matching was verified in the simulator and unit tests only.
- ARCO calibration on the SESTO SENSO 3 was not exercised.
- Only Linux was run.

## Verification verdict rationale
All acceptance criteria pass in the simulator and on hardware, and the recorded run is clean.

## Register reconciliation
FND-20261023080442-7930-primaluce-arco-sestosenso3-001 to FND-20261023080442-7930-primaluce-arco-sestosenso3-003 are archived (kind fixed) with task reference `20261023080442-7930-primaluce-arco-sestosenso3/verification.md`. next_id is 64.
