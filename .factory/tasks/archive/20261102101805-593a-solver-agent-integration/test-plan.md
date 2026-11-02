# Test plan

| Criterion | Scenario | Test |
| --- | --- | --- |
| AC-001 | Related "Platesolver Agent Test" peer with `AGENT_PLATESOLVER_SOLVE_IMAGES`; start an EXPOSURE batch; `DISABLED` becomes set. Must fail on the previous source. | `test_agent_imager` "platesolver agent solving disabled"; existing "mount transit and solver coordination" keeps the Astrometry path |
| AC-002 | Solver peer renamed to "Platesolver Agent"; `set_solver_filter`, `sync_center`, `precise_goto` reach it; explicit fourth argument overrides; `ASTROMETRY_AGENT === SOLVER_AGENT` | `test_agent_scripting_sequencer` "platesolver orchestration" |
| AC-002 | Solver peer only as "Astrometry Agent @ remote" with local imager; precise goto reaches it; imager "@ remote" and "@ other" both resolve to it; stale empty local entry ignored | `test_agent_scripting_sequencer` "remote solver orchestration" |
| AC-002/006 | Existing Astrometry orchestration and custom agent arguments unchanged | existing "astrometry orchestration", "custom agent arguments" |
| AC-003 | Abort sends `AGENT_PLATESOLVER_ABORT` to the solver slot | code inspection; the fake solver peer has no abort property (limitation recorded) |
| AC-004 | Scratch indigo_server (port 7700, private HOME) with Platesolver Agent, then with Astrometry Agent; headless Firefox via Marionette: solver properties found, `solverAgent()` value, no waiting banner, related selections both ways, no "Conflicting driver" errors in the server log; screenshot | `web-ui-check.py` (retained in this task) |
| AC-005 | Sequencer.md text matches behaviour | review |
| AC-006 | Full Imager Agent and Sequencer.js integration suites | `make -C indigo_test test-agent-imager test-agent-scripting-sequencer` |

No hardware is involved; these are bus agents and browser resources. Recorded driver runs (`tools/run_driver_test.py`) were not requested for this work.
