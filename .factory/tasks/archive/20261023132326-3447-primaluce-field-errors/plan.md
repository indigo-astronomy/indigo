# Development plan
Status: complete

## Approach and affected components
- Library: `primalucelib.c/.h` (acknowledgement helper, calibration and sync acknowledgements).
- Driver: `.driver` (move and sync reasons, version) and the regenerated `.c`.
- Simulator: `fielderror` fault.
- Tests: unit case and simulator case.
- Docs: REFACTOR.md, MIGRATION_STATUS.md, the test record.

## Baseline
- Commands and environment: Linux x64 at master `82a071af5` (merged with origin/master `825b4cab2`).
- Last record: 65/65 at 3.0.0.33.
- Existing failures: none. The quick check of the merged tree passed (unit tests, SS3 and rotator cases).
- Repository revision / working-tree state: the task branch from master.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Acknowledgement helper and its use; driver reasons | AC-001 to AC-003 | None | tests fail without it | done |
| 2 | Fault and tests; recorded run; docs; reviews | AC-004 | 1 | recorded run passes | done |

## Test strategy
- A unit case with a scripted peer.
- A simulator case covering all four refusals.
- Each new case checked to fail without the change.
- The recorded run.

## Security risks and mitigations
See security.md.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable beyond reverting the task commit.

## Deviations and owner decisions
None.
