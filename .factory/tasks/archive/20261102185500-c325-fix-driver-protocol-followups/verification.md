# Verification

Verdict: pass

Final product source5a6280dc5. Parent AI coordinator reported a technical source/security/artifact review PASS in collaboration; this is agent review evidence, not human authorization or risk acceptance. All six acceptance scopes pass with the limitations below.

## Environment and scope

Working directory `/private/tmp/indigo-driver-protocol-followups`, base9557ca85e, macOSarm64. Ignored Makefile.inc absolute paths remapped and existing common library/header prerequisites copied from primarycheckout; no primary buildartifact writer. Sameagent implemented/tested, root independent source review requested. No physical hardware, Bluetooth radio, boardflash or image recognition. Mac Bluetooth callback bodies execute with synthetic Foundation objects and bus/delegate stubs; full actual .m implementation units compile universal x86_64/arm64 without diagnostics. No claim to full CoreBluetooth lifecycle/race/hardware certification.

## Baselines and found defects

Four new source probes were compiled/executed before production edits. WeMacro loss callback left device_count1 and reconnect skipped connect; MJKZZ corrupted checksum frame changed cached position; entire manual Temma sketch answered R1 instead of stn-on; AO accepted oversized firmware configuration. Each expected regression failed its behavioral assertion. Probe setup compiler issues (fixture local speed variable shadow and Arduino shim stringappend, switchcase wrapper) were corrected before retained defect baselines; no passing claim for setup failures.

Lakeside original exact finalizer aborts after40 identicalreports, so100 rapid reports inside0.1s fail active/no-stop assertion. New finalizer instead uses monotonic30s without actualpositionprogress, independent of reportarrival/count; changed validposition renews deadline. Exact real parser/finalizer probe covers rapid sameposition, slow progress just below30s, missing/malformed replies, expiry/CH/ALERT and DONE success. Oldsource expected failure retained.

NexStar original complete driver compiled with new test and existingpark shortenedfixture: new stalled GOTO test fails missing ALERT. Fixed complete driver with test GOTO/PARK1s overrides passes both park and GOTO stop/recovery. Productiondefaults300s each, matching existingpark. Cancellation runs under serialmutex even while guiding/readback invalid; public coordinate timeout goes ALERT. Existing early-BUSY behavior remains before deadline and short-ended slew retains previousfailure handling; no test weakened into indefiniteBUSY acceptance.

Related MJKZZ signed-negative decode used signedleftshifts that overflow when high data byte is set (source confirmation, not a separately executed oldnegative UB repro). Same decodedvalue now assembled unsigned then cast to int32_t as portable MJKZZ implementation; validnegative notification covered under UBSan. Secondary canonical provenance is recorded in this task namespace.

## Executed focused checks

- `python3 -B indigo_test/unit/test_focuser_wemacro_bt.py`:1/1 pass, repeated connected→loss→reconnect, explicitdisconnect and duplicatecallback. Actual loss callback/connection branch, no radio.
- `python3 -B indigo_test/unit/test_focuser_mjkzz_bt.py`:1/1 pass: healthyposition/speed/stop and signednegative; badchecksum/address/responsebit/shortframe and notificationerror cannot mutate values/state. Exact callback with NSData inputs, no radio.
- `python3 -B indigo_test/unit/test_mount_temma_manual.py`:1/1 pass, actual full .ino compiled with host Arduino String/serial/time shims; T/I configuration, Z/s/LK connectioncontrols, standbyON/OFF/query/motor state, bothspeed replies/state. Protocolrevision2 removes contradictory configurable STNpolarity and follows maintaineddriver; no physicalvariant/board validation.
- `python3 -B indigo_test/unit/test_ao_sx_simulator_options.py`:1/1 pass; actual optionparser/replycase,6-byte firmware boundary,7/1000-byte rejects and defensive no-send on truncation. Simulatorrevision2. No PTY needed for this memory-boundary regression.
- `python3 -B indigo_test/unit/test_focuser_lakeside_deadline.py`:1/1 pass, actual finalizer/parser and deterministicclock with transport/state stubs.
- The five same probes with `SANITIZERS=address,undefined`:5/5 pass. Only probe/extracted or included source instrumented; Foundation/system runtime uninstrumented. No leak-sanitizer certification.
- Native NexStar and Lakeside `make -C indigo_drivers/<driver> -f ../../Makefile.drv all`: exit0 universal build. Mac actual .m source compile commands/output retained in bt-compile-commands.json and *_compile.txt; no diagnostics.
- Complete old/fixed NexStar direct compile commands in nexstar-commands.json. Old filtered deadlinecase exit1 expected; fixed `/tmp/nexstar-fixed-deadline`2/2 pass. GOTO filter final driver wrapper6/6 passes: existingrefusal/completion/abort/disconnect/guideoverlap plus newdeadline. Test-only GOTO1s override passed as make TEST_CFLAGS; productionarchive retains300s. Guideoverlap measured8003ms single control, not physical relay accuracy.
- `LAKESIDE_TEST_FILTER=frozen_motion python3 tools/run_driver_test.py focuser_lakeside --no-build --no-record`:1/1 pass exercising real production30s no-progress stop and recovery. Initial run hit obsolete30s scenario watchdog (SIGALRM14) before new30s production deadline plus setup; only frozen/stalled scenario watchdogs raised60s and only their waits40s. Other waits/watchdogs unchanged20s/30s. Rerun passed. Relative_motion, abort_motion and disconnect_motion filtered wrappers each1/1 pass.

