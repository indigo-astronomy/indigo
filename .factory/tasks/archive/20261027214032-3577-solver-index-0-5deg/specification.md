# Specification

Status: accepted (owner instruction in conversation, 2026-10-27: "vytvor a pridaj aj index pre 0.5°"; changed the same
evening: the 0.5° index is built from Gaia DR3 instead of Tycho-2, see Owner decisions)

## Problem and outcome
The Solver Agent gets a sixth Tycho-2 quad index of scale 0.5° for fields narrower than the 1° index serves,
built by the project's indexer, offered and downloaded by the agent like the others, documented and validated.

## In scope
- Indexer: a 0.5° tier in `select_params()` (HEALPix nside 128, quad and star-per-cell parameters chosen from the
  measured builds) and a Gaia DR3 CSV input (`-g <directory>` with the TAP extract columns ra, dec,
  phot_g_mean_mag, pmra, pmdec, proper motion from epoch 2016.0); `indigo_solver_indexer -g gaia 0.5` writes
  `gaia-0.5deg.isix`. Scales below 1° are Gaia, the others stay Tycho-2 (`indigo_solver_index_catalog()`).
- Solver: `indigo_solver_index_fovs` = { 0.5, 1, 2, 5, 10, 20 }, `INDIGO_SOLVER_INDEX_FOV_COUNT` 6; index
  selection unchanged in rule (largest scale at most 1.5 x the field size, smallest available otherwise).
- Agent: item names of `X_AGENT_SOLVER_INDEX` and `AGENT_PLATESOLVER_USE_INDEX` are `<CATALOG>_<scale>` with an
  underscore for a decimal point (`GAIA_0_5`, `TYC2_1` …), parsed back to the scale; labels and messages name the
  catalogue; the size table gets the measured size of the built file; download from `index_release_v1` under the
  file name `gaia-0.5deg.isix`.
- The index file built from the Tycho-2 catalogue, validated by the agent's header check and by solving, copied
  to the persistent cache `~/.indigo/solver`, handed to the owner for the release upload (not part of the
  repository).
- Documentation: README (index table, indexer command, limitation line), PROPERTIES.md item list, REFACTOR.md;
  driver version bump.
- Tests: unit (index selection with the 0.5° scale, item naming round trip through the agent's public behaviour
  where observable), agent integration (`TYC2_0_5` item offered when the file exists), opt-in simulator suite: a
  self-consistency case that renders a 0.5° x 0.4° field from the built index's own stars at a random position
  with noise and solves it through the agent with the 0.5° index selected alone.

## Out of scope
Hosting, the release upload itself (owner, `gh` authenticated by the owner), a new index format, other
catalogues, Linux/Windows runs, changes to the other tiers.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `indigo_solver_indexer 0.5` builds `tyc2-0.5deg.isix` with header scale 0.5, star and quad counts recorded; the file size is near the 1° index (within about 2 x of 534 MB) or the parameters are adjusted and the decision recorded | build log, `ls -l`, header read by the agent's validation |
| AC-002 | With the file in the cache the agent offers `TYC2_0_5` in `X_AGENT_SOLVER_INDEX` (selected) and in `AGENT_PLATESOLVER_USE_INDEX`; clearing it deletes the file and withdraws the item | agent test `index_property_reflects_installed_files` extended (synthetic 0.5° file), simulator setup through `install_index("TYC2_0_5")` |
| AC-003 | A field of about 0.5° rendered from the index's stars solves with the 0.5° index alone: centre within 10", scale within 1 %, angle within 0.2°, parity as rendered | simulator case `index 0.5 self consistency` |
| AC-004 | The index selection picks 0.5° for a field of about 0.3° x 0.25° and 1° for 1° x 0.75° when both exist | unit `index_selection` extended |
| AC-005 | Existing suites pass: unit, agent, simulator (recorded run), `make -C indigo_test test-unit` | `tools/run_driver_test.py agent_solver`, `make -C indigo_test test-unit` |
| AC-006 | Docs updated: README table/command/limitation (owner approval), PROPERTIES.md item list, REFACTOR.md, MIGRATION_STATUS.md, version | diff review |

## Failure behavior and edge cases
- Without the file the item is offered unselected and selecting it downloads from the release; until the owner
  uploads the asset the download fails with the agent's existing message, which the test does not exercise.
- A field larger than the 0.5° index serves falls back to the other selected indexes as before.

## Permissions, data, and security requirements
The catalogue is read from the owner's local copy; the index is written to a scratch directory and the cache.
No repository binary. The release upload is an owner action with the owner's GitHub credentials.

## Compatibility and nonfunctional requirements
Existing item names stay (`TYC2_1` … `TYC2_20`); the new name only applies to the fractional scale. The index
load time and memory of the 0.5° index are comparable to the 1° index.

## Assumptions and open questions
- The Hipparcos based CCD simulators cannot render dense enough fields; validation therefore uses the index's
  own stars (documented limitation: not independent of the index). Complexity: medium; risk: low to medium.

## Owner decisions and requirement changes
- 2026-10-27, Peter Polakovic: after the Tycho-2 variants (1 to 13 Tycho-2 stars per 0.5° x 0.4° field, at most 6 of
  12 test fields solvable) the owner decided "s Tycho-2 to nema zmysel, skus najst hlbsi katalog" and, after the
  review of the astrometry.net catalogues (Tycho-2 above 1°, Tycho-2 + Gaia DR2 below 1°, 2MASS not recommended),
  approved the Gaia DR3 extract G < 13 ("Áno, stiahni Gaia DR3 rez G < 13 a postav z neho 0.5° index"). The index
  file is `gaia-0.5deg.isix`, the item `GAIA_0_5`; AC-001 to AC-003 apply to the Gaia build, the Tycho-2 0.5°
  variants are evidence only. The owner also allowed a one-off many-hour verification run after the build while the
  standard suite stays at the 120 combinations.
- 2026-10-28, coordinator (owner asleep, "pokracuj samostatne"): the G < 13 extract gives 7 to 19 index stars per
  0.5° x 0.4° field with 16 stars per cell and the fields with 7 to 10 stars still fail (1 to 3 catalogue quads
  among them), so the extract is deepened to G < 14 (about 20 million stars in total, the same ESA TAP download in
  2.5° bands) and the tier uses a higher star cap per cell with few quads per star; reported to the owner for
  confirmation, the download volume (about 1.5 GB) is the deviation from the approved "G < 13".
- 2026-10-28, coordinator: final tier nside 128, quads 0.05° to 0.3°, 20 stars per cell, anchored quads (10 nearest
  neighbours, 10 quads per anchor, duplicates removed): 3.84 M stars, 31.3 M quads, 907 MB (within the owner's
  0.5 to 1 GB), 12 of 12 fixed fields and 497 of 500 random fields solve; the cell-owned quad enumeration of the
  Tycho-2 tiers is unchanged.
- 2026-10-27, Peter Polakovic: audit deferral recorded in `.factory/maintenance.json` (`solver_index_0_5_deferral`);
  `gh` downloaded from the official release without Homebrew, placed in /usr/local/bin by the owner.
