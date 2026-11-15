# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-005 | quick start: `submit_test.js`, START, CURRENT, STATE, history, automatic stop, delete | private server, Scripting, Scheduler and Mount Agents | as described in the guide | after writing |
| T-002 | AC-001, AC-005 | the four example tasks uploaded | same | accepted, labels as shown | after writing |
| T-003 | AC-002, AC-005 | `submit_m31.js` (duplicate, rejection message), `submit_urgent.js`, `hooks.js`, hooks and settings with `indigo_prop_tool`, empty value rejected | same | as described | after writing |
| T-004 | AC-003, AC-004, AC-005 | `scheduler_status.js`, `scheduler_watch.js`, `edit_task.js`, DISABLE, delete, unknown name, clear history, restart, `Server.LOG_LEVEL.INFO` | same | as described | after writing |
| T-005 | AC-006 | link and anchor check | none | no broken link | final |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Duplicate name, unknown task name, empty prop tool value.

## Isolation and cleanup
Private server on port 7700 with a scratch HOME; stopped and removed afterwards.

## Deferred tests and coverage limitations
Equipment-dependent behaviour (slews, guiding, urgent preemption of a real capture) is described from the implementation and covered by the scheduler's own test suites, not run for the guide. The web pages were not opened in a browser.
