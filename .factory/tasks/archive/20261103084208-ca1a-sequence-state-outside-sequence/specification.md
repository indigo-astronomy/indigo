# Specification

AC-001 Outside a sequence: when `indigo_sequencer.abort()` runs with no sequence running (a meridian flip failing after the sequence ended), `SEQUENCE_STATE` and `SEQUENCE_STEP_STATE` are not published, their state and values stay as they were, and "Sequence aborted" is not sent. This holds after a sequence that finished (Ok), failed (Alert) or was aborted (Alert), and after `SEQUENCE_RESET` (Ok).

AC-002 Inside a sequence: an abort by the client, or a failed flip under `abort_on_failure()`, sets `SEQUENCE_STATE` and the current step to Alert and sends "Sequence aborted", as before.

AC-003 The rest of the abort is unchanged outside a sequence: `FLIPPER_STATE` Alert, the flip failure message, the agents aborted, `AGENT_ABORT_PROCESS` published Ok. An abort requested by a client with no sequence running still answers with `AGENT_ABORT_PROCESS` Alert and leaves `SEQUENCE_STATE` unchanged.

AC-004 Documentation: Sequencer.md describes AC-001.

AC-005 Tests cover AC-001 to AC-003 and fail against Sequencer.js without the change and with the Alert never published; no regressions in the Sequencer.js integration suite.
