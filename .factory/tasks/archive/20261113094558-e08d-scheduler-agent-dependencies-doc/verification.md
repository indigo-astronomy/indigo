# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5)
Date: 2026-11-13
Revision and working-tree state: branch `scheduler` at e33784568 with this task's uncommitted fixes
Review independence: not independent; the same agent wrote the first version and reviewed it on a running server.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | table matches `observe_property()`, `agent_busy_bit()`, the two `AGENT_ABORT_PROCESS` changes and the `AGENT_SCRIPTING_RUN_SCRIPT` changes in `indigo_agent_scheduler.c`; `needs_sky()` uses only `@target` and `@twilight`, and the other sky keys require `@target` |
| AC-002 | pass | after the fixes below |
| AC-003 | pass | T-001..T-005 as expected |
| AC-004 | pass | after the fixes below |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `indigo_server -p 7700 indigo_agent_scheduler`, scratch HOME; `set_script` of a `@target` task; `CONTROL.START` | scratch directory, Linux arm64 | 0 | REASON `waiting for the Scripting Agent` |
| `set "Server.LOAD.DRIVER=indigo_agent_scripting"` | same | 0 | REASON `site coordinates unknown` |
| `set_script` of a task without sky keys | same | 0 | task ran, DONE 1, REASON still `site coordinates unknown` for the sky task |
| `set "Server.LOAD.DRIVER=indigo_agent_mount"`, then waits of 5, 15 and 30 s (separate server with the Scripting Agent) | same | 0 | `site coordinates unknown` until the next check, then `target below the horizon` with LATITUDE 0, LONGITUDE 0; IDLE_CHECK_INTERVAL 30 |
| `set "Mount Agent.GEOGRAPHIC_COORDINATES.LATITUDE=42.7;LONGITUDE=23.3;ELEVATION=550"` | same | 0 | `all tasks finished`; `AGENT_SITE_DATA_SOURCE.HOST` ON |
| `set "Server.DRIVERS.indigo_agent_scripting=ON"` against a server started with `indigo_agent_scheduler` only | same | 0 | not loaded: DRIVERS lists only `indigo_agent_scheduler` |
| link and anchor check of both documents (Python) | repository root | 0 | no broken link |

## Baseline comparison and regressions
Documentation only.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| Guide said the Mount Agent takes the site from a selected mount or GPS | wrong setup advice | the source is chosen by `AGENT_SITE_DATA_SOURCE`, default `HOST` | site source explained; direct set replaced when the source is a device | code review of `handle_site_change()` | review |
| Guide loaded agents with `Server.DRIVERS...=ON` | agents not loaded on a server started with a driver list | DRIVERS lists only known drivers | start-up list and `Server.LOAD.DRIVER`, one per command | T-006 | review |

## Manual checks and limitations
Site sources `MOUNT`, `DOME` and `GPS` not exercised with devices.

## Verification verdict rationale
All acceptance criteria pass after the fixes.

## Final test summary
- Simulated tests: private-server checks above.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
No findings.
