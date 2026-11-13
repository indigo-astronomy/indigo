# Development plan

Status: complete

## Approach and affected components
Add a threshold constant and one conditional `indigo_send_message(device, BUSY_PROPERTY, ...)` at the end of the SYNC branch in `indigo_libs/indigo_mount_driver.c`. Extend `indigo_test/unit/test_mount_alignment_load.c`, which already includes the driver source.

## Baseline
- Commands and environment: macOS 27.0, Apple clang; `make -C indigo_libs`, `make -C indigo_test test-unit`
- Existing failures: `test_protocol_security` case "device ACL requires device or nonzero master token" fails on master `ddd7ad5b6` (FND-20261112204155-dae8-warn-sync-near-pole-001)
- Repository revision / working-tree state: master `ddd7ad5b6`, clean

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Warning in `indigo_libs/indigo_mount_driver.c` | AC-001, AC-002, AC-003 | None | build | done |
| 2 | Unit test in `indigo_test/unit/test_mount_alignment_load.c` | AC-001..AC-004 | 1 | test passes with, fails without the warning | done |
| 3 | Unit suites | AC-005 | 1 | 34/35 pass, the failure is pre-existing | done |

## Test strategy
Unit test through `indigo_mount_change_property()` with a capturing bus client. Integration suites are not rerun: the change only adds a message in the framework SYNC branch; mount suites that sync near the pole (lx200) use CONTROLLER mode, which does not reach it.

## Security risks and mitigations
None; constant message, no new input.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable; revert the commit to roll back.

## Deviations and owner decisions
None.
