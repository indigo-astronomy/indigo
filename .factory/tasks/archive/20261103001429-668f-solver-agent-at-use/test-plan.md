# Test plan

| Criterion | Scenario | Test |
| --- | --- | --- |
| AC-001 | Sequence with only `enable_meridian_flip(true,0)` finishes with "Astrometry Agent"; solver stand-in renamed to "Platesolver Agent"; flip entered at the end of the slew; `CENTER` reaches "Platesolver Agent". Must fail against the previous Sequencer.js | "meridian flip uses solver loaded after sequence" |
| AC-001/004 | Solver loaded before `start()` and solver selected by `select_solver_agent()` still used by the flip | existing "meridian flip uses selected solver", "meridian flip uses solver selected by step" |
| AC-002 | Solver stand-in renamed to a non-solver name before the flip; flipper not waiting, state Alert, handler removed, `FLIPPER_STATE` Alert | "meridian flip without solver fails" |
| AC-003 | Solver replaced during `wait(1)` before `precise_goto`; target reaches "Platesolver Agent" | "solver replaced during sequence" |
| AC-003 | Solver replaced during a running sequence, then abort; `AGENT_PLATESOLVER_ABORT` set on the new agent | "abort reaches replaced solver" |
| AC-004 | `select_solver_agent('Custom Solver')` used for `precise_goto` | existing "select solver agent step" |
| AC-005 | Stand-in reports `AGENT_START_PROCESS` Alert; `sync_center`, `precise_goto`, `set_solver_filter('Missing')` under `recover_on_failure()` each skip to the next recovery point | "solver failures recover" |
| AC-006 | Sequencer.md matches behaviour | review |
| AC-007 | Full Sequencer.js integration suite; candidate tests against the previous Sequencer.js | `make -C indigo_test test-agent-scripting-sequencer`; `mutation-checks.txt` |

Limits: the flip tests enter the flipper at the end of the slew with a stand-in solver; transit detection, a real mount slew, a real solver and Ain Imager are not exercised. No hardware; recorded driver runs (`tools/run_driver_test.py`) do not apply to Sequencer.js.
