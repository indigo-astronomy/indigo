# Specification

Status: approved by owner request 2026-11-12, design `indigo_docs/SCHEDULER.md`

## Problem and outcome
See brief.md.

## Current-state audit
Phase 2 agent (`indigo_drivers/agent_scheduler/`) runs tasks only inside the test harness; hooks, history, estimates and measured durations are designed but not implemented; `Sequencer.js` has no `estimate()`. Hardware decision: no hardware testing.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | In a real `indigo_server`, the scheduler loads Sequencer.js itself, runs calibration, slew, guiding and capture tasks, saves the frames, preempts a guided capture for an urgent task (exposure and guiding stopped, urgent frames saved, preempted task rerun without using a retry), and keeps a target below the horizon waiting | `run_scheduler_simulators.py` |
| AC-002 | STARTUP runs before the first task, SHUTDOWN on STOP, ABORT and the automatic stop, IDLE once per idle period after IDLE_PARK_AFTER; hooks are never scheduled as tasks; a failed startup hook stops the scheduler with Alert | integration cases, server case "shutdown hook parks the mount" |
| AC-003 | The history lists finished runs newest first with result and reason, survives a restart, can be cleared, and cannot be written by clients | integration `history_records_runs` |
| AC-004 | Without `@duration`, the duration is estimated from the sequence (exposure, frames, operations, waits with the overhead settings), measured after a successful run, discarded on edit, kept across restarts; unknown estimates use 60 minutes with a message; script errors are found by the estimate and the task is not run | integration `duration_*`, `library_without_estimate_*`, `repeat_retry_and_failures`; server case "slew, guiding and capture task" |
| AC-005 | `Sequence.prototype.estimate()` counts captures, frames, operations, waits and unknown waits, including `repeat()` | `test_agent_scripting_sequencer` "estimate counts" |
| AC-006 | No regression in existing scheduler, sequencer and scripting tests | suite runs |

## Failure behavior and edge cases
As in SCHEDULER.md Hooks, Run history, Task duration.

## Permissions, data, and security requirements
History text sanitised (tabs and line breaks); restore only from the configuration reader.

## Compatibility and nonfunctional requirements
Older Sequencer.js without `estimate()` falls back to the default duration.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
2026-11-12: non-interactive simulator mode; phase 3a after step 1. Design change recorded in SCHEDULER.md: script errors found by the estimate mark the task instead of rejecting the upload, because the estimate is asynchronous.
