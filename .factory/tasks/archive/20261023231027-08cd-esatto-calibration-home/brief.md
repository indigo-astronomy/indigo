# Task brief

- Task ID: 20261023231027-08cd-esatto-calibration-home
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
On the requester's ESATTO2 (application firmware 3.05.28) the focuser calibration fails at once, and an inward
move from 500 to 400 ended ALERT with no reason. The requester supplied the PrimaLuce Lab "Communication
Protocol" v4.0 (ESATTO, SESTOSENSO2 and ARCO models) and asked what must be handled differently on an ESATTO.
The outcome is that the driver handles the ESATTO as the document and the hardware log describe it.

## Users and use cases
ESATTO owners, with or without an ARCO.

## Scope and non-goals
In scope:
- hiding the focuser calibration on an ESATTO;
- an ESATTO-specific needs-calibration message;
- the reason for the home-sensor reset;
- correcting the "the ARCO has no relative command" wording;
- the simulator model and the regression tests.

Non-goals:
- switching to the relative `MOVE` command;
- the README text proposal.

## Constraints and references
- Root, driver and test AGENTS.md.
- `~/Downloads/PrimaluceLab - Communication Protocol - v4.0.pdf` (not in the repository).
- The requester's log `indigo_focuser.log` (not in the repository).
- INDI `esatto.cpp` and `primalucacommandset.cpp`.

## Initial questions
Resolved: the requester approved the proposal (2026-10-24).
