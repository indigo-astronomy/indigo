# Verification

Verdict: pass

## Candidate and limits

Task branch includes master **114b80ff5157c6c4923e09e232a37f6dcacae0dc** (039, 006, 005). The final tested working-tree content is bound by version-2 process identities before the single final task commit; committed-candidate gate results and SHA are reported externally. Same agent implemented and performed separate verification/review passes; no independent-context/provider review is claimed.

All commands run from `/private/tmp/indigo-20261023180923-1f46-fix-synscan-autohome` unless stated. Host: macOS 27.0.1 arm64, Apple clang 21.0.0, Python 3.9.6. Production builds are universal x86_64/arm64; execution is arm64. Matching-base framework libraries and unchanged generator were copied read-only from the primary checkout to ignored local build outputs; this task did not rebuild or certify the whole framework. Incoming RAW/FITS regressions separately compile their current production source. No primary-checkout write, physical hardware run, Linux/Windows execution or push occurred.

## Acceptance

| Criterion | Result and evidence |
| --- | --- |
| AC-001 protocol/public behavior | Pass: retained original commands and six normalized public state/value transitions match; normal/negative-start/extra-offset scenarios preserve 58/48/64 ordered writes, exact preparation/return distances, encoder/counter/mode/ramp/period/start order and three-second dwell. See autohome-reference.json and compare-autohome.py. |
| AC-002 queue responsiveness/cancellation | Pass: every motion/index/dwell stage yields; abort/disconnect cases cover both axes, extra offset, reverse stopping, forward search/stopping, backoff and return. Tests observe L1/L2, latency below the 1.5 s simulator budget, cleared state and no later home J/E writes. Queued start, restart/reconnect and shared-guider lifecycle pass. No real-hardware latency bound is claimed. |
| AC-003 errors/deadlines | Pass: encoder/counter/move/stop refusal, malformed index, absent reply, motion/index timeout and emergency-stop refusal/retry end with appropriate ALERT and best-effort independent stops. Timeout tests advance only the driver monotonic clock; queues, timers and protocol I/O remain real. |
| AC-004 preserved interfaces/requests | Pass: all 47 existing cases remain, plus 47 added cases (94 total); previous guide finalizer cannot reclaim a homing axis; pending tracking/encoders retain targets and complete. No public property/count/visibility change or generator implementation change. |

## Baseline and preserved trace

Fresh-worktree component builds initially failed because generated Makefile.inc/output directories were absent. `make Makefile.inc` and local build directories corrected setup. `make -C indigo_drivers/mount_synscan -f ../../Makefile.drv all` and `make -C indigo_test build/integration/test_mount_synscan_simulator` then passed on original version20. The original full suite first hit sandbox loopback bind denial; authorized PTY/UDP rerun passed **47/47**. Original normal, negative sensor start and extra-offset characterization passed. Initial-stop abort reproducer failed waiting for queued abort OK, confirming FND-20261023101753-bc8d-complete-repository-audit-026. The original blocked callback also prevented prompt teardown; that failed process was not accepted as clean verification.

`autohome-reference.json` retains timestamp-normalized original commands from source c5a8b69ab55098f93e723f3d6ffc2a6cf5cae441 and public transitions. An original-source object was separately compiled for the passive property observer after the test observer was added. `python3 .factory/tasks/20261023180923-1f46-fix-synscan-autohome/compare-autohome.py /private/tmp/synscan-staged-traces` passes all three success variants. It removes only read-only f/j/q queries and sensor markers, canonicalizes the independent adjacent forward K1/K2 pair, and normalizes only live-index backoff overshoot. All other arguments are exact; 12000-step offsets and 24000-step returns are asserted. Property transitions match after dropping repeated identical updates per property. Relative dwell observations are retained in the JSON; final RA/DEC sensor-to-K intervals are 3.103/3.106 s versus original 3.050/3.024 s, including poll latency.

Intentional differences are queue availability/read-only polling, immediate cancellation/failure cleanup, checked stop acknowledgements and truthful failure states. Stop refusal cannot guarantee physical stopping; abort reports ALERT and retry works, disconnect reports the refusal and still releases transport.

## Final checks

