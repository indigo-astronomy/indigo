# Task brief

- Task ID: 20261110112225-c7fc-solver-index-download
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-10

## Problem and desired outcome
The quad indexes used by the Platesolver Agent and by the `indigo_solver` command line tool can only be downloaded
through the agent's `X_AGENT_SOLVER_INDEX` property: the download code (WinINet on Windows, curl elsewhere, the
`.part` file, header validation and the atomic rename) lives inside `indigo_agent_platesolver.c`. A user with only
the command line tool has no way to install indexes.

Owner request (2026-11-10): "move it to the library first, then use it in indigo_platesolver as described", after the
recommendation to keep index management in the same program as `indigo_solver` modes (`--list`, `--download`,
`--remove`) on top of a shared download function in the solver library.

## Users and use cases
- Command line users installing, listing and removing indexes without running an INDIGO server.
- Platesolver Agent users: unchanged behaviour of `X_AGENT_SOLVER_INDEX`.

## Scope and non-goals
- In scope: shared index download/validate/abort API in `indigo_drivers/agent_platesolver/indigo_solver.[ch]`, the
  agent switched to it, `indigo_solver` list/download/remove modes, tests, `indigo_solver.vcxproj` link dependency.
- Non-goals: download progress reporting, resuming partial downloads, checksums, changes to the published index set.

## Constraints and references
- Root AGENTS.md, indigo_drivers/AGENTS.md, indigo_test/AGENTS.md; agent version bump for the behavioural refactor.
- No README change without approval; no new property.

## Initial questions
None blocking; option names chosen by the agent and presented with the commit plan.
