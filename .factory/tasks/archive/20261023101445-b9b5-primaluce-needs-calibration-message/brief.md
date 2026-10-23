# Task brief

- Task ID: 20261023101445-b9b5-primaluce-needs-calibration-message
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-23

## Problem and desired outcome
When the requester's ARCO or focuser needs calibration, the driver says so at connect. A move then
just ends ALERT without saying why. The move has to fail with a message that gives the reason.

## Users and use cases
Owners of PrimaLuce focusers with an ARCO who connect while a motor is flagged for calibration.

## Scope and non-goals
In scope: the reason given for moves of flagged motors, the simulator profile, and regression tests.
Non-goals: refusing such moves up front (a SESTO SENSO 2 with the flag still moves), and the
relative rotator move (asked separately, not decided).

## Constraints and references
Root, driver and test AGENTS.md; the earlier primaluce tasks; the hardware log of the flagged ARCO.

## Initial questions
None. The recurring audit stays due; the requester asked for this fix directly.
