# Specification

Status: approved by owner request 2026-11-13

## Problem and outcome
See brief.md.

## Current-state audit
After the merge, `indigo_server.c` on `scheduler` is the scheduler registration plus the master fix; no task records this content.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The merged `indigo_server.c` differs from the pre-merge `scheduler` version only by the gphoto2 removal and compiles with and without `STATIC_DRIVERS` | diff, syntax checks |
| AC-002 | Finding 005 is closed with the master task as evidence | findings check |
| AC-003 | The scheduler still works after the merge | `run_driver_test.py agent_scheduler --no-record` |

## Failure behavior and edge cases
None.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
None.