| Command/check | Result |
| --- | --- |
| `INDIGO_SIMULATOR_TRACE_DIR=/private/tmp/synscan-staged-traces INDIGO_SYNSCAN_PROPERTY_TRACE=/private/tmp/synscan-staged.properties python3 tools/run_driver_test.py mount_synscan` | **94/94 pass**, final confined run, driver3.0.0.21, recorded 2026-10-23 20:44; README Testing and TEST_SUMMARY regenerated. |
| `INDIGO_TEST_FILTER=synscan_autohome ASAN_OPTIONS=detect_leaks=0 /private/tmp/test_mount_synscan_sanitize` (from indigo_test/) | **46/46 pass**, ASan/UBSan production driver and test code; framework dylib uninstrumented, leak detection disabled. No sanitizer/teardown diagnostics. |
| `INDIGO_TEST_FILTER=synscan_mount_connects_with_udp_autodetection ...` normal and instrumented binaries (indigo_test/) | **1/1 pass each**, real discovery retry/framing to loopback only. |
| `./.factory/scripts/test` | **175/175 pass**, final included master, 113.081 s. |
| `./.factory/scripts/check`; `./.factory/scripts/security`; `python3 .factory/scripts/decision_index.py --check` | Pass; repeat after archival and on committed candidate. |
| Strict production syntax: clang arm64/gnu11, `-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -fsyntax-only` with INDIGO_MACOS/uint definitions and indigo_libs include | Pass; suppressions cover existing generated boilerplate. Strict simulator `-pthread -Wall -Wextra -Werror -Wno-unused-parameter -fsyntax-only` also passes. |
| Unchanged generator regeneration | Byte identical C/header/main. Generated C SHA-256 c032522cdff1777d0711b4592304aed4d21170f77e41d4753e659ac424c2bfc7. Property declaration/count/visibility and all 16 generated init-property/item calls unchanged; no MAX_DEVICES override. |
| `plutil -lint indigo.xcodeproj/project.pbxproj` plus PBX graph traversal | Pass; exact parents/unique reachability checked for repaired039 and preserved006/005/026 refs. Final retained paths checked after archival. |
| `make -C indigo_test test-imager-download NODE=/Users/polakovic/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node` | Incoming039 **18/18 pass**. |
| `make -C indigo_test test-raw-image-sanitize` | Incoming006 **15/15 pass**, current RAW production helpers and ASan/UBSan/float-cast-overflow. Missing local libraw.a prerequisite was copied read-only before the passing run. |
| Compile current indigo_fits.c and test_raw_to_fits.c with `-O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined` (`-Wno-unused-function` on the test) and run `/private/tmp/test_synscan_incoming_fits` | Incoming005 FITS regressions pass. No unrelated production edits. |

Sanitizer build used:

```sh
clang -arch arm64 -std=gnu11 -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -DINDIGO_MACOS -Duint=unsigned -Dindigo_monotonic_time=synscan_test_monotonic_time -Dindigo_perform_active_discovery=synscan_test_active_discovery -Iindigo_libs -c indigo_drivers/mount_synscan/indigo_mount_synscan.c -o /private/tmp/synscan-sanitize.o
clang -arch arm64 -std=gnu11 -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -DINDIGO_MACOS -Duint=unsigned -D_GNU_SOURCE -I. -Iindigo_libs -Iindigo_drivers indigo_test/integration/test_mount_synscan_simulator.c /private/tmp/synscan-sanitize.o -Lbuild/lib -lindigo -lindigocat -lm -lpthread -o /private/tmp/test_mount_synscan_sanitize
```

`guider-timing.json` contains 120 retained software command-edge measurements, 24 discarded warmups, all directions at20/100/500 ms under tracking/polling and added enumeration bus traffic. It includes per-group signed/absolute error, percentages, min/mean/median/p95/p99/max/stddev/max-absolute statistics. RA endpoints are guide/restored period writes; DEC endpoints are start/stop writes. Camera acquisition is not applicable; physical guide output was not measured.

## Test-boundary fix, corrections and synchronization

FND-20261023180923-1f46-fix-synscan-autohome-001: legacy broadcast discovery could connect/initialize a physical responder before its locality check, then skip as a pass. Baseline logs selected a local simulator, not a nonlocal skip; this is not hardware certification. Test-only discovery now passes127.0.0.1 to the real API, fails nonlocal response and keeps socket cases in OPT_IN_DRIVER_TESTS. The unchanged runner selects the suite exactly once; it is absent from normal no-socket integration. Confined normal/instrumented retests and the final94-case run pass. Production discovery is unchanged.

A draft disconnect-stop-failure test skipped shutdown after disconnect, contaminating later bus starts; the affected full/sanitizer runs were deliberately stopped, teardown corrected with a started flag and clean runs repeated. The interrupted runner honestly recorded15/14 Failed, superseded by the passing final record. A guide test initially marked before the pulse write; moving the marker after that edge preserved its no-restore assertion. Runner `--dry-run` was misread as a selection preview; the extra executing run was stopped without changing the saved record, then selection was checked via Make variables. Confirmed owned orphan/extra simulators were stopped. None of these interrupted/draft runs is acceptance evidence.

Master114b80ff includes039/006/005. For both target advances, only owned Xcode/Makefile edits were saved outside the repository and reversed, master fast-forwarded, then changes restored via three-way file merge. Independent addition conflicts retained both sides. Coordinator authorized correcting four pre-existing039 PBX parent placements; source/Node target and006/005 refs are preserved. No checkpoint commit or history rewrite was needed. The due repository audit is explicitly deferred by the owner for this repair, with no audit checkpoint reset.

Physical hardware: **0 run /0 passed**. No MountSim or Linux/Windows execution. Available hardware suite has19 cases; MIGRATION_STATUS availability is not a run claim. Required final source/check evidence is complete; integration/publication/cleanup are separate coordinator delivery steps, no push authorized.
