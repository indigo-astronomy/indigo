# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `gcc -fsyntax-only` with and without `-DSTATIC_DRIVERS` | Linux arm64 | no error | after the merge |
| T-002 | AC-002 | `./.factory/scripts/findings check` | none | pass | final |
| T-003 | AC-003 | `python3 tools/run_driver_test.py agent_scheduler --no-record` | after `make all` | 43/43 | after the merge |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
In the master task 20261113005229-eff7-static-server-gphoto2-include.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
Not applicable.

## Deferred tests and coverage limitations
No full static link; macOS and Windows not built.
