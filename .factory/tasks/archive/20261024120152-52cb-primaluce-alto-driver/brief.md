# Task brief

- Task ID: 20261024120152-52cb-primaluce-alto-driver
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
The follow-up of task 20261024092953-7ee4-primaluce-giotto-alto-drivers: an INDIGO driver for the PrimaLuce Lab ALTO cover
on the shared `primalucelib`, following the conventions of the existing dust cap drivers.

## Users and use cases
ALTO owners covering and uncovering the telescope, also from automated sequences.

## Scope and non-goals
In scope: `primalucelib` cover calls and unit tests, the ALTO simulator, driver, integration and opt-in hardware suites,
build and project registration, documentation.

Non-goals: hardware verification of the calibration (planned later with the requester), hold current and torque settings.

## Constraints and references
- Root, driver and test AGENTS.md; DRIVER_TESTING_RULES.md.
- No PrimaLuce document for the ALTO; INDI `alto.cpp`, `primalucacommandset.cpp`.
- The requester's ALTO1 (firmware 1.03.11), full access granted on 2026-10-24.

## Initial questions
Resolved 2026-10-24: full access to the ALTO; calibration as in INDI, recalibration on hardware later; README approved. The
recurring repository audit stays deferred under the requester's deferral for the GIOTTO and ALTO driver work.
