# Verification

Verdict: pass

This verdict covers diagnostic execution, truthful registered results and documentation; the bulb accuracy measurement itself FAILED and full camera-class acceptance FAILED. No production fix or timing acceptance is claimed. Source e0a2d5490, exact SHA256 b7b78630bfeb2855891ec1450885e5dd9d53736e9132bdf000ee3c4584651830, unchanged during this task. Masterd021e659b synchronized before final documentation/checks; the owner's separate refactor-note archival is preserved.

## Actual hardware runs

Owner explicitly confirmed awake K-m and requested complete registered camera testing. Ran ssh -A indigo@indigosky.local 'bash /tmp/indigo-pentax-20261027181603/run-bulb-accuracy.sh'. Outer90s watchdog, two existing single-frame/no-record captures. Both image transfers/decodes passed. Copied native JPEG artifacts outside Git and ran measure.py with1.2 and3 expected. EXIF1.09/2.88s, signed−110/−120ms, both fail±50ms criterion (timing-results.json). This is EXIF metadata comparison, not independent curtain metrology.

Canonical registered runs in isolated RPi root used LD_LIBRARY_PATH=.../build/lib and python3 tools/run_driver_test.py ccd_pentax --hw --no-build --type LABEL -- PENTAX_HW_ARGS=PROFILE:

| Profile / label | Result |
| --- | --- |
| --run / PENTAX K-m |1/0 FAILED: physical B refuses the initial change to M; no full-class pass. |
| --bulb-only / PENTAX K-m (bulb) |1/1 passed: JPEG capture, abort and subsequent recovery frame. |
| --single-frame / PENTAX K-m DNG (single frame) |1/1 passed:16,647,909byte transfer, LibRaw unpack3896x2616, ISO800,1.100s metadata for1.2s request. |
| --single-frame / PENTAX K-m PEF (single frame) |1/1 passed:10,955,425byte transfer, LibRaw unpack3896x2616, ISO800,1.100s metadata for1.2s request. |

Exact hardware stdout/commands, config/restore status are retained in hardware-evidence/. Initial RAW discovery happened before the kernel block node reappeared; passive sysfs inspection confirmedPENTAX DSC_K-m and bounded identity-only wait preceded the continuation. No SSD access, USB reset, physical unplug or extra mode speculation. Original upstream pkTriggerCord changed only capture format for RAW profiles. JPEG and latent DNG restoration succeeded; ISO800/f8/physical B retained. Camera use ended and owner was told it could be powered off.

Canonical README records copied using existing update_readme helper;144-driver TEST_SUMMARY regenerated. Full failure remains distinct from partial passes. Neither EXIF timing failures nor skipped Manual controls are represented as green hardware certification.

## Preparation and documentation

Before camera use, source identity/build/archive freshness, remote/local bash -n and retained-JPEG analyzer positive/negative controls passed (prep-review.md). User explicitly authorized README edits. Documentation explains physical M/B setup and one-second boundary, actual capture identity/platform scope, native formats, unavailable streaming/ROI/binning, privilege/claim limitations and observed EXIF errors. No production code, dependencies or privileges changed.

Final quality/security and configured checks run after documentation; exact evidence retained with this task. The timing finding remains open and is not risk-accepted. No repeated hardware access is needed for review or task closure. Integration/publication require separate owner authorization; this task will remain on its own branch after a local outcome commit.

Final configured checks: ./.factory/scripts/check passed (findings28active/206archived, decision index and boundaries); ./.factory/scripts/test passed190/190 in115.707s; ./.factory/scripts/security passed. Logs retained. Independent final quality/security reports pass for diagnostic/doc scope; open timing finding remains unresolved. No new source/harness change requiring driver regression rerun. Committed-range gate will run on the local outcome commit before delivery.
