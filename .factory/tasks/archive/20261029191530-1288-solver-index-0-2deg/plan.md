# Development plan

Status: complete

## Approach and affected components
Scale the anchored Gaia tier of the 0.5° index down to 0.2° in the indexer, add the scale to the solver list, the
agent size table and the tests, measure size variants on rendered and real frames, pick the smallest variant that
keeps the solve rate and solves the owner's frame.

## Baseline
- Commands and environment: macOS arm64, master `8e6a1d484`; recorded run 3.0.0.9 150/150 OK
- Existing failures: `make -C indigo_tools all` failed to link `indigo_solver_indexer` and `indigo_solver` on macOS
  (missing libjpeg/libraw since a1e7f53d0)
- Repository revision / working-tree state: clean master, task branch created from it

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | indexer tier, tools Makefile link | AC-001 | None | build log | done |
| 2 | solver scale list, agent item/size/version | AC-002, AC-004 | 1 | unit, agent tests | done |
| 3 | simulator case per Gaia index, variants measured | AC-003 | 1, 2 | 100-field runs | done |
| 4 | owner's frame, independent cross-match | AC-005 | 1 | indigo_solver, Gaia TAP | done |
| 5 | docs, recorded run | AC-006, AC-007 | 2-4 | run_driver_test.py | done |

## Test strategy
Unit index selection, agent item listing, simulator self-consistency (12 fixed fields standard, 100 random one-off),
real frame with independent check.

## Security risks and mitigations
See security.md; no new trust boundary.

## Dependencies and architecture decisions
No new dependency; the Makefile now links the static libjpeg/libraw the solver core already uses.

## Migration, rollout, and rollback
The asset was uploaded by the owner to `index_release_v2`; rollback removes the scale from the list.

## Deviations and owner decisions
See specification.md.
