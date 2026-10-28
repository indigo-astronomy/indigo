# Test and regression plan

Status: accepted

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-004 | unit `indigo_test/unit/test_agent_solver_core.c` `index_selection` | empty files `gaia-0.5deg.isix` and `tyc2-1deg.isix` | 0.3 x 0.25° and 0.6 x 0.45° fields select 0.5, 1 x 0.75° selects 1; the scale list starts with 0.5 and has six entries | with implementation |
| T-002 | AC-002 | agent `indigo_test/integration/test_agent_solver.c` `index_property_reflects_installed_files` | the synthetic index also written under the 0.5° file name `gaia-0.5deg.isix` | `GAIA_0_5` selected in `X_AGENT_SOLVER_INDEX`, offered in `AGENT_PLATESOLVER_USE_INDEX`, no dotted item | with implementation |
| T-003 | AC-002 | agent `deselecting_index_removes_file` | as T-002 | clearing `GAIA_0_5` deletes its file and withdraws the item, `TYC2_5` unaffected until cleared | with implementation |
| T-004 | AC-001, AC-003 | simulator `indigo_test/integration/test_agent_solver_simulator.c` `index 0.5 self consistency` | the built index in the cache; twelve fixed fields (1000 x 800 at 1.8"/px, angle 30°, parity -1) rendered from the index stars | every field with at least 8 index stars solves (centre within 36", scale within 2 %, angle within 0.2°, parity -1, index 0.5) and at least half of the fields solve; with Gaia all twelve are expected to solve | after the build |
| T-006 | AC-003 (evidence) | the same case with `INDIGO_SOLVER_NARROW_FIELDS=500` pseudo-random positions, one-off long run | as T-004 over 500 fields | done: 497/500, see verification.md |
| T-005 | AC-005 | recorded run `tools/run_driver_test.py agent_solver`, `make -C indigo_test test-unit` | existing | all pass, record written | after implementation |

## Baseline evidence
Recorded run of 3.0.0.8: 149/149 (README `## Testing` of 2026-10-27 20:37); unit 11/11; agent 19/19.

## Bug reproduction
Not a bug fix.

## Negative cases and permission boundaries
T-002 checks that no dotted item name exists; the download failure before the asset is uploaded is not exercised
(it would wait on the network and depends on the release state).

## Isolation and cleanup
Agent tests write the synthetic files into the private home and delete them; the simulator test reads the index
from the persistent cache and uploads the rendered frame as a blob.

## Deferred tests and coverage limitations
- The self-consistency case renders the index's own stars; it validates the index file, the naming and the solver
  pipeline for a 0.5° field, not the astrometric truth of Tycho-2 positions. A real narrow-field frame fixture
  would be needed for that. Owner: coordinator.
- Tycho-2 builds (6, 8 and 12 stars per cell, quads 0.03° to 0.5°): 1 to 13 index stars per 0.5 x 0.4° field, at
  most 6 of 12 fixed fields solvable, see verification.md; this is why the catalogue changed to Gaia DR3.
