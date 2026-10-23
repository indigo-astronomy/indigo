# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-23
Revision and working-tree state: `task/20261023132326-3447-primaluce-field-errors` at master `82a071af5` (merged with origin/master `825b4cab2`), uncommitted task changes as listed in handoff.md.
Review independence: not independent; the same agent implemented and verified the change. Mitigation: both new cases fail without the change.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `field_errors_refuse_calibration_requests` (V2); fails without the acknowledgement check (V3) |
| AC-002 | pass | `sestosenso3_field_errors`: START ends ALERT with "Error: invalid parameter", and no StoreAsMinPos follows (V2). Without the check, START does not end ALERT (V3). |
| AC-003 | pass | the same case: "Move refused: Error: invalid parameter" for the focuser and rotator moves, "Sync refused: …" for the sync (V2) |
| AC-004 | pass | recorded run 66/66 (V4) |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V0 merged tree before the change: `make -j8 all`; `./build/unit/test_primalucelib`; `PRIMALUCE_TEST_FILTER=sestosenso3` and `=rotator` | root / indigo_test | 0 | build OK; 26/26; 9 and 10 cases pass |
| V1 `make -C indigo_drivers/focuser_primaluce -f ../../Makefile.drv all` | root | 0 | regenerated, version 34 |
| V2 `./build/unit/test_primalucelib`; `PRIMALUCE_TEST_FILTER=sestosenso3`, `=calibration`, `=rotator` | indigo_test | 0 | 27/27; 10, 8 and 10 cases pass |
| V3 acknowledgement check and acknowledgement reason removed, rebuilt, unit and `=sestosenso3_field_errors`, restored | root / indigo_test | 1 | both new cases fail; restore verified |
| V4 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 66/66 OK, recorded `2026-10-23 16:31 3.0.0.34 linux x64 simulator 66/66 OK`; TEST_SUMMARY.md regenerated |
| V5 `./.factory/scripts/security`, `./.factory/scripts/findings check`, `git diff --cached --check`, process check from origin/master | root, Git wrapper | 0 | see handoff.md delivery |

The protocol review itself is documented in REFACTOR.md ("Review against the SESTO SENSO 3 protocol document").

## Baseline comparison and regressions
- Baseline: 65/65 at 3.0.0.33. Now 66/66, with one new simulator case.
- No regression was found.

## Findings
- **FND-20261023132326-3447-primaluce-field-errors-001, fixed:** field-level refusals were ignored. Retest: V2, V3, V4.

## Manual checks and limitations
- There is no hardware check.
- There is no SESTO SENSO 2 hardware here; INDI requires "done" from both models.
- Only Linux was run.

## Verification verdict rationale
The acceptance criteria pass, and the recorded run is clean.

## Register reconciliation
FND-20261023132326-3447-primaluce-field-errors-001 is archived as fixed with task reference `20261023132326-3447-primaluce-field-errors/verification.md`.
