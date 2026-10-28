# Task brief

- Task ID: 20261027214032-3577-solver-index-0-5deg
- Owner: Peter Polakovic
- Created: 2026-10-27

## Problem and desired outcome
The Solver Agent offers Tycho-2 quad indexes of scale 1°, 2°, 5°, 10° and 20° (`tyc2-<scale>deg.isix`, quads of
0.1 to 0.8 times the scale), so fields narrower than about 0.7° have no fitting index. The owner asked on
2026-10-27 to create a 0.5° index and add it to the agent: build it with `indigo_solver_indexer`, make the agent
offer, download, select and use it, document it and validate it.

## Users and use cases
Users solving with long focal lengths (fields of about 0.3° to 0.7°, for example 1000 px at 1.5 to 2.5"/px)
through the native Solver Agent instead of astrometry.net.

## Scope and non-goals
In scope: a 0.5° tier in `indigo_tools/indigo_solver_indexer.c` (`select_params()`), the scale list of the
solver (`indigo_solver_index_fovs`, `INDIGO_SOLVER_INDEX_FOV_COUNT`), the agent's item naming for a fractional
scale (`X_AGENT_SOLVER_INDEX` and `AGENT_PLATESOLVER_USE_INDEX` item `TYC2_0_5`, since a dot in an item name is not
safe for every client), the published size table, PROPERTIES.md, README, REFACTOR.md, tests (unit index
selection and naming, agent property items, an opt-in self-consistency solve of a field rendered from the
built index), the index file itself built from the Tycho-2 catalogue in `/Users/polakovic/Development/Solver/I_259`
and copied into the persistent cache `~/.indigo/solver`.
Non-goals: a new index file format, hosting changes, other catalogues. Publishing the file as an asset of the
GitHub release `index_release_v1` is the owner's action: `gh` is not installed on this host and the agent needs no
code change for the download once the asset exists under the file name `tyc2-0.5deg.isix`.

## Constraints and references
Root `AGENTS.md`, `indigo_drivers/AGENTS.md`, `indigo_test/AGENTS.md`; the index format in
`indigo_drivers/agent_solver/indigo_solver_utils.h`; the indexer tiers (1° tier: quads 0.1° to 0.8°, nside 64, 8
stars per cell, 534 MB). The 0.5° tier is planned as quads 0.05° to 0.4°, nside 128 (four times the cells of the
1° tier) and 6 stars per cell so the file stays near the size of the 1° index; the actual size and star/quad
counts are recorded after the build and the parameters adjusted if the file is far larger. Tycho-2 has about 61
stars per square degree, a nside 128 cell is 0.21 square degrees, so about 13 stars are available per cell. The
CCD simulators render the Hipparcos catalogue only (to magnitude 9.3), which is too sparse for 0.5° fields, so the
validation renders fields from the built index's own stars. README changes need the owner's approval. The full
repository audit is due (`maintenance status`: 55 non-audit tasks since the last full review); the owner's
deferral for this task is requested.

## Requirement change (2026-10-27, evening)
The Tycho-2 builds showed 1 to 13 index stars per 0.5° x 0.4° field, so the owner redirected the task to a deeper
catalogue; after the astrometry.net comparison the owner approved Gaia DR3 (G < 13, about 7.4 million stars,
downloaded from the ESA archive TAP in declination bands). The index is `gaia-0.5deg.isix`, item `GAIA_0_5`.

## Initial questions
- Owner deferral of the due full repository audit for this task.
- Owner upload of `tyc2-0.5deg.isix` to the `index_release_v1` release after the build.
