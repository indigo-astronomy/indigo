# Task brief

- Task ID: 20261103112500-6690-fix-flip-failed-timer-race
- Owner: Rumen G. Bogdanovski (requester of this work)
- Created: 2026-11-03

## Problem and desired outcome
The owner asked to fix FND-20261103095233-2a74-fix-step-timers-after-abort-001. Under the continue and recover policies `flip_failed()` cancels the stored timer; when that timer is already firing the cancel throws and the failure policy is not applied. The failure must be applied exactly once.

## Users and use cases
Unattended sessions with a meridian flip failing just as a `wait()` or altitude step ends.

## Scope and non-goals
In scope: `indigo_drivers/agent_scripting/library/Sequencer.js`, `indigo_test/integration/test_agent_scripting_sequencer.c`.
Non-goals: the C timer binding, unchecked results of `indigo_set_timer`, the other open Sequencer findings.

## Constraints and references
Previous task 20261103095233-2a74-fix-step-timers-after-abort. Root AGENTS.md: product and Factory artifacts in separate commits, owner approval before commits, no history rewrite, no README change. `DRIVER_VERSION` not bumped, as in the previous Sequencer.js-only commits.

## Initial questions
None.
