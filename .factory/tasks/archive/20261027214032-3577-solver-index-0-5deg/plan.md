# Development plan

Status: accepted

## Approach and affected components
A new indexer tier and a Gaia DR3 CSV input produce the 0.5° index (`gaia-0.5deg.isix`); the solver's scale list
grows to six and names the catalogue per scale; the agent encodes `<CATALOG>_<scale>` item names and knows the
file size; documentation and tests follow. The Gaia extract (G < 13) is downloaded from the ESA archive TAP in
5° declination bands (anonymous jobs abort above about a minute, so wide bands are split), the index is built
locally, verified by the agent and by the self-consistency solve over twelve fixed fields, copied into the
persistent cache and uploaded to the release with the owner's `gh`.

## Baseline
- Commands and environment: Darwin arm64; `make -C indigo_tools all`, `make -C indigo_drivers/agent_solver -f ../../Makefile.drv all`, unit/agent/simulator suites, `python3 tools/run_driver_test.py agent_solver`
- Existing failures: none; recorded run of 3.0.0.8 is 149/149 (README `## Testing`)
- Repository revision / working-tree state: branch `task/20261027214032-3577-solver-index-0-5deg` from master `1e6a6e01f`

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | `indigo_tools/indigo_solver_indexer.c`: `fov <= 0.5` tier (nside 128; quad range and stars per cell from the measured builds) and the `-g` Gaia CSV reader with proper motion from epoch 2016.0 | AC-001 | none | tool builds, `indigo_solver_indexer -g gaia 0.5` runs | done |
| 2 | `indigo_solver.h/.c`, `indigo_solver_utils.c/.h`: `INDIGO_SOLVER_INDEX_FOV_COUNT` 6, scale list { 0.5, 1, 2, 5, 10, 20 }, `indigo_solver_index_catalog()` and the catalogue prefix in `indigo_solver_index_path()`; `indigo_agent_solver.c`: `GAIA_0_5`/`TYC2_n` naming both ways, catalogue in labels and messages, size table entry, version 3.0.0.9 | AC-002, AC-004 | none | driver builds, unit and agent suites pass | done |
| 3 | Tycho-2 variants measured (evidence only); Gaia DR3 extract downloaded in 5° bands; build the index, record size/counts, keep it within about 1 GB, copy to `~/.indigo/solver/gaia-0.5deg.isix` | AC-001 | 1 | header validated by the agent, file in the cache | running |
| 4 | Tests: unit `index_selection` (0.5 scale, file name), agent `index_property_reflects_installed_files` and `deselecting_index_removes_file` (`GAIA_0_5`), simulator `index 0.5 self consistency` (twelve fixed fields rendered from the index's stars, solved with the 0.5° index alone; `INDIGO_SOLVER_NARROW_FIELDS` for the one-off long run) | AC-002, AC-003, AC-004 | 3 | all pass | pending |
| 5 | Docs: README table/command/limitation (owner approval), PROPERTIES.md, REFACTOR.md, MIGRATION_STATUS.md; recorded run; `make -C indigo_test test-unit` | AC-005, AC-006 | 4 | records updated | pending |
| 6 | One-off long verification (hundreds of random fields, evidence only), release asset upload with `gh`, reviews, process.json, archive, single commit, integration on instruction | all | 5 | process check passes | pending |

## Test strategy
Unit for selection and names; agent integration for the property items with a synthetic file under the 0.5° name;
the opt-in simulator suite for the real index (self-consistency, since the CCD simulators cannot render dense
enough fields); the recorded run as the gate.

## Security risks and mitigations
None new; the index file is local data outside the repository.

## Dependencies and architecture decisions
Item names `<CATALOG>_<scale>` (`GAIA_0_5`): a dot in an INDIGO item name is avoided and the catalogue is visible; file names `gaia-0.5deg.isix` / `tyc2-<scale>deg.isix`. Gaia DR3 (ESA/Gaia/DPAC, CC BY-SA 3.0 IGO) is the source below 1° because Tycho-2 (complete to V 11.5) gives 1 to 13 stars per 0.5° field; astrometry.net makes the same split (Tycho-2 above 1°, Gaia below).

## Migration, rollout, and rollback
Users get the new item once the asset is on the release; without the asset the download fails with the existing
message. Rollback is the previous driver.

## Deviations and owner decisions
`gh` downloaded from the official cli/cli release (no Homebrew, owner instruction), placed in /usr/local/bin by the
owner with sudo; the asset upload is the owner's step.
