# Task brief

- Task ID: 20261024161128-3861-giotto-request-versus-poll
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
FND-20261024121920-3bd2-repository-audit-021 (repository audit 20261024121920-3bd2): the GIOTTO idle poll can replace a light or
brightness request that is queued but not yet handled. The requester asked to fix it, and the same flaw in the ALTO driver, in
separate commits (2026-10-24). This task fixes the GIOTTO and closes the finding.

## Users and use cases
Clients setting the light while the panel is also changed elsewhere (web page, WiFi client) or recovering from a failed poll.

## Scope and non-goals
In scope: the poll guard, the simulator delay, the regression test, documentation, the finding closure. Non-goals: the ALTO
(next task).

## Constraints and references
Root, driver and test AGENTS.md; the focuser driver's poll guard and its `request_versus_poll` test.

## Initial questions
None.
