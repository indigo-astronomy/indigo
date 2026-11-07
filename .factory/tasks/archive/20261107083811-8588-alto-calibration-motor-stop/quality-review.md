# Task quality review

Verdict: pass
Reviewer and independence: separate general-purpose agent context (claude-opus-5-5, read-only), not the writer of the diff; two rounds, given the code, the instructions and the specification, not the implementer's conclusions. Fix reinspection by the implementer.
Reviewed revision and working-tree state: base 86d3e884e plus the uncommitted task diff (round 1: the `Motor_Stop` change; round 2: the version 11 delta from the hardware run), excluding `TEST_SUMMARY.md` (generated) and `.factory/`

## Scope and method
Round 1: `indigo_aux_alto.driver` and its regenerated `.c`, `primalucelib.h`, `PROTOCOL.md`, `alto_simulator.c`,
`test_aux_alto_simulator.c`, `test_primalucelib.c`, `MIGRATION_STATUS.md`, README test record. Round 2 additionally:
`primalucelib.c`, `test_aux_alto_hw.c`, `indigo_test/Makefile`, `PROPERTIES.md`. Checked against AGENTS.md (formatting,
generated drivers, `_finalizer` semantics), indigo_drivers/AGENTS.md, indigo_test/AGENTS.md and the specification. In
both rounds the reviewer regenerated the driver in a scratch directory: outputs byte-identical to the checked-in files.

## Quality findings
Round 1 (all low or info, not blocking):
1. The stopped-position check slept 2.5 s against a 2 s poll period (false-failure risk).
2. The simulator stops the arm on `StoreAs...` by itself; suggested a comment on the assumption.
3. Info: `Motor_Stop` outside a calibration rests on the existing `calibrating` guard. Accepted.
4. Evidence for the document facts in `PROTOCOL.md` was missing from the task record.

Round 2 (not blocking):
5. Medium: `X_CALIBRATION_MOTION` accepted motion steps in phase 3 (after `StoreAsMaxOpenPos`, while the travel is
   awaited), with CANCEL blocked by BUSY.
6. Low: `cover_show_position()` published ALERT on a failed read without setting `poll_alert`, so the ALERT stuck
   until the cover moved.
7. Low: a failed `CAL_MAXPOS` read before the store left the previous travel 0, so the stale provisional value would
   count as new.
8. Low: the simulator calibrates at its move speed, which the new `PROTOCOL.md` note contradicts.
9. Info: the hot-plug case must stay last because the case count leaves it out; `STALE_TRAVEL_TIME` is a model choice.

Confirmed by the reviewer: every X_CALIBRATION branch sets its state and publishes once now that the generator drops
the OK prologue and final update; removing the trailing `return;` is correct; the finalizer, the poll timer and the
handlers share the device queue; disconnect cancels the pending finalizer; tests fail if the driver reverts; formatting
rules are met.

## Fix reinspection
1–4: fixed in round 1 (3 s waits, simulator comment, document excerpts in verification.md).
5. Fixed: phase 3 refuses motion steps with "Wait until the new travel is read"; `calibration_travel_not_reported`
   checks that no `CAL_ALTO` request is sent.
6. Fixed: `poll_alert` is set on a failed read.
7. Fixed: `PRIMALUCE_COVER_PROVISIONAL_TRAVEL` is never taken; the same case injects a garbage reply on the pre-read.
8. Fixed by documentation: the simulator speeds are commented as deliberate time compression with the measured values.
9. Comment added at the hot-plug case.

The first rerun exposed a race in the new test assertion (BUSY published before the handler's requests); the test
now waits for `StoreAsMaxOpenPos`. Retest: verification.md "Executed checks" (simulator 36/36 three times, hardware 9/9).

## Quality verdict
The diff stays scoped to the calibration and its hardware findings, with tests that fail on the earlier driver
versions. Remaining limitations: no second calibration on version 11 hardware; hot-plug run predates the round 2
refinements, which do not touch that path.
