# Development plan

Status: complete

## Approach and affected components
Write the Factory record for the published commit `a1e7f53d0` on its own task branch, re-run the solver checks on
the current tree, review the committed diff after the fact and archive the task. Affected: `.factory/tasks/` (this
task), `.factory/maintenance.json` (deferral entry), `.factory/completions/` (archival). No application path changes.

## Baseline
- Commands and environment: `make init`, `make -C indigo_libs all`, `make -C indigo_drivers/<driver> -f ../../Makefile.drv all`
  for agent_solver, agent_imager, agent_guider and ccd_simulator, `make -C indigo_test build/unit/test_agent_solver_core
  build/integration/test_agent_solver build/integration/test_agent_solver_simulator`; macOS Darwin 27.0.0 arm64, clang;
  Tycho-2 indexes `tyc2-2deg.isix` and `tyc2-5deg.isix` present in `~/.indigo/solver`.
- Existing failures: none known; the baseline tree is `master` `05f14156a` (3.0.0.8), the recorded run of
  `a1e7f53d0` was 141/141.
- Repository revision / working-tree state: task branch created from `master` `05f14156a4b8a7c91e568f9d5c65b04c44735a80`
  (equal to `origin/master`), clean tree; the worktree is a fresh Git worktree without a build tree.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Read rules and routines; allocate `20261027195534-4608-solver-retro-evidence`, create `task/<id>` from synchronized `master`, create the record | AC-001 | None | branch and `.factory/tasks/<id>/` exist | done |
| 2 | Describe the reviewed commit in brief, specification, plan and test plan from `git show a1e7f53d0`, REFACTOR.md and the README record | AC-002, AC-004 | 1 | documents populated | done |
| 3 | Build the library, drivers and tests; run `test_agent_solver_core`, `test_agent_solver`, `test_agent_solver_simulator` | AC-003 | 1 | exit codes and counts in verification.md | done |
| 4 | Review the committed diff after the fact: quality-review.md and security.md with the one-agent limitation | AC-002, AC-004 | 2 | passing verdicts with limitations | done |
| 5 | Ask the owner for the audit deferral; record the answer in `.factory/maintenance.json` and handoff.md | AC-006 | None | entry present | done |
| 6 | Fill process.json; test version 1 evidence against a dangling candidate commit; fall back to version 2 for the task's own change | AC-005 | 2 to 5 | checker result documented | done |
| 7 | Archive, commit once, run the committed-range process check, report; no integration or push | AC-001, AC-005, AC-007 | 6 | check output in handoff.md | done |

## Test strategy
No new tests: the task changes no source. The reviewed commit's tests are re-run as they stand in the current tree
(3.0.0.8, which contains them unchanged in intent and extended by the later task). The record of the commit's own run
(141/141) is quoted from the committed README.

## Security risks and mitigations
The reviewed commit parses client-supplied image data (FITS header and data lengths, JPEG through libjpeg, camera
raw through libraw). The after-the-fact security review inspects the length checks, the error paths and the
allocation sizes; findings would be registered under this task's namespace.

## Dependencies and architecture decisions
None new. The reviewed commit uses libjpeg and libraw through the existing INDIGO library (`indigo_dslr_raw.h`), as
the Astrometry agent does.

## Migration, rollout, and rollback
Not applicable: an evidence-only change. Rolling back means deleting the task branch before integration.

## Deviations and owner decisions
- The task records a change that is already published; the reviews are after the fact and by the same agent that
  writes the record (the author of the commit worked in another session). Independence: none.
- Version 1 process evidence (`reviewed_revision` = `a1e7f53d0`) was tried against a dangling candidate commit; see
  handoff.md for the result and the chosen evidence version.
