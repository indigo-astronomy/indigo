# Verification report

Verdict: pass
Reviewer: verifier pass by the implementing agent (Claude Code desktop, claude-opus-5-5)
Date: 2026-10-25
Revision and working-tree state: `task/20261025112041-9ddc-ptp-sony-hardware-test` at `a89afe87d` plus the uncommitted task diff; the Pi checkout (`~/indigo`, branch pushed to the Pi's own repository) held hash-identical copies of every changed source for the recorded runs.
Review independence: not independent; the diff review in quality-review.md was done by a fresh read-only subagent.

Mode: non-interactive hardware run without physical hot-plug (owner, 2026-10-25). Host: Raspberry Pi 5 indigosky.local, Debian 12, 6.12.87+rpt-rpi-2712, aarch64, root on /dev/sda2. Camera: Sony ILCE-7RM4, firmware 1.20, USB 054c:0ccc.

## Acceptance evidence
| Criterion | Result | Evidence |
| --- | --- | --- |
| AC-001 | pass | hardware `Discovery and identity` (ILCE-7RM4, 1.20, serial); fake `device_identity_published` |
| AC-002 | pass | hardware `Reconnect`; fake `sony_busy_after_connect` (fails with the window set to 0) |
| AC-003 | pass | hardware `Image sizes` (L 9504x6336, M 6240x4160, S 4752x3168), `RAW+JPEG with preview`, `Bulb exposures`, `Lens focuser`; fake `sony_dropped_setting_write` (fails without `refresh_properties`) |
| AC-004 | pass | recorded `python3 tools/run_driver_test.py ccd_ptp --hw` 15/15 OK, 2026-10-25 11:26 UTC |
| AC-005 | pass | recorded fake run 162/162 OK (12:29 local); ASan/UBSan 162/162 |

## Executed checks
| Command | Where | Exit | Result |
| --- | --- | --- | --- |
| `make -j4` (whole tree) | Pi | 0 | build OK |
| `make -C indigo_drivers/ccd_ptp all`, `make -C indigo_test build/hardware/test_ccd_ptp_hw` | Pi and macOS | 0 | no new diagnostics |
| `indigo_test/build/hardware/test_ccd_ptp_hw --run` (intermediate, with and without `INDIGO_TEST_DEBUG`) | Pi | 1 / 0 | 9 diagnostic runs: identity, read-only shutter, DeviceBusy after reconnect, dropped writes; last 15/15 |
| `python3 tools/run_driver_test.py ccd_ptp --hw` | Pi | 0 | 15/15 OK (final, after review fixes) |
| `make -C indigo_test test-ccd-ptp-sdk-sanitize test-ccd-ptp-ica-sanitize` | macOS arm64 | 0 | 155/155 + 7/7 |
| `python3 tools/run_driver_test.py ccd_ptp` | macOS arm64 | 0 | 162/162 OK |
| `./.factory/scripts/findings check`, `./.factory/scripts/security` | macOS | 0 | valid |

## Baseline comparison and regressions
First hardware run on 3.0.0.56: 4/15 (identity missing, shutter read-only in the then-current camera mode, cascade). Diagnosis used a temporary container dump compiled on the Pi only and removed before the recorded runs. Each driver defect has a fake regression confirmed failing without its fix.

## Findings
D-HW1..3 fixed in this task, recorded in the driver REFACTOR.md; no finding records opened (none left open).

## Manual checks and limitations
No hot-plug (owner decision); one body; lens travel not observed; Windows not run. Hardware record times are the Pi clock (UTC), the fake record is local time, as the script records them.

## Verification verdict rationale
All criteria pass on hardware and hardware-free after the final changes.

## Register reconciliation
No change to central findings.
