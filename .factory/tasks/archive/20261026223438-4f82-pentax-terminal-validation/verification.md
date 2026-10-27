# Verification

Verdict: pass
Scope: owner-authorized software delivery only; physical3.0.0.4 acceptance remains deferred and is not claimed.
Verifier: root, also production implementer; separate test, quality and security agents supplied independent evidence.
Base:34d0c2869422978a012f84c218d39eae05ba5f7c; integration target unchanged after authorized fetch/merge.

Working directory: /Users/polakovic/Development/indigo. Base revision 34d0c2869422978a012f84c218d39eae05ba5f7c; production source unchanged, only this new task record is untracked.

- `make -C indigo_drivers/ccd_pentax -f ../../Makefile.drv all`: exit 0; compiled source and linked archive, dylib and standalone executable. See driver-build.log.
- `make -C indigo_test build/hardware/test_ccd_pentax_hw`: exit 0; linked the existing hardware test with the rebuilt driver archive. See hardware-test-build.log.
- `build/bin/indigo_driver_metadata build/drivers/indigo_ccd_pentax.dylib`: reported `"indigo_ccd_pentax", "PENTAX Camera", 03000003`. INFO only, no USB initialization.
- `python3 tools/run_driver_test.py ccd_pentax --no-build --no-record`: exit 0, 31/31 fake USB cases passed. See fake-usb.log. No camera access.
- `/bin/bash -n /private/tmp/run-pentax-km-test.command`: exit 0. Snapshot: terminal-command.sh. The wrapper stops if a competing server/test is found or process inspection fails, checks module version, then invokes the canonical hardware runner using sudo and captures its exit status through pipefail.
- Escalated process inventory found no running indigo_server or test_ccd_pentax. The initial sandbox process inventory was denied; it was not treated as proof of absence.
- `/usr/bin/python3 --version`: Python 3.9.6, compatible with the existing runner.

No hardware test has run. Next gate is the owner's camera wake confirmation. The task remains open for hardware evidence and final quality/security reviews; preparation is not physical acceptance.

## Physical attempt after owner wake confirmation

Owner explicitly confirmed K-m awake and instructed the sudo Terminal run. Launched `/private/tmp/run-pentax-km-test.command` in a dedicated Terminal tab through AppleScript. sudo authentication completed; canonical runner invoked the prebuilt test executable. Module metadata was 03000003. At 23:37:38 local time the test discovered PENTAX K-m, then `libusb_claim_interface` returned LIBUSB_ERROR_OTHER (-99). One case planned, zero passed; make failed and the runner recorded failure in the Pentax README and TEST_SUMMARY. See terminal-hardware.log.

Failure occurred before connection initialization, settings changes or exposures. No physical unplug/replug was attempted. No further hardware retry was started; owner was told the camera may sleep. No USB cleanup failure was logged (this alone is not independent confirmation of kernel binding restoration).

The failing executable was `indigo_test/build/hardware/test_ccd_pentax_hw`, not the original successful `build/bin/indigo_server`. Thus this result does not yet isolate a driver regression from a process/framework/environment difference. Next diagnostic should compare the actual current server invocation before concluding macOS sudo access is impossible.

## Actual server preparation

Prepared server-terminal-command.sh and server-client-check.sh (live copies under /private/tmp). `/bin/bash -n` passed for both. Module metadata check again reported 03000003. Property tool syntax, value/state output and timeout behavior were checked in indigo_tools/indigo_prop_tool.c; metadata INFO action does not initialize USB. No production source changes and no physical access during this preparation. Server binary is the same existing build/bin/indigo_server; rebuilt Pentax dylib is newer and will be verified in the server's actual load log. Await owner's wake confirmation before launching either script.

## Actual sudo server attempt

Owner confirmed K-m awake and explicitly requested actual sudo indigo_server. Launched the dedicated Terminal wrapper. Its initial broad process-name preflight stopped before launch; follow-up process and socket inspection found no competing server. Narrowed the temporary preflight to exact server executable names and an anchored hardware-test command, then relaunched. No production source changes.

