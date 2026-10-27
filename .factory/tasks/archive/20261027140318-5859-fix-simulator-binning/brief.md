# Task brief

- Task ID: 20261027140318-5859-fix-simulator-binning
- Owner: Peter Polakovic
- Created: 2026-10-27

## Problem and desired outcome
`indigo_ccd_simulator` bins the CCD Imager Simulator frame by taking every `horizontal_bin`-th / `vertical_bin`-th
pixel of the fixed Rosetta image, and the CCD Guider Simulator divides its star centres by the bin factor without
narrowing the rendered profile. A real camera sums (or averages) the bin block, which keeps the star profile and its
flux. The subsampling turned the Rosetta stars into single pixel spikes whose quad codes do not match, so the solver
simulator test of task 20261027131225-a980-solver-simulator-tests had to leave camera binning out (see its
`REFACTOR.md` section "Simulator verification"). The owner asked for intensive simulator testing of the solver; fixing
the simulator was out of that task's scope and was proposed as this task by the session that ran it.

Desired outcome: a binned Rosetta frame (CCD_BIN 2 x 2 through the Imager Agent) solves in the Solver Agent to the
same centre and the field scale of 8.44"/px per unbinned pixel, the guider camera renders binned stars with the shape
of the unbinned ones scaled down, and the simulator based suites still pass. The starting session allowed a clamped
sum or an average if summing would saturate the fixed Rosetta image.

## Users and use cases
INDIGO developers and tests using the camera simulators as stand-ins for real cameras: the Imager, Guider,
Astrometry and Solver agents and their integration tests exercise binned frames and expect the stars of a binned
frame to look like those of a real binned sensor.

## Scope and non-goals
In scope: `create_frame()` of `indigo_drivers/ccd_simulator/indigo_ccd_simulator.driver` (imager binning as the
mean of the bin block, guider star profile scaled down with the unbinned amplitude), driver version 45, regenerated outputs,
regression cases in `indigo_test/integration/test_ccd_simulator.c`, the `rosetta camera binned` case of
`indigo_test/integration/test_agent_solver_simulator.c`, the simulator and solver records (REFACTOR.md, README test
record through `tools/run_driver_test.py ccd_simulator`, MIGRATION_STATUS.md, TEST_SUMMARY.md).
Non-goals: the guider background gradient and noise (per binned pixel, unchanged), the eclipse mode (already
rendered in unbinned coordinates), the imager's bin handler and frame geometry, any solver change, hardware runs,
Linux/Windows runs, publication.

## Constraints and references
Root `AGENTS.md` (formatting, generated drivers, version bump, Git rules), `indigo_drivers/AGENTS.md`,
`indigo_test/AGENTS.md`; the `.driver` file is the source of truth and the outputs are regenerated with the
unchanged generator; `indigo_drivers/agent_solver/REFACTOR.md` section "Simulator verification" for the symptom;
`indigo_ccd_simulator_mono.h` is 16-bit with 60 of 480 000 2 x 2 blocks summing beyond 65535.

## Initial questions
- Sum or average? Average: the summed and clamped frame did not solve (5 of 57 consistent matches, deterministic),
  the averaged one does (6 of 23), see the simulator REFACTOR.md; the average also keeps the frame's range at every
  binning, as the agent's own downsampling does.
- What scale does the solver report for a camera-binned frame? The agent divides by its own downsampling only, so
  the WCS scale is per binned pixel (16.88"/px for 2 x 2) over 800 x 600 pixels, the same field as unbinned.
