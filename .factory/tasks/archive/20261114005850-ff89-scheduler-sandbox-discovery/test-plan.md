# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `indigo_prop_tool discover` while the sandbox runs | default options | "Scheduler Sandbox -> host:port" | after change |
| T-002 | AC-002 | sandbox start-up output; curl of both URLs | default options | both URLs printed, HTTP 200 | after change |
| T-003 | regression | `make -C indigo_test test-scheduler-sandbox` | night scenario | exit 0 | after change |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Before: `-b-` in the server command, nothing to discover.

## Negative cases and permission boundaries
`--no-bonjour`.

## Isolation and cleanup
Sandbox directories removed after the runs.

## Deferred tests and coverage limitations
No macOS run.
