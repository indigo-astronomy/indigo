# Specification

Status: approved by owner request 2026-11-13

## Problem and outcome
See brief.md.

## Current-state audit
The guide listed drivers to load but did not separate the scheduler's own dependencies from the task bodies'; it loaded agents with `Server.DRIVERS`, which only switches drivers the server already lists. SCHEDULER.md listed the agents without the properties used or the effect of a missing agent.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | SCHEDULER.md lists each agent the Scheduler Agent uses directly, whether it is needed, the properties used and the effect of its absence | review against the code |
| AC-002 | The user guide lists the same agents with their drivers, separates the agents the task bodies use, and explains how to load them and set the site | review against the code; private server |
| AC-003 | Every claim about missing agents and the site holds on a running server | private server: no Scripting Agent, no Mount Agent, unconfigured and configured Mount Agent, task without sky keys |
| AC-004 | The documented commands work | private server |

## Failure behavior and edge cases
An unconfigured Mount Agent (0°/0°) is accepted as a site; documented.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
- 2026-11-13: owner committed the first version (e33784568) and asked for a review; fixes go in a new commit.
