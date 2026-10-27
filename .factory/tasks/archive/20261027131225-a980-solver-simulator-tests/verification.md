# Verification report

Verdict: pass
Reviewer: Claude Fable 5.1 (Claude Code desktop app) acting as verifier
Date: 2026-10-27
Revision and working-tree state: task branch `task/20261027131225-a980-solver-simulator-tests` on master `b7a52e6c4`, uncommitted working tree
Review independence: not independent, the same agent implemented the change.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `test_agent_solver_simulator rosetta`: 8/8; full resolution RA 6.52684 h Dec 5.0934° angle 179.19° 8.438"/px parity −1 index 2°, centre error 6.4" (expected 6.52687 h / 5.095° / 179.2° / 8.44"/px); downsampled, sub-frame, JNow, index 2 only, index 5 only, wrong scale hint, three repeated solves all within tolerance |
| AC-002 | pass | recorded run: 120/120 guider setups (24 positions down to Dec ±85°, both epochs, both pier sides, seven sizes, noise 100 to 400 ADU, hot pixels/columns/rows), centre errors 1 to 8", angles within 0.03° of the expectation (0.2° tolerance); six of them through the Guider Agent |
| AC-003 | pass | reference `test_agent_astrometry_solver "solve only"` (real astrometry.net, index 4112): angle 36.00°, parity 1 for rotation 36° on the guider simulator; the sweep after the fix reports parity 1 and rotation + 180·(1 − pier) on the same frames, the Rosetta frame parity −1 and 179.19° as before |
| AC-004 | pass | first run with empty `~/.indigo/solver`: "index TYC2_2 is not in /Users/polakop/.indigo/solver, downloading it once", same for TYC2_5, both downloaded by the agent in 44 s; the recorded run and every later run print no download message (grep of the run log: 0 matches) |
| AC-005 | pass | `test_agent_solver_core` 9/9 (mirrored field expects parity 1 and angle 197°), `test_agent_solver` 6/6 (parity −1), `make -C indigo_test test-unit` whole suite passed, `test_agent_astrometry` 54/54 |
| AC-006 | pass | README `## Testing` line `2026-10-27 14:37 3.0.0.5 mac arm64 simulator 134/134 OK` written by `tools/run_driver_test.py`, TEST_SUMMARY.md regenerated, REFACTOR.md section, MIGRATION_STATUS.md `134 / 0`, `DRIVER_VERSION 0x03000005`, PROPERTIES.md unchanged (no property change) |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `python3 tools/run_driver_test.py agent_solver` | repo root, Darwin arm64 | 0 | driver rebuilt, `test_agent_solver` 6/6, `test_agent_solver_simulator` 128/128, recorded 134/134 OK |
| `indigo_test/build/integration/test_agent_solver_simulator rosetta` | repo root | 0 | 8/8 (after removing the camera binning case) |
| `indigo_test/build/integration/test_agent_solver_simulator guider` (build before the precession expectation) | repo root | 1 | 115/120, the five failures were Dec +85° JNow frames whose expected angle lacked the 0.83° precession turn; the recorded run with the corrected expectation passed all 120 |
| `INDIGO_SOLVER_SETUP="7.5,-60,{0,90},0,1600,1200,9.5,{2000,0},100,0,0,0,0" … "guider 001"` | repo root | 0 | angles 179.97° / 269.97° (J2000) and 179.71° / 269.69° (JNow, expectation 179.72° / 269.72°) |
| `INDIGO_SOLVER_SETUP="2,85,36,0,1600,1200,9.5,0,250,0,500,0,0" … "guider 001"` | repo root | 0 | angle 215.17° equals the expectation 215.17° |
| `indigo_test/build/integration/test_agent_astrometry_solver "solve only"` | repo root, astrometry.net in /usr/local/bin | 0 | reference angle 36.00°, parity 1, 11.99"/px (after rebuilding the stale driver archives it links) |
| `indigo_test/build/unit/test_agent_solver_core` | repo root | 0 | 9/9 |
| `indigo_test/build/integration/test_agent_solver` | repo root | 0 | 6/6 |
| `make -C indigo_test test-unit` | repo root | 0 | whole native unit suite passed |
| `indigo_test/build/integration/test_agent_astrometry` | repo root | 0 | 54/54 (platesolver base regression) |
| `./.factory/scripts/check`, `./.factory/scripts/security` | repo root | 0 | passed |
| `git diff --check`, `plutil -lint indigo.xcodeproj/project.pbxproj` | repo root | 0 | clean |
| `make -C indigo_test test-clean` | repo root | 0 | build artifacts removed after the checks |

## Baseline comparison and regressions
Before the fixes the guider frames solved to the right centre and scale but the angle was 180° off plus up to 1.1°
(tangent point), and the parity sign was inverted against astrometry.net. No other test changed its result; the
astrometry agent regression is unchanged at 54/54.

## Findings
No findings records opened: the two defects were fixed within the task (REFACTOR.md), their regressions are in the
sweep and the unit tests.

## Manual checks and limitations
The astrometry.net reference covers one frame (rotation 36°, pier 1); the angle rule for pier 0 follows the
assertion of `test_agent_astrometry_solver` (`rotation + 180`), which that suite records as passing. The CCD
simulator's camera binning (subsampling) is excluded, see REFACTOR.md. An uncommitted change of
`indigo_server/indigo_server.c` (static registration of the solver agent) exists in the working tree; it is not part
of this task and was left unstaged. Linux and Windows not run.

## Verification verdict rationale
All six acceptance criteria have direct evidence from the recorded run and the reference run; the remaining
limitations are the single-frame astrometry.net reference and the same-agent review.

## Register reconciliation
No FND records; nothing to reconcile.
