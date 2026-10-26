# Test and regression plan

Status: final

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `solver filter list mirrored` | fake imager Lum/Red/Ha | CURRENT + 3, CURRENT selected | after implementation |
| T-002 | AC-002 | `solve with filter` | Ha, `solve_1s` | `F3 C F1`, return while WCS BUSY | after |
| T-003 | AC-002 | `polar alignment switches filter per exposure` | `polar` | `F3 C F1` ×3, then ×1 for recalculation, no overlap | after |
| T-004 | AC-003 | `current or same filter keeps wheel` | CURRENT, Lum | captures only | after |
| T-005 | AC-004 | capture/solve failure, abort during capture, abort during filter change, filter change failure | knobs | one return, ALERT | after |
| T-006 | AC-005 | `filter list follows Imager Agent` | rename, fewer slots, unrelate, relate | slot kept / CURRENT | after |
| T-007 | AC-006 | `run_driver_test.py agent_astrometry`, `test-agent-imager`, end-to-end, `test-server-leaks` | | pass | after |

## Baseline evidence
77/77 recorded 2026-10-25 at the previous master.

## Bug reproduction
Not applicable (feature). Mutation check: without the returns 8 of the 10 new cases fail.

## Negative cases and permission boundaries
Failed filter change, vanished slot, unrelated Imager Agent, abort during the selection.

## Isolation and cleanup
Every case runs in its own forked process and temporary directory; the end-to-end check uses a private HOME and port.

## Deferred tests and coverage limitations
No `agent_solver` suite exists; physical wheels not tested.
