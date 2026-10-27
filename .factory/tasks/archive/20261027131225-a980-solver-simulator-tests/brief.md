# Task brief

- Task ID: 20261027131225-a980-solver-simulator-tests
- Owner: Peter Polakovic
- Created: 2026-10-27

## Problem and desired outcome
The native quad solver of `agent_solver` (task 20261027102245-431a-port-quad-solver) was verified with a synthetic
index and one real frame only. The owner asked, in conversation on 2026-10-27, for more intensive testing: one test
set against the CCD Imager Simulator (the Rosetta nebula frame) and one against the CCD Guider Simulator with RA/Dec
and the other parameters of its `SIMULATION_SETUP` property varied, so that at least 100 different parameter
combinations are verified. The real Tycho-2 indexes from the `index_release_v1` GitHub release are used; the owner
requires that they are never downloaded on every run: an index present in the persistent cache is reused, a missing
one is downloaded within the test, once.

## Users and use cases
INDIGO maintainers verifying the Solver Agent against the production Imager and Guider agents and the CCD simulators
before releases; users who rely on the WCS angle and parity of the Solver Agent being interchangeable with those of
the Astrometry Agent.

## Scope and non-goals
In scope: an opt-in integration test `test_agent_solver_simulator` with both sets, its build and project registration,
fixes of every solver defect the sets reveal with regression coverage, driver record and documentation updates.
Out of scope: changes to the CCD simulator, hardware runs, Linux/Windows runs, FITS/JPEG uploads.

## Constraints and references
Root `AGENTS.md`, `indigo_test/AGENTS.md`, `indigo_drivers/AGENTS.md`; `indigo_test/integration/test_agent_astrometry_solver.c`
as the reference for driving the simulators through the agents and for the angle convention of astrometry.net;
`indigo_drivers/ccd_simulator/indigo_ccd_simulator.c` for what the guider simulator renders. The index files must not
enter the repository (`*.isix` ignored) and must not be downloaded repeatedly.

## Initial questions
- Which indexes do the two sets need? The Rosetta frame (3.75° x 2.8°) selects the 2° index, the guider frames
  (4° over the height) the 5° index; both are selected so the fallback order is exercised too.
- Where is the persistent cache? `$INDIGO_CACHE_BASE/solver`, by default `~/.indigo/solver` of the real home, the
  same directory the user's own Solver Agent uses.
