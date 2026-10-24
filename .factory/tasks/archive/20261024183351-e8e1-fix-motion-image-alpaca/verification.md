# Verification

Verdict: pass

## Candidate and independence

Working directory `/Users/polakovic/Development/indigo`, task branch `task/20261024183351-e8e1-fix-motion-image-alpaca`, local master/origin-master/base `463eb538daf440fa0f459b53a5f02fa949e244ed`. Both target refs remain equal to the task base; no remote freshness beyond local refs is claimed. Platform macOS arm64; normal component builds produce x86_64/arm64 universal artifacts. Parent implements capacity and combines delegated repairs; an independent subagent inspects actual source/test diffs. No physical hardware, Linux/Windows/native 32-bit or full ConformU execution.

## Baseline and focused acceptance evidence

- `make -C indigo_drivers/agent_alpaca -f ../../Makefile.drv all` and `make -C indigo_test test-agent-alpaca-cache`: baseline pass, 1/1 cached-image cases.
- `make -C indigo_test test-agent-alpaca-mapping`: repaired synthetic INFO discovery pass 1/1. Covers last free Bayer slot, full capacity omission/null mapping, preserved128 entries, and rediscovery pointer rebinding. No invalid-memory pre-fix reproducer was executed.
- `/usr/bin/clang -g -O1 -arch arm64 -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -D_GNU_SOURCE -fsanitize=address,undefined -fno-omit-frame-pointer -Iindigo_libs -Ibuild/include indigo_test/integration/test_agent_alpaca_mapping.c build/drivers/indigo_agent_alpaca.a -Lbuild/lib -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -lindigo -lm -lpthread -lz -o /private/tmp/test_agent_alpaca_mapping_sanitize` followed by that executable: pass1/1, no ASan/UBSan diagnostic. Existing source `sprintf` deprecation warnings only. The full main agent C source is included/instrumented; other dependencies are prebuilt.
- Delegated RAW original and sanitizer/source-current final evidence: [raw-repair.md](raw-repair.md), 17RAW+3cached-image+4production-consumer cases pass. LeakSanitizer unavailable on this runtime; successful runs explicitly disable leak detection.

## Combined build and recorded runs

`make -C indigo_libs all`: pass, including changed RAW/platesolver objects and library products. No driver sweep performed.

Final driver runs use unchanged `tools/run_driver_test.py` main through `/private/tmp/run_indigo_driver_locked.py`; the temporary wrapper imports that module, retains its test/build/counting logic, and serializes `update_readme` through the subsequent summary subprocess using a process file lock. This coordinates parallel final suites without concurrent README/summary writes. No tests are filtered and no repository runner code changes. Wrapper source will be retained alongside this report for review.

`make -C indigo_test test-raw-image-sanitize`: pass17/17 on combined source. Final complete driver totals are recorded after the ongoing runs below.

## Configured workflow checks

At task working-tree state (before final archival): `./.factory/scripts/check` pass, `./.factory/scripts/test` pass188/188 in113.671s, `./.factory/scripts/security` pass. These cover Factory workflow, not application behavior. Checks include findings validity, decision index, project/local security boundaries and whitespace. Final structural/committed-range gates remain pending until evidence archival and commit.

## ASI full-suite fixture correction and retest

First combined ASI full run fails at `compensation_recovers_without_losing_baseline`: its successful retry sets the fake motor idle while measured position remains350 and the issued correction requests550. The new production contract correctly reports ALERT; subsequent shared-fixture scenarios cascade. The motion writer and independent reviewer separately confirmed this obsolete successful-arrival fixture. The first proposed correction used `finish_motion_at`, but a second full run exposed that helper’s extra wait for FOCUSER_STEPS, which automatic mode deletes. After two unsuccessful full attempts, the parent reassessed with both motion writer and independent reviewer. The final correction explicitly captures requested_position, supplies measured arrival before idle, retains the original POSITION OK assertion and adds exact measured-position verification. Existing retry-count/mode assertions and separate early-stop regression remain. A temporary first-five-scenarios prefix fixture checks the dependent compensation sequence before the next full run. No production expectation is weakened. Final ASI result follows below.

## Other first-pass limits and diagnostic reruns

Lunatico rotator first full attempt passed29 serial cases but UDP simulator failed to bind its loopback socket (sandbox Operation not permitted), giving58 planned/29 passed; permitted complete rerun required. First focuser_lunatico full pass attempt had55/57 passing, failing two unchanged focuser scenarios (`single_read_failure`, `settings_during_motion`). Source/trace diagnosis found existing connection settle synchronization waits for command receipt before its readback publication, so requests can race initial polling under concurrent load; both failing cases independently pass traced targeted diagnostics with no production or expectation changes. A complete serial rerun with retained simulator traces is required. These are recorded as failures, never waived.

