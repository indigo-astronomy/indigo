# Task brief

- Task ID: 20261027102245-431a-port-quad-solver
- Owner: Peter Polakovic
- Created: 2026-10-27

## Problem and desired outcome
The native quad based plate solver was developed in the external `Solver` macOS tester (C core plus a SwiftUI
front end). The INDIGO `agent_solver` driver was a skeleton whose `solve` callback returned `false`. The owner asked,
in conversation on 2026-10-27, to port the C part (not the Swift part) into `agent_solver` following the repository
rules, to design the index handling without putting index files into the repository, and not to commit or push
until they review the result.

## Users and use cases
INDIGO users who want plate solving without installing astrometry.net or ASTAP: the Solver Agent solves frames from
the related Imager/Guider agents or direct RAW uploads against downloadable Tycho-2 quad indexes, and the maintainer
builds those indexes with a command line tool.

## Scope and non-goals
In scope: the solver core, HEALPix helpers and index format in `indigo_drivers/agent_solver/`, the index builder in
`indigo_tools/`, the agent's index management property and native solve, build/project registration, property
documentation, unit and integration tests, driver record. Out of scope: the Swift tester, publishing the index
files, FITS/JPEG upload support, Windows build of the agent, committing or integrating the task branch.

## Constraints and references
Root `AGENTS.md`, `indigo_drivers/AGENTS.md`, `indigo_test/AGENTS.md`, `indigo_docs/PROPERTIES.md`; the astrometry
agent (`indigo_drivers/agent_astrometry/`) as the reference for index management. Index files (`*.isix`) must never be
checked in. The full repository audit is overdue (45 completed non-audit tasks since the last one); the owner's
explicit request for this work on 2026-10-27 is recorded as the deferral authority for this task.

## Initial questions
Where will the prebuilt index files be hosted (`AGENT_SOLVER_INDEX_URL` is a placeholder)? Does the owner approve
the README.md update of the agent?
