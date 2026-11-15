# Specification

Status: approved by owner request 2026-11-13

## Problem and outcome
See brief.md.

## Current-state audit
`indigo_agent_scheduler.c` defines fixed names `SCRIPTING_AGENT`, `MOUNT_AGENT` and `GUIDER_AGENT` (lines 49-52) and is not a filter agent; SCHEDULER.md "Architecture" states it is not a filter agent; safety (phase 4) mentions a related AUX device without saying how it is selected.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The plan contains a phase that makes the Scheduler Agent a filter agent with `FILTER_RELATED_AGENT_LIST` in which only the Scripting Agent is selected, defaulting to the local "Scripting Agent" | review of SCHEDULER.md |
| AC-002 | The safety phase selects its AUX device through the filter agent's AUX device list | review of SCHEDULER.md |
| AC-003 | SCHEDULER.md and the user guide state the current same-server requirement | review against the agent code |

## Failure behavior and edge cases
The planned phase rejects a change of the selection while a task runs.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Default selection keeps existing setups unchanged.

## Assumptions and open questions
The Mount and Guider Agents keep their fixed names; only the Scripting Agent is selectable (owner decision).

## Owner decisions and requirement changes
- 2026-11-13: only the Scripting Agent is selected; AUX is needed for safety.
