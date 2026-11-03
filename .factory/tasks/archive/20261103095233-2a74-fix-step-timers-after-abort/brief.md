# Task brief

- Task ID: 20261103095233-2a74-fix-step-timers-after-abort
- Owner: Rumen G. Bogdanovski (requester of this work)
- Created: 2026-11-03

## Problem and desired outcome
The owner asked to fix FND-20261103084208-ca1a-sequence-state-outside-sequence-001. Most step timers in Sequencer.js are not stored in `wait_for_timer`, so they survive `abort()` and `failure()` under the abort policy. A late timer set the step state back to Ok while `SEQUENCE_STATE` stayed Alert, and it advanced a sequence started in the meantime, running its steps too early.

## Users and use cases
Clients aborting a sequence, especially when starting another one right after it; the second sequence could start mount or dome steps too early.

## Scope and non-goals
In scope: `indigo_drivers/agent_scripting/library/Sequencer.js`, `indigo_test/integration/test_agent_scripting_sequencer.c`.
Non-goals: the C timer binding, the `flip_failed()` cancel race (recorded as a finding), the other open Sequencer findings.

## Constraints and references
Previous task 20261103084208-ca1a-sequence-state-outside-sequence. Root AGENTS.md: product and Factory artifacts in separate commits, owner approval before commits, no history rewrite, no README change. `DRIVER_VERSION` not bumped, as in the previous Sequencer.js-only commits; the owner did not ask for a bump when this was raised.

## Initial questions
None.