## Lunatico reinspection and synchronization correction

The permitted complete rotator rerun exposed three failures: serial no-op SYNC, UDP refused GOTO, and the Exp focuser request. Reinspection found an unnecessary newly added external-poll target rewrite could overwrite queued rotator requests; it was removed, restoring original target semantics while retaining the commanded integer-endpoint check. Both exported drivers were explicitly regenerated/rebuilt because their make dependencies do not track shared implementation includes.

A first proposed test barrier waited for a fresh POSITION OK callback, but focused tests disproved that assumption: unchanged readbacks are deduplicated by `indigo_update_property`, so the poll may finish without publication. This unsuccessful attempt was removed. The final shared test helper captures the actual selected in-process device pointer from definitions and, after observing the initial ismoving command, queues a no-op marker on its serialized master queue. Marker completion proves the in-flight poll returned; no transport commands, public state changes, sleep-based ordering assumption or production scheduling changes are introduced. Direct explicit overlap tests remain intact. Both motion writer and independent reviewer verified the queue contract; all15 focused checks then passed (see lunatico-focused-results.txt and exact dispatch in lunatico-focused.py). Final full suites follow below.

The separate pre-existing focuser request-ownership race remains open as FND-20261024183351-e8e1-fix-motion-image-alpaca-001. Stabilizing setup for tests that intend to start after initialization does not fix or accept that product defect.

## Final candidate acceptance

AC-001 through AC-004 pass on the combined source. Each driver has a stopped-short regression preserving actual/requested values; both Lunatico exports and serial/UDP rotator variants are exercised. Backlash, reverse arrival, quantized fractional angle, abort and external-motion contracts pass retained tests. RAW malformed/valid-format and mapping-capacity cases pass. No public properties were added or removed; the properties reference therefore requires no update. All10 changed exported driver versions are higher than their base versions; eight generator outputs reproduce byte-for-byte with unchanged generator.

| Recorded driver | Final planned/passed |
| --- | --- |
| agent_alpaca | 4/4 |
| agent_imager | 63/63 |
| focuser_asi | 24/24 |
| focuser_dmfc | 40/40 |
| focuser_dsd | 59/59 |
| focuser_fc3 | 40/40 |
| focuser_mypro2 | 36/36 |
| focuser_usbv3 | 42/42 |
| focuser_lunatico | 57/57 |
| rotator_lunatico | 58/58 |

Total final recorded cases423/423; these include3 RAW cache cases,1 mapping case and4 numerical/consumer cases. Shared RAW unit sanitizer17/17 is separate. Delegated focused instrumented24/24 and DSD2/2 repeat scoped cases; they are not added to unique driver totals. Driver final commands were `python3 /private/tmp/run_indigo_driver_locked.py <driver>` in the root, executing unchanged `tools/run_driver_test.py` main; final Lunatico commands had permission for loopback UDP. Both complete final Lunatico variants and the focuser export pass after the queue-marker correction. Final ASI inline-fixture sequence passed5/5 before the full24/24.

Final `./.factory/scripts/check` and `./.factory/scripts/security` pass after source/doc updates; earlier configured Factory test188/188 remains applicable (no Factory implementation changed). README Testing, generated TEST_SUMMARY and MIGRATION_STATUS counts match the recorded scope; manual Comment columns are unchanged. Available physical USBv3 cases16 remain historical availability; physical hardware executions this task0/0.

Pre-existing findings remain separately tracked, including new task001; no new introduced blocking failure remains. Full main integration/publication is unauthorized and pending. Final committed-range process/structural checks are required after the single result commit and are reported externally; reports bind staged content identities rather than claiming their own future commit SHA.

`make -C indigo_test test-clean` passed after final runs, removing generated native test artifacts. Independent final documentation recheck passes; noted stale pending prose was corrected. Full-candidate structural/process checks are the remaining delivery gate.

After task/finding archival, `./.factory/scripts/check`, `./.factory/scripts/security` and `./.factory/scripts/maintenance check` pass; completion count62, uncovered work tasks10, full review not due. Archive preview reported0 Markdown link rewrites; findings references resolve the retained archive. Local master/origin-master remain the reviewed base.

First staged whitespace check detected trailing spaces in captured RAW diagnostic transcript (not production code). These spaces were normalized and labelled; final staged whitespace check must pass before commit. Earlier unstaged checks could not inspect untracked evidence.
