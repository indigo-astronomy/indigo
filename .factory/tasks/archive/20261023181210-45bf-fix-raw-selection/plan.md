# Plan

1. Add public-helper regressions for AC-001/002 across all six formats and AC-003 mono16 statistics; capture failures against unchanged production source.
2. Validate ROI before integer conversion and reads, without overflow in radius arithmetic; use double PSF variance. Document selection contract in the existing public header. No dependencies.
3. Run native RAW suite and ASan/UBSan against separately compiled production objects/library; run configured Factory checks. Inspect final diff in separate verifier, quality and security passes (same-agent limitation).
4. Wait for coordinator target after 039/026, incorporate that master, reconcile metadata, refresh checks/reviews, archive task and prepare tested commit. Coordinator integrates and cleans up.
