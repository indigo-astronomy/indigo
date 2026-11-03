# Task brief

- Task ID: 20261103084208-ca1a-sequence-state-outside-sequence
- Owner: Rumen G. Bogdanovski (requester of this work)
- Created: 2026-11-03

## Problem and desired outcome
The owner made an uncommitted change in `indigo_sequencer.abort()` of Sequencer.js: a failed meridian flip also reaches `abort()` after the sequence that enabled the flip has ended, and it turned `SEQUENCE_STATE` and the last step to Alert although no sequence was running. With the change the state is kept outside a sequence and set to Alert only when the failure happens inside the sequence. The owner asked for tests and a Factory record of this change.

## Users and use cases
GUI clients showing the result of a finished sequence while the meridian flip it enabled stays active.

## Scope and non-goals
In scope: tests in `indigo_test/integration/test_agent_scripting_sequencer.c`, the matching sentence of `Sequencer.md`, this record. The Sequencer.js code change is the owner's; this task corrects only two comments there.
Non-goals: other flip failure handling, FND-20261103010401-8b51-flip-failure-recovery-004, FND-20261103001429-668f-solver-agent-at-use-003.

## Constraints and references
Previous task 20261103010401-8b51-flip-failure-recovery. Root AGENTS.md: product and Factory artifacts in separate commits, owner approval before commits, no history rewrite, no README change. Sequencer.js has no driver version.

## Initial questions
None. The Sequencer.md sentence was not requested explicitly; it is included because it described the old behaviour.
