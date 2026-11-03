# Test plan

| Criterion | Scenario | Test |
| --- | --- | --- |
| AC-001 | Pending unstored timer, abort; step Alert, next step not run | "step timer after abort keeps step state" |
| AC-001 | Pending unstored timer, failure under the abort policy | "step timer after failure keeps step state" |
| AC-001 | Mount process completion schedules the 0.5 s timer through `on_update`, abort inside that window | "mount delay timer after abort keeps step state" |
| AC-002 | Pending unstored timer, abort and a new sequence with `wait(10)`; the new sequence stays on step 0 | "step timer after abort does not advance next sequence" |
| AC-003 | Failure with a stored timer and an altitude poll | "failure cancels stored timer" |
| AC-004 | Stored timer that can't be cancelled, abort | "abort with timer not cancellable" |
| AC-005 | Existing failure policy, recovery, flip and imaging cases | full suite |
| AC-006 | Full suite; candidate tests against the previous Sequencer.js and four partial mutations | `make -C indigo_test test-agent-scripting-sequencer`; `mutation-checks.txt` |

A sentinel timer scheduled after the stale timer gives a bounded wait instead of a fixed sleep. Limits: unstored timers are simulated by forgetting the timer of `wait()`; the mount completion event is injected; a timer really firing during the cancel is simulated with an empty slot. No hardware.
