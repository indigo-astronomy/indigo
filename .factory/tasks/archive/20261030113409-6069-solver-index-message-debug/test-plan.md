# Test and regression plan

Status: approved

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | integration, `blind_search_goes_stage_by_stage` and `scaled_search_uses_fitting_index_first` in `indigo_test/integration/test_agent_solver.c` | synthetic rendered frame and indexes | client messages contain no "Solving with index" | before implementation (fails), after (passes) |
| T-002 | AC-002 | same tests, existing order and repetition assertions on the captured debug log | debug level and log handler only around the solve | same order as before the change | after implementation |
| T-003 | AC-003 | `python3 tools/run_driver_test.py agent_solver`, `make -C indigo_test test-unit` | — | all pass | final |

## Baseline evidence
verification.md, Baseline.

## Bug reproduction
T-001 fails on the unchanged agent, recorded in verification.md.

## Negative cases and permission boundaries
Not applicable; no input handling changes.

## Isolation and cleanup
The handler and log level are restored after each of the two solves, so other tests and `INDIGO_TEST_DEBUG` output
are unaffected.

## Deferred tests and coverage limitations
None.
