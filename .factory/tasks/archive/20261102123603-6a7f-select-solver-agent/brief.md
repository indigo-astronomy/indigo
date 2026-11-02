# Task brief

- Task ID: 20261102123603-6a7f-select-solver-agent
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-11-02

## Problem and desired outcome
`Sequence.prototype.start()` accepts an optional fourth `solver_agent` argument, unlike the select steps used for the other agents, and its automatic choice runs only once, so a solver loaded later by the sequence is missed. Owner decision: remove the argument, keep automatic selection in `start()` because either Astrometry or Platesolver may be loaded, and add a `select_solver_agent(agent)` sequence step behaving like the other `select_*_agent` steps.

## Users and use cases
Sequence authors and clients that embed Sequencer.js, including AIN support for changeable solver agents, whose implementing agent raised the flipper question.

## Scope and non-goals
In scope: `indigo_drivers/agent_scripting/library/Sequencer.js`, `Sequencer.md`, `indigo_test/integration/test_agent_scripting_sequencer.c`.
Non-goals: choosing the solver on each use; web UI; client changes; the placeholder device lists in `indigo_flipper`/`indigo_sequencer`.

## Constraints and references
Clients embed their own Sequencer.js copy, and indigo_server serves one at `/Sequencer.js`, so behaviour reaches clients only when their copy is updated. Follows the separate Factory commit rule in AGENTS.md.

## Initial questions
None; the owner specified the behaviour.
