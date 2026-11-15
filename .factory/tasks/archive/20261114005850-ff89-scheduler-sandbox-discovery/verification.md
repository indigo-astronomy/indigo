# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5)
Date: 2026-11-14
Revision and working-tree state: branch `scheduler` at d23f31f4d with this task's uncommitted change
Review independence: not independent; small tool change, checked by runs.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `indigo_prop_tool discover` listed `Scheduler Sandbox -> vega.local:44883`; with `--no-bonjour` nothing was listed |
| AC-002 | pass | start-up lines print the port and `http://localhost:PORT/ctrl.html` and `http://192.168.1.242:PORT/ctrl.html`; both returned HTTP 200 |
| AC-003 | pass | guide section updated |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| curl of ctrl.html before the change (port 7720) | repository root, Linux x86_64 | 0 | 200 on localhost, 127.0.0.1 and the LAN address: the panel worked, the server was not announced |
| `python3 tools/scheduler_sandbox.py` + `indigo_prop_tool discover` | repository root | 0 | listed as Scheduler Sandbox |
| `python3 tools/scheduler_sandbox.py --port 7721 --no-bonjour` + discover + curl | repository root | 0 | not listed; both URLs 200 |
| `make -C indigo_test test-scheduler-sandbox` | repository root | 0 | night scenario passes |
| `./.factory/scripts/findings check` | repository root | 0 | valid |

## Baseline comparison and regressions
Night scenario unchanged.

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| Sandbox server not listed by clients, only a localhost URL printed | owner could not find the server | `-b-` and the output | Bonjour name by default, LAN URL printed | T-001, T-002 | owner report |

## Manual checks and limitations
Linux only.

## Verification verdict rationale
All criteria pass.

## Final test summary
- Simulated tests: 3 runs passed.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
No findings.
