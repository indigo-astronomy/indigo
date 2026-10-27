# Task brief

- Task ID: 20261027192244-16bd-solver-hints-input-diagnostics
- Owner: Peter Polakovic
- Created: 2026-10-27

## Problem and desired outcome
The Solver Agent (`indigo_drivers/agent_solver`, version 3.0.0.7 after commit `a1e7f53d0`) solves blind: of the
shared `AGENT_PLATESOLVER_HINTS` property it uses only SCALE (order of the indexes tried), EPOCH and DOWNSAMPLE.
RADIUS, RA and DEC, PARITY, DEPTH and CPU_LIMIT have no effect, an abort is only honoured between index attempts,
Bayer mosaics from colour cameras are detected as they are, colour inputs are averaged instead of weighted, and the
result message says only "Solved". The owner asked in conversation on 2026-10-27 to implement every proposal of the
groups "speed and robustness", "input images" and "diagnostics (message)" from the improvement review of the same
day: use the position hints to restrict the catalogue to a cone, use the parity hint, progressive depth, a CPU time
limit, abort checkpoints inside matching, keep the loaded index cached and verify it, try the indexes nearest to the
field size first, bin Bayer mosaics automatically, weight colour channels by luminance, and report the matched
star count and fit residual in the solved message.

## Users and use cases
INDIGO users plate solving with the native Solver Agent from the Mount Agent and imaging workflows, where the
mount position is known and the solve should be fast and reliable in sparse fields; users with colour cameras
sending RAW mosaics or FITS with BAYERPAT; maintainers diagnosing solve quality from the agent messages.

## Scope and non-goals
In scope: `indigo_drivers/agent_solver/*` (agent, solver core, docs, record), the solver unit test, the agent
integration test and the opt-in simulator test under `indigo_test/`, PROPERTIES.md only if a property changes.
Non-goals: a new index file format or partial (HEALPix cell) index loading, a new `X_` diagnostic property,
distortion terms for fields above 15°, a libraw fixture, Linux and Windows runs, the shared `indigo_platesolver.c`
base and the Astrometry agent.

## Constraints and references
Root `AGENTS.md`, `indigo_drivers/AGENTS.md`, `indigo_test/AGENTS.md`, `indigo_test/DRIVER_TESTING_RULES.md`.
`indigo_drivers/agent_astrometry/indigo_agent_astrometry.c` defines how the shared hints are interpreted
(RA in hours and DEC in degrees with RADIUS in degrees when RADIUS > 0, PARITY -1/0/1, DEPTH as the number of
field objects, CPU_LIMIT in seconds). The semantics of existing properties must not change; RAW MONO16 stays the
preferred input. The owner approves README.md changes explicitly. The full repository audit is due
(`maintenance status`: 47 non-audit tasks since the last full review); this task needs the owner's deferral.

## Initial questions
- Owner deferral of the due full repository audit for this task (asked in conversation).
- Whether the hint coordinates follow the EPOCH hint (JNow when EPOCH is 0) like the solved centre does: assumed yes,
  the Astrometry agent passes them to solve-field unchanged and solve-field treats them as J2000, but the Mount Agent
  fills them from its current coordinates; to be confirmed from `indigo_platesolver.c` during analysis.
