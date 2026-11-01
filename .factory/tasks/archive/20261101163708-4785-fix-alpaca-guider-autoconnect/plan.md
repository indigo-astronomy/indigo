# Development plan

Status: complete, all steps done (see verification.md)

## Approach and affected components
Add a per-device flag in `indigo_alpaca_common.h`, set by `alpaca_set_connected()`, cleared when CONNECTION reports a settled disconnect or ALERT, and required by `indigo_alpaca_connect_paired_guider()`. Bump `DRIVER_VERSION`.

## Baseline
- Commands and environment: see verification.md.
- Existing failures: see verification.md.
- Repository revision / working-tree state: origin/master cf9496e26 in worktree `task/20261101163708-4785-fix-alpaca-guider-autoconnect`.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Regression test in `indigo_test/integration/test_agent_alpaca_mapping.c` | AC-001 to AC-004 | None | Fails before the fix | done |
| 2 | Guard and flag in `indigo_drivers/agent_alpaca/`, version bump | AC-001 to AC-004, AC-006 | 1 | Test passes | done |
| 3 | Server scenario with the SynScan serial simulator | AC-005 | 2 | Guider selected, sender is Guider Agent | done |
| 4 | Independent diff review, process evidence | all | 3 | Reports pass | done |

## Test strategy
Lowest useful layer: the agent's existing in-process mapping test with stub devices on the real bus, then one end-to-end server scenario. Only Alpaca Agent targets run, per the owner's narrow-test preference.

## Security risks and mitigations
None new, see security.md.

## Dependencies and architecture decisions
No new dependency. A flag on the device record was chosen over matching the sending client, because the agent's bus callback receives no sender for a device update.

## Migration, rollout, and rollback
Rollback by reverting the task commit. No stored state changes.

## Deviations and owner decisions
A separate worktree was used because the main checkout holds another writer's unfinished investigation task on its own branch. That task is left untouched.