No new full driver README/summary record is claimed from filtered scopes. Existing unrelated preserved behavior/older full acceptance remains referenced by canonical origin task; this is a bounded fix, not migration or full recertification. New cases/probes need root shared registration. NexStar inventory39→40/12; Lakeside existingregistered43/0 corrects stale42row; others unchanged.

## Remaining final steps

Root source review already accepted contractchoices300s/30s, requested MJKZZ earlyguard braces and simplification of constant Temma motorbranches; both done and affected probes rerun normally/sanitized. Parent AI coordinator reported final technical source/artifact review PASS. Timing benchmark complete status and exactmetrics appended after execution. Root owns registryclosure of six requested IDs, Xcode/Makefile/MIGRATION integration and final delivery. No main merge/push.

## Final timing and source checks

`make -C indigo_test build/benchmark/bench_mount_nexstar_guider_timing` then `cd indigo_test && build/benchmark/bench_mount_nexstar_guider_timing`: exit0, explicit TIMING status=complete and24 sets ×4 samples=96 measured pulses after24 discarded warmups. Requests20/100/500ms, allfour directions, idle-polling and mount-motion workloads. Endpoints are simulator-command-start-to-stop, not physical relay/motor output. Signed/absolute/percentile/percentage statistics retained in nexstar-timing-result.txt and guider-timing.json; maximum absolute error6.145ms. No new precision threshold or hardware certification; benchmark exitcode alone is not treated as completion evidence.

Changed NexStar full driver/test with ASan/UBSan (`nexstar-sanitize-command.json`) plus ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:2/2 deadline cases pass, uninstrumented common framework/SDK. Normal and instrumented deadline cases rerun after tightening command trace: event history cleared before GOTO and exactly one freshcancelcommand required. This test refinement is retained as an additive codecommit5a6280dc5 under no-rewrite rule; earlier six per-driver source commits unchanged.

Production common libraries are prebuilt/uninstrumented prerequisites; full common-library build provenance was not re-established here. Changed driver/fixture units were rebuilt; no broad framework or cross-platform certification. Source generated.c/.h/main bytes are identical after rerunning unchanged generator for Lakeside/NexStar (6 outputs), see generation-check.json. No generatorimplementation or publicproperty changes.

Configured `./.factory/scripts/check` and `./.factory/scripts/security`: exit0, retained Factory outputs; parent owns combined197 tests. Six source repairs plus fixed secondary MJKZZnegative-value provenance retain exact findings/evidence references; root owns originalsix canonical closures.

AC-1 through AC-6 pass. Final focused sourceprobes5/5 normal and5/5 ASan/UBSan; NexStar6/6 filtered wrapper cases plus2/2 shortdeadline controls and2/2 sanitizer controls (overlapping scopes, not summedunique); Lakeside4/4 selected scenarios including real30s timeout/recovery. Timing96 measured samples complete as qualified above. Hardware0 run/0 passed. No full-driver recertification, radio, boardflash, image recognition or Linux/Windows runtime claim.

`make -C indigo_test test-clean`: exit0 after retaining all requested evidence. Six new unit Python files and parent-owned registrations remain specified in handoff; no unrelated artifact/source cleanup.

Final provenance: runtime checks were performed by the implementation agent. The parent AI coordinator separately reported inspecting actual source through5a6280dc5, accepted the six technical contracts and verified the requested corrections. This report does not assert owner approval, authenticated human sign-off or acceptance of risk. User-requested repairs and user-supplied AGENTS.md govern routine evidence retention; archival itself is not approval. No further product changes after technical review. Immutable committed process gate follows the Factory commit and is recorded externally.
