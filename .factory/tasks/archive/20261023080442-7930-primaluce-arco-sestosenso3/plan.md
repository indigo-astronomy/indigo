# Development plan
Status: complete

## Approach and affected components
- Library: `primalucelib.c/.h` (DEG acknowledgement path, reply prefix skip).
- Driver: `indigo_focuser_primaluce.driver` (angle tolerance, version 29 to 30; the audit repair already took 29) and the regenerated `.c`.
- Simulator: SESTO SENSO 3 ARCO replies and angle.
- Tests: `test_primalucelib.c`, `test_focuser_primaluce_simulator.c` (two SESTO SENSO 3 cases, one readback tolerance).
- Docs: PROTOCOL.md, REFACTOR.md, MIGRATION_STATUS.md, the test record.

## Baseline
- Commands and environment: Linux x64 at `b0c5cf377` (master). The task 012 record is 61/61.
- Existing failures: none in the simulator; on hardware, the three defects.
- Repository revision / working-tree state: `task/20261023080442-7930-primaluce-arco-sestosenso3` from master `b0c5cf377`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Accept the DEG move acknowledgement; SESTO SENSO 3 move case | AC-001 | None | case fails without the fix | done |
| 2 | Angle tolerance; simulator angle rounding | AC-002 | 1 | case fails with the exact comparison | done |
| 3 | Sync acknowledgement and prefix skip; sync case and unit case | AC-003, AC-004 | 1 | both fail without each fix | done |
| 4 | Hardware checks, recorded run, docs, reviews | AC-005 | 1 to 3 | recorded run passes | done |

## Test strategy
- SESTO SENSO 3 variants of the existing rotator move and sync cases.
- A library unit case for the prefix skip.
- A check that each new case fails without its fix.
- Supervised hardware checks.

## Security risks and mitigations
See security.md.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
None.
