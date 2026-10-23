# Development plan
Status: complete

## Approach and affected components
- Library: `primalucelib.c/.h`, with a new `PRIMALUCE_REPLY_ROTATOR_ENABLED` path.
- Driver: `.driver` (rotator connect and disconnect, version) and the regenerated `.c`.
- Simulator: the ARCO port switch and the loss of calibration when it is switched off.
- Tests: `rotator_lifecycle` updated, plus one new case.
- Docs: REFACTOR.md, MIGRATION_STATUS.md and the test record.

## Baseline
- Commands and environment: Linux x64 at origin/master `110f87e54`.
- Last record: 66/66 at 3.0.0.34.
- Existing failures: none in the simulator. The loss of calibration is reproduced in the session logs.
- Repository revision / working-tree state: task branch from origin/master.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Connect reads the port switch; disconnect leaves it on | AC-001, AC-002 | None | build | done |
| 2 | Simulator loss model; tests | AC-001 to AC-003 | 1 | tests fail with the old disconnect | done |
| 3 | Hardware check; recorded run; docs; reviews | AC-003, AC-004 | 2 | cycles keep the flag at 0; recorded run passes | done |

## Test strategy
- Simulator cases with the loss model.
- A check that the cases fail with the old disconnect.
- A scripted hardware check with `indigo_prop_tool`, with the requester watching the calibration.

## Security risks and mitigations
See security.md.

## Dependencies and architecture decisions
None. The change follows INDI's handling.

## Migration, rollout, and rollback
The first connect after the update may still find the port off, left that way by the old driver.
That connect switches the port on once, and the ARCO then needs one calibration.

## Deviations and owner decisions
`rotator_lifecycle` encoded the old switch-off. The expectation is changed, with the reason recorded
in REFACTOR.md.
