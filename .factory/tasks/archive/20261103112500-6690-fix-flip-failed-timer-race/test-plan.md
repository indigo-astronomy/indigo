# Test plan

The script holds the agent mutex in a busy loop past the timer's due time, so the timer is RUNNING and blocked on the mutex and `indigo_cancel_timer()` really fails. The script fails if the failure was not stored, so a run missing the race is visible.

| Criterion | Scenario | Test |
| --- | --- | --- |
| AC-001 | Recover policy, firing `wait()` timer; a following `wait(10)` shows that there is no second advance | "meridian flip failure while step timer fires recovers" |
| AC-001 | Continue policy, same; step Alert | "meridian flip failure while step timer fires continues" |
| AC-002/005 | Firing altitude poll timer, poll not run, no second advance | "meridian flip failure while altitude poll fires recovers" |
| AC-003 | Abort in the same script after the failure was stored | "abort after stored flip failure" |
| AC-004 | Pause, firing timer, failure held until the pause ends | "paused stored flip failure recovers" |
| AC-006 | Full suite twice; candidate tests against the previous Sequencer.js and three mutations | `make -C indigo_test test-agent-scripting-sequencer`; `mutation-checks.txt` |

Limits: `flip_failed()` is called directly instead of through a real flip; the altitude poll is a stand-in on the production handler; the 60 s polling of real altitude steps is not run. AC-005 is confirmed by code inspection only. No hardware.
