# Verification report

Verdict: pass
Reviewer: verifier pass by the implementing agent (Claude Code desktop, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `task/20261025113906-c862-ptp-fuji-hardware-test` at `e49bfce27` plus the uncommitted task diff; the Pi checkout (`~/indigo`, its own task branch) held hash-identical copies of every changed source for the recorded runs.
Review independence: not independent; the diff review in quality-review.md was done by a fresh read-only subagent.

Mode: non-interactive hardware run without physical hot-plug (owner, 2026-10-25). Host: Raspberry Pi 5 indigosky.local, Debian 12, 6.12.87+rpt-rpi-2712, aarch64. Camera: Fujifilm X-T1, firmware 5.52, USB 04cb:02bf, shutter dial on B. Also attached: Sony ILCE-6000 in charging mode (054c:0994, HID).

## Acceptance evidence
| Criterion | Result | Evidence |
| --- | --- | --- |
| AC-001 | pass | hardware `Bulb exposures` skipped as not offered; fake `fuji_bulb_not_offered` (fails without the fix) |
| AC-002 | pass | fake `rejected_setting_readback` (fails without the fix) |
| AC-003 | pass | hardware discovery lists only the X-T1; fake `sony_charger_not_attached` (fails without the fix) |
| AC-004 | pass | recorded `python3 tools/run_driver_test.py ccd_ptp --hw --device "Fujifilm X-T1"` 15/15 OK, 2026-10-25 12:38 UTC, starting from and restoring camera priority with the dial on B |
| AC-005 | pass | recorded fake run 165/165 OK (13:42 local); ASan/UBSan fake USB and ICA pass |

## Executed checks
| Command | Where | Exit | Result |
| --- | --- | --- | --- |
| `make -j4` (whole tree) | Pi | 0 | build OK |
| `indigo_test/build/hardware/test_ccd_ptp_hw --run` (intermediate, with and without `INDIGO_TEST_DEBUG`) | Pi | 1 / 0 | 6 runs: 5/15 (camera powered off, charger selected), 3/15, 4/15, 11/15, 15/15 after fixes |
| `python3 tools/run_driver_test.py ccd_ptp --hw` (X-T1) | Pi | 1, 1, 0, 1, 1, 1, 0 | 10/15, 13/15 (AF-S could not focus; owner switched to MF), 15/15 at 12:09; after the review fixes the camera was briefly on the Mac (suite cannot drive ICA, 0 found), then 12/15, 11/15, 14/15 with the camera in the owner's priority/B state (suite restore and abort timing, Bulb written back within the poll); 15/15 at 12:38 (recorded) |
| `python3 tools/run_driver_test.py ccd_ptp --hw --device "Sony Alpha A7RIV"` | Pi | 1, 1, 1 | 14/15 (suite restore of read-only settings, fixed), 13/15 twice: Reconnect loses the capture (open finding FND-20261025113906-c862-ptp-fuji-hardware-test-001; 13:00 recorded) |
| `make -C indigo_test test-ccd-ptp-sdk-sanitize test-ccd-ptp-ica-sanitize` | macOS arm64 | 0 | all passed |
| `python3 tools/run_driver_test.py ccd_ptp` | macOS arm64 | 0 | 165/165 OK (13:42, final) |
| `./.factory/scripts/findings check`, `./.factory/scripts/security` | macOS | 0 | valid |

## Baseline comparison and regressions
First Fuji run on 3.0.0.57: 3/15 (Sony-specific labels in the suite, Bulb rejected, charger attached). Each driver defect has a fake regression confirmed failing without its fix. USB BULB diagnosis used a temporary Pi-only build (BULB refusal and PC priority skipped, PTP response logging) that was removed before the recorded runs: InitiateCapture answers DeviceBusy (0x2019) under camera priority.

## Findings
D-FJ1..3 fixed in this task, recorded in the driver REFACTOR.md. Opened FND-20261025113906-c862-ptp-fuji-hardware-test-001 (Sony capture lost after reconnect, medium, nonblocking), see [findings.md](findings.md).

## Manual checks and limitations
No hot-plug (owner decision); USB BULB not available on the X-T1; no lens focuser device on this body; macOS ICA transport not driven by the hardware suite; Windows not run. The last suite change (read-only settings skipped before the deferred restore) came after the recorded X-T1 run and was not rerun on the X-T1 (camera battery); it does not change the X-T1 paths. Hardware record time is the Pi clock (UTC), the fake record local time. The camera was left in camera priority, so the dial controls the shutter again.

## Verification verdict rationale
All Fujifilm criteria pass on hardware and hardware-free; the extra Sony run is not clean and is tracked as an open finding.

## Register reconciliation
One new active finding, FND-20261025113906-c862-ptp-fuji-hardware-test-001.
