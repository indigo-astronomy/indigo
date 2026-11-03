# Task brief

- Task ID: 20261103010401-8b51-flip-failure-recovery
- Owner: Rumen G. Bogdanovski (requester of this work)
- Created: 2026-11-03

## Problem and desired outcome
Reported by the owner: when the solving after a meridian flip fails, the sequence does not use its recovery points but fails directly; it should follow them. Every flipper failure branch in Sequencer.js (slew, no solver agent, sync & center, guiding) calls `indigo_sequencer.abort()` regardless of `continue_on_failure()`/`recover_on_failure()`. Desired outcome: a failed flip follows the sequence failure policy, the HA limit disabled by the flip is restored on failure, and the abort behaviour stays for `abort_on_failure()` and when no sequence runs.

## Users and use cases
Unattended Sequencer.js sessions with meridian flip and recentering that protect imaging blocks with `recover_on_failure()`/`recovery_point()` (for example a recovery point before a sync & center and capture block) or use `continue_on_failure()`.

## Scope and non-goals
In scope: `indigo_drivers/agent_scripting/library/Sequencer.js`, `Sequencer.md`, `indigo_test/integration/test_agent_scripting_sequencer.c`.
Non-goals: retrying a failed flip, Imager/Mount/Guider/solver agent changes, FND-20261103001429-668f-solver-agent-at-use-003 (waits that never end when the solver agent is unloaded).

## Constraints and references
Previous task 20261103001429-668f-solver-agent-at-use (solver agent selection at use, commits e2c72fb23 and d39211bd4). Root AGENTS.md: product and Factory artifacts in separate commits, owner approval before commits, no history rewrite, no README change. Sequencer.js has no driver version.

## Initial questions
Applied to all flip failure causes, not only the solving failure, so that the flip behaves consistently; reported to the owner with the commit plan.
