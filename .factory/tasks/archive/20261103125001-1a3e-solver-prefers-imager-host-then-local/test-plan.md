# Test plan

| Criterion | Scenario | Test |
| --- | --- | --- |
| AC-001 | Loaded imager on host a, solvers on a, b, stale and local | "solver autoselection prefers imager host then local", also through `start()` |
| AC-002 | Loaded imager on host c without a solver, local solver last in `indigo_devices`; imager on b not loaded; stale entry without `AGENT_IMAGER_BATCH` with a solver on its host | same test |
| AC-002/003 | No local solver: nothing loaded, then a solver on another host only | "solver autoselection without local solver" |
| AC-004 | `start()` with nothing loaded, then imager on a, a decoy solver first in `indigo_devices` and a solver on a appear; the step reaches the solver on a | "solver autoselection corrected at use" |
| AC-005 | Sequencer.md matches behaviour | review |
| AC-006 | Full suite; candidate tests against the previous Sequencer.js and three mutations | `make -C indigo_test test-agent-scripting-sequencer`; `mutation-checks.txt` |

Limits: remote agents are device entries defined in JavaScript, except the renamed solver stand-in; no real second server, real enumeration timing, real solver or hardware.
