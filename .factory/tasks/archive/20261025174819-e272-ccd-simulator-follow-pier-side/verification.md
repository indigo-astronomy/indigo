# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Link quality-review.md for diff quality and security.md for security conclusions; do not copy those reviews or the handoff timeline.

Verdict: pass
Reviewer: verifier role, same agent as the developer (Claude Code, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `task/20261025174819-e272-ccd-simulator-follow-pier-side` at `b2e62cc29`, uncommitted task changes.
Review independence: not independent; the case failed before the change.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | V1, V2 |
| AC-002 | pass | V3 |

## Executed checks
Linux x86_64 (Ubuntu), built from `b2e62cc29`.

| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| V1 `INDIGO_TEST_CASE_FILTER=simulator_guider_camera_follows_mount ./build/integration/test_ccd_simulator` before | indigo_test | 1 | north pulse shift -1.168, -1.451 px: the expected 1.88 px reversed (east side expected: +1.105, +1.521) |
| V2 the same after | indigo_test | 0 | the mount on the west side; north -1.168, -1.451 px and west -1.464, 1.049 px match the west-side axes; PASS |
| V3 `python3 tools/run_driver_test.py ccd_simulator` | root | 0 | 26/26, recorded `2026-10-25 19:48 3.0.0.43 linux x64 simulator 26/26 OK` |
| V4 `./.factory/scripts/security`, `git -c core.whitespace=cr-at-eol diff --cached --check`, `findings check`, process check | root, Git wrapper | 0 | pass |

## Baseline comparison and regressions
25/26 before, 26/26 now.

## Findings
FND-20261025152706-abbf-fix-server-leaks-002 closed (time-dependent test).

## Manual checks and limitations
The east side of this case was not run today (the hour angle gave west); the axis model it uses is checked for both
sides by the case that sets SIDE_OF_PIER explicitly.

## Verification verdict rationale
The case no longer depends on the time of day.

## Register reconciliation
One finding closed.
