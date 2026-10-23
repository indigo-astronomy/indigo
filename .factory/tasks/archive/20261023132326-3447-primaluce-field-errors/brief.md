# Task brief

- Task ID: 20261023132326-3447-primaluce-field-errors
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-23

## Problem and desired outcome
The requester asked to check the driver against the SESTO SENSO 3 protocol document v1.0 and then
to fix item 1 of the review: refusals reported in a parameter's field are not recognised. The
review also covers the other findings: the calibration deviation, the missing features, and the
behaviour seen on the unit but not documented.

## Users and use cases
Owners of SESTO SENSO 2 and 3 focusers with an ARCO.

## Scope and non-goals
In scope: field-level refusals in the library, the reasons in the driver, a simulator fault, tests,
and the documentation of the review.
Non-goals:
- the closed-end search of the semi-automatic calibration (item 2, not yet decided by the requester);
- the manual procedure, the fine steps, the SC automatic calibration and `RECOVER_DELAY`;
- the relative rotator move.

## Constraints and references
- The protocol document v1.0 (the requester's copy, not committed).
- Root, driver and test AGENTS.md.
- INDI `primalucacommandset.cpp`, which requires "done" for every command.

## Initial questions
Resolved: the requester chose to merge the previous task first and then fix item 1 (2026-10-23).
The recurring audit is still reported as due.
