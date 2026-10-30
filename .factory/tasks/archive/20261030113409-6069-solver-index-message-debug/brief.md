# Task brief

- Task ID: 20261030113409-6069-solver-index-message-debug
- Owner: Rumen G. Bogdanovski
- Created: 2026-10-30

## Problem and desired outcome
During a solve the solver agent sends a client message "Solving with index <label>..." for every index it tries
(every index once per stage). The messages fill the client log. They should no longer reach clients; the
information stays available in the driver debug log.

## Users and use cases
Clients of indigo_agent_solver (AIN, scripts) whose message log fills with per-index lines during every solve;
developers who still need the search order when debugging.

## Scope and non-goals
In scope: the per-index message of `indigo_agent_solver.c`, the driver version, and the agent integration tests
that observe the search order. Non-goals: the `indigo_solver` command line tool keeps printing the line to stdout
(owner decision); no change of the search algorithm or of other agent messages.

## Constraints and references
Owner instructions 2026-10-30: demote to the debug log; keep the tool output; keep the agent search-order
assertions by capturing the debug log in the test. Root AGENTS.md version rule (bump DRIVER_VERSION).

## Initial questions
None open; resolved by the owner decisions above.
