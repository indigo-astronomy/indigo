# Test plan

| Criterion | Scenario | Test |
| --- | --- | --- |
| AC-001/003 | Flip fails at recentering after a finished sequence: state Ok, no new revision of `SEQUENCE_STATE` or `SEQUENCE_STEP_STATE`, no "Sequence aborted", `AGENT_ABORT_PROCESS` Ok | "meridian flip failure keeps finished state" |
| AC-001 | Same after a failed sequence, state Alert | "meridian flip failure keeps failed state" |
| AC-001 | Same after a client abort, state Alert, one "Sequence aborted" only | "meridian flip failure keeps aborted state" |
| AC-001 | Same after `SEQUENCE_RESET`, state Ok | "meridian flip failure keeps reset state" |
| AC-001/003 | No solver agent loaded after a finished sequence: `FLIPPER_STATE` Alert, state Ok, no "Sequence aborted" (expectation corrected from Alert, it described the replaced behaviour) | "meridian flip without solver fails" |
| AC-002 | Failed flip under the default policy: Alert, "Sequence aborted", step state Alert | "meridian flip failure aborts" (step state assertion added) |
| AC-002/003 | Client abort of a running sequence ends in Alert; client abort when idle answers Alert, `SEQUENCE_STATE` not published (assertions added) | "pause resume abort reset" |
| AC-005 | Full suite; candidate tests against the previous Sequencer.js and against the Alert never published | `make -C indigo_test test-agent-scripting-sequencer`; `mutation-checks.txt` |

Limits: the flip is entered at the end of the slew with a stand-in solver, as in the existing flip tests. The agents being aborted is not asserted separately. No hardware; the suite has no recorded driver run.
