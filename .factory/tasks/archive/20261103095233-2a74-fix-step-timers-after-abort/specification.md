# Specification

AC-001 After `abort()`, or after `failure()` under the abort policy, a step timer scheduled earlier by that sequence does nothing: the step state stays Alert and no further step runs.

AC-002 A step timer of an aborted sequence does not advance a sequence started after the abort.

AC-003 The abort branch of `failure()` cleans up like `abort()`: the stored timer is cancelled, `wait_for_timer` and `altitude_poll` are cleared.

AC-004 `abort()` completes when the stored timer can't be cancelled because it is already firing: `SEQUENCE_STATE` Alert, "Sequence aborted", `AGENT_ABORT_PROCESS` published.

AC-005 Timers of a running sequence are not affected, including the continue and recover failure policies.

AC-006 Tests cover AC-001 to AC-004 and fail against the previous Sequencer.js; no regressions in the Sequencer.js integration suite.
