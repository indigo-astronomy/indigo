# Specification

AC-001 Common flip failure: every meridian flip failure (slew Alert, no solver agent loaded, sync & center Alert, guiding Alert) removes the flipper handler, restores `ENABLE_HA_LIMIT` on the mount agent when the flip disabled it, sets `FLIPPER_STATE` Alert and reports the cause ("Meridian flip failed (...)").

AC-002 Abort policy: with `abort_on_failure()` (default) or when no sequence runs (flip enabled by a finished sequence) the sequence and the agents are aborted as before.

AC-003 Continue and recover policies: otherwise the step during which the flip ran fails and the failure policy applies (`continue_on_failure()` runs the next step, `recover_on_failure()` continues at the next `recovery_point()`). When that step is a capture waiting for the imager's `AGENT_START_PROCESS` (the capture paused by the flip), the imager process is aborted and the step fails when the imager reports Alert, so the following steps start with the imager stopped. For other steps, pending waits and the step timer are cancelled, the imager process is aborted and the step fails.

AC-004 HA limit bookkeeping: the flip remembers per transit whether it disabled the HA limit (previously the flag was never cleared, so a later flip could re-enable a limit disabled by the user).

AC-005 Documentation: Sequencer.md describes AC-001 to AC-003.

AC-006 Tests cover the recover, continue and abort policies, a capture interrupted by the flip with a real Imager Agent batch, and the HA limit restoration; tests of changed behaviour fail against the previous Sequencer.js; no regressions in the Sequencer.js integration suite.
