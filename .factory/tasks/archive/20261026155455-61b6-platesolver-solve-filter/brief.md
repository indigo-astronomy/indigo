# Task brief

- Task ID: 20261026155455-61b6-platesolver-solve-filter
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-26

## Problem and desired outcome
The requester asked that the solver agents can solve frames taken through a specific filter: the agent exposes the
filters of the related agent plus "current filter" (the default); each solve changes the filter, takes the exposure
and, while solving, returns to the previously selected filter; on failure it returns to the previous filter.

## Users and use cases
Users who plate-solve through a narrowband or colour filter that is not the one in the light path, e.g. solving with
the luminance filter while the sequence uses Ha, or the reverse.

## Scope and non-goals
In scope: the shared plate-solver library (`indigo_libs/indigo_platesolver.c`, header, names), the two agents built on
it (`agent_astrometry`, `agent_solver`; version bumps), PROPERTIES.md, tests and the astrometry REFACTOR notes.
Non-goals: the Imager Agent, the guider fallback, the web UI (`astrometry.html`), the obsolete `agent_astap`.

## Constraints and references
Root AGENTS.md (properties reference, versions, commits); `indigo_test/AGENTS.md`; the Imager Agent's
`AGENT_WHEEL_FILTER` (`indigo_drivers/agent_imager/indigo_agent_imager.c`, `filter_handler`).

## Initial questions
Multi-exposure processes (polar alignment): the requester chose to switch the filter per exposure.
