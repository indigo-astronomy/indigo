# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | indigo_test/unit/test_prop_tool_parser.c empty_values, malformed_requests | parser built as in the harness | empty values parsed, empty item names rejected | after change |
| T-002 | AC-001, AC-002 | private server with the Scheduler Agent | indigo_prop_tool set / set_script | text cleared, number/switch/script name errors | after change |
| T-003 | AC-003, AC-004 | tools/scheduler_sandbox.py scripted runs | private server | hooks at the expected seconds, Sun within 0.0001 deg of the agent | after change |
| T-004 | AC-005 | `make -C indigo_test test-scheduler-sandbox` | tools/scheduler_sandbox_night.txt | exit 0; a missing message exits 1 | after change |
| T-005 | review findings | sandbox: busy port, stale until, script stop, Ctrl-C, hooks without = | private servers | as fixed | after review |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
T-002 before the change: `IDLE=` failed with "Invalid property string format".

## Negative cases and permission boundaries
Empty number, switch and script file name; busy port; unknown command; server exit.

## Isolation and cleanup
Private servers on scratch ports and HOMEs, stopped; sandbox directories removed after passing runs.

## Deferred tests and coverage limitations
No macOS or Windows run of the tool or the sandbox.
