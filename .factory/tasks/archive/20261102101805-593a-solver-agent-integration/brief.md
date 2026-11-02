# Task brief

- Task ID: 20261102101805-593a-solver-agent-integration
- Owner: Rumen G. Bogdanovski (requester of this work)
- Created: 2026-11-02

## Problem and desired outcome
The Platesolver Agent (`indigo_agent_platesolver`, device "Platesolver Agent") already acts as a solver for the Imager, Guider and Mount agents, and the Imager and Mount agents accept it as a related agent. Other components that drive a solver still address only "Astrometry Agent" (or "ASTAP Agent"): the Imager Agent does not disable automatic solving on it before its own processes, Sequencer.js sends sync/center, precise goto, solver filter, meridian flip recentering and abort to a fixed "Astrometry Agent", and the web UI (astrometry, imager and mount pages) works only with "Astrometry Agent". Desired outcome: these components use whichever solver agent is loaded, including remote "<name> @ <host>" agents, without changing behaviour for existing Astrometry Agent users.

## Users and use cases
Users running the Platesolver Agent instead of the Astrometry Agent with the Imager Agent, Sequencer.js sequences (sync & center, precise goto, meridian flip with solver) and the built-in web UI; users with a solver agent on a remote INDIGO server.

## Scope and non-goals
In scope: `indigo_drivers/agent_imager/indigo_agent_imager.c`, `indigo_drivers/agent_scripting/library/Sequencer.js` and `Sequencer.md`, `indigo_server/resource/astrometry.html`, `imager.html`, `mount.html`, and their tests `indigo_test/integration/test_agent_imager.c`, `indigo_test/integration/test_agent_scripting_sequencer.c`.
Non-goals: solver library or Platesolver Agent changes, Mount/Guider Agent changes (they need none), loading the Platesolver Agent by default from the web UI, remote "@ host" solvers in the web UI (pre-existing limit, offered as follow-up), the indigo_server Makefile resource dependency (pre-existing, offered as follow-up).

## Constraints and references
Solver agents are mutually exclusive on one server (`indigo_agent_platesolver.c` INDIGO_DRIVER_INIT, `indigo_agent_astrometry.c` INDIGO_DRIVER_INIT). Root AGENTS.md rules: driver version bump for behaviour changes, no README change without approval, ask before commits (owner Rumen G. Bogdanovski), no history rewrite.

Process deviation: the implementation was committed directly to master as 07c373f20, f7f2b3257, ac3fdb9fb and 47f3d63a9 (owner-approved commits) before this Factory task existed. This task records the work retroactively over the range f3bb131e4..47f3d63a9; history is not rewritten. See handoff.md.

## Initial questions
Resolved with the owner during the work: solver agents are mutually exclusive (no priority between them needed); Sequencer and Imager Agent first, then the web UI; remote "@ host" solvers must be found by Sequencer.js.
