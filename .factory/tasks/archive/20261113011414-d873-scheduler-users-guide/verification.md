# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5); independent fact check by a fresh reviewer subagent (quality-review.md)
Date: 2026-11-13
Revision and working-tree state: branch `scheduler` at c52655486 with this task's uncommitted changes
Review independence: implementer verified the examples; the fact check was independent.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | writing tasks: keys, priorities and score, windows, moon, duration, repeat and retry, body rules, examples; fact-checked against the parser, sky and plan code |
| AC-002 | pass | submitting, urgent tasks, control, hooks and settings; examples run |
| AC-003 | pass | state, current, task list, plan, history, messages, reasons; labels compared with live output |
| AC-004 | pass | commands, order, edit, delete, restart, troubleshooting; examples run |
| AC-005 | pass | every code block extracted from the Markdown file and run, see below |
| AC-007 | pass | `set_script` with `TASK` files: upload, urgent, edit of `TASK_0`, rejection reason printed; every `indigo_prop_tool` command and task file extracted from the final guide and run, see below |
| AC-008 | pass | the agent uses the duration only for the start check, the plan and the label (`task_duration()` callers), no run watchdog exists; example task `M31 until 02:00` uploaded and estimated without error, a broken body reported as a script error for comparison |
| AC-006 | pass | HOME.md link, Xcode file reference and group entry; link and anchor check: no broken link |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| private `indigo_server -v -- -p 7700 -b- -c- -w-` with `indigo_agent_scripting`, `indigo_agent_scheduler`, `indigo_agent_mount`, scratch HOME | repository root, Linux arm64 | 0 | agents loaded |
| quick start: `set_script submit_test.js`, `CONTROL.START`, `get CURRENT`, `get_state STATE`, status script | same | 0 | `Test - QUEUED, 0h01 set`; CURRENT `Test` while running; DONE, `Test #1  succeeded`, STATE Ok, REASON `all tasks finished` |
| upload of the four example tasks | same | 0 | all accepted; `M31 LRGB ... 1h48 estimated` (6000 s + 20 × 10 s + 2 × 120 s) |
| `submit_m31.js` (duplicate name) | same | 0 | `Task rejected: a task named 'M31 LRGB' already exists` logged by the script |
| `submit_urgent.js`, `hooks.js`, hooks and settings with `indigo_prop_tool` | same | 0 | urgent task queued with the not-running message; hooks set; settings changed |
| `indigo_prop_tool set "...HOOKS.IDLE="` | same | 1 | `Invalid property string format`, as documented |
| `edit_task.js`, TASK_NAME + DISABLE, DELETE of a task and of an unknown name | same | 0 | text printed, retry changed; DISABLED; task removed; `Task 'Nothing' not found` |
| CLEAR_HISTORY, server restart without `-v`, `Server.LOG_LEVEL.INFO=ON`, status script | same | 0 | no script output before, full output after; history empty; urgent task dropped, other tasks restored; night plan with reasons |
| `make` in `indigo_tools` after the `indigo_prop_tool.c` change | `indigo_tools` | 0 | built without warnings |
| `indigo_prop_tool set_script -p 7700 -s` with `TASK=` files to `ADD_TASK`, `URGENT_TASK` and `TASK_0`; a task with `;` in its name and body; a task with an unknown key | scratch directory, private server with Scripting and Scheduler Agents | 0 | `[OK]` for valid uploads and the edit; `[ALERT]` plus `Scheduler Agent: Task rejected: line 2: @bogus: unknown key`; urgent duplicate rejected with its reason; file content no longer echoed |
| `indigo_prop_tool set_script ... AGENT_SCRIPTING_ADD_SCRIPT.SCRIPT=s.js;NAME=s.js` | same | 0 | the existing SCRIPT upload still works |
| all 24 `indigo_prop_tool` commands and the 5 task files extracted from the final guide, run in order | same | 0 | all as documented; duplicate M31 upload rejected with its reason; `list -e` shows the task labels; with no Mount Agent REASON is `site coordinates unknown` |
| `set_script` of the `M31 until 02:00` example and of a broken body, server with `-vv` | scratch directory | 0 | example accepted with no script error; broken body: `has a script error: TypeError ...` |
| link and anchor check of the guide | `indigo_docs` | 0 | 17 links, none broken |

## Baseline comparison and regressions
Documentation, plus a small `indigo_prop_tool` change; the existing `SCRIPT` upload was rerun.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| Draft scripts lost the rejection text | user sees no reason | an update's message arrives later as a separate message | scripts log the message in `on_message` | rerun | example run |
| Draft said options may precede the prop tool command | commands fail | options are parsed after the command word | guide corrected | rerun | example run |
| Draft said script output always reaches the log | nothing visible | `indigo_log` is info level | guide explains `-v` and `Server.LOG_LEVEL.INFO` | rerun | example run |
| Draft said a hook can be cleared with the prop tool | command fails | empty values are rejected | script line given | rerun | example run |
| 3 wrong and 17 misleading statements | wrong expectations | see quality-review.md | corrected | review | fact check |
| `set_script` echoed the uploaded file on the terminal | noisy output | file opened at error log level | trace level | rerun | example run |
| `indigo_prop_tool` did not show rejection reasons | user sees only Alert | no send_message callback | callback prints the target device's messages | rerun | example run |
| Failure reason of a sequence that ends Alert is lost | history shows plain `failed` | bus message split, no send_message callback | not fixed (code); FND-20261113011414-d873-scheduler-users-guide-001; guide states the behaviour | — | fact check |

## Manual checks and limitations
The web pages were not opened in a browser. Equipment-dependent behaviour is described from the implementation and the scheduler's test suites.

## Verification verdict rationale
All acceptance criteria pass; the code defect found is recorded.

## Final test summary
- Simulated tests: guide examples on a private server, all as documented.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
FND-20261113011414-d873-scheduler-users-guide-001 opened.
