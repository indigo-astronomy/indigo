# Task brief

- Task ID: 20261022235021-bcb6-primaluce-sestosenso3
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-23

## Problem and desired outcome
On a SESTO SENSO 3 the requester saw `FOCUSER_POSITION` alternate between OK and ALERT and moves
fail. The log shows every reply answering the previous request (the serial link slipped by one
reply), and once that happens the driver never recovers. Desired outcome: `focuser_primaluce`
works with SESTO SENSO 2 and 3, following the requester's protocol document
"PrimaluceLab - Communication Protocol - SS3 v1.0" (06 Oct 2025, not committed), and the simulator
models the SESTO SENSO 3.

## Users and use cases
Owners of SESTO SENSO 2 and 3 focusers using INDIGO over USB or WiFi.

## Scope and non-goals
In scope: resynchronization of the request/reply link (library), SESTO SENSO 3 behaviour in the
driver (uncalibrated controller, semi-automatic calibration sequence), simulator SESTO SENSO 3
model and regression tests, hardware validation on the requester's unit.
Non-goals: the SESTO SENSO 3 SC automatic calibration and `RECOVER_DELAY` (new properties; follow-up).

## Constraints and references
Root, driver and test AGENTS.md; task 010 (primalucelib); the protocol document above.
Hardware: SESTO SENSO 3 (sub-model standard), application firmware 1.0.9, web 03.06.01, CP2102N on
`/dev/ttyUSB0`, uncalibrated (`CAL_MINPOS = CAL_MAXPOS = 0`, `CALRESTART.MOT1 = 1`,
`MOT1.ERROR = "CalRestart; CalRestart_WrongMaxPos; "`). The requester approved small moves and a
supervised calibration (2026-10-23).

## Initial questions
Resolved: the requester's instruction to proceed with this work defers the overdue full repository
audit until after this task (2026-10-23).
