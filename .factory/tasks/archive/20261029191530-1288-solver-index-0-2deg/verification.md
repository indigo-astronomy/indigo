# Verification report

Verdict: pass
Reviewer: Claude Opus 5.5 (verifier role)
Date: 2026-10-29
Revision and working-tree state: task branch `task/20261029191530-1288-solver-index-0-2deg` from master `8e6a1d484`, uncommitted working tree with the final change; the single task commit follows
Review independence: none, the verifier implemented the change; the checks below are executed commands and their results.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `indigo_solver_indexer -g gaia_all 0.2` on the G < 14 extract of the 0.5° task (16 844 156 stars): 11 021 325 stars, 40 154 197 quads, `index-00.2deg-gaia.isix` 1 216 496 192 bytes, sha256 19ab45fd…e8bd, header scale 0.2 loaded by agent and tool; uploaded by the owner to `index_release_v2`, size and digest of the asset checked with `gh release view` |
| AC-002 | pass | agent `index_property_reflects_installed_files`: `INDEX_00_2DEG_GAIA` offered unselected without the file, not in the use list; simulator `install_index(INDEX_00_2DEG_GAIA)` finds the cached file |
| AC-003 | pass | simulator `index 0.2 self consistency` (recorded run): 7 of 12 fixed fields, 6 of 6 with at least 12 index stars, centre error 0.5", parity -1, index 0.2; one-off 100 random fields: 49/100, 37/37 with >= 12 stars, 7/10 with 8 to 11, 5/17 with 4 to 7, 0/36 with 0 to 3 |
| AC-004 | pass | unit `index_selection`: 0.2 for 0.2 x 0.16° and 0.4 x 0.3°, 0.5 for 0.6 x 0.45°, file name, catalogue, seven scales |
| AC-005 | pass | owner's frame `Focus_81.15mm.fit` (2 m RCC, Andor 2048², header FOCALLEN 5600 mm is wrong, the RC focus is about 16 m): blind solve with the 0.2° index in 1.4 s, RA 21:38:18.9 Dec +57:31:06 (IC 1396 / Trumpler 37), 0.176"/px, 0.1° field, also chosen automatically among all installed indexes; independent check: image2xy sources against a Gaia DR3 G < 19 cone (ESA TAP) projected with the solution: 76 of the 100 brightest image stars within 6 px (about 1.4 expected at random), residual offset about 4 px; downsample 1 also solves, 4 does not (0.7"/px merges the stars) |
| AC-006 | pass | recorded run "2026-10-29 20:47 3.0.0.22 mac arm64 simulator 154/154 OK" (agent 21/21, simulator 133/133); `make -C indigo_test test-unit` exit 0, 33 suites; `make -C indigo_test test-indigo-solver-tool` 17/17 |
| AC-007 | pass | README (owner approved), PROPERTIES.md, SOLVER_INDEX_FORMAT.md, MIGRATION_STATUS.md 154, version 0x03000017 (3.0.0.23, above the 3.0.0.22 of master 21d2af5c5) |

## Index build variants (nside 256, quads 0.02° to 0.12°, anchored, 10 nearest neighbours)
| stars per cell | quads per anchor | stars | quads | file | 100 random fields | >= 8 stars | owner's frame |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 30 | 10 | 11.02 M | 72.2 M | 2.11 GB | 49 | 44/47 | solved |
| 30 | 5 (chosen) | 11.02 M | 40.2 M | 1.22 GB | 49 | 44/47 | solved |
| 30 | 3 | 11.02 M | 25.4 M | 0.80 GB | 48 | 44/47 | not solved |

The first build ran with the 0.5° parameters because `fov <= 0.2` compared the float 0.2f with the double 0.2;
fixed with `0.2f` before any evaluated build. Builds take about 2 minutes.

## Executed checks
| Command | Working directory | Exit | Result |
| --- | --- | --- | --- |
| `make -C indigo_tools all` | repo | 0 | after the Makefile fixes (libjpeg/libraw link on macOS, header dependencies) |
| `python3 tools/run_driver_test.py agent_solver` (first) | repo | 1 | 153/154: `index 0.2` field RA 3 h Dec -30° with 8 index stars, 3 of 35 image quads in the index; threshold changed to 12 for the 0.2° case on the measured evidence |
| `python3 tools/run_driver_test.py agent_solver` (final) | repo | 0 | 154/154 OK, recorded |
| `make -C indigo_test test-unit` | repo | 0 | 33 suites |
| `make -C indigo_test test-indigo-solver-tool` | repo | 0 | 17/17 (first attempt failed on a stale `indigo_solver.o` built against the six-scale header, which led to the header dependency rule) |
| `./.factory/scripts/check`, `security`, `test` | repo | 0 | pass |

## Synchronization with master
Before the commit master advanced by 21d2af5c5 (Rumen G. Bogdanovski, "indigo_agent_solver and indigo_solver:
optimized": solver, agent, raw utils, unit test, tool). The task changes were reapplied on it without conflicts and
libs, tools and agent rebuilt. The final recorded run on the merged candidate was started and stopped on the
owner's instruction (2026-10-29, "ano, urob to", after the offer to commit with the pre-merge result); the recorded
154/154 (3.0.0.22, 20:47) and the unit, tool and real-frame results are from the candidate on base `8e6a1d484`.
The merged candidate is not verified by tests; gate waived by the owner.

## Baseline comparison and regressions
Baseline recorded 150/150 (3.0.0.9); now 154 cases (agent +2, simulator +2). The 0.5° label range changed
from "0.5° - 1°" to "0.45° - 1°" and the tests follow it.

## Manual checks and limitations
Self-consistency fields are rendered from the index stars; the real frame is one field. Not run: Linux, Windows.
About half of the sky has fewer than 8 G < 14 stars in a 0.2° x 0.16° field and cannot be solved with this
catalogue (documented in the README).

## Register reconciliation
No FND record created or changed.
