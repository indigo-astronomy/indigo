# Task brief

- Task ID: 20261023211312-b017-primaluce-rotator-relative-move
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
The requester asked whether the ARCO has a relative move command, and whether the rotator can offer
a relative move property like the ZWO rotator. It has no such command, and the requester asked to
proceed: ROTATOR_RELATIVE_MOVE is computed from the current angle, as in the ZWO driver.

## Users and use cases
Clients that rotate the field by an offset, for example a framing or plate-solving agent.

## Scope and non-goals
In scope:
- ROTATOR_RELATIVE_MOVE;
- its end state, abort and range check;
- tests.

Non-goals:
- wrap-around across the range limits;
- the README text proposal.

## Constraints and references
- Root, driver and test AGENTS.md.
- `indigo_drivers/rotator_asi`.
- INDI `primalucacommandset.cpp`.

## Initial questions
Resolved: the requester asked to proceed (2026-10-24).
