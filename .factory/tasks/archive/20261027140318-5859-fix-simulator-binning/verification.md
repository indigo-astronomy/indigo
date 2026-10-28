# Verification report

Verdict: pass
Reviewer: Claude Fable 5.1 (Claude Code desktop app) acting as verifier
Date: 2026-10-27
Revision and working-tree state: task branch `task/20261027140318-5859-fix-simulator-binning` on master `42c0960a2` during the first runs, `32d7396d7` (the owner's server commit) at the task commit `855c8f571`; the merged candidate `34f1dea8f` (origin/master `1e7c23390`) rerun on 2026-10-28, see Merged candidate
Review independence: not independent, the same agent implemented the change.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `test_agent_solver_simulator rosetta` 9/9; `rosetta camera binned`: WCS RA 6.52689 h Dec 5.0933° angle 179.11° scale 16.873"/px parity −1 index 2° size 3.750 x 2.812° (expected 6.52687 h / 5.095° / 179.2° / 16.880"/px), centre error 6.1"; with debug logging 71 detected stars, 6 of 23 consistent matches, rms 4.42" |
| AC-002 | pass | `simulator_imager_binning_averages_the_bin_block`: bin 1 128 x 96, bin 2 64 x 48 (3 blocks with a sum beyond 65535), bin 4 32 x 24 (2 such blocks), every pixel within [mean, mean + 127] |
| AC-003 | pass | `simulator_guider_binning_scales_the_star_profile`: unbinned star at 265.15, 884.44, root mean square radius 2.00 px, flux 97 945 ADU; 2 x 2 binned at 132.60, 442.17, radius 1.00 px, flux 24 489 ADU (ratio 0.2501) |
| AC-004 | pass | `test_ccd_simulator` 28/28 (recorded run), `test_agent_imager` 39/39, `test_agent_guider` 95/95, `test_agent_astrometry` 54/54, `test_agent_solver_simulator rosetta` 9/9, all against the rebuilt `build/drivers/indigo_ccd_simulator.a` |
| AC-005 | pass | `version = 45` / `DRIVER_VERSION 0x0300002D`; `git diff indigo_ccd_simulator.c` shows the version and the two rendering blocks only, written by the unchanged generator through `Makefile.drv`; README `## Testing` line `2026-10-27 15:19 3.0.0.45 mac arm64 simulator 28/28 OK`, TEST_SUMMARY.md regenerated, MIGRATION_STATUS.md `28 / 0` (ccd_simulator) and `135 / 0` (agent_solver), both REFACTOR.md files updated, the solver test header no longer excludes camera binning |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make -C indigo_drivers/ccd_simulator -f ../../Makefile.drv all` | repo root, Darwin arm64, Apple clang, universal arm64/x86_64 | 0 | `.c`, `.h`, `_main.c` regenerated, archive and dylib rebuilt; generator warnings about hidden properties are the pre-existing ones |
| `indigo_test/build/integration/test_ccd_simulator simulator_imager_binning_averages_the_bin_block` | repo root | 0 | pass, see AC-002 |
| `indigo_test/build/integration/test_ccd_simulator simulator_guider_binning_scales_the_star_profile` | repo root | 0 | pass, see AC-003 |
| `python3 tools/run_driver_test.py ccd_simulator` | repo root | 0 | driver and test rebuilt, `test_ccd_simulator` 28/28, recorded `2026-10-27 15:19 3.0.0.45 mac arm64 simulator 28/28 OK`, TEST_SUMMARY.md regenerated |
| `indigo_test/build/integration/test_agent_solver_simulator rosetta` | repo root, indexes in `~/.indigo/solver` (nothing downloaded) | 0 | 9/9 cases passed |
| `INDIGO_TEST_DEBUG=1 indigo_test/build/integration/test_agent_solver_simulator "rosetta camera binned"` (summed and clamped variant, three runs) | repo root | 1 | deterministic failure: 106 detected stars, 2905 and 1433 quads matched by hash, 42 and 40 by scale, `no consistent set of matches found (5 of 57)` and `(5 of 53)`; decided against the sum |
| `INDIGO_TEST_DEBUG=1 indigo_test/build/integration/test_agent_solver_simulator "rosetta downsampled"` (reference) | repo root | 0 | the agent's averaged frame: 96 detected stars, 7 of 32 consistent matches, rms 4.76" |
| `INDIGO_TEST_DEBUG=1 indigo_test/build/integration/test_agent_solver_simulator "rosetta camera binned"` (averaged variant) | repo root | 0 | 71 detected stars, 6 of 23 consistent matches, rms 4.42", centre error 6.1" |
| `indigo_test/build/integration/test_agent_imager` | repo root | 0 | 39/39 |
| `indigo_test/build/integration/test_agent_guider` | repo root | 0 | 95/95 |
| `indigo_test/build/integration/test_agent_astrometry` | repo root | 0 | 54/54 |
| `./.factory/scripts/check`, `./.factory/scripts/security` | repo root | 0 | passed (findings valid: 29 active, 199 archived at the time of the run) |
| `git diff --check` | repo root | 0 | clean |

## Baseline comparison and regressions
Before the fix the camera binned Rosetta case did not solve (task 20261027131225-a980) and the binned frames held
one sample per block; the new simulator cases fail against the old rendering by construction (test-plan.md). No other
case changed its result: the imager, guider and astrometry suites pass with the same counts as their records, the
Rosetta set grew from 8 to 9 cases. The first version of this task (sum clamped at 65535) passed the simulator cases
and the imager suite but not the Rosetta case; it was replaced by the mean before the recorded run.

## Found defects
- Reproduced: a 2 x 2 binned Rosetta frame of `CCD Imager Simulator` did not solve in the Solver Agent while the
  agent's own 2 x 2 downsampling of the unbinned frame did (task 20261027131225-a980 left camera binning out of its
  Rosetta set). Root cause: `create_frame()` binned the imager by taking every `horizontal_bin`-th /
  `vertical_bin`-th pixel of the fixed image, so a binned star kept one sample of its core and the stars became single
  pixel spikes whose quad codes no centroid can place. Fix: a binned imager pixel is the integer mean of its bin block
  (`indigo_ccd_simulator.driver`, version 45). Regression: `simulator_imager_binning_averages_the_bin_block` and
  `test_agent_solver_simulator "rosetta camera binned"`.
- Source audit: the guider camera divided the star centres by the bin but rendered the same `exp(-(x² + y²) / 4)`
  profile in binned pixels, so a binned star was as wide in binned pixels as an unbinned one in unbinned pixels. Fix:
  the profile is evaluated in unbinned pixels, `(centre - pixel) * bin`, with the unbinned amplitude. Regression:
  `simulator_guider_binning_scales_the_star_profile`.
- Rejected by measurement: a first version summed the block, clamped at 65535 like a saturated well (60 of the
  480 000 2 x 2 blocks of the 16-bit image exceed it), with the guider amplitude multiplied by the block size. That
  frame did not solve either, deterministically in three runs: 106 detected stars (96 on the agent's averaged frame),
  57 stars matched by votes but only 5 consistent (5 of 53 with the 5° index). The averaged camera frame gives 71
  detected stars and solves with 6 of 23 consistent matches, the agent's own downsampling with 7 of 32. Whether the
  clamped cores or the additional faint detections break the summed frame was not separated; registered as
  FND-20261027140318-5859-fix-simulator-binning-002 for the solver.
- Source audit, out of scope: the hot pixel loop tests `y > frame_height` where `y >= frame_height` is meant,
  FND-20261027140318-5859-fix-simulator-binning-001.

## Findings
- FND-20261027140318-5859-fix-simulator-binning-001 (functional, low, deferred): hot pixel row bound `y > frame_height`
  in `create_frame()`, out of scope, found by source audit.
- FND-20261027140318-5859-fix-simulator-binning-002 (functional, low, deferred): thin matching margin of the solver on
  the 800 x 600 Rosetta field (6 of 23 / 7 of 32 consistent matches pass, 5 of 57 fails), for the solver.

## Manual checks and limitations
Same-agent review. Linux and Windows not run. The full `test_agent_solver_simulator` suite (120 guider setups) was
not re-run: the guider rendering at binning 1 is unchanged by construction (`(centre - pixel) * 1`), so its record of
2026-10-27 14:37 stands; MIGRATION_STATUS.md counts the re-added case. The owner's `indigo_server/indigo_server.c` change
(static solver agent registration) was committed by the owner as `32d7396d7` on this branch during the task; it is not
part of this task and the server was not rebuilt or tested here.

## Verification verdict rationale
All five acceptance criteria have direct evidence from the recorded run, the Rosetta set and the agent suites; the
remaining limitations are the same-agent review, the platforms not run and the two deferred findings.

## Merged candidate
Origin/master `1e7c23390` (20 commits: FITS, JPEG, hint and 0.5° index cases of the solver test, solver noise filter
and hints, archived REFACTOR.md files, driver build lists, Pentax validation) was merged as `34f1dea8f` on 2026-10-28.
Rebuilt `indigo_agent_solver` (3.0.0.9) and `indigo_ccd_simulator`, relinked the test binaries and rerun:

| Command | Exit code | Result |
| --- | --- | --- |
| `python3 tools/run_driver_test.py ccd_simulator` | 0 | 28/28, recorded `2026-10-28 08:24 3.0.0.45 mac arm64 simulator 28/28 OK`, TEST_SUMMARY.md regenerated |
| `make -C indigo_drivers/agent_solver -f ../../Makefile.drv all` | 0 | `indigo_agent_solver` 3.0.0.9 rebuilt |
| `indigo_test/build/integration/test_agent_solver_simulator rosetta` | 0 | 11/11 (master's `rosetta fits` and `rosetta jpeg` included); `rosetta camera binned`: RA 6.52682 h Dec 5.0934° angle 179.19° 16.888"/px, centre error 6.4" |
| `indigo_test/build/integration/test_agent_imager` | 0 | 39/39 |
| `indigo_test/build/integration/test_agent_guider` | 0 | 95/95 |
| `indigo_test/build/integration/test_agent_astrometry` (in the chain, machine loaded) | 1 | 53/54: `index abort download` timed out waiting for the ALERT state after aborting the hanging fake download, a bounded wait unrelated to the simulator (master changed only that driver's archived notes) |
| `indigo_test/build/integration/test_agent_astrometry` (alone) | 0 | 54/54 |
| `python3 .factory/scripts/process_check.py --base 1e7c23390 --head HEAD` | see the external delivery log | run on the evidence commit |

## Final test summary
Simulated (hardware-free) tests on the merged candidate: 227 run, 227 passed (`test_ccd_simulator` 28,
`test_agent_solver_simulator rosetta` 11, `test_agent_imager` 39, `test_agent_guider` 95, `test_agent_astrometry` 54 on
its solo run). Hardware tests: 0 run, 0 passed. The 120 guider setups and the 0.5° index case of the solver suite were
not rerun here (unaffected by the simulator change at binning 1; master's record of 2026-10-28 02:11 covers them).

## Register reconciliation
Two findings created from this task, both active and deferred, owner coordinator; nothing closed.
