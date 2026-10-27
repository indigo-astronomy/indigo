# Development plan

Status: complete

## Approach and affected components
A new opt-in integration test modelled on `test_agent_astrometry_solver.c` (observer cache, per case fork with a
private home, process runner) drives the Solver Agent with the production Imager/Guider agents and the CCD
simulators. The sweep combinations are generated deterministically from parameter tables so every case has a
descriptive name; `INDIGO_SOLVER_SETUP` lets one setup be reproduced by hand. Defects found are fixed in the solver
core with the existing tests adapted. Components: `indigo_test/integration/test_agent_solver_simulator.c`,
`indigo_test/Makefile`, `indigo_test/AGENTS.md`, `indigo_drivers/agent_solver/{indigo_solver.c,
indigo_solver_utils.c, indigo_solver_utils.h, indigo_agent_solver.c, README.md, REFACTOR.md}`,
`indigo_tools/indigo_solver_indexer.c`, `indigo_test/unit/test_agent_solver_core.c`,
`indigo_test/integration/test_agent_solver.c`, `indigo.xcodeproj/project.pbxproj`, `MIGRATION_STATUS.md`,
`TEST_SUMMARY.md`.

## Baseline
- Commands and environment: Darwin arm64, Xcode 27 SDK, `make all` tree built; astrometry.net 0.9x installed in
  `/usr/local/bin` for the reference run.
- Existing failures: none; `test_agent_solver_core` 9/9, `test_agent_solver` 6/6 at baseline.
- Repository revision / working-tree state: master `b7a52e6c4` (Name the solver index release index_release_v1), clean.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Test with both sets, Makefile rule and opt-in listing, Xcode entry | AC-001, AC-002, AC-004 | None | builds, first cases pass | complete |
| 2 | Reference run of astrometry.net on the simulator, fix angle and parity convention and tangent point accuracy in the solver, adapt unit/agent tests and the indexer caller, version bump | AC-003, AC-005 | 1 | unit 9/9, agent 6/6, sweep angles within 0.2° | complete |
| 3 | Tune the sweep tables to physically solvable inputs, run the full sweep, document exclusions | AC-002 | 2 | 120/120 | complete |
| 4 | Records: README run line, REFACTOR.md, MIGRATION_STATUS.md, `indigo_test/AGENTS.md`, task evidence | AC-006 | 3 | files updated, factory checks pass | complete |
