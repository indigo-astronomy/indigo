# Historical Rosetta finding recheck

Verdict: pass

## Assignment, source and independence

Independent verifier/investigator subagent, current baseline `067293e2f55f97e960c3284b0366f8eacb0e9ba1`, task `20261102195100-d209-fix-hot-pixel-recheck-solver`. No production solver, detector, simulator, generator, tracked test or threshold changes were made. Root concurrently owns the unrelated hot-pixel boundary fix. This investigation wrote only solver-prefixed task evidence and temporary `/tmp/indigo-rosetta-recheck` artifacts. Current source identities are retained in solver-source-identities.json. Common INDIGO and catalogue libraries were existing prebuilt dynamic libraries; all three native solver core source units and CLI front end were freshly compiled for this probe. No shared driver build was used, no hardware or index download occurred.

## Input reconstruction

The specifically requested historical report was inspected: `.factory/tasks/archive/20261027140318-5859-fix-simulator-binning/verification.md`. The original failing RAW bytes and random state were not retained there, so this is a controlled reconstruction of its documented case, not a byte-identical replay. The maintained 1600x1200 static mono Rosetta image is binned offline into 800x600 RAW MONO16 using either integer `sum / 4` or `min(sum, 65535)` for each 2x2 block. Exactly 60 blocks exceed 65535, agreeing with the historical report. No gain/offset/gamma/defocus transform is applied (neutral controls); nonzero injected hot pixels/rows/columns are absent. Each pair uses identical per-pixel noise, either none or Darwin libc `srand(1)` / `srand(5859)` followed by `rand() & 127`, added after binning and clamped to 65535, matching the documented simulator noise operation.

The source image hash and all six generated RAW hashes, byte lengths and constructions are in solver-manifest.json; reproducible generator script solver-generate.py. The retained script accepts --root (default current working directory) and --output (default a new temporary directory); argument handling was added after execution without changing its pixel construction formula. Generated RAW images are temporary and not committed. Existing local current-format Tycho-2 indexes `~/.indigo/solver/index-02deg-tyc2.isix` (357684520 bytes) and `index-05deg-tyc2.isix` (178045032 bytes) are read only. They postdate the original October27 test; compatibility with today's indexes is the requested current behavior. No claim identifies which historical solver or index change caused improvement.

## Commands and outcomes

Working directory `/Users/polakovic/Development/indigo`, Darwin arm64. Initial compile omitted the existing libraw include directory and failed with missing libraw.h; corrected include-only command below passes with zero diagnostics (solver-build.log). This was build setup, not a production/test change.

```
clang -std=gnu11 -O2 -Iindigo_libs -Iindigo_drivers -Iindigo_libs/externals/libjpeg -Iindigo_libs/externals/libraw/libraw indigo_tools/indigo_solver.c indigo_drivers/agent_platesolver/indigo_solver.c indigo_drivers/agent_platesolver/indigo_solver_utils.c indigo_drivers/agent_platesolver/indigo_solver_healpix.c -Lbuild/lib -lindigo -lindigocat -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -o /tmp/indigo-rosetta-recheck/indigo_solver
python3 /tmp/indigo-rosetta-recheck/generate.py
/tmp/indigo-rosetta-recheck/indigo_solver -vv -D 1 -x 2,5 -c 30 /tmp/indigo-rosetta-recheck/<case>.raw
```

Replay using the retained script after its argument-only portability refinement (not rerun):

```
python3 .factory/tasks/20261102195100-d209-fix-hot-pixel-recheck-solver/solver-generate.py --root /Users/polakovic/Development/indigo --output /tmp/indigo-rosetta-recheck
```

All six mean/sum-clamped cases solve with both historical selected scales (2 and5), automatic default depth stages40/100/all, no position/parity/scale hints, no extra downsampling, J2000. They all recover the Rosetta centre within10 arcsec and expected approximately16.88 arcsec/pixel, parity−1. Solutions use the appropriate2° index. The summed seed1 case reaches stage2 (100 brightest stars, 13/31 consistent/voted matches); the stage progression for each case is retained in its summary and full log. Full per-case debug/output logs are retained as reproducible gzip files (`*.log.gz`, empty filename and mtime0); small `*.summary.txt` extracts and solver-summary.json retain coordinates and metrics. Compression is lossless; no execution was repeated.

| Frame | Noise | Detected stars | Consistent / voted matches | RMS arcsec | Exit |
| --- | --- | ---: | ---: | ---: | ---: |
| Mean | None | 420 | 8/8 | 4.39 | 0 |
| Mean | Seed1 | 344 | 8/8 | 4.17 | 0 |
| Mean | Seed5859 | 359 | 8/8 | 3.76 | 0 |
| Sum/clamp | None | 421 | 10/27 | 3.76 | 0 |
| Sum/clamp | Seed1 | 403 | 13/31 | 5.28 | 0 |
| Sum/clamp | Seed5859 | 409 | 11/26 | 4.23 | 0 |

Two diagnostic runs use identical seed1 frames but `-x 5` alone. Both fail normally with exit1 after all stages: mean ends at5/63 consistent/voted, sum at5/69. This does not distinguish summed binning: the averaged control also fails. The5° index currently documents FOV4.5°–10°, larger than the reconstructed3.75°x2.81° field; automatic current selection appropriately succeeds with2°. The diagnostically unsupported index-only failure must not be reported as all selected indexes failing or as a regression introduced by summed binning.

## Disposition

The recorded defect—deterministic failure of summed/clamped Rosetta while averaged frames solve under the normal2°/5° configuration—does not reproduce on current source and current local indexes. The summed frames now give10–13 consistent matches rather than historical5. Recommend closing FND-20261027140318-5859-fix-simulator-binning-002 as obsolete, with retained reconstruction limits and diagnostic5°-only limitation. Do not lower consistency thresholds or change production binning on this evidence. No general solver robustness guarantee follows from these six positive fixtures.

Simulated image probes:6/6 expected positive solves; diagnostic unsuitable-index controls:2/2 expected non-solves observed (both command exits1 are retained, not mislabeled successful solves). Hardware:0 run,0 passed. Full solver suite, full camera acquisition pipeline, old index files, sanitizer suite and byte-identical historical RAW replay were not run and are not claimed. Original report did not retain enough information for the last replay. Parent coordinator owns canonical finding disposition and final quality review.

## Final evidence inspection

The retained generator was independently reread: it asserts1600x1200 samples, indexes all four distinct2x2 pixels, uses integer mean or pre-noise saturation of their sum, adds identical per-pixel noise arrays to each paired construction, saturates after noise, and emits12-byte RAW2 header plus480000 little-endian uint16 values. Manifest hashes and recomputed summaries agree. The zero-noise pair removes random-state dependence. CLI `-D 1 -x 2,5` matches un-downsampled binned images and limits indexes to the two requested scales; no position/scale hints are added. The5°-only diagnostic fails both constructions, while paired default-scale controls succeed. The index label states FOV4.5°–10°, beyond the3.75°x2.81° field. These observations support the bounded obsolete recommendation rather than a universal solver guarantee.

Lossless gzip readback was compared byte-for-byte with all eight temporary full logs; all six retained RAW hashes were checked against their original generated files. No solver, image construction or application check was rerun during this evidence-compaction pass.
