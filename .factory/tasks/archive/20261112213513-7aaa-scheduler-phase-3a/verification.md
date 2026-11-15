# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5); independent diff review by a fresh reviewer subagent (quality-review.md)
Date: 2026-11-13
Revision and working-tree state: branch `scheduler` at 6898c7094 with this task's uncommitted changes
Review independence: implementer verified; the diff review was independent.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `run_scheduler_simulators.py` 6/6: library loaded by the scheduler, guider calibration, slew + guiding + 3 frames, urgent preemption of a guided 40-frame capture (exposure and guiding stopped, 2 urgent frames, preempted task rerun with its retry intact), target below the horizon waits, shutdown hook parks the mount |
| AC-002 | pass | four hook cases in `test_agent_scheduler`; server case "shutdown hook parks the mount" |
| AC-003 | pass | `history_records_runs` |
| AC-004 | pass | `duration_estimated_measured_and_set`, `library_without_estimate_uses_the_default`, `repeat_retry_and_failures`; server capture task estimated then measured |
| AC-005 | pass | "estimate counts" |
| AC-006 | pass | see executed checks; only pre-existing failures remain |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `./indigo_test/build/integration/test_agent_scheduler` (twice) | repository root, Linux arm64 | 0 | 30/30 each |
| `ASAN_OPTIONS=detect_leaks=1 ./indigo_test/build/integration/test_agent_scheduler_asan` | repository root | 0 | 30/30, no report |
| unit `test_agent_scheduler_header`, `test_agent_scheduler_sky` | repository root | 0 | all passed |
| `gcc -O2 -D_FORTIFY_SOURCE=2 -Wall -Wextra` on the agent | `indigo_drivers/agent_scheduler` | 0 | no overflow warning (only the intentional INDIGO_COPY_VALUE truncation notes) |
| `./indigo_test/build/integration/test_agent_scripting_sequencer` | repository root | 0 | 65/65 |
| `python3 tools/run_driver_test.py agent_scheduler` | repository root | 0 | 36/36 OK (30 integration + 6 server), recorded |
| `python3 tools/run_driver_test.py agent_scripting` | repository root | 1 | 238/236, recorded; the two failures are pre-existing: FND-20261105112837-9c3a-sequencer-simulator-suite-004 and -005 |

## Baseline comparison and regressions
The Sequence API inventory expectations changed by one name/method, on purpose (estimate()). No other expectation changed.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| Last history entry lost on restart | history incomplete after restart | the configuration reader trims trailing whitespace; empty mark and reason trailed the last line | mark saved as `-`, trailing empty fields tolerated | `history_records_runs` | reproduced |
| Task could run before its estimate | script error run; wrong duration in the constraint check | selection ignored pending estimates | tasks wait for their estimate | `repeat_retry_and_failures` | reproduced |
| Task uploaded during the automatic stop never ran | lost night | stop requested one pass before it completed | stop cancelled by new work before its shutdown hook | server "slew, guiding and capture task" | reproduced |
| History name overflow | crash in fortified builds, overflow from a long restored name | INDIGO_COPY_NAME on a 41-byte field | snprintf into the field | fortify compile | review |
| START during the shutdown hook lost | scheduler stopped against START | hook end stopped unconditionally | stop only while requested, startup hook runs again | `start_during_shutdown_hook_continues` | review |
| Stale estimate failure after an edit | task stuck in SCRIPT ERROR | old estimate slot kept | slot cleared on edit, success clears the flag | source review only | review |
| Edit of a running hook applied at once | hook renamed mid-run | deferral checked the task state only | checks the running slot | `start_during_shutdown_hook_continues` | review |
| Shutdown hook skipped silently / ABORT during startup lost | no park, wrong message | missing message and branch | message; abort runs the shutdown hook | source review, `abort_runs_the_shutdown_hook` | review |
| NaN or huge estimate cast to int | undefined behaviour | unchecked cast | finite check and clamp | source review only | review |

## Manual checks and limitations
Hardware-test decision: none. Windows and macOS not built. Covered by source review only: edit during an in-flight estimate, NaN estimate.

## Verification verdict rationale
All acceptance criteria pass; remaining failures are pre-existing and tracked.

## Final test summary
- Simulated tests: scheduler 36 run / 36 passed (30 integration, 6 server acceptance), Sequencer 65/65, unit 22/22, recorded Scripting Agent run 238/236 with 2 pre-existing failures.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
No new findings; FND-20261112184305-e0a6-scheduler-agent-core-003 (next-start search cost) stays open for phase 3b.
