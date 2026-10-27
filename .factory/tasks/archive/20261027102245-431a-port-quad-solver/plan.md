# Development plan

Status: complete

## Approach and affected components
Mechanical port of the tester sources with a generator script (headers, includes, formatting rules), API extension
for index directory and index list selection, new agent logic modelled on the astrometry agent's index handling,
tool and tests. Components: `indigo_drivers/agent_solver/*`, `indigo_tools/indigo_solver_indexer.c` and Makefile,
`indigo_test/unit/test_agent_solver_core.c`, `indigo_test/integration/test_agent_solver.c`, `indigo_test/Makefile`,
`indigo.xcodeproj/project.pbxproj`, `indigo_docs/PROPERTIES.md`, `MIGRATION_STATUS.md`, `.gitignore`.

## Baseline
- Commands and environment: `make -C indigo_drivers/agent_solver -f ../../Makefile.drv all` on Darwin arm64, Xcode 27 SDK.
- Existing failures: none for the driver; `build/lib/libindigo.dylib` was stale (2026-10-23) and had to be rebuilt
  with `make -C indigo_libs` before the integration test could run (the platesolver private data layout changed on
  2026-10-26).
- Repository revision / working-tree state: master `aabaed8ec963631cca9ba37826b1777339cc43e7`, clean.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Port core sources to `indigo_drivers/agent_solver/` | AC-001 | None | driver builds | complete |
| 2 | Port the indexer to `indigo_tools/` and register it | AC-001, AC-002 | 1 | tool builds, indexes reproduced | complete |
| 3 | Agent index management and native solve, version 3.0.0.4 | AC-005 | 1 | driver builds | complete |
| 4 | Xcode, test Makefile, `.gitignore`, PROPERTIES.md, MIGRATION_STATUS.md | AC-007, AC-008 | 1-3 | plutil lint, grep | complete |
| 5 | Unit and integration tests | AC-003, AC-004, AC-005 | 1-3 | tests pass | complete |
| 6 | Tester frame verification with tool-built indexes | AC-006 | 2 | recorded in verification.md | complete |
| 7 | REFACTOR.md, README.md, task evidence, Factory checks | AC-008 | 1-6 | checks pass | complete |

## Test strategy
Deterministic unit tests of the geometry, codes, file format and solving on synthetic data; an in-process bus
integration test of the agent with a synthetic index placed in `INDIGO_CACHE_BASE`; the native unit suite as
regression; the astrometry agent integration test as a regression of the shared platesolver base; a tester frame
against real Tycho-2 indexes recorded manually.

## Security risks and mitigations
Index download executes curl: no shell on POSIX (`execlp`), URL built from a constant and a FOV; downloaded data is
validated (magic, version, FOV, counts) before use and star indices are range checked on load. See security.md.

## Dependencies and architecture decisions
`curl` at runtime for downloads (as in the astrometry agent); no new libraries. Index files live outside the
repository and are published separately; the solver selects by field size with the pixel scale hint.

## Migration, rollout, and rollback
No data migration. Users need to select an index once; without published index files the agent reports a download
failure. Rollback: revert the task branch.

## Deviations and owner decisions
README.md was updated to describe the index handling; it needs the owner's approval per repository rules.
