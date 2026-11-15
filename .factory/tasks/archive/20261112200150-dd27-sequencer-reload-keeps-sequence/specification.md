# Specification

Status: approved by owner instruction 2026-11-12

## Problem and outcome
See brief.md.

## Current-state audit
`Sequencer.js` ends with `var indigo_sequencer = {...}` (unconditional), `var indigo_flipper = {...}`, ten `indigo_delete_property(LOOP_n)` calls and the handler registration. A reload during a sequence replaces the object that holds the step list, timers generation and loop state; the flipper registered in `indigo_event_handlers` while a flip is enabled would diverge from the global `indigo_flipper`. The library is not compiled into the Scripting Agent; the Scheduler Agent embeds it as `Sequencer.js.dat`, the server as a web resource.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A reload during a running sequence keeps the sequence object, its handler registration and its loop properties; the sequence completes; the message "Sequencer reloaded, the running sequence continues" is sent | `test_agent_scripting_sequencer` case "library reload keeps running sequence" |
| AC-002 | A reload of an idle sequencer replaces it as before | same case |
| AC-003 | A reload keeps an enabled flipper and replaces a disabled one | same case |
| AC-004 | A scheduled task survives a client reloading the library | `test_agent_scheduler` case `library_reload_keeps_scheduled_task` |
| AC-005 | No regression in the Sequencer, Scripting Agent and Scheduler suites | suite runs |

## Failure behavior and edge cases
A paused sequence counts as running. The kept object keeps its own (older) methods until it is idle.

## Permissions, data, and security requirements
No change.

## Compatibility and nonfunctional requirements
Backward compatible: first load and idle reloads behave as before.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-11-12: owner asked to fix the sequencer after the scheduler commits.
