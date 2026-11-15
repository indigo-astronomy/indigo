# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-003 | scheduler started without the Scripting Agent | private server with the Scheduler Agent only | REASON `waiting for the Scripting Agent` | after review |
| T-002 | AC-003 | Scripting Agent loaded, no Mount Agent, `@target` task | same | REASON `site coordinates unknown` | after review |
| T-003 | AC-003 | task without sky keys, no Mount Agent | same | runs, DONE 1 | after review |
| T-004 | AC-003 | Mount Agent loaded, site 0°/0° | same | accepted at the next check: `target below the horizon` | after review |
| T-005 | AC-003, AC-004 | site set with the documented command | same | `all tasks finished`; source `HOST` | after review |
| T-006 | AC-004 | `Server.DRIVERS...=ON` against a server started with a driver list vs `Server.LOAD.DRIVER` | same | DRIVERS does not load an unlisted driver; LOAD does | after review |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
T-006 reproduces the wrong loading command.

## Negative cases and permission boundaries
Missing agents, unconfigured site.

## Isolation and cleanup
Private server on port 7700 with a scratch HOME; stopped afterwards.

## Deferred tests and coverage limitations
Site sources `MOUNT`, `DOME` and `GPS` described from `indigo_agent_mount.c` only; no mount, dome or GPS simulator was selected.
