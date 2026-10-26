# Task brief

- Task ID: 20261026185516-13cf-astrometry-getline-leak
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-26

## Problem and desired outcome
Fix FND-20261026155455-61b6-platesolver-solve-filter-001: `execute_command()` in the astrometry agent leaks the buffer `getline()` allocates for the call that reports
the end of the solver output (120 bytes per image2xy/solve-field command).

## Users and use cases
Long-running servers that solve many frames.

## Scope and non-goals
In scope: `indigo_drivers/agent_astrometry/indigo_agent_astrometry.c` (fix, version), its README test record,
REFACTOR.md, TEST_SUMMARY.md. Non-goals: anything else.

## Constraints and references
FND-20261026155455-61b6-platesolver-solve-filter-001; root AGENTS.md.

## Initial questions
None.
