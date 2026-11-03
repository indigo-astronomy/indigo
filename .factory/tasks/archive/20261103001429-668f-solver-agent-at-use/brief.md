# Task brief

- Task ID: 20261103001429-668f-solver-agent-at-use
- Owner: Rumen G. Bogdanovski (requester of this work)
- Created: 2026-11-03

## Problem and desired outcome
Reported by the owner: a sequence with the single step "enable meridian flip" finishes at once, but the flip stays enabled (Imager Agent `PAUSE_AFTER_TRANSIT`). When the solver agent is changed after the sequence finished (Astrometry Agent and Platesolver Agent are mutually exclusive, so changing means unloading one and loading the other), at transit the mount flips, the imager stays paused and the flip never finishes. Sequencer.js selects the solver agent once at `start()`, so the flipper sends `CENTER` to the agent that is no longer loaded and waits forever. The owner then asked for the same for the other solver usage in Sequencer.js.

Desired outcome: the meridian flip, the solver steps (`sync_center()`, `precise_goto()`, `set_solver_filter()`) and the abort use the solver agent loaded when they run; with no solver agent loaded the flip fails instead of waiting forever; solver failures keep using the recovery strategy.

## Users and use cases
Users of Sequencer.js sequences (Ain Imager, web UI, scripts) who switch between the Platesolver Agent and the Astrometry Agent, or load a solver agent during or after a sequence, and use meridian flip with solver recentering, sync & center or precise goto.

## Scope and non-goals
In scope: `indigo_drivers/agent_scripting/library/Sequencer.js`, `Sequencer.md`, `indigo_test/integration/test_agent_scripting_sequencer.c`; closing FND-20261102123603-6a7f-select-solver-agent-001 (no test asserted that abort reaches the solver), because this task changes the abort path.
Non-goals: solver agents, Imager/Mount/Guider agents, the web UI, re-selecting the imager/mount/guider agents, retries of a failed flip.

## Constraints and references
Solver agents are mutually exclusive on one server (`indigo_agent_platesolver.c`, `indigo_agent_astrometry.c` INDIGO_DRIVER_INIT). Earlier solver selection work: archived tasks 20261102101805-593a-solver-agent-integration and 20261102123603-6a7f-select-solver-agent. Root AGENTS.md rules: no README change, owner approval before commits, no history rewrite. Sequencer.js has no driver version; earlier Sequencer.js changes bumped none.

Process deviation: the implementation was made on master in the working tree before this task existed; the task branch was created from master carrying the uncommitted change (nothing was committed). The owner asked for the Factory record on 2026-11-03.

## Initial questions
Resolved with the owner: solver failures in `sync_center`/`precise_goto`/`set_solver_filter` must go to the next recovery point under `recover_on_failure()` (confirmed by a test). Factory scripts need Git 2.44 (`--no-lazy-fetch`); the owner approved a scratch wrapper that drops this flag on this host (Git 2.43, not a partial clone).
