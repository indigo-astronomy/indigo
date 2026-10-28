# Preparation review

Verdict: pass

Independent reviewer context, source HEAD e0a2d5490880f9964e391988449b8fad180d52e9 plus final task-local uncommitted preparation scripts. Scope: run-on-rpi.sh, measure.py and specification. No camera, SSH, production edit or hardware test was performed by this reviewer.

The launcher requests exactly two single-frame runs,1.2s and3s, uses existing native decoding/restoration, unique exclusive artifact paths, --no-record, and exact published source hash. It stops on first capture failure. EXIF comparison is explicitly a metadata sanity check rather than physical curtain metrology. Bash syntax passes; the analyzer reads the retained old JPEG as1.4s, accepts a1.4s expected value, and rejects1.2s with signed and absolute error200ms.

Reinspection confirms all preparation corrections: anchored pgrep -f handles the full process command instead of Linux's truncated comm name; the entry point reexecutes under a90s outer watchdog with5s forced-stop fallback; absolute error is reported alongside signed error. Inner captures retain40s bounds. These are prepared scripts only; they were not launched against hardware during this review. The coordinator separately verifies remote binary/archive freshness and uploads. No blocking preparation finding remains. Final hardware accuracy remains unverified until actual execution and EXIF analysis.
