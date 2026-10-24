# Task brief

- Task ID: 20261024163549-1b30-alto-request-versus-poll
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
The flaw of FND-20261024121920-3bd2-repository-audit-021 (closed for the GIOTTO by task 20261024161128-3861) is present in
the ALTO driver, which the audit predates: its idle poll can undo a queued move request. The requester asked to fix both drivers
in separate commits (2026-10-24); this task fixes the ALTO.

## Users and use cases
Clients moving the cover while it is also moved elsewhere (web page, another client) or while a poll is in flight.

## Scope and non-goals
In scope: the poll guard, the simulator delay, the regression test, documentation. Non-goals: the GIOTTO (done).

## Constraints and references
Root, driver and test AGENTS.md; the focuser driver's poll guard and its `request_versus_poll` test.

## Initial questions
None.
