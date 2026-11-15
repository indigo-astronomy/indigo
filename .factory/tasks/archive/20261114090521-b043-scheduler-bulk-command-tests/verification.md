# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5)
Date: 2026-11-14
Revision and working-tree state: branch `scheduler` at 2ed5675dd with this task's uncommitted changes
Review independence: not independent; test-only change checked by mutations and the full suites.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | reset_all_runs_finished_work_again: "Tasks reset: 0" on an empty list; after the automatic stop "Tasks reset: 4" with TASK_NAME "Nobody"; Twice "QUEUED, run 0/2", Broken "QUEUED, retries left 1", second script-error message for Syntax, Off DISABLED, DONE/FAILED 0; after START Twice and Broken started 4 times each, Syntax and Off never |
| AC-002 | pass | delete_all_waits_for_the_shutdown_hook: "Tasks deleted: 0" on an empty list; Alert after STOP while Long runs and again while hook 'Park' runs; SHUTDOWN hook named "park" keeps task Park; "Tasks deleted: 1, tasks named as hooks kept: 1"; AGENT_SCHEDULER_TASK_1 deleted, TASK_0 is Park, history count unchanged, plan without Long; a new "Long" takes TASK_1 |
| AC-003 | pass | FND-20261114090521-b043-scheduler-bulk-command-tests-001 recorded; findings check valid |
| AC-004 | pass | SCHEDULER.md testing list; MIGRATION_STATUS 57 / 0 |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `test_agent_scheduler reset_all_runs` / `delete_all_waits` | indigo_test, Linux x86_64 | 0 | both pass |
| mutations: script error not cleared by reset_task(); retries_used not cleared | indigo_test | 1 each | reset_all_runs_finished_work_again fails; agent restored and verified identical (cmp) |
| `test_agent_scheduler_asan` | indigo_test | 0 | 57/57, no sanitizer report |
| `python3 tools/run_driver_test.py agent_scheduler` | repository root | 0 | 65/65 OK (57 integration + 8 server acceptance), recorded 3.0.0.8 |
| `./.factory/scripts/findings check` | repository root | 0 | valid, 33 active |

## Baseline comparison and regressions
Baseline recorded run 63/63 at 3.0.0.8; no regressions.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| JSON protocol drops item hints | JSON clients never see warn_on_set on items | indigo_json.c writes and parses no item hints | not fixed, owner chose a finding: FND-20261114090521-b043-scheduler-bulk-command-tests-001 | none yet | source review while planning a network test |

## Manual checks and limitations
No network test of the hints, see the finding. The `findings create` script requires a task/<id> branch; this owner-directed work runs on `scheduler`, so the record was written directly with the task-scoped ID and validated by `findings check`.

## Verification verdict rationale
All criteria pass on the final candidate.

## Final test summary
- Simulated tests: recorded run 65 run, 65 passed; ASan 57 run, 57 passed.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
One open finding, low, non-blocking: FND-20261114090521-b043-scheduler-bulk-command-tests-001.
