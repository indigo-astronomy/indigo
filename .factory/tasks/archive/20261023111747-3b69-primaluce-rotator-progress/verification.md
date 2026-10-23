# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/20261023111747-3b69-primaluce-rotator-progress` at master `218d3bf175b7565fe34ed60b0d6dd2f6a2511ee0`, with the uncommitted task changes listed in handoff.md.
Review independence: not independent; the same agent implemented and verified the change. Mitigation: every new check fails without its change, and the requester checked both changes on hardware.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `rotator_move` and `sestosenso3_rotator_move` (V2) fail without the progress publication (V3). Hardware H1: the angle counted 10.0, 10.4, 11.1, … during the move. |
| AC-002 | pass | All rotator cases (V2); recorded run (V5). |
| AC-003 | pass | `absolute_move`, `rotator_move` and `sestosenso3_rotator_move` check the value of the first BUSY update (V2). Without `preserve_values` it shows the request, 19000 or 200 (V4). Hardware H2: the first BUSY update shows the angle the ARCO reported (0) with target 20.556. |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` | root | 0 | regenerated as version 33; both positions use `INDIGO_COPY_TARGETS_PROCESS_CHANGE` |
| V2 `PRIMALUCE_TEST_FILTER=rotator`, `=absolute_move`, `=position`, `=move ./build/integration/test_focuser_primaluce_simulator` | indigo_test | 0 | 10, 1, 3 and 13 cases pass |
| V3 progress publication removed, rebuilt, `=rotator_move`, restored | root / indigo_test | 1 | "Rotator never published an angle between 10 and 200 while moving" for both move cases |
| V4 `preserve_values` removed, rebuilt, `=absolute_move` and `=rotator_move`, restored | root / indigo_test | 1 | "First BUSY update shows 19000 instead of 18075" and "shows 200 instead of 10" |
| V5 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 65/65 OK, recorded `2026-10-23 15:55 3.0.0.33 linux x64 simulator 65/65 OK`; TEST_SUMMARY.md regenerated |
| V6 `./build/unit/test_primalucelib` | indigo_test | 0 | 26/26 |
| V7 `./.factory/scripts/security`, `./.factory/scripts/findings check`, `git diff --cached --check` | root, Git wrapper | 0 | valid / clean |

An earlier recorded run of the progress fix alone was stopped before it completed, once the value fix was added. It wrote no record.

Hardware: the requester's SESTO SENSO 3 with an ARCO, through indigo_server `-vvv`. The logs are kept in the session scratchpad.

| ID | Build | Result |
| --- | --- | --- |
| H1 | progress publication | Move to 17°: the angle was published every poll (10.0 → 13.9 → …) and the move ended OK. The first BUSY update showed 17 as its value (FND-002). |
| H2 | final | Moves to 20.556° and 32.005° start their BUSY updates at the ARCO's reported angle (0), and the focuser move was confirmed by the requester. |

## Baseline comparison and regressions
- The baseline is 65/65 at 3.0.0.32.
- Still 65/65; existing cases were extended.
- No regression was found.

## Findings
- **FND-20261023111747-3b69-primaluce-rotator-progress-001, fixed:** the angle was not published during a move. Retest: V2, V3, V5, H1.
- **FND-20261023111747-3b69-primaluce-rotator-progress-002, fixed:** the first BUSY update showed the request as the value. Retest: V2, V4, V5, H2.

## Manual checks and limitations
- The hardware checks were manual; there is no `--hw` record.
- Only Linux was run.

## Verification verdict rationale
The acceptance criteria pass in the simulator and on hardware, and the recorded run is clean.

## Register reconciliation
FND-20261023111747-3b69-primaluce-rotator-progress-001 and FND-20261023111747-3b69-primaluce-rotator-progress-002 are archived as fixed with task reference `20261023111747-3b69-primaluce-rotator-progress/verification.md`.
