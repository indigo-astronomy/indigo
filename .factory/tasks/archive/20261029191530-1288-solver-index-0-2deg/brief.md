# Task brief

- Task ID: 20261029191530-1288-solver-index-0-2deg
- Owner: Peter Polakovic
- Created: 2026-10-29

## Problem and desired outcome
Owner request (2026-10-29): "potrebujeme index pre agent_solver s rozlisenim 0.2deg z rovnakeho katalogu ako 0.5deg".
The Solver Agent and `indigo_solver` get a seventh quad index of scale 0.2° built from the same Gaia DR3 extract
(G < 14) as the 0.5° index, for fields narrower than the 0.5° index serves.

## Users and use cases
Long focal length imaging (fields of about 0.1° to 0.45°), for example the owner's test frame from a 2 m RCC with a
2048 x 2048 Andor camera (0.176"/px, 0.1° field).

## Scope and non-goals
In scope: indexer tier, solver scale list and labels, agent item and size, tests, documentation, the built file.
Not in scope: a deeper catalogue (owner asked for the same one), changes of the other tiers, the release upload
(owner step).

## Constraints and references
Archived task `.factory/tasks/archive/20261027214032-3577-solver-index-0-5deg/` (Gaia extract, anchored quads).
Audit deferral `solver_index_0_2_deferral` in `.factory/maintenance.json`.

## Initial questions
None blocking; the file size and the sky coverage limit of G < 14 at 0.2° are reported to the owner.
