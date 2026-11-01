# Test and regression plan

Status: active

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `indigo_test/integration/test_agent_alpaca_mapping.c` | Stub mount and guider on the bus | 0 guider requests | Before and after fix |
| T-002 | AC-002 | same | Same | 1 request after `Connected=true` | After fix, preserved |
| T-003 | AC-003 | same | Guider paired after the request | Request sent by timer | After fix, preserved |
| T-004 | AC-004 | same | Client disconnect, foreign disconnect, ALERT | No further request | After fix |
| T-005 | AC-005 | `indigo_server` trace, SynScan serial simulator | Mount, guider and Alpaca agents loaded | Selection OK, guider sender `Guider Agent` | Before (reproduced) and after fix |

## Baseline evidence
See verification.md.

## Bug reproduction
Before the fix, with the agent loaded, the bus trace shows `Change 'Mount SynScan (guider)'.'CONNECTION' ... 'Alpaca Agent'` right after the mount connects, and the selection ends in ALERT.

## Negative cases and permission boundaries
T-001 and T-004 are negative cases. No permission boundary changes.

## Isolation and cleanup
`indigo_test_use_private_home()` for the test, a temporary HOME and port 7625 for the server scenario. Processes are stopped afterwards.

## Deferred tests and coverage limitations
No ConformU or OmniSim run and no hardware run. The `system_alpaca` late attachment is covered through its pairing order only, not through that driver. Owner: Peter Polakovic, who may request those runs.
