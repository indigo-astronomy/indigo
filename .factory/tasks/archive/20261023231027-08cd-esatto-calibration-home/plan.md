# Development plan

Status: final

## Approach and affected components
- Driver: an `is_esatto` flag, X_CALIBRATE visibility per model, the needs-calibration message and the
  move completion reason.
- Simulator: the ESATTO profiles.
- Tests.
- Documentation.

## Baseline
- Commands and environment: `python3 tools/run_driver_test.py focuser_primaluce`, Linux x86_64.
- Existing failures: none (69/69 at 3.0.0.38).
- Repository revision / working-tree state: master `ba09a1367`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Driver `.driver` and regenerated `.c` | AC-001..003 | None | build | done |
| 2 | Simulator profiles | AC-001..003 | None | build | done |
| 3 | Tests, confirmed to fail without step 1 | AC-001..003 | 1, 2 | filtered run | done |
| 4 | PROTOCOL.md, PROPERTIES.md, REFACTOR.md, MIGRATION_STATUS.md | AC-004 | None | review | done |
| 5 | Recorded run | AC-005 | 1–4 | 71/71 | see verification.md |

## Test strategy
Simulator integration tests and a recorded run. A supervised check on the requester's ESATTO.
