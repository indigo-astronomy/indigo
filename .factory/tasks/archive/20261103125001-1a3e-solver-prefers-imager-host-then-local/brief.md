# Task brief

- Task ID: 20261103125001-1a3e-solver-prefers-imager-host-then-local
- Owner: Rumen G. Bogdanovski (requester of this work)
- Created: 2026-11-03

## Problem and desired outcome
The owner asked that the Sequencer.js solver agent autoselection prefers the solver on the imager agent's host and chooses the local one when there is no imager agent, recalling that it preferred the local one. The imager host was in fact already preferred. Two differences remained: the host was taken from the imager agent's name even when that agent was not loaded, and without a solver on that host the first one found was used instead of the local one.

## Users and use cases
Setups with several INDIGO servers, each possibly with its own solver agent.

## Scope and non-goals
In scope: `indigo_drivers/agent_scripting/library/Sequencer.js`, `Sequencer.md`, `indigo_test/integration/test_agent_scripting_sequencer.c`.
Non-goals: searching other hosts for an imager agent when the one the sequence uses is not loaded, a `select_imager_agent()` step changing the solver agent (documented earlier), the web UI and Imager Agent selection.

## Constraints and references
Earlier tasks 20261102101805-593a-solver-agent-integration, 20261102123603-6a7f-select-solver-agent, 20261103001429-668f-solver-agent-at-use. Root AGENTS.md: product and Factory artifacts in separate commits, owner approval before commits, no history rewrite, no README change. `DRIVER_VERSION` not bumped, as in the previous Sequencer.js-only commits.

## Initial questions
"Local over other hosts" when the imager's host has no solver was not stated explicitly; chosen because it follows the owner's fallback, reported with the commit plan.
