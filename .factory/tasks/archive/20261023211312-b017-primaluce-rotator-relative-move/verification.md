# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-24
Revision and working-tree state: `task/20261023211312-b017-primaluce-rotator-relative-move` at master `17681f4f8f747010922e4c149dc02bada4661985`, uncommitted task changes as listed in handoff.md.
Review independence: not independent; the same agent implemented and verified the change. Mitigations: a simulator case caught the rounding defect before the fix, and a supervised hardware check was done.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `rotator_relative_move` and `sestosenso3_rotator_relative_move` (V2). Hardware H1: +10 and -20 end OK. |
| AC-002 | pass | The same cases (V2). Hardware H2: -185 is refused with no `MOVE_ABS`. |
| AC-003 | pass | The same cases (V2). Hardware H3: an abort during +90 ends both properties ALERT. |
| AC-004 | pass | `sestosenso3_rotator_relative_move` failed before the origin change (-20.00006) and passes after it (V2). Hardware H1: -20 from 9.99492 went to exactly -10. |
| AC-005 | pass | Recorded run 69/69 (V3). |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` | root | 0 | regenerated, version 38; `INDIGO_COPY_TARGETS_PROCESS_CHANGE(ROTATOR_RELATIVE_MOVE_PROPERTY, …)` |
| V2 `PRIMALUCE_TEST_FILTER=rotator ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 1, then 0 | first run: `sestosenso3_rotator_relative_move` waited for `"DEG":-20` (sent -20.0001); after the origin change: 12/12 |
| V3 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 69/69 OK, recorded `2026-10-24 00:18 3.0.0.38 linux x64 simulator 69/69 OK`; TEST_SUMMARY.md regenerated |
| V4 `./.factory/scripts/security`, `git diff --cached --check`, process check | root, Git wrapper | 0 | valid / clean |

Hardware: the requester's SESTO SENSO 3 with ARCO on `/dev/ttyUSB0`. The server was `build/bin/indigo_server -vvv indigo_focuser_primaluce`, and the moves were driven with `build/bin/indigo_prop_tool` while the requester watched. The log is kept in the session scratchpad.

| ID | Step | Result |
| --- | --- | --- |
| H0 | Connect | `"ARCO":1`, `CALRESTART.MOT2` 0; no calibration notice |
| H1 | +10, then -20 | `MOVE_ABS` 10 (read back 9.99492), then `MOVE_ABS` -10; both relative moves OK |
| H2 | -175 from -10 | ALERT "The target -185 is outside the rotator range"; no `MOVE_ABS` sent |
| H3 | +90 from -10, abort after 2.5 s | `MOVE_ABS` 80, `MOT_STOP` done; ROTATOR_RELATIVE_MOVE and ROTATOR_POSITION ALERT at -4.75 |
| H4 | Disconnect | no `"ARCO":0` sent |

## Baseline comparison and regressions
- Baseline: 67/67 at 3.0.0.37.
- Now: 69/69, with one case run on two profiles.
- No regression was found.

## Findings
None registered. The rounding defect was found and fixed within this task, before delivery.

## Manual checks and limitations
- The hardware check was scripted, not a `--hw` run, so there is no hardware record.
- Only Linux was run.

## Verification verdict rationale
The acceptance criteria pass in the simulator and on hardware, and the recorded run is clean.

## Register reconciliation
No finding.
