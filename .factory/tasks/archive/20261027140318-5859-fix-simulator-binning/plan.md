# Development plan

Status: complete

## Approach and affected components
Change the two rendering loops in the `code` block of the generated driver's `.driver` source, bump the version and
regenerate with the unchanged generator through `Makefile.drv`. Add two regression cases to the simulator suite,
one comparing a binned imager region pixel by pixel with the block means of the fixed image, one comparing the
profile moments of an unsaturated guider star unbinned and 2 x 2 binned. Re-add the camera binned case to the solver
simulator test. Components: `indigo_drivers/ccd_simulator/indigo_ccd_simulator.{driver,c}`,
`README.md` (test record), `indigo_test/integration/test_ccd_simulator.c`,
`indigo_test/integration/test_agent_solver_simulator.c`, `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`; the defect
record and test summary are in verification.md (master archived the driver-local REFACTOR.md files during the task).

## Baseline
- Commands and environment: Darwin arm64 (macOS 27), Apple clang, `make all` tree of 2026-10-27 in `build/`,
  Tycho-2 indexes in `~/.indigo/solver`.
- Existing failures: none known; `test_ccd_simulator` recorded 26/26, `test_agent_solver_simulator rosetta` 8/8
  with the camera binned case excluded.
- Repository revision / working-tree state: master `42c0960a2` (Complete the process evidence of the solver
  simulator task); the owner's `indigo_server/indigo_server.c` change (static solver agent registration) was
  uncommitted at the start and committed by the owner as `32d7396d7` during the task, not part of this task.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Imager block mean, guider profile scaling, version 45 in the `.driver`; regenerate and build | AC-002, AC-003, AC-005 | None | driver builds, generated diff limited to the changed block and version | complete |
| 2 | Regression cases in `test_ccd_simulator.c` | AC-002, AC-003 | 1 | both cases pass, old driver would fail them | complete |
| 3 | `rosetta camera binned` case and header in `test_agent_solver_simulator.c` | AC-001 | 1 | Rosetta set passes | complete |
| 4 | Regression suites, recorded run, records | AC-004, AC-005 | 2, 3 | suites pass, README line written, documents updated | complete |
| 5 | Synchronize with origin/master (`1e7c23390`, 20 commits: FITS/JPEG/hint/0.5° solver cases, archived REFACTOR.md files, driver lists), resolve, rebuild, rerun the recorded run and the suites, refresh the evidence | AC-001, AC-004, AC-005 | 4 | merge commit, suites pass on the merged candidate | complete |

## Test strategy
Unit layer is not applicable (the rendering is inside the driver); the simulator integration suite reads the RAW
frames through the bus and compares them with the fixed image and between binnings. The solver simulator test is the
end to end acceptance. The agent suites are the regression net for consumers of binned simulator frames.

## Security risks and mitigations
None new: the change touches pixel arithmetic over fixed-size buffers whose geometry is unchanged; indexes stay
within the fixed image because `frame_left + i` and `frame_top + j` are in binned pixels and the block offsets stay
below the bin.

## Dependencies and architecture decisions
No new dependencies. Average rather than sum: the summed and clamped frame was implemented first and did not solve
in the Solver Agent (deterministic, 5 of 57 consistent matches), the averaged frame does; the average also keeps the
frame's range independent of the binning, like the agent's own downsampling and a software-binned camera.

## Migration, rollout, and rollback
Not applicable; no stored data. Rollback is the previous driver version.

## Deviations and owner decisions
The clamped sum of the brief was replaced by the mean after measurement; the brief allowed either. The notes first
appended to the two REFACTOR.md files were moved into verification.md when master archived those files.
