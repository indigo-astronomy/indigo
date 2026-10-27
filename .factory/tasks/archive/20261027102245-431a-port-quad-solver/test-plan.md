# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 tangent plane round trip, far hemisphere, RA wrap | AC-003 | unit `indigo_test/unit/test_agent_solver_core.c` | deterministic RNG | round trip within 1e-6 rad | with implementation |
| T-002 hash packing and clamping | AC-003 | unit | fixed values | components restored | with implementation |
| T-003 quad code invariance | AC-003 | unit | 300 random quads, random similarity, mirror | code within one quantum or a stored variant | with implementation |
| T-004 canonical variant first, code bounds | AC-003 | unit | 200 random quads | variants[0] == canonical | with implementation |
| T-005 index file round trip, truncation, bad magic/version, dangling star, unsorted | AC-003 | unit, temp file | mkstemp | errors reported, load sorts | with implementation |
| T-006 index selection by field size | AC-003 | unit, temp dir | empty files | 2° for 3.75°x2.8°, fallback rules | with implementation |
| T-007 synthetic field solve, mirrored solve | AC-004 | unit | 28 stars, all quads | centre < 5", scale 0.2%, parity, reprojection | with implementation |
| T-008 unrelated field rejected | AC-004 | unit | random stars | no reprojecting solution | with implementation |
| T-009 installed index reflected | AC-005 | integration `indigo_test/integration/test_agent_solver.c` | synthetic 5° index in cache | TYC2_5 selected, in USE_INDEX | with implementation |
| T-010 RAW solve full resolution and downsampled | AC-005 | integration | rendered MONO16 frame | WCS OK, centre < 10"/30", scale, angle, parity, index | with implementation |
| T-011 no index selected, non-RAW upload | AC-005 | integration | as above | WCS ALERT with message | with implementation |
| T-012 index deletion | AC-005 | integration | as above | file removed, item withdrawn | with implementation |
| T-013 tester frame with real indexes | AC-006 | manual harness | tool-built 5° and 20° indexes | known solution / no false solution | after implementation |
| T-014 native unit suite, astrometry agent integration test | regression | `make -C indigo_test test-unit`, `test_agent_astrometry` | built library | pass | after implementation |

## Baseline evidence
No driver test existed; the native unit suite was not run at baseline (no change to shared libraries in this task).

## Bug reproduction
Defects found in the ported code (float normal equations, Cramer typo, sigma based inlier limit, single quad
solution, mirrored rotation, acos distance) were reproduced by T-007, T-008, T-010 and T-013 before their fixes;
see REFACTOR.md.

## Negative cases and permission boundaries
T-005, T-008, T-011; the download path is not exercised (no network in tests).

## Isolation and cleanup
Unit tests use `mkstemp`/`mkdtemp` under `/tmp` and remove their files; the integration test forks every case with a
private home and its own `INDIGO_CACHE_BASE`, removed afterwards. `make -C indigo_test test-clean` after validation.

## Deferred tests and coverage limitations
Download/abort of a download (network), FITS/JPEG uploads (unsupported), Linux and Windows builds, sanitizer runs.
Owner: Peter Polakovic; follow-up when the index hosting exists.
