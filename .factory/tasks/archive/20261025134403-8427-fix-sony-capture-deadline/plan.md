# Development plan

Status: complete

## Approach and affected components
Reproduce on the camera with a diagnostic loop (discovery + repeated reconnect/expose, temporary PTP trace on the Pi only), identify the failure modes, fix in the Sony capture path and the generic connection handler, model both in the fake USB backend.

## Baseline
- Commands and environment: Pi 5 indigosky.local (Debian 12 aarch64), ILCE-7RM4 fw 1.20, master 4387df2c0, driver 3.0.0.58; diagnostic loop `/tmp/diag_ptp` built from the hardware suite with only discovery + 10/20/30 reconnect iterations.
- Existing failures: reconnect loop failed in 6 of 6 runs without fixes (iterations 1–7), two modes; see verification.md.
- Repository revision / working-tree state: task branch from master 4387df2c0, clean.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Reproduce and trace on hardware | AC-001, AC-002 | None | both modes seen in traces | done |
| 2 | Shot confirmation and retry in `ptp_sony_exposure()` | AC-001 | 1 | retry observed on hardware; fake case | done |
| 3 | Sony reopen delay in `handle_connection()` | AC-002 | 1 | 30 hardware reconnects without DeviceBusy; fake case | done |
| 4 | Fake quirks and regressions, version 3.0.0.59 | AC-001, AC-002, AC-004 | 2, 3 | cases fail without fixes, pass with | done |
| 5 | Recorded fake and hardware runs, records | AC-003, AC-004 | 4 | script runs | see verification.md |

## Test strategy
Fake USB regressions for both quirks (negative controls with the fixes removed), full recorded fake suite, recorded hardware suite.

## Security risks and mitigations
No new input parsing; retry and wait are bounded.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable; revert the commit to roll back.

## Deviations and owner decisions
A longer shutter hold (100 ms) and repeating the SDIOConnect handshake were tried on hardware and rejected (no effect / camera left remote mode).
