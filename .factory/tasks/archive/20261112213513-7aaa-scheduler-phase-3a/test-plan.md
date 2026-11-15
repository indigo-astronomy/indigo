# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `indigo_test/integration/run_scheduler_simulators.py` (opt-in `test_agent_scheduler_simulators`) | private indigo_server, CCD and mount simulators | 6 cases pass, frames on disk | after implementation |
| T-002 | AC-002 | `test_agent_scheduler`: hooks_run_at_start_idle_and_stop, failed_startup_hook_stops_the_scheduler, abort_runs_the_shutdown_hook, start_during_shutdown_hook_continues | Scripting Agent | hooks in order, never scheduled | after implementation |
| T-003 | AC-003 | `test_agent_scheduler`: history_records_runs | restart in the case | entries, order, restore, clear, client write rejected | after implementation |
| T-004 | AC-004 | `test_agent_scheduler`: duration_estimated_measured_and_set, library_without_estimate_uses_the_default, repeat_retry_and_failures | Scripting Agent | label sources, script errors | after implementation |
| T-005 | AC-005 | `test_agent_scripting_sequencer`: estimate counts, inventory | Scripting Agent | counts | after implementation |
| T-006 | AC-006 | full scheduler, Sequencer, Scripting suites, ASan | as recorded | no new failure | final |

## Baseline evidence
plan.md "Baseline".

## Bug reproduction
Defects found are listed in verification.md with their regression tests.

## Negative cases and permission boundaries
Client writes of the history rejected; script errors not run; library without estimate().

## Isolation and cleanup
Private HOME per case and per server run; server output removed when the run passes.

## Deferred tests and coverage limitations
An edit during an in-flight estimate and a NaN estimate are covered by source review only; the night plan preview is phase 3b.
