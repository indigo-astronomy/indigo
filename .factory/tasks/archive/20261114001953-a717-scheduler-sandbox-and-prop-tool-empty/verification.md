# Verification report

Verdict: pass
Reviewer: coordinator (Claude Code, claude-opus-5-5); independent reviewer subagent (fresh context, same model)
Date: 2026-11-14
Revision and working-tree state: branch `scheduler` at ebc39919d with this task's uncommitted changes
Review independence: the reviewer received the diff and run instructions, not the writer's conclusions.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | parser test empty_values and malformed_requests pass; private server: `IDLE=` clears IDLE, STARTUP unchanged |
| AC-002 | pass | `IDLE_PARK_AFTER=` → "Item 'IDLE_PARK_AFTER' of a number property needs a value", exit 1; `START=` → "needs ON or OFF", exit 1; `set_script ...TASK=` → "Item 'TASK' needs a file name", exit 1 |
| AC-003 | pass | scripted runs: tasks, urgent, hooks set and cleared, start/stop, status, plan, history, messages |
| AC-004 | pass | Sun altitude through libindigocat/libindigo: max 0.00005 deg from indigo_scheduler_sun_altitude over 16 dates and longitudes; `dusk 20` started the night start hook 20 s later, `dusk 10`/`dawn 10` on time |
| AC-005 | pass | `make -C indigo_test test-scheduler-sandbox` exit 0 (about 40 s); `until "never" 2` exits 1 and stops the script |
| AC-006 | pass | guide section matches the commands; empty-value examples use captured output |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `indigo_prop_tool set` / `set_script` against `indigo_server -p 7712..7714 indigo_agent_scheduler` | scratch, Linux x86_64 | as listed | AC-001, AC-002 |
| parser test built with the harness flags | repository root | 0 | 6 cases passed |
| `python3 -B indigo_test/unit/test_tool_input_safety.py` | repository root | 1 | 11/14 pass; 3 fail in indigo_raw_crop and the Gaia reader, unrelated and previously hidden by the build failure (FND-...-002) |
| `python3 tools/scheduler_sandbox.py --script ...` (several scenarios) | repository root | 0 / 1 as expected | see T-003..T-005 |
| `make -C indigo_test test-scheduler-sandbox` | repository root | 0 | night scenario |
| `./.factory/scripts/findings check` | repository root | 0 | `Findings valid: 32 active, 296 archived; task evidence checks passed.` |
| anchor check; `git diff --check` | repository root | 0 | clean |

## Baseline comparison and regressions
The parser test's expectation that `D.P.I=` and `D.P.I=  ` are invalid was replaced by the opposite on owner request; empty item names stay invalid (`D.P.=`, `D.P. =`).

## Found defects
| Defect | Impact | Cause | Fix | Regression test | Found by |
| --- | --- | --- | --- | --- | --- |
| `ITEM=` rejected | text items could not be cleared without quotes; guide claimed it was impossible | emptiness check in the parser | empty value accepted for text items | empty_values | owner request |
| Empty number/switch values would be sent as 0/unchanged | silent wrong change | no type check | error before sending | private server | writer |
| test_tool_input_safety.py did not build on Linux | parser test never ran | missing -D_GNU_SOURCE and Avahi libraries | flags and libraries as in the Makefile | the suite builds | writer |
| Sandbox: busy --port drove the server holding it | could change a real observatory server | server bind failure not detected | port checked before start, bind failure in the log aborts | busy port run | reviewer (reproduced) |
| Sandbox: until matched old messages; script continued after a miss; Ctrl-C exit 0; server death unnoticed; `hooks idle` cleared silently; print-time stamps; temp dir left when Popen failed | wrong or slow results | sandbox logic | fixed as described | T-005 runs | reviewer |
| Sandbox Sun 0.375 deg from the agent | dusk/dawn up to 1.5 min off | different ephemeris | INDIGO libraries via ctypes | AC-004 comparison | writer |

## Manual checks and limitations
Linux x86_64 only. Interactive line editing tried only through piped input.

## Verification verdict rationale
All acceptance criteria pass; the remaining tool-safety failures are unrelated and recorded as a finding.

## Final test summary
- Simulated tests: parser 6/6; sandbox scenarios 5 run (3 expected passes, 2 expected failure exits) as designed; night scenario 1/1; private-server checks 6/6.
- Hardware tests: 0 run, 0 passed.

## Register reconciliation
Opened FND-20261114001953-a717-scheduler-sandbox-and-prop-tool-empty-001 (Sun/Moon equinox in the scheduler sky) and -002 (three tool-safety tests).
