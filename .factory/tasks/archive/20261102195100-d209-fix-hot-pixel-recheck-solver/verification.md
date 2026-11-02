# Verification

Verdict: pass

## Scope and provenance

Baseline067293e2f55f97e960c3284b0366f8eacb0e9ba1; root implemented simulator repair and executed its runtime checks in /Users/polakovic/Development/indigo, Darwin arm64. A separate subagent reconstructed historical solver inputs, compiled current core and executed isolated probes; another independent subagent reviewed source and retained artifacts without rerunning tests. Technical AI review is not human approval/risk acceptance. No hardware or index download.

## Found defects and acceptance

FND-20261027140318-5859-fix-simulator-binning-001: the generated create_frame hot-pixel loop accepted y equal to frame_height. Both hot-pixel and hot-row writes can escape the logical frame. New regression against original production reproduced guard-row corruption at column0 (expected baseline failure, exit1). Repair changes > to >= in .driver, version45→46; unchanged generator produced matching C guard and0x0300002E, with header/main unchanged. No property, rendering, queue or timing contract otherwise changes.

The test-only wrapper includes actual generated production C and forwards device attachment unchanged; it seeds idle imager hot-pixel coordinates and guards one row beyond a64x64 subframe. Existing public bus exposure then exercises production create_frame and RAW publication. At bins1/2/4, points at bottom and below, top/left/right outside are excluded; valid top, last-row and bottom-right pixels remain65535 and guard remains0x5A5A. RAW metadata does not occupy the guard for this imager. Production makes no test API available. Initial fixture tried legacy timer capture, received no imager and failed setup; adjusted only test attachment capture before obtaining actual baseline guard failure.

FND-20261027140318-5859-fix-simulator-binning-002: see solver-verification.md for exact commands, input construction/hashes, source identities and retained compressed logs. Current unchanged CLI/core solves6/6 reconstructed Rosetta mean/sum-clamped800x600 inputs under normal selected2°/5° indexes, using appropriate2° index. Sum inputs yield10–13 consistent matches, correct center within10arcsec. Diagnostic5°-only controls fail both mean and sum and are below that index's field range; they do not show binning-specific failure. Original exact noisy RAW/random state and historical indexes were not retained. Close current finding as obsolete/not_a_bug, without claiming a new solver fix or attributing improvement to a specific change. No detector/threshold tuning or general robustness certification.

## Commands and results

All commands from repository root unless noted; full command lines including compiler flags in evidence logs.

- make -C indigo_drivers/ccd_simulator -f ../../Makefile.drv all: baseline and final pass, universal x86_64/arm64. Generated warnings are retained. Final source diff only version/guard.
- make -C indigo_test build/integration/test_ccd_simulator: pass original and fixed. Existing simulator_imager_binning_averages_the_bin_block control1/1 baseline; added simulator_hot_pixels_respect_subframe_edges fails original1/1 and passes fixed1/1.
- clang derived from existing Makefile compile/link commands, arm64 -O1 -fsanitize=address,undefined -fno-omit-frame-pointer: changed production include-wrapper and integration test instrumented. ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/indigo-hotpixel-sanitize simulator_hot_pixels_respect_subframe_edges: pass1/1. Common support and mount libraries prebuilt/uninstrumented; no leak certification.
- python3 tools/run_driver_test.py ccd_simulator: final29/29 pass, driver3.0.0.46, recorded2026-11-02 20:55 mac arm64 simulator. README testing row and generated TEST_SUMMARY updated; MIGRATION availability29/0, manual comment/hardware count retained.
- ./.factory/scripts/test:197/197 pass106.963s; no Factory helper changes.
- plutil -lint indigo.xcodeproj/project.pbxproj: pass. New private test wrapper registered; no .factory reference.
- ./.factory/scripts/check and ./.factory/scripts/security: pass; final findings/process metadata checks recorded at closure.

Evidence/ retains baseline/final build, control, boundary, sanitizer, recorded-driver and Factory results. Solver evidence remains solver-prefixed. Simulator guider timing characterization retained separately with endpoints/limits; not physical relay timing. No full solver suite or unrelated recognition/hardware run. Linux/Windows unavailable, not claimed.

## Final test summary

Fixed simulator checks:31/31 (29 recorded suite +1 additional focused normal +1 focused sanitizer); baseline healthy control1/1, expected new-test defect1/1 failure before fix. Solver positive probes6/6; unsuitable-index diagnostic controls2 expected non-solves observed (exit1, not successful solves). Factory197/197. Hardware0 run,0 passed. Timing samples are characterization, separately reported, not additional acceptance cases.

## Guider timing characterization

make -C indigo_test build/benchmark/bench_ccd_simulator_guider_timing and indigo_test/build/benchmark/bench_ccd_simulator_guider_timing: exit0, TIMING status=complete. Durations20/100/500ms, EAST/WEST/NORTH/SOUTH, idle and concurrent shared imaging,4 measured samples plus1 warmup each combination:96 measured+24 warmups. Endpoints are request-to-public-OK on software simulator; per-group signed min/mean/median/p95/p99/max, SD and maximum absolute errors retained in evidence/guider-timing.txt. Maximum absolute error across groups 6.295ms. No tight host-dependent threshold or physical relay timing claim.
