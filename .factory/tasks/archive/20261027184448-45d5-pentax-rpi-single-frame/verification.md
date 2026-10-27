# Verification

Verdict: pass

Scope: final software candidate and honestly labelled physical profiles, not complete camera-class certification. Root also implemented the changes; independent reviewers own quality-review.md, security.md and timing-review.md. Worktree /private/tmp/indigo-pentax-rpi, branch task/20261027184448-45d5-pentax-rpi-single-frame, prior baseline commit e1b9f0cf4 retained. Master05f14156a and the subsequent evidence-only master a5b41ff89 were merged without conflicts before the final affected checks. Unrelated solver changes were not edited.

## Reproductions and repairs

Strict fake USB reproduced rejected split nonlegacy control arguments, initially unselected ISO800, delayed aperture application and original B0/0 shutter restoration. Baseline logs are retained under pentax-*-before.log; protocol-fake.log and final-fake-42.log record intermediate progress. Five canonical repaired findings are in findings.md. Normal wire framing is characterized for both legacy and nonlegacy cameras; no unsupported mode-unlock command was invented.

For the final timing defect, the new upper/lower bound test failed before production repair: requested1.2s, command interval1.408s with200ms readiness delay (timing-before.log). The old after-ACK lower-bound assertion was independently identified as an incorrect contract. The replacement uses command dispatch after argument staging, preserving parameter/command/status order. Final100ms argument-stage/200ms readiness test measures1.205s for1.2s requested; a1.4s over-budget acknowledgement closes0.000s after return. This verifies software command timing only, with±30ms tolerance; no constant hardware calibration.

## Final commands and results

All local commands ran from the worktree root after master synchronization unless stated otherwise.

| Command | Result / evidence |
| --- | --- |
| python3 /private/tmp/pentax-protocol-tests/build.py; /private/tmp/pentax-protocol-tests/test_ccd_pentax_usb | Compiled native macOS arm64;44/44 passed, timing-final-macos.log. Exact build helper retained as evidence-build.py. |
| python3 /private/tmp/pentax-protocol-tests/build-sanitize.py; ASAN_OPTIONS=detect_leaks=0 /private/tmp/pentax-protocol-tests/test_ccd_pentax_usb_sanitize | ASan/UBSan44/44 passed, timing-sanitize-macos.log. LeakSanitizer unavailable on macOS. Helper retained. |
| Linux temporary-root: make -C indigo_drivers/ccd_pentax -f ../../Makefile.drv all; LD_LIBRARY_PATH=.../build/lib python3 tools/run_driver_test.py ccd_pentax --no-build | Production plugin built; canonical fake44/44 passed, timing-final-linux.log. No camera opened; fake backend only. |
| python3 -B -m unittest discover -s indigo_test/unit -p test_driver_test_recording.py |25/25 passed independently, quality/security reports and recorder logs. |
| ./.factory/scripts/test |190/190 passed, factory-final.log. |
| ./.factory/scripts/check | Whitespace, findings, decision index and project boundaries passed, closure-check.log. |
| ./.factory/scripts/security | Project security boundary check passed, security.md. |
| python3 tools/make_test_summary.py |144 driver summary regenerated; partial records remain distinct. |

Prior Linux hardware-harness compilation and invalid absolute-path preflight passed before USB. PENTAX_HW_EXPOSURE rejects nonfinite/out-of-range values before initialization; numeric RAW exposure1.2s passed twice. No final hardware execution after the owner's offline instruction. Linux runtime root /tmp/indigo-pentax-20261027181603 is isolated; installed package and original /home/indigo/indigo checkout were preserved.

## Physical evidence before final offline timing repair

Manual M group passed ISO/aperture/timed-shutter controls,0.1s JPEG transfer/decode, timed abort and subsequent recovery frame. Full mixed-mode case remained failed because camera ignored remote M→B; this was also observed with original upstream pkTriggerCord0.85.01 at c1177a4881f363455255866f6f6d8149574e4b35. Its status/mode logs and no-exposure0/1 shutter probe are retained; the probe clamped1/4000 and was restored, so no zero-rational shortcut entered production.

After owner-confirmed physical B setup, distinct bulb profile passed JPEG exposure, abort and recovery (hardware-bulb.log). DNG and PEF single-frame profiles each passed complete LibRaw unpack,3896x2616, ISO800; sizes16,664,256 and11,258,021bytes respectively (test-DNG.log, test-PEF.log). Config/restore logs verify JPEG and latent DNG restored, ISO800/f8/physical B retained. Hardware labels are distinct; full mixed-mode failure preserved. Latest profile totals: full1/0, bulb1/1, DNG1/1, PEF1/1. The conditional C hardware case is counted once in inventory44/1.

Native images remain outside Git. Timed JPEG sample /private/tmp/pentax-km-sample.jpeg has EXIF0.1s/f8/ISO800 and SHA256 b3d8f11bd94cf6593e8bbf7bf6d43e17a80a3c063af0d9783d3c315e1b59d069. Bulb JPEG/DNG/PEF metadata showed1.40–1.42s for1.2s before the final timing correction. Final physical timing remains unverified by explicit owner instruction. No final hardware claim derives from fake timing tests. No Windows, additional models, multiple devices or physical hotplug acceptance; macOS claim failure remains unconfirmed on hardware. The one physical USB reconnect was owner setup only.

## Delivery gates

Final independent reviews follow documentation. Capture reviewed Git content identities after archival, then create one outcome commit. Required committed-range process check and final immutable-candidate checks run after commit, before owner-authorized fast-forward integration and push; their exact candidate SHA/outcomes are reported externally. Recurring audit remains overdue under the owner's Pentax-specific deferral, with no reset or unrelated work.
