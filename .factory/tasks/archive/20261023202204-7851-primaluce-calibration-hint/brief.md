# Task brief

- Task ID: 20261023202204-7851-primaluce-calibration-hint
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-23

## Problem and desired outcome
Item 2 of the protocol review: the calibration stores the current position as the minimum instead
of searching for the closed end, so the user has to move the draw tube fully in before START, and
nothing says so. The requester chose option (a): keep the flow and make the precondition visible.

## Users and use cases
Anyone calibrating a SESTO SENSO 2 or 3 through INDIGO.

## Scope and non-goals
In scope: the X_CALIBRATE label, a START message, and the regression test.
Non-goals: the closed-end search (option b), and README changes. The README needs the requester's
separate approval; a proposal is given with the commit plan.

## Constraints and references
Root, driver and test AGENTS.md; the protocol review in REFACTOR.md.

## Initial questions
Resolved: option (a), chosen by the requester on 2026-10-23.
