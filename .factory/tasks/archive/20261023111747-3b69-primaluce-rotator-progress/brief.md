# Task brief

- Task ID: 20261023111747-3b69-primaluce-rotator-progress
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-23

## Problem and desired outcome
During an ARCO move the published angle stays at the start value until the move ends. It has to
follow the rotation.

## Users and use cases
Clients that show or monitor the rotator angle.

## Scope and non-goals
In scope: progress publication in the rotator movement finalizer, and a regression test.
Also in scope at the requester's request: the first BUSY update showed the request as the value (focuser and rotator).
Non-goals: the relative rotator move.

## Constraints and references
Root, driver and test AGENTS.md; the focuser movement finalizer as the model.

## Initial questions
None. The recurring audit stays due; the requester asked for this fix directly.
