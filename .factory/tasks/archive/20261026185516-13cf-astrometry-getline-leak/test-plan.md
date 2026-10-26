# Test and regression plan

Status: final

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | valgrind `test_agent_astrometry "solve with filter"` | fake tools | no definite loss | before and after |
| T-002 | AC-002 | `run_driver_test.py agent_astrometry` | | all pass | after |

## Baseline evidence
3 × 120 bytes definitely lost in `execute_command()` (task 20261026155455-61b6-platesolver-solve-filter, V7).

## Bug reproduction
The valgrind run above.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
Per-case forked processes and temporary directories.

## Deferred tests and coverage limitations
No automated leak assertion is added; valgrind is run by hand.
