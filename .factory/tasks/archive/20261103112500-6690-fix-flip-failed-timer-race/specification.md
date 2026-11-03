# Specification

AC-001 Under `continue_on_failure()` or `recover_on_failure()`, when the stored timer can't be cancelled because it is firing, the flip failure is stored and applied by that timer: the step fails and the policy applies, exactly one advance results.

AC-002 The same holds for an altitude poll timer: the poll does not run and does not schedule itself again.

AC-003 An abort after the failure was stored drops it: the following steps don't run and the firing timer does nothing.

AC-004 A failure stored during a pause is applied after the pause ends.

AC-005 Altitude poll timers of an aborted or failed sequence do nothing, like step timers.

AC-006 Tests cover AC-001 to AC-004 with a timer really firing and fail against the previous Sequencer.js; no regressions in the Sequencer.js integration suite.
