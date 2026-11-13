# Development plan

Status: complete

## Approach and affected components
Queued commands kept in a list under a mutex in the agent; the handler unlinks its command; `stop_commands()` stops new requests, cancels the queue and frees the rest. Driver version 0x03000005.

## Baseline
- Revision 3f2a6fb28 on branch `scheduler`; recorded run 42/42; the new case fails under the sanitizer before the fix (6 leaked allocations from `agent_change_property`).

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | Regression case with explicit leak check | AC-001 | None | fails before the fix | done |
| 2 | Agent fix | AC-001, AC-002 | 1 | case passes, suites pass | done |
| 3 | Review, records, recorded run | all | 2 | verification.md | done |

## Test strategy
The case runner ends children with `_exit()`, so the case calls the leak check itself in the sanitizer build.

## Security risks and mitigations
None beyond memory safety; sanitizer build.

## Dependencies and architecture decisions
None.

## Migration, rollout, and rollback
Not applicable.

## Deviations and owner decisions
A per-case leak check over the whole suite found one framework leak (indigo_libs thread-local escape buffer); recorded as FND-20261113001457-32cc-scheduler-shutdown-command-leak-001 instead of changing the framework.
