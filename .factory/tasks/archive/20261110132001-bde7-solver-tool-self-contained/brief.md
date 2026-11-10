# Task brief

- Task ID: 20261110132001-bde7-solver-tool-self-contained
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-10

## Problem and desired outcome
Owner request (2026-11-10): "make indigo_solver independent of indigo client. use the source files it needs and use
local escaping function for json and xml". The tool linked libindigo; it should depend on no INDIGO library.

## Users and use cases
Users of the `indigo_solver` command line tool, which can then be copied and run without the INDIGO libraries.

## Scope and non-goals
- In scope: `indigo_tools/Makefile`, `indigo_tools/indigo_solver.c`, `indigo_tools/indigo_solver.vcxproj`.
- Non-goals: library changes, other tools, the agent.

## Constraints and references
Follows the trial link in the previous conversation; the Windows client library repair is a separate, committed task.

## Initial questions
None.
