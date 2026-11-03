# Test plan

| Criterion | Scenario | Test |
| --- | --- | --- |
| AC-001/003/004 | `recover_on_failure()`, flip enabled, `wait(10)` running; `resume_ha_limit` set; solver stand-in fails the recentering; sequence continues at recovery point #1, step timer cleared, handler removed, `FLIPPER_STATE` Alert, Mount Agent `ENABLE_HA_LIMIT` on | "meridian flip failure recovers" |
| AC-003 | `recover_on_failure()`, real Imager Agent batch (CCD simulator, 30 s exposure) busy when the flip fails; imager ends in Alert, sequence continues at recovery point #1 | "meridian flip failure during capture recovers" |
| AC-003 | `continue_on_failure()`, flip fails during `wait(10)`; the next step runs | "meridian flip failure continues" |
| AC-002 | default policy, flip fails during `wait(10)`; "Sequence aborted", following steps not run | "meridian flip failure aborts" |
| AC-002 | flip after the sequence finished, no solver agent | existing "meridian flip without solver fails" |
| AC-005 | Sequencer.md matches behaviour | review |
| AC-006 | full Sequencer.js integration suite; candidate tests against the previous Sequencer.js | `make -C indigo_test test-agent-scripting-sequencer`; `mutation-checks.txt` |

Limits: the flip is entered at the end of the slew; transit detection, a real mount slew, a real solver and the guiding failure branch are not exercised (the guiding and slew branches share `fail()` with the tested branch). No hardware; recorded driver runs do not apply.
