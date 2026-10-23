# Task brief

- Task ID: 20261023193406-58c7-primaluce-arco-keep-enabled
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-23

## Problem and desired outcome
The requester's ARCO needs calibration after almost every connect, which is not normal. The
requester asked to analyze the initial handshake and the connection close, fix the cause, and test
the fix on the connected unit.

## Users and use cases
Owners of an ARCO on a SESTO SENSO 3 who connect and disconnect the rotator.

## Scope and non-goals
In scope:
- the ARCO port switch at connect and at disconnect;
- the simulator model of the loss;
- tests;
- the hardware check.

Non-goals: the relative rotator move and item 2 of the protocol review.

## Constraints and references
- Root, driver and test AGENTS.md.
- INDI `esattoarco.cpp`, which only enables the ARCO.
- The session logs.

## Initial questions
Resolved:
- The requester approved the fix and the hardware test (2026-10-23).
- The requester approved one ARCO calibration while watching (2026-10-23).
