# Task brief

- Task ID: 20261106215623-197e-fix-focuser-qhy-json-handshake
- Owner: Peter Polakovic
- Created: 2026-11-06

## Problem and desired outcome
A user reported that a QHY Q-Focuser (firmware 20231207, board version 108, USB id 28e9:018a) cannot connect with
`indigo_focuser_qhy` on INDIGO Sky. The focuser answers the `{"cmd_id":1}` handshake with

```
{"idx":1,"id":"@\u0019SC8HP\u0006TH2A","version":20231207,"bv":108}
```

(67 bytes; the `\u` sequences are literal JSON escapes). The driver reports "Failed to connect to Q-Focuser on
/dev/ttyACM0" and disconnects. The driver must accept this reply and connect.

## Users and use cases
Every user of a Q-Focuser with this firmware; the driver has been unusable for them since the migration to the code
generator (commit c252b65ec, 2026-09-13).

## Scope and non-goals
In scope: the driver's JSON reply parser, the handshake field handling, the simulator's handshake reply, the
integration tests, the driver version. Non-goals: decoding JSON escapes into characters, other protocol commands,
hardware validation (no Q-Focuser is available).

## Constraints and references
Root AGENTS.md driver and generated-driver rules; `indigo_drivers/AGENTS.md`; `indigo_test/AGENTS.md`;
driver source `indigo_drivers/focuser_qhy/indigo_focuser_qhy.driver`; simulator
`indigo_drivers/focuser_qhy/focuser_qhy_simulator/focuser_qhy_simulator.c`; suite
`indigo_test/integration/test_focuser_qhy_simulator.c`; the user's server log and report (quoted above).

## Initial questions
None blocking. Whether older firmware sends `version`/`bv` as strings is unknown; the driver accepts both forms.
