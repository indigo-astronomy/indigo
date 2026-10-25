# Task brief

- Task ID: 20261025152706-abbf-fix-server-leaks
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-25

## Problem and desired outcome
The leak check of task 20261025093558-ece5-server-leak-check found three definite leaks
(FND-20261025093558-ece5-server-leak-check-001 to 003). The requester asked to fix them (2026-10-25).

## Users and use cases
Long-running servers: every CCD driver leaked per camera attach/detach (hot-plug), the guider and astrometry agents per
load/unload.

## Scope and non-goals
In scope: the three releases, versions of the two agents, the leak check rerun. Non-goals: the possibly lost libusb
contexts and thread-local storage; the pre-existing test failures found on the way (registered).

## Constraints and references
Root, driver and test AGENTS.md.

## Initial questions
None.
