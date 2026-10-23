# Task brief

- Task ID: 20261023091538-dfa4-primaluce-calibration-position-check
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-23

## Problem and desired outcome
After a calibration the requester's SESTO SENSO 3 once showed FOCUSER_POSITION as OK with target
13995 and value 100000. The logs show that the controller reports `ABS_POS_STEP` 1000000 with the
motor off (HIZ 1) when it seems to have lost its coordinate. The requester asked that the
calibration fail in this case with a message that it has to be run again.

## Users and use cases
Owners of a SESTO SENSO 3 who calibrate through INDIGO.

## Scope and non-goals
In scope: a position check at the end of the calibration, a simulator fault, and a regression test.
Non-goals: the handling of the 1000000 reading outside calibration (the requester chose this
narrower behaviour) and moves from that state.

## Constraints and references
Root, driver and test AGENTS.md; the previous primaluce tasks; session logs with the reading
(`ABS_POS_STEP` 1000000, HIZ 1).

## Initial questions
Resolved: the requester chose to fail the calibration (2026-10-23). The recurring audit stays due;
the requester asked for this follow-up directly.
