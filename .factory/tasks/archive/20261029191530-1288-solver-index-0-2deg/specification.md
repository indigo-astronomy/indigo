# Specification

Status: accepted (owner instruction in conversation, 2026-10-29)

## Problem and outcome
A 0.2° Gaia DR3 quad index for the Solver Agent and `indigo_solver`, built from the G < 14 extract of the 0.5° index.

## In scope
- Indexer: a 0.2° tier in `select_params()`: the 0.5° tier scaled down (HEALPix nside 256, quads 0.02° to 0.12°,
  anchored quads); `indigo_solver_indexer -g <directory> 0.2` writes `index-00.2deg-gaia.isix`.
- Solver: `indigo_solver_index_fovs` = { 0.2, 0.5, 1, 2, 5, 10, 20 }, count 7, fields "0.2° - 0.45°" for 0.2 and
  "0.45° - 1°" for 0.5 (boundary of the nearest-scale rule at 0.7 x the field).
- Agent: item `INDEX_00_2DEG_GAIA` in `X_AGENT_SOLVER_INDEX` / `AGENT_PLATESOLVER_USE_INDEX`, size table entry,
  download from `index_release_v2` under the file name; version bump.
- `indigo_tools/Makefile`: the macOS link of `indigo_solver_indexer` and `indigo_solver` needs libjpeg and libraw
  since the solver core reads JPEG and camera raw (broken on master since a1e7f53d0, needed to build the index).
- Tests and documentation (README with owner approval, PROPERTIES.md, SOLVER_INDEX_FORMAT.md).

## Out of scope
Deeper catalogue, other tiers, release upload, Linux/Windows runs.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The indexer builds `index-00.2deg-gaia.isix` with header scale 0.2 from the G < 14 extract | build log, header load |
| AC-002 | The agent offers `INDEX_00_2DEG_GAIA` (download item, use item when installed) | agent test, simulator `install_index` |
| AC-003 | Rendered 0.2° x 0.16° fields with at least 8 index stars solve with the 0.2° index alone (fixed 12 fields); the random-field rate is recorded | simulator `index 0.2 self consistency`, one-off 100-field runs |
| AC-004 | Index selection: 0.2 for 0.2 x 0.16° and 0.4 x 0.3°, 0.5 for 0.6 x 0.45° | unit `index_selection` |
| AC-005 | A real narrow frame solves with the 0.2° index and the solution is confirmed independently | owner's frame `Focus_81.15mm.fit`, cross-match with Gaia DR3 |
| AC-006 | Existing suites pass; recorded run | `tools/run_driver_test.py agent_solver`, `make -C indigo_test test-unit` |
| AC-007 | Docs and version updated | diff review |

## Failure behavior and edge cases
Until the owner uploads the asset, selecting the item fails with the existing download message. Fields with fewer
than about 8 stars of G < 14 (about half of random 0.2° fields, high galactic latitude) are not solvable with this
catalogue; that is a catalogue limit, documented.

## Permissions, data, and security requirements
Catalogue and index stay outside the repository; Gaia data ESA/Gaia/DPAC CC BY-SA 3.0 IGO. The owner's frame is
used for verification only and not committed. One small public Gaia TAP cone query for the independent check.

## Compatibility and nonfunctional requirements
Existing item names and files unchanged; only the 0.5° label's field range changes (0.5° to 0.45°).

## Assumptions and open questions
Size target about 1 GB like the 0.5° index (owner's earlier bound); measured variants in verification.md.

## Owner decisions and requirement changes
- 2026-10-29, Peter Polakovic: same catalogue as the 0.5° index; audit deferred ("Odložiť, index najprv").
- 2026-10-29, Peter Polakovic: supplied `Focus_81.15mm.fit` to test the 0.2° index.
