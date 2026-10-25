# Task brief

- Task ID: 20261025093558-ece5-server-leak-check
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-25

## Problem and desired outcome
The requester asked for a memory leak test of indigo_server and the drivers with simulators and a summary of the
result, and then to add tests for it (2026-10-25). The repository had no leak checking (sanitize builds use
`detect_leaks=0`; no Valgrind target).

## Users and use cases
Developers who want to know whether the server, the framework or a driver leaks during connect, operation and unload.

## Scope and non-goals
In scope: an opt-in Valgrind check of the `indigo_server` executable with the simulator drivers, its make target and
documentation, the leak report and findings. Non-goals: fixing the leaks (registered as findings), hardware drivers.

## Constraints and references
Root and `indigo_test/AGENTS.md`; `indigo_docs/DRIVER_TESTING_BASICS.md`.

## Initial questions
None.
