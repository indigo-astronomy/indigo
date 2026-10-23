# Task brief

- Task ID: 20261023181340-736e-fix-audit-002
- Owner: Peter Polakovic
- Created: 2026-10-23

## Problem and desired outcome
Repair FND-20261023101753-bc8d-complete-repository-audit-002: both public network URL helpers copy an unbounded explicit-port hostname into a fixed stack buffer. Reject oversized hostnames before copying and validate hostname/port syntax while preserving supported protocol hints.

## Scope and non-goals
Assigned implementation: indigo_libs/indigo_io.c and indigo_libs/indigo_uni_io.c, relevant hardware-free unit regressions and task/finding evidence. No driver, generator, hardware, publication or primary-checkout changes.

## Constraints and references
Use isolated task worktree. Integration order is 039, 026, 006, 005, 002; coordinator supplies verified master SHA after 005. No integration before synchronization and final verification. Explicit audit deferral extension covering 002 is required before implementation.

## Initial questions
Await trusted owner authorization for audit deferral extension and coordination messages. Current delegated authorization was rejected by automatic approval review for messaging.
