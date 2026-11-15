# Specification

Status: approved by owner request 2026-11-13

## Problem and outcome
See brief.md.

## Current-state audit
`SCHEDULER.md` is a design document; no usage guide exists. No client has a scheduler UI. `indigo_prop_tool set` cannot upload tasks (it splits values at `;` and limits text to 511 bytes), `set_script` reads a file only for an item named `SCRIPT`, the tool does not print device messages, and it shows light items without labels; the web control panel edits text in one line.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The guide explains writing a task (header keys, priorities, windows, moon, duration, repeat and retry, body rules) with examples | review against the header parser and SCHEDULER.md |
| AC-002 | It explains submitting stored and urgent tasks, starting, stopping, hooks and settings | examples run on a private server |
| AC-003 | It explains monitoring: state, current task, task list, night plan, history, messages, reasons | examples run on a private server; labels compared with the output |
| AC-004 | It explains managing tasks: commands, order, download and edit, delete, restart behaviour, troubleshooting | examples run on a private server |
| AC-005 | Every command and script in the guide works as shown | private `indigo_server` with the Scripting, Scheduler and Mount Agents |
| AC-006 | The guide is linked from HOME.md and registered in the Xcode project | diff |
| AC-008 | The guide states that `@duration` does not stop a run, lists what does, and shows checkpoint steps; SCHEDULER.md says the same and lists `@timeout` as a planned extension | review against the agent code; example task uploaded |
| AC-007 | Tasks are uploaded, edited and submitted as urgent with `indigo_prop_tool` from task files, without Scripting Agent scripts; rejections show their reason | private server with the rebuilt tool |

## Failure behavior and edge cases
Documented limitations of the tools (prop tool option order, empty values, log level for script output, messages delivered separately from updates).

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
None.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
- 2026-11-13: the owner rejected the Scripting Agent scripts for adding tasks as confusing: tasks are uploaded with `indigo_prop_tool` directly to the Scheduler Agent. `indigo_prop_tool set_script` therefore also reads a file for an item named `TASK`, prints the messages of the device whose property it changes (so a rejection reason is visible), and logs file traffic at trace level instead of echoing the file. The guide uses task files and `indigo_prop_tool` throughout; the status, watch and edit scripts are removed, and lists are read in a client.
- 2026-11-13: the owner asked that the guide state that `@duration` is not a time limit, with the in-sequence alternatives, and that a hard per-run limit `@timeout` be added to the plan in SCHEDULER.md (not implemented in this task).
