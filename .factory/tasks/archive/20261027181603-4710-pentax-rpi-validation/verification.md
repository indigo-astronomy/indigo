# Verification

Verdict: pass
Scope: connection/status/disconnect only plus software regression. No exposures, abort, image capture or physical-hotplug acceptance claimed.
Verifier root also implemented the test portability fix; independent quality/security contexts inspect final diff. Base aabaed8ec963631cca9ba37826b1777339cc43e7.

RPi Linux6.12.87+rpt-rpi-2712 aarch64 sees0a17:00ce K-m. USB root0644 requires sudo for write access. Existing remote checkout /home/indigo/indigo clean on e7cbb62a643cd43a76f324185157a0a79b5254b6; source3.0.0.1 but compiled2.0.0.0. Original sudo server baseline passed connection/model/firmware/disconnect, then terminated under timeout18s/SIGINT+3s grace. No exposures. See remote-evidence/original-server.log.

Built current driver in isolated /tmp/indigo-pentax-20261027181603. Initial linking against old framework exposed indigo_monotonic_time/indigo_get_switch_target/indigo_ccd_exposure_setup missing; did not use that binary with hardware. Built matching current framework/server/headers, reusing old external archives/catalogue library read-only, then rebuilt driver. Initial server build required supplying Sequencer.js resource. Final ldd-r logs show no undefined symbols; metadata03000004. Existing checkout/packages were never modified. Source/build/link logs retained in remote-evidence. Runtime selected isolated library with LD_LIBRARY_PATH.

Owner confirmed awake camera. Current actual server loaded3.0.0.4, connectedOK, identifiedPENTAX K-m(00012d73)/firmware1.11.1.8, disconnectedOK, shut down and left no indigo_server. No exposures/control writes beyond connection. remote-evidence/current-server.log; property values observed in SSH output. Kernel was already absent before current claim; exact restoration is not certified.

Final short hardware command in isolated remote root: `sudo -n env LD_LIBRARY_PATH=/tmp/indigo-pentax-20261027181603/build/lib python3 tools/run_driver_test.py ccd_pentax --hw --no-build --type 'PENTAX K-m (connection baseline)' -- PENTAX_HW_ARGS=--baseline`. Exit0,1/1pass, explicit baseline case, no control writes/exposures. Distinct type preserves full K-m failure record. See hardware-baseline-recorded.log. Owner informed camera may sleep; no later hardware use.

Focused old Linux fake output-recovery case failed because /private/tmp was absent (test-baseline.log). Fixed mkdtemp template to /tmp, retaining isolation/assertions. Full canonical Linux fake run36/36pass (fake-recorded.log), command `env LD_LIBRARY_PATH=/tmp/indigo-pentax-20261027181603/build/lib python3 tools/run_driver_test.py ccd_pentax --no-build`. PENTAX_HW_ARGS defaults--run; existing --baseline branch now selectable and explicitly named. Inventory remains36/1; MIGRATION_STATUS already matches. Local README receives only generated Linux Testing records; full local TEST_SUMMARY regenerated, remote one-driver summary retained as evidence only.

No production driver/library behavior changes. Real capture/bulb/abort, physical unplug, non-root USB, exact kernel rebinding and macOS-99 remain unverified. Remote source checkout clean after execution; local original dirty agent_solver files preserved in separate worktree. No key material read/copied. No merge/push authorized for this task.

## Recorder profile safety

Review identified that exposing baseline mode could otherwise replace a full-camera record if --type omitted. Added a pre-build/pre-hardware guard requiring explicit '(connection baseline)' type for recorded Pentax baseline profiles from make arguments or environment; --no-record remains allowed. Current successful hardware invocation already used this distinct label. Pre-fix actual-recorder regression ran the synthetic hardware target unexpectedly (recorder-before.log). After fix all11 real-recorder tests pass (recorder-after.log): omitted/plain labels rejected before execution, environment cannot bypass, explicit labelled baseline and no-record allowed, full --run overrides baseline environment. These tests use isolated synthetic processes and no camera.

The recording guard does not change accepted labelled hardware execution; no redundant physical rerun after this recording-only fix, preserving owner battery limits. Production driver/control/exposure code unchanged.

Final updated recorder verified on Linux:11/11 unittest cases passed (remote-evidence/recorder-tests.log); full canonical fake USB36/36 passed again (remote-evidence/fake-final.log), recording timestamp18:30. No additional hardware. Local Factory190/190 passed, final configured check/security exited0. `git diff --check` passed; an initial shell typo attempting `./ .factory/scripts/check` returned126 before execution and was corrected, not counted as passing.

Final guard also rejects alternate Make assignment operators/nonliteral Make-expanded profiles. Final actual-recorder tests13/13 on macOS/Linux; canonical Linux fake36/36 at18:34 (remote-evidence/recorder-final.log and fake-final-guard.log). No hardware after baseline.

Master advanced32d7396d7257e33a3afc3520990151022c6fafcc with independent solver work. Synchronized task byff, saved/reapplied task Makefile patch, regenerated144-driver summary. CCD/core/Pentax source unchanged by main advance; no redundant camera rerun for unrelated solver or pre-execution recorder changes. Physical binaries tested fromaabaed8ec, distinct from newer whole-repository target. Scoped recorder/check/security passed after sync.
