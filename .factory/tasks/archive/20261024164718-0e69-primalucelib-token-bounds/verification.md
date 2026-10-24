# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-24
Revision and working-tree state: `task/20261024164718-0e69-primalucelib-token-bounds` at master `679053b19`, uncommitted task changes.
Review independence: not independent. Mitigation: the regression case was run against the unchanged library first and overflowed.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | V1 (overflow on the unchanged library), V2, V3 |
| AC-002 | pass | V2, V4, V5, V6 |

## Executed checks
All commands ran on Linux x86_64 (Ubuntu, gcc 13.3.0, Python 3), from the repository root unless noted, with `libindigo` built from the sources of `d75fb905d` (unchanged since).

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `unit/test_primalucelib.c` built with `-fsanitize=address,undefined` against `primalucelib.[ch]` of `HEAD` | indigo_test | 1 | `global-buffer-overflow` in `primalucelib_find_token` at `primalucelib.c:399` (the skip loop), from the new case |
| V2 `./build/unit/test_primalucelib` (built by the unit make rule against `libprimalucelib.a`) | indigo_test | 0 | 30/30 PASS |
| V3 the V1 build against the fixed sources | indigo_test | 0 | 30/30 PASS, no sanitizer report |
| V4 `python3 tools/run_driver_test.py focuser_primaluce` | root | 0 | 72/72, recorded `2026-10-24 20:16 3.0.0.41 linux x64 simulator 72/72 OK` |
| V5 `python3 tools/run_driver_test.py aux_giotto` | root | 0 | 26/26, recorded `2026-10-24 19:50 3.0.0.3 linux x64 simulator 26/26 OK` |
| V6 `python3 tools/run_driver_test.py aux_alto` | root | 0 | 29/29, recorded `2026-10-24 19:51 3.0.0.3 linux x64 simulator 29/29 OK` |
| V7 `./.factory/scripts/security`, `git -c core.whitespace=cr-at-eol diff --cached --check`, `findings check`, process check | root, Git wrapper | 0 | pass |

## Baseline comparison and regressions
Unit suite 29/29 before, 30/30 with the new case; the three simulator suites have the same counts as their previous records.

The first recorded focuser run (19:54) failed `focuser_calibration` once: END was published ALERT instead of OK
(`X_CALIBRATE never published state 1 (current 3)`, test line 1300). It did not recur in 14 isolated runs of the case
(8 idle, 6 with every core loaded by busy loops) nor in the repeated full run V4. The library change is equivalent to the old
walk for replies below 1024 tokens (the calibration replies have a few dozen), so it cannot change that path. The failure is
recorded as an unexplained, non-reproduced timing flake of the calibration case; its record was replaced by V4.

## Findings
FND-20261024121920-3bd2-repository-audit-017 fixed.

## Manual checks and limitations
No hardware run for this change; a 1024-token reply is not produced by real controllers in normal operation. Only Linux.

## Verification verdict rationale
The reproduced overflow no longer happens, and the three dependent drivers pass their simulator suites.

## Register reconciliation
FND-20261024121920-3bd2-repository-audit-017 archived as fixed with this report as evidence.
