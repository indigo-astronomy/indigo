# Task brief

- Task ID: 20261023080442-7930-primaluce-arco-sestosenso3
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-23

## Problem and desired outcome
On the requester's SESTO SENSO 3 with an ARCO, an absolute rotator move turns the ARCO but
ROTATOR_POSITION goes ALERT at once. Desired outcome: ARCO moves, aborts and syncs work on the
SESTO SENSO 3, and nothing changes for the SESTO SENSO 2.

## Users and use cases
Owners of a SESTO SENSO 3 with an ARCO rotator using INDIGO.

## Scope and non-goals
In scope: the ARCO move, sync and stop acknowledgements, the angle completion check, reply
parsing, the simulator SESTO SENSO 3 ARCO replies, and regression tests.
Non-goals: ARCO calibration on the SESTO SENSO 3 (not exercised), other rotator features.

## Constraints and references
Root, driver and test AGENTS.md; tasks 011 and 012 (archived); hardware evidence from supervised
sessions with the requester's unit.

## Initial questions
Resolved:
- The recurring repository audit is overdue. The requester reported this defect on the same driver
  right after task 012 (2026-10-23), so the fix goes first and the audit follows.
- The angle tolerance is 0.1°, chosen by the requester (2026-10-23).
