# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `gcc -fsyntax-only -DSTATIC_DRIVERS indigo_server.c` | Linux arm64 | no error | before and after |
| T-002 | AC-002 | `gcc -fsyntax-only indigo_server.c` | Linux arm64 | no error | after |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
T-001 fails before the fix.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
Separate worktree of the task branch.

## Deferred tests and coverage limitations
No full static link of the server (it needs every vendor SDK); macOS and Windows not built (the removed lines were Linux only).
