# Specification

Status: accepted (owner instruction in conversation, 2026-10-27: implement every proposal of the groups "speed and
robustness", "input images" and "diagnostics: message" of the improvement review)

## Problem and outcome
The Solver Agent solves every frame blind and ignores most of the shared `AGENT_PLATESOLVER_HINTS`, has no time
limit, honours an abort only between index attempts, detects stars on Bayer mosaics as they are, averages colour
channels, and reports a bare "Solved". After this task the agent uses the hints the way the Astrometry agent does
where the native solver can, bounds its run time, aborts promptly, handles Bayer and colour inputs properly and
reports the quality of its solution.

## In scope
- `AGENT_PLATESOLVER_HINTS` RADIUS/RA/DEC: when RADIUS > 0 the catalogue is restricted to stars within RADIUS plus
  the field extent of the hint position (RA in hours, DEC in degrees, in the hints epoch: JNow when EPOCH is 0, J2000
  otherwise). A field centred outside the cone is not solved.
- PARITY (-1, 0, 1): a solution whose parity differs from a non-zero hint is rejected and the search continues with
  the next stage or index. The quad codes of this solver are parity invariant, so the hint cannot narrow the
  candidate search itself; this is recorded as a limitation.
- DEPTH: the brightest DEPTH image stars are tried first, then all detected stars; DEPTH 0 uses the built-in
  progression 40, 100, all. Each stage builds its own image quads and matches.
- CPU_LIMIT (seconds, 0 = none, default 180): matching and voting check a monotonic deadline; when it passes the
  solve fails with "CPU time limit exceeded".
- Abort: the abort flag is checked inside quad matching, voting and star matching, not only between indexes; an
  abort fails the solve with "Aborted" within a bounded number of iterations.
- Index cache: the last loaded index stays in memory across solves (existing behaviour) and its reuse is visible
  in the debug log; documented.
- Index order: with a known pixel scale the index fitting the field is tried first and the other selected indexes
  follow ordered by their distance (ratio) from the field size, so the two nearest scales are tried first.
- Bayer input: a RAW MONO8/MONO16 frame with a `BAYERPAT` extension keyword or a FITS with a `BAYERPAT` card is
  binned at least 2 x 2 (an odd DOWNSAMPLE hint is raised to the next even value) before detection; the WCS stays in
  unbinned pixels.
- Colour input: RAW RGB24/RGB48, FITS with three planes, JPEG with three components and libraw colour output are
  reduced to mono with the luminance weights 0.299 R + 0.587 G + 0.114 B.
- Solved message: "Solved: N of M stars matched, rms X.XX"" with the inliers, the matched stars and the residual of
  the final fit; failure messages name the limit ("CPU time limit exceeded", "Aborted", "Solution has the wrong
  parity" when no other solution was found).
- Driver version bump, REFACTOR.md, README.md (owner approval), the record run.

## Out of scope
New index format or partial (HEALPix) index loading, memory mapping, a diagnostic `X_` property, distortion terms,
a libraw fixture, changes to the shared `indigo_platesolver.c` or the Astrometry agent, Linux/Windows runs.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | With RADIUS > 0 and RA/DEC within RADIUS of the true centre the frame solves to the same solution as blind; with the hint more than RADIUS + field away it does not solve ("No solution found") | `test_agent_solver`: `solve_with_position_hint`, `wrong_position_hint_fails`; simulator: a guider setup solved with the hint in JNow (EPOCH 0) |
| AC-002 | With PARITY equal to the frame's parity the solution is unchanged; with the opposite parity the solve fails with "Solution has the wrong parity" | `test_agent_solver`: `parity_hint_rejects_mirror` |
| AC-003 | With DEPTH set to a value below the detected star count the frame still solves (brightest stars first) and reports the same solution | `test_agent_solver`: `solve_with_depth_hint` |
| AC-004 | A deadline already passed makes quad matching, voting and star matching return at once with the limit flagged; the agent reports "CPU time limit exceeded"; an abort flag does the same with "Aborted" | unit `match_respects_limits`; agent `cpu_limit_zero_time_fails` using a CPU limit the solve cannot meet is not deterministic, so the agent path is covered by the unit limit struct and by the existing abort handling |
| AC-005 | With a known scale and several selected indexes the agent tries the fitting index first and then the nearest other one; a wrong scale hint still solves (existing case) | simulator `rosetta wrong scale hint`, `rosetta index 2 only`, `rosetta index 5 only`; debug log order inspected in verification |
| AC-006 | A RAW MONO16 frame with a BAYERPAT extension and a FITS with a BAYERPAT card solve with the WCS scale and size in unbinned pixels; the debug log shows the automatic 2 x 2 binning | `test_agent_solver`: `bayer_raw_is_binned`, `bayer_fits_is_binned`; simulator `rosetta bayer` with the simulator's BAYERPAT keyword |
| AC-007 | RGB RAW, three-plane FITS and colour JPEG solve to the same solution as mono (luminance weighting) | `test_agent_solver`: existing `solve_rgb48_raw_image`, new `solve_rgb_fits_image`, `solve_colour_jpeg_image` |
| AC-008 | The solved message contains the matched count and the rms, e.g. "Solved: 21 of 22 stars matched, rms 0.02"" | `test_agent_solver`: `solve_raw_image` message assertion; simulator messages |
| AC-009 | The standard suite passes: unit, agent, simulator (recorded run), `make -C indigo_test test-unit` | `tools/run_driver_test.py agent_solver`, `make -C indigo_test test-unit` |
| AC-010 | Documentation and record updated: README hints section, REFACTOR.md, version, MIGRATION_STATUS.md, TEST_SUMMARY.md; PROPERTIES.md unchanged | diff review |

## Failure behavior and edge cases
- RADIUS > 0 with an index whose cone contains fewer than 4 catalogue stars: no quads, "No solution found".
- Hint coordinates outside their ranges are validated by the framework (property limits); the solver converts
  RA hours to radians and clamps nothing.
- PARITY hint with the opposite parity on every index: "Solution has the wrong parity".
- CPU_LIMIT 0 means no limit; the deadline is taken when the solve starts, so index loading counts towards it.
- A Bayer frame with DOWNSAMPLE 1 or 3 is binned 2 or 4; DOWNSAMPLE 2 stays 2.
- A frame with fewer than the DEPTH stars is tried once with all stars.

## Permissions, data, and security requirements
No new network, file or process access. Inputs remain untrusted blobs; the loaders keep their bounds checks. The
hint cone only filters already loaded catalogue data.

## Compatibility and nonfunctional requirements
No property is added or renamed. Blind solving (RADIUS 0, PARITY 0, DEPTH 0) behaves as before apart from the
message text and the progressive depth stages, which never remove a solution that the full star set finds (the full
set is always the last stage). The solve time of the standard suite must not grow noticeably; the cone filter and
depth stages are expected to shorten sparse-field solves.

## Assumptions and open questions
- Hint coordinates follow the EPOCH hint (verified in `indigo_platesolver.c`, `update_hints_from_mount()`).
- A CPU-limit agent test is not deterministic on a fast host and is replaced by a unit test of the limit mechanism;
  low risk, documented.
- Complexity: medium (several solver entry points, hint semantics shared with another agent, three test layers);
  risk: medium (wrong results would mislead mount workflows; no security boundary).

## Owner decisions and requirement changes
- 2026-10-27, Peter Polakovic: implement all proposals of the three groups; audit deferral recorded in
  `.factory/maintenance.json` (`solver_hints_deferral`).
