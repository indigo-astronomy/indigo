# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | Item 1: filtered detection of a noisy mono frame with hot pixel, column and row | unit `indigo_test/unit/test_agent_solver_core.c` `image_stars_noisy_frame` | 30 gaussian stars of 200 to 500 ADU over noise sigma 72, 20 hot pixels, a hot column and a hot row | every star detected within 1.5 px, no defect detected, frame unchanged | re-run in this task |
| T-002 | Item 2: FITS 16 bit integer input | agent `indigo_test/integration/test_agent_solver.c` `solve_fits_image` | rendered synthetic field written as FITS | solved to the rendered centre | re-run |
| T-003 | Item 2: FITS -32 floating point with BZERO/BSCALE | agent `solve_float_fits_image` | same field as float FITS | solved | re-run |
| T-004 | Item 2: RAW MONO8 and RGB48 input | agent `solve_mono8_raw_image`, `solve_rgb48_raw_image` | same field as 8 bit and colour RAW | solved | re-run |
| T-005 | Item 2: JPEG input | agent `solve_jpeg_image` | same field as JPEG | solved | re-run |
| T-006 | Item 3: unreadable input reported | agent rejection cases | random bytes; a FITS header without data | messages "Unsupported image format" and "Truncated FITS image" seen | re-run |
| T-007 | Items 1 and 2 end to end through the Imager Agent and the CCD simulator | simulator `indigo_test/integration/test_agent_solver_simulator.c` `rosetta fits`, `rosetta jpeg`, guider sweep with every fifteenth setup `as fits` | Tycho-2 indexes installed, CCD simulator frames | solved like the RAW captures | re-run (opt-in, indexes present) |
| T-008 | Preserved contract: all earlier solver cases | unit, agent and simulator suites as a whole | as above | all cases pass | re-run |
| T-009 | Record integrity | `./.factory/scripts/check`, `./.factory/scripts/security`, `./.factory/scripts/test`, `archive-task --dry-run`, committed-range process check | task branch | exit 0 | this task |

## Baseline evidence
Recorded run of the reviewed commit: "2026-10-27 19:48 3.0.0.7 mac arm64 simulator 141/141 OK" (README of
`a1e7f53d0`; unit suite 10/10 at that revision by its case table). The current tree carries 3.0.0.8 with its
recorded run 149/149; the re-runs in verification.md are of the current tree, which contains the reviewed tests.

## Bug reproduction
The defect of the reviewed commit (sparse fields at noise 250 to 400 lost by the per pixel star detector) is
reproduced by `image_stars_noisy_frame` before the filter and fixed after it; this task re-runs the fixed state only
(REFACTOR.md records the nine failed sweep setups and their solution).

## Negative cases and permission boundaries
T-006 covers unreadable input. The security review inspects the FITS length checks, JPEG error path and camera raw
path. No permission boundary changes.

## Isolation and cleanup
Tests run from `indigo_test/` with built binaries in `indigo_test/build/`; the build tree `build/` and
`Makefile.inc` are ignored by Git. No server or simulator process is left running (the suites start and stop their
own bus).

## Deferred tests and coverage limitations
- Linux and Windows builds of the reviewed commit: not run (owner, macOS host only; the agent is not built on
  Windows per MIGRATION_STATUS.md).
- libraw camera raw input: no fixture in the suite (`dslr_image` inspected only); owner, follow-up when a small
  fixture is available.
- A new recorded run through `tools/run_driver_test.py` is deliberately not made: it would record 3.0.0.8 and change
  README.md, outside this task's scope.