Actual command: `sudo build/bin/indigo_server -vv indigo_ccd_pentax`, cwd /Users/polakovic/Development/indigo. Log confirms actual server binary build identifier 64e965d7f (same existing server build as the owner's successful old log), current dynamically loaded driver 3.0.0.3, then K-m VID:PID 0a17:00ce detection. At local 23:42:27, connection failed with `libusb_claim_interface: Other error (-99)`. The companion client exited 1 and explicitly observed `Connection state=ALERT connected=OFF`. No settings changes or exposures were requested. Evidence: actual-server.log and actual-server-client.log.

Process inspection confirmed both dedicated server-related processes ran UID 0. Sent Ctrl-C into that dedicated Terminal tab; a subsequent exact process-name inventory found no indigo_server. No further hardware retry. The server log contains no shutdown narrative, so process absence is evidence of termination, not independent certification of complete graceful cleanup or kernel reattachment. No USB cleanup error was logged before termination.

Conclusion: failure also occurs in the actual sudo server, not only the in-process test executable. The owner's successful 2.0.0.0 log and current 3.0.0.3 failure justify comparing old/new USB initialization and runtime ownership. They do not by themselves prove which change or external condition causes the failure. This was diagnostic only and did not overwrite canonical hardware-suite records.

## Offline USB restoration repair

Owner requested compare/fix while keeping K-m asleep. No hardware requests were made during repair. Source comparison confirmed original auto-detach=1/claim sequence versus completion explicit detach; both use the same Darwin capture helper, with different handle lock coverage and an internal kernel-state recheck. Source alone does not prove the physical -99 cause.

Restored the original auto-detach acquisition and incremented driver to 3.0.0.4. Added a minimal Darwin backend rollback using successful-capture ownership, preserving claim errors and logging rollback errors. The driver releases only a claimed interface, verifies expected system binding after automatic release, and blocks retry after claim/release/restore uncertainty until driver restart. No public API or ABI changes.

Pre-fix regression compiled the actual original Darwin auto-claim wrapper and failed `expected 1, got 0` for reattachment after successful capture plus failed claim (auto-baseline.log). Tests generate a build-only include of the actual production wrapper; fake tests do not duplicate its rollback implementation. Updated explicit-detach expectations are obsolete because restoring the original auto path is the owner-requested behavior.

Normal36/36 and ASan/UBSan36/36 passed (auto-full.log, auto-sanitize.log). Baseline and checks were run by a separate test agent; final canonical runner evidence follows. `make -C indigo_drivers/ccd_pentax -f ../../Makefile.drv all`, `make -C indigo_libs/externals/libusb -j4`, and `make -C indigo_libs/externals/libusb/libusb install-libLTLIBRARIES` exited0. Install destination is only build/lib, not the system. `cmp` confirms installed local dylib matches the built dylib. Metadata reports03000004. No new persistent source files, Xcode references or public interfaces introduced. MIGRATION_STATUS now records Pentax36/1; Windows has no project registration and generator migration has not been done.

The old actual-server and harness hardware failures remain recorded. Physical verification of3.0.0.4 is0 attempted/0 passed at the owner's request. Real USB ownership, actual image capture, other platform runtimes and failure-time OS restoration still require hardware validation.

Final camera-free canonical run: `python3 tools/run_driver_test.py ccd_pentax` exited0,36/36, recorded3.0.0.4 fake SDK and regenerated TEST_SUMMARY (auto-recorded.log). Configured `./.factory/scripts/check` and `./.factory/scripts/security` exited0. `git diff --check` passed. Master remains34d0c2869422978a012f84c218d39eae05ba5f7c. No hardware access during repair.

Quality review found a missing dependency: normal libusb target lacked source prerequisites, so existing builds could retain the old backend. Added the touched Darwin backend source prerequisite in indigo_libs/Makefile. `make -C indigo_libs -n -W externals/libusb/libusb/os/darwin_usb.c libusb` selects rebuild/install (auto-dependency-dry-run.log); `make -C indigo_libs libusb` exits0, installed library current (auto-supported-libusb-build.log). No production C changed after passing36/36. Canonical runner expects prebuilt framework dependencies as documented; normal root make now refreshes this backend on source change.

Final preparation: rebuilt existing hardware test executable against3.0.0.4 without running it; command `make -C indigo_test build/hardware/test_ccd_pentax_hw` exited0 (auto-hardware-test-build.log). Both driver and local libusb dylibs contain x86_64/arm64. Updated the temporary direct-server wrapper to require03000004, syntax check passed. Quality/security reviewed the corrected software diff and passed; both fixed findings are archived, registry check reports27 active/199 archived. No new maintained files outside task/finding metadata. Physical hardware validation and full task closure remain pending; committed-range delivery process gate has not been claimed.

## Authorized software closure

Owner instruction on2026-10-27: `urob commit, merge, push`. See delivery-decision.md for scope and prior explicit physical-validation deferral. Software acceptance criteria in the final specification are met; physical verification remains a separate follow-up, without a passing claim. Previous chronology saying task closure is pending describes the pre-authorization state.

Final configured `./.factory/scripts/test`: exit0,190/190 tests in99.717s (closure-factory-tests.log). Final configured check and security groups exited0 (closure-check.log, closure-security.log). Production source/tests are unchanged from the36/36 recorded and sanitized runs. `git fetch origin master` succeeded; origin/master and master both matched34d0c2869422978a012f84c218d39eae05ba5f7c. `git merge --ff-only origin/master` on the task returned already up to date. No repeated native test run was needed because integration target and production content did not change.

Closure bookkeeping includes final software scope, retained owner deferrals, normalized reviewer verdict syntax and exact process identity records. Integration, publication and cleanup remain pending until the post-commit external result. Prebuilt camera test artifacts intentionally retained for the owner's future battery-conscious test session.
